//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// disk_scheduler.h
//
// Identification: src/include/storage/disk/disk_scheduler.h
//
// Copyright (c) 2015-2025, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#pragma once

#include <future>  // NOLINT
#include <optional>
#include <thread>  // NOLINT
#include <vector>

#include "common/channel.h"
#include "storage/disk/disk_manager.h"

namespace bustub {

/**
 * @brief 表示一个交给 `DiskManager` 执行的读请求或写请求。
 */
struct DiskRequest {
  /** 标志位：这个请求是写（`true`）还是读（`false`）。 */
  bool is_write_;

  /**
   * 指向一块内存的起始地址，页数据就在那里：
   *   1. 读请求：从磁盘读出来的数据写进这块内存。
   *   2. 写请求：这块内存里的数据被写到磁盘上。
   */
  char *data_;

  /** 要读 / 写的页的 ID。 */
  page_id_t page_id_;

  /** 回调，用来在请求完成后通知发起请求的一方。 */
  std::promise<bool> callback_;
};

/**
 * @brief `DiskScheduler` 负责调度磁盘的读写操作。
 *
 * 调用 `DiskScheduler::Schedule()` 并传入合适的 `DiskRequest` 对象即可提交一个请求。调度器维护一个后台工作线程，
 * 由它使用磁盘管理器处理已提交的请求。后台线程在 `DiskScheduler` 构造函数里创建，在析构函数里 join。
 */
class DiskScheduler {
 public:
  explicit DiskScheduler(DiskManager *disk_manager);
  ~DiskScheduler();

  void Schedule(std::vector<DiskRequest> &requests);

  void StartWorkerThread();

  using DiskSchedulerPromise = std::promise<bool>;

  /**
   * @brief 创建一个 promise 对象。如果你想实现自己版本的 promise，可以改这个函数，
   * 这样我们的测试用例就能用上你的实现。
   *
   * @return std::promise<bool>
   */
  auto CreatePromise() -> DiskSchedulerPromise { return {}; };

  /**
   * @brief 释放磁盘上的一个页。
   *
   * 注意：使用本方法之前，请先看 `BufferPoolManager` 里 `DeletePage` 的文档。
   *
   * @param page_id 要从磁盘上释放的页的 ID。
   */
  void DeallocatePage(page_id_t page_id) { disk_manager_->DeletePage(page_id); }

 private:
  /** 指向磁盘管理器的指针。 */
  DiskManager *disk_manager_ __attribute__((__unused__));
  /** 一个共享队列，用于并发地提交和处理请求。`DiskScheduler` 析构时会往队列里放一个
   * `std::nullopt`，通知后台线程停止运行。 */
  Channel<std::optional<DiskRequest>> request_queue_;
  /** 后台线程，负责把已提交的请求交给磁盘管理器执行。 */
  std::optional<std::thread> background_thread_;
};
}  // namespace bustub
