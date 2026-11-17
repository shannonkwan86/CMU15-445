//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// page_guard.cpp
//
// Identification: src/storage/page/page_guard.cpp
//
// Copyright (c) 2015-2025, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#include "storage/page/page_guard.h"
#include <memory>
#include <vector>
#include "buffer/arc_replacer.h"
#include "common/macros.h"

namespace bustub {

/**
 * @brief `ReadPageGuard` 唯一的构造函数，用来创建一个有效的 guard。
 *
 * 注意只有缓冲池管理器才能调用这个构造函数。
 *
 *
 * @param page_id 要读的页的 ID。
 * @param frame 指向存放该页的 frame 的共享指针。
 * @param replacer 指向缓冲池管理器 replacer 的共享指针。
 * @param bpm_latch 指向缓冲池管理器 latch 的共享指针。
 * @param disk_scheduler 指向缓冲池管理器磁盘调度器的共享指针。
 */
ReadPageGuard::ReadPageGuard(page_id_t page_id, std::shared_ptr<FrameHeader> frame,
                             std::shared_ptr<ArcReplacer> replacer, std::shared_ptr<std::mutex> bpm_latch,
                             std::shared_ptr<DiskScheduler> disk_scheduler)
    : page_id_(page_id),
      frame_(std::move(frame)),
      replacer_(std::move(replacer)),
      bpm_latch_(std::move(bpm_latch)),
      disk_scheduler_(std::move(disk_scheduler)) {
  is_valid_ = true;
}

/**
 * @brief ReadPageGuard 的移动构造函数。
 *
 * ### 实现
 *
 * 如果你对移动语义还不熟，请先上网看学习资料。有很多很好的资源（文章、微软教程、YouTube 视频）
 * 深入地讲解了这件事。
 *
 * 一定要把另一个 guard 置为无效，否则会碰到 double free 问题！两个对象各自都**至少**要更新 5 个字段。
 *
 *
 * @param that 另一个 page guard。
 */
ReadPageGuard::ReadPageGuard(ReadPageGuard &&that) noexcept
    : page_id_(that.page_id_),
      frame_(std::move(that.frame_)),
      replacer_(std::move(that.replacer_)),
      bpm_latch_(std::move(that.bpm_latch_)),
      disk_scheduler_(std::move(that.disk_scheduler_)),
      is_valid_(that.is_valid_) {
  that.is_valid_ = false;
}

/**
 * @brief ReadPageGuard 的移动赋值运算符。
 *
 * ### 实现
 *
 * 如果你对移动语义还不熟，请先上网看学习资料。有很多很好的资源（文章、微软教程、YouTube 视频）
 * 深入地讲解了这件事。
 *
 * 一定要把另一个 guard 置为无效，否则会碰到 double free 问题！两个对象各自都**至少**要更新 5 个字段；
 * 对当前对象来说，还要确保释放掉它原本可能持有的资源。
 *
 *
 * @param that 另一个 page guard。
 * @return ReadPageGuard& 新的有效 `ReadPageGuard`。
 */
auto ReadPageGuard::operator=(ReadPageGuard &&that) noexcept -> ReadPageGuard & {
  if (this == &that) {
    return *this;
  }

  Drop();

  page_id_ = that.page_id_;
  frame_ = std::move(that.frame_);
  replacer_ = std::move(that.replacer_);
  bpm_latch_ = std::move(that.bpm_latch_);
  disk_scheduler_ = std::move(that.disk_scheduler_);
  is_valid_ = that.is_valid_;

  that.is_valid_ = false;

  return *this;
}

/**
 * @brief 获取本 guard 所保护的页的 ID。
 */
auto ReadPageGuard::GetPageId() const -> page_id_t {
  BUSTUB_ENSURE(is_valid_, "tried to use an invalid read guard");
  return page_id_;
}

/**
 * @brief 获取一个 `const` 指针，指向本 guard 所保护的页数据。
 */
auto ReadPageGuard::GetData() const -> const char * {
  BUSTUB_ENSURE(is_valid_, "tried to use an invalid read guard");
  return frame_->GetData();
}

/**
 * @brief 返回这个页是否是脏的（被修改过但还没写回磁盘）。
 */
auto ReadPageGuard::IsDirty() const -> bool {
  BUSTUB_ENSURE(is_valid_, "tried to use an invalid read guard");
  return frame_->is_dirty_;
}

/**
 * @brief 安全地把本页数据写回磁盘。
 */
void ReadPageGuard::Flush() {
  BUSTUB_ENSURE(is_valid_, "tried to use an invalid read guard");

  auto promise = disk_scheduler_->CreatePromise();
  auto future = promise.get_future();

  std::vector<DiskRequest> requests;
  requests.push_back({true, frame_->GetDataMut(), page_id_, std::move(promise)});

  disk_scheduler_->Schedule(requests);
  future.get();

  frame_->is_dirty_ = false;
}

/**
 * @brief 手动释放一个有效的 `ReadPageGuard` 所持有的数据。如果本 guard 无效，本函数什么也不做。
 *
 * ### 实现
 *
 * 注意别 double free！另外，**非常非常**仔细地想一想你持有哪些资源、以及释放这些资源的顺序。
 * 顺序搞错的话，后面很可能会挂掉某个 Gradescope 测试。
 * 在某个特定的场景下，你也许还需要去拿缓冲池管理器的 latch……
 */
void ReadPageGuard::Drop() {
  if (!is_valid_) {
    return;
  }

  is_valid_ = false;

  frame_->rwlatch_.unlock_shared();

  {
    std::scoped_lock lock(*bpm_latch_);
    frame_->pin_count_--;
    if (frame_->pin_count_ == 0) {
      replacer_->SetEvictable(frame_->frame_id_, true);
    }
  }
}

/** @brief `ReadPageGuard` 的析构函数，只是简单地调用 `Drop()`。 */
ReadPageGuard::~ReadPageGuard() { Drop(); }

/**********************************************************************************************************************/
/**********************************************************************************************************************/
/**********************************************************************************************************************/

/**
 * @brief `WritePageGuard` 唯一的构造函数，用来创建一个有效的 guard。
 *
 * 注意只有缓冲池管理器才能调用这个构造函数。
 *
 *
 * @param page_id 要写的页的 ID。
 * @param frame 指向存放该页的 frame 的共享指针。
 * @param replacer 指向缓冲池管理器 replacer 的共享指针。
 * @param bpm_latch 指向缓冲池管理器 latch 的共享指针。
 * @param disk_scheduler 指向缓冲池管理器磁盘调度器的共享指针。
 */
WritePageGuard::WritePageGuard(page_id_t page_id, std::shared_ptr<FrameHeader> frame,
                               std::shared_ptr<ArcReplacer> replacer, std::shared_ptr<std::mutex> bpm_latch,
                               std::shared_ptr<DiskScheduler> disk_scheduler)
    : page_id_(page_id),
      frame_(std::move(frame)),
      replacer_(std::move(replacer)),
      bpm_latch_(std::move(bpm_latch)),
      disk_scheduler_(std::move(disk_scheduler)) {
  is_valid_ = true;
  frame_->is_dirty_ = true;
}

/**
 * @brief WritePageGuard 的移动构造函数。
 *
 * ### 实现
 *
 * 如果你对移动语义还不熟，请先上网看学习资料。有很多很好的资源（文章、微软教程、YouTube 视频）
 * 深入地讲解了这件事。
 *
 * 一定要把另一个 guard 置为无效，否则会碰到 double free 问题！两个对象各自都**至少**要更新 5 个字段。
 *
 *
 * @param that 另一个 page guard。
 */
WritePageGuard::WritePageGuard(WritePageGuard &&that) noexcept
    : page_id_(that.page_id_),
      frame_(std::move(that.frame_)),
      replacer_(std::move(that.replacer_)),
      bpm_latch_(std::move(that.bpm_latch_)),
      disk_scheduler_(std::move(that.disk_scheduler_)),
      is_valid_(that.is_valid_) {
  that.is_valid_ = false;
}

/**
 * @brief WritePageGuard 的移动赋值运算符。
 *
 * ### 实现
 *
 * 如果你对移动语义还不熟，请先上网看学习资料。有很多很好的资源（文章、微软教程、YouTube 视频）
 * 深入地讲解了这件事。
 *
 * 一定要把另一个 guard 置为无效，否则会碰到 double free 问题！两个对象各自都**至少**要更新 5 个字段；
 * 对当前对象来说，还要确保释放掉它原本可能持有的资源。
 *
 *
 * @param that 另一个 page guard。
 * @return WritePageGuard& 新的有效 `WritePageGuard`。
 */
auto WritePageGuard::operator=(WritePageGuard &&that) noexcept -> WritePageGuard & {
  if (this == &that) {
    return *this;
  }

  Drop();

  page_id_ = that.page_id_;
  frame_ = std::move(that.frame_);
  replacer_ = std::move(that.replacer_);
  bpm_latch_ = std::move(that.bpm_latch_);
  disk_scheduler_ = std::move(that.disk_scheduler_);
  is_valid_ = that.is_valid_;

  that.is_valid_ = false;

  return *this;
}

/**
 * @brief 获取本 guard 所保护的页的 ID。
 */
auto WritePageGuard::GetPageId() const -> page_id_t {
  BUSTUB_ENSURE(is_valid_, "tried to use an invalid write guard");
  return page_id_;
}

/**
 * @brief 获取一个 `const` 指针，指向本 guard 所保护的页数据。
 */
auto WritePageGuard::GetData() const -> const char * {
  BUSTUB_ENSURE(is_valid_, "tried to use an invalid write guard");
  return frame_->GetData();
}

/**
 * @brief 获取一个可写指针，指向本 guard 所保护的页数据。
 */
auto WritePageGuard::GetDataMut() -> char * {
  BUSTUB_ENSURE(is_valid_, "tried to use an invalid write guard");
  frame_->is_dirty_ = true;
  return frame_->GetDataMut();
}

/**
 * @brief 返回这个页是否是脏的（被修改过但还没写回磁盘）。
 */
auto WritePageGuard::IsDirty() const -> bool {
  BUSTUB_ENSURE(is_valid_, "tried to use an invalid write guard");
  return frame_->is_dirty_;
}

/**
 * @brief 安全地把本页数据写回磁盘。
 */
void WritePageGuard::Flush() {
  BUSTUB_ENSURE(is_valid_, "tried to use an invalid write guard");

  auto promise = disk_scheduler_->CreatePromise();
  auto future = promise.get_future();

  std::vector<DiskRequest> requests;
  requests.push_back({true, frame_->GetDataMut(), page_id_, std::move(promise)});

  disk_scheduler_->Schedule(requests);
  future.get();

  frame_->is_dirty_ = false;
}

/**
 * @brief 手动释放一个有效的 `WritePageGuard` 所持有的数据。如果本 guard 无效，本函数什么也不做。
 *
 * ### 实现
 *
 * 注意别 double free！另外，**非常非常**仔细地想一想你持有哪些资源、以及释放这些资源的顺序。
 * 顺序搞错的话，后面很可能会挂掉某个 Gradescope 测试。
 * 在某个特定的场景下，你也许还需要去拿缓冲池管理器的 latch……
 */
void WritePageGuard::Drop() {
  if (!is_valid_) {
    return;
  }

  is_valid_ = false;

  frame_->rwlatch_.unlock();

  {
    std::scoped_lock lock(*bpm_latch_);
    frame_->pin_count_--;
    if (frame_->pin_count_ == 0) {
      replacer_->SetEvictable(frame_->frame_id_, true);
    }
  }
}

/** @brief `WritePageGuard` 的析构函数，只是简单地调用 `Drop()`。 */
WritePageGuard::~WritePageGuard() { Drop(); }

}  // namespace bustub
