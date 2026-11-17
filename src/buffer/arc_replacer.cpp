// :bustub-keep-private:
//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// arc_replacer.cpp
//
// Identification: src/buffer/arc_replacer.cpp
//
// Copyright (c) 2015-2025, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#include "buffer/arc_replacer.h"
#include <algorithm>
#include <optional>
#include "common/config.h"

namespace bustub {

/**
 * @brief 构造一个 `ArcReplacer`，四张 list 初始为空，目标大小为 0。
 * @param num_frames `ArcReplacer` 最多需要缓存多少个 frame
 */
ArcReplacer::ArcReplacer(size_t num_frames) : replacer_size_(num_frames) {}

/**
 * @brief 执行实验说明里描述的 Replace 操作：按照平衡策略，从 `mfu_` 或 `mru_`
 * 里挑一个淘汰出去，放进它对应的 ghost list。
 *
 * 如果你去翻原始的 ARC 论文，注意我们的实现有两处不同：
 * 1. 当 `mru_` 的大小正好等于目标大小时，我们不像论文那样去看最近一次访问
 * 来决定从哪张 list 淘汰。这没问题，因为论文里也说了这个选择本来就是任意的。
 * 2. 不可淘汰的条目会被跳过。如果目标那一侧（`mru_` / `mfu_`）的条目全被 pin 住，
 * 我们改为去淘汰另一侧（`mfu_` / `mru_`），并把它挪进对应的 ghost list
 * （`mfu_ghost_` / `mru_ghost_`）。
 *
 * @return 被淘汰的 frame 的 ID；如果没有能淘汰的就返回 `std::nullopt`
 */
auto ArcReplacer::Evict() -> std::optional<frame_id_t> {
  std::scoped_lock lock(latch_);

  auto find_victim = [this](std::list<frame_id_t> &lst) -> std::shared_ptr<FrameStatus> {
    for (auto rit = lst.rbegin(); rit != lst.rend(); ++rit) {
      auto &status = alive_map_.at(*rit);
      if (status->evictable_) {
        return status;
      }
    }
    return nullptr;
  };

  std::shared_ptr<FrameStatus> victim;

  if (mru_.size() < mru_target_size_) {
    victim = find_victim(mfu_);
    if (victim == nullptr) {
      victim = find_victim(mru_);  // 首选边全 pinned，换边
    }
  } else {
    victim = find_victim(mru_);
    if (victim == nullptr) {
      victim = find_victim(mfu_);
    }
  }

  if (victim == nullptr) {
    return std::nullopt;  // 两边都没得淘汰
  }

  if (victim->arc_status_ == ArcStatus::MRU) {
    mru_.erase(victim->alive_it_);
    victim->arc_status_ = ArcStatus::MRU_GHOST;
    victim->ghost_it_ = mru_ghost_.insert(mru_ghost_.begin(), victim->page_id_);
  } else {
    mfu_.erase(victim->alive_it_);
    victim->arc_status_ = ArcStatus::MFU_GHOST;
    victim->ghost_it_ = mfu_ghost_.insert(mfu_ghost_.begin(), victim->page_id_);
  }

  frame_id_t victim_frame = victim->frame_id_;
  victim->evictable_ = false;
  alive_map_.erase(victim_frame);
  ghost_map_[victim->page_id_] = victim;
  --curr_size_;

  return victim_frame;
}

/**
 * @brief 记录一次对 frame 的访问，并相应更新 ARC 的记账：
 * 如果被访问的页已经在某张 list 里，就把它挪到 `mfu_` 的头部；否则放到 `mru_` 的头部。
 *
 * 这里执行的是原始论文里除 REPLACE 之外的所有操作，REPLACE 由 `Evict()` 负责。
 *
 * 请分别处理以下四种情况：
 * 1. 命中 `mru_` 或 `mfu_`
 * 2/3. 命中 `mru_ghost_` / `mfu_ghost_`
 * 4. 四张 list 都没命中
 *
 * 本函数会把四张 list 都调整好，这样 `Evict()` 只需要找到并淘汰一个受害者、
 * 把它挪进 ghost list 就行了。
 *
 * 注意：存活页用 `frame_id` 做标识，ghost 页用 `page_id` 做标识 ——
 * 因为页死掉之后，`page_id` 才是它的唯一标识。
 * 存活页用 `page_id` 其实也一样（一一对应），但用 `frame_id` 更直观一点。
 *
 * @param frame_id 收到新访问的 frame 的 ID。
 * @param page_id 映射到这个 frame 的页的 ID。
 * @param access_type 这次访问的类型。这个参数只在 leaderboard 测试里用得上。
 */
void ArcReplacer::RecordAccess(frame_id_t frame_id, page_id_t page_id, [[maybe_unused]] AccessType access_type) {
  std::scoped_lock lock(latch_);

  if (auto it = alive_map_.find(frame_id); it != alive_map_.end()) {
    auto &status = it->second;
    if (status->arc_status_ == ArcStatus::MRU) {
      mfu_.splice(mfu_.begin(), mru_, status->alive_it_);  // 从 mru_ 搬到 mfu_ 头部
      status->alive_it_ = mfu_.begin();                    // 更新位置记录
      status->arc_status_ = ArcStatus::MFU;                // 状态从 MRU 变 MFU
    } else {
      mfu_.splice(mfu_.begin(), mfu_, status->alive_it_);  // 源和目标都是 mfu_：内部重排
      status->alive_it_ = mfu_.begin();
    }
  } else if (auto it = ghost_map_.find(page_id); it != ghost_map_.end()) {
    auto &status = it->second;

    // delta 要在被命中的条目被删掉之前算，这样除数永远不会是 0。
    if (status->arc_status_ == ArcStatus::MRU_GHOST) {
      size_t delta = std::max<size_t>(mfu_ghost_.size() / mru_ghost_.size(), 1);
      mru_target_size_ = std::min(mru_target_size_ + delta, replacer_size_);
      mru_ghost_.erase(status->ghost_it_);
    } else {
      size_t delta = std::max<size_t>(mru_ghost_.size() / mfu_ghost_.size(), 1);
      mru_target_size_ = mru_target_size_ > delta ? mru_target_size_ - delta : 0;
      mfu_ghost_.erase(status->ghost_it_);
    }
    ghost_map_.erase(it);
    auto st = std::make_shared<FrameStatus>(page_id, frame_id, false, ArcStatus::MFU);
    st->alive_it_ = mfu_.insert(mfu_.begin(), frame_id);
    alive_map_[frame_id] = st;
  } else {
    if (mru_.size() + mru_ghost_.size() == replacer_size_) {
      ghost_map_.erase(mru_ghost_.back());  // 删 map
      mru_ghost_.pop_back();
    } else if (mru_.size() + mru_ghost_.size() + mfu_.size() + mfu_ghost_.size() == replacer_size_ + replacer_size_) {
      ghost_map_.erase(mfu_ghost_.back());  // 删 map
      mfu_ghost_.pop_back();
    }
    auto st = std::make_shared<FrameStatus>(page_id, frame_id, false, ArcStatus::MRU);
    st->alive_it_ = mru_.insert(mru_.begin(), frame_id);
    alive_map_[frame_id] = st;
  }
}

/**
 * @brief 切换一个 frame 是否可淘汰。本函数同时维护 replacer 的 size ——
 * 注意 size 等于**可淘汰条目**的数量。
 *
 * 如果某个 frame 原本可淘汰、现在要设成不可淘汰，size 要减一；
 * 如果原本不可淘汰、现在要设成可淘汰，size 要加一。
 *
 * 如果 frame ID 非法，抛异常或终止进程。
 *
 * 其它情况直接返回，不做任何修改。
 *
 * @param frame_id 要修改「可淘汰」状态的 frame 的 ID
 * @param set_evictable 这个 frame 是否可淘汰
 */
void ArcReplacer::SetEvictable(frame_id_t frame_id, bool set_evictable) {
  std::scoped_lock lock(latch_);
  if (frame_id < 0) {
    throw std::invalid_argument("frame_id out of range");
  }
  auto it = alive_map_.find(frame_id);
  if (it == alive_map_.end()) {
    return;
  }
  auto &status = it->second;
  if (status->evictable_ == set_evictable) {
    return;  // 状态没变
  }
  status->evictable_ = set_evictable;
  if (set_evictable) {
    ++curr_size_;
  } else {
    --curr_size_;
  }
}

/**
 * @brief 从 replacer 里移除一个可淘汰的 frame。
 * 移除成功时本函数还要把 replacer 的 size 减一。
 *
 * 注意这和「淘汰」不同：淘汰移除的是 ARC 算法挑出来的 frame。
 *
 * 如果对一个不可淘汰的 frame 调 `Remove`，抛异常或终止进程。
 *
 * 如果指定的 frame 找不到，直接返回。
 *
 * @param frame_id 要移除的 frame 的 ID
 */
void ArcReplacer::Remove(frame_id_t frame_id) {
  std::scoped_lock lock(latch_);
  if (frame_id < 0) {
    throw std::invalid_argument("frame_id out of range");
  }
  auto it = alive_map_.find(frame_id);
  if (it == alive_map_.end()) {
    return;
  }
  auto &status = it->second;
  if (!status->evictable_) {
    throw std::invalid_argument("the frame is unevictable");
  }
  if (status->arc_status_ == ArcStatus::MRU) {
    mru_.erase(status->alive_it_);
  } else {
    mfu_.erase(status->alive_it_);
  }
  alive_map_.erase(it);
  --curr_size_;
}

/**
 * @brief 返回 replacer 的 size，它跟踪的是**可淘汰 frame** 的数量。
 *
 * @return size_t
 */
auto ArcReplacer::Size() -> size_t {
  std::scoped_lock lock(latch_);
  return curr_size_;
}

}  // namespace bustub
