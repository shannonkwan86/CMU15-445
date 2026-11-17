//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// disk_scheduler.cpp
//
// Identification: src/storage/disk/disk_scheduler.cpp
//
// Copyright (c) 2015-2025, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#include "storage/disk/disk_scheduler.h"
#include <vector>
#include "common/macros.h"
#include "storage/disk/disk_manager.h"

namespace bustub {

DiskScheduler::DiskScheduler(DiskManager *disk_manager) : disk_manager_(disk_manager) {
  // 启动后台线程
  background_thread_.emplace([&] { StartWorkerThread(); });
}

DiskScheduler::~DiskScheduler() {
  // 往队列里放一个 `std::nullopt`，通知后台线程退出循环
  request_queue_.Put(std::nullopt);
  if (background_thread_.has_value()) {
    background_thread_->join();
  }
}

/**
 * @brief 提交一批请求，交给 `DiskManager` 执行。
 *
 * @param requests 要提交的请求。
 */
void DiskScheduler::Schedule(std::vector<DiskRequest> &requests) {
  for (auto &req : requests) {
    request_queue_.Put(std::move(req));
  }
}

/**
 * @brief 后台工作线程的函数，负责处理已提交的请求。
 *
 * 只要 `DiskScheduler` 还存在，后台线程就要一直处理请求 —— 也就是说本函数不应该返回，
 * 直到 `~DiskScheduler()` 被调用。到那时你必须保证本函数确实能返回。
 */
void DiskScheduler::StartWorkerThread() {
  while (true) {
    auto req = request_queue_.Get();

    if (!req.has_value()) {
      return;
    }
    if (req->is_write_) {
      disk_manager_->WritePage(req->page_id_, req->data_);
    } else {
      disk_manager_->ReadPage(req->page_id_, req->data_);
    }
    req->callback_.set_value(true);
  }
}

}  // namespace bustub
