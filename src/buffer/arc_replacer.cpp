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
 *
 * TODO(P1): Add implementation
 *
 * @brief a new ArcReplacer, with lists initialized to be empty and target size to 0
 * @param num_frames the maximum number of frames the ArcReplacer will be required to cache
 */
ArcReplacer::ArcReplacer(size_t num_frames) : replacer_size_(num_frames) {}

/**
 * TODO(P1): Add implementation
 *
 * @brief Performs the Replace operation as described by the writeup
 * that evicts from either mfu_ or mru_ into its corresponding ghost list
 * according to balancing policy.
 *
 * If you wish to refer to the original ARC paper, please note that there are
 * two changes in our implementation:
 * 1. When the size of mru_ equals the target size, we don't check
 * the last access as the paper did when deciding which list to evict from.
 * This is fine since the original decision is stated to be arbitrary.
 * 2. Entries that are not evictable are skipped. If all entries from the desired side
 * (mru_ / mfu_) are pinned, we instead try victimize the other side (mfu_ / mru_),
 * and move it to its corresponding ghost list (mfu_ghost_ / mru_ghost_).
 *
 * @return frame id of the evicted frame, or std::nullopt if cannot evict
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
 * TODO(P1): Add implementation
 *
 * @brief Record access to a frame, adjusting ARC bookkeeping accordingly
 * by bring the accessed page to the front of mfu_ if it exists in any of the lists
 * or the front of mru_ if it does not.
 *
 * Performs the operations EXCEPT REPLACE described in original paper, which is
 * handled by `Evict()`.
 *
 * Consider the following four cases, handle accordingly:
 * 1. Access hits mru_ or mfu_
 * 2/3. Access hits mru_ghost_ / mfu_ghost_
 * 4. Access misses all the lists
 *
 * This routine performs all changes to the four lists as preperation
 * for `Evict()` to simply find and evict a victim into ghost lists.
 *
 * Note that frame_id is used as identifier for alive pages and
 * page_id is used as identifier for the ghost pages, since page_id is
 * the unique identifier to the page after it's dead.
 * Using page_id for alive pages should be the same since it's one to one mapping,
 * but using frame_id is slightly more intuitive.
 *
 * @param frame_id id of frame that received a new access.
 * @param page_id id of page that is mapped to the frame.
 * @param access_type type of access that was received. This parameter is only needed for
 * leaderboard tests.
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

    // delta is computed before the hit entry is erased, so the divisor is never zero.
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
 * TODO(P1): Add implementation
 *
 * @brief Toggle whether a frame is evictable or non-evictable. This function also
 * controls replacer's size. Note that size is equal to number of evictable entries.
 *
 * If a frame was previously evictable and is to be set to non-evictable, then size should
 * decrement. If a frame was previously non-evictable and is to be set to evictable,
 * then size should increment.
 *
 * If frame id is invalid, throw an exception or abort the process.
 *
 * For other scenarios, this function should terminate without modifying anything.
 *
 * @param frame_id id of frame whose 'evictable' status will be modified
 * @param set_evictable whether the given frame is evictable or not
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
 * TODO(P1): Add implementation
 *
 * @brief Remove an evictable frame from replacer.
 * This function should also decrement replacer's size if removal is successful.
 *
 * Note that this is different from evicting a frame, which always remove the frame
 * decided by the ARC algorithm.
 *
 * If Remove is called on a non-evictable frame, throw an exception or abort the
 * process.
 *
 * If specified frame is not found, directly return from this function.
 *
 * @param frame_id id of frame to be removed
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
 * TODO(P1): Add implementation
 *
 * @brief Return replacer's size, which tracks the number of evictable frames.
 *
 * @return size_t
 */
auto ArcReplacer::Size() -> size_t {
  std::scoped_lock lock(latch_);
  return curr_size_;
}

}  // namespace bustub
