//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// page_guard.h
//
// Identification: src/include/storage/page/page_guard.h
//
// Copyright (c) 2015-2025, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#pragma once

#include <memory>

#include "buffer/arc_replacer.h"
#include "buffer/buffer_pool_manager.h"
#include "storage/disk/disk_scheduler.h"
#include "storage/page/page.h"

namespace bustub {

class BufferPoolManager;
class FrameHeader;

/**
 * @brief 一个 RAII 对象，授予对某个页数据的线程安全**读**访问。
 *
 * BusTub 系统与缓冲池页数据打交道的**唯一**途径就是 page guard。因为 `ReadPageGuard` 是 RAII 对象，
 * 系统永远不需要手动给页加锁 / 解锁。
 *
 * 多个 `ReadPageGuard` 可以让多个线程共享同一个页的读访问。反过来说，只要某个页上存在任何
 * `ReadPageGuard`，就没有线程能修改这个页的数据。
 */
class ReadPageGuard {
  /** @brief 只有缓冲池管理器才能构造出有效的 `ReadPageGuard`。 */
  friend class BufferPoolManager;

 public:
  /**
   * @brief `ReadPageGuard` 的默认构造函数。
   *
   * 注意：我们**永远**不希望使用一个只经过默认构造的 guard。定义这个默认构造函数的唯一原因，
   * 是让我们能在栈上放一个「未初始化」的 guard，之后再用 `=` 移动赋值给它。
   *
   * **使用未初始化的 page guard 是未定义行为。**
   *
   * 换句话说，拿到有效 `ReadPageGuard` 的唯一途径就是通过缓冲池管理器。
   */
  ReadPageGuard() = default;

  ReadPageGuard(const ReadPageGuard &) = delete;
  auto operator=(const ReadPageGuard &) -> ReadPageGuard & = delete;
  ReadPageGuard(ReadPageGuard &&that) noexcept;
  auto operator=(ReadPageGuard &&that) noexcept -> ReadPageGuard &;
  auto GetPageId() const -> page_id_t;
  auto GetData() const -> const char *;
  template <class T>
  auto As() const -> const T * {
    return reinterpret_cast<const T *>(GetData());
  }
  auto IsDirty() const -> bool;
  void Flush();
  void Drop();
  ~ReadPageGuard();

 private:
  /** @brief 只有缓冲池管理器才能构造出有效的 `ReadPageGuard`。 */
  explicit ReadPageGuard(page_id_t page_id, std::shared_ptr<FrameHeader> frame, std::shared_ptr<ArcReplacer> replacer,
                         std::shared_ptr<std::mutex> bpm_latch, std::shared_ptr<DiskScheduler> disk_scheduler);

  /** @brief 本 guard 保护的那个页的 ID。 */
  page_id_t page_id_;

  /**
   * @brief 存放本 guard 所保护页的那个 frame。
   *
   * 这个 page guard 的几乎所有操作都应该通过这个指向 `FrameHeader` 的共享指针完成。
   */
  std::shared_ptr<FrameHeader> frame_;

  /**
   * @brief 指向缓冲池 replacer 的共享指针。
   *
   * 因为缓冲池无法知道这个 `ReadPageGuard` 什么时候被析构，所以我们自己持有一个指向缓冲池 replacer 的指针，
   * 以便在析构时把这个 frame 重新设为可淘汰。
   */
  std::shared_ptr<ArcReplacer> replacer_;

  /**
   * @brief 指向缓冲池 latch 的共享指针。
   *
   * 因为缓冲池无法知道这个 `ReadPageGuard` 什么时候被析构，所以我们自己持有一个指向缓冲池 latch 的指针，
   * 以便在需要更新 frame 在 replacer 里的淘汰状态时使用。
   */
  std::shared_ptr<std::mutex> bpm_latch_;

  /**
   * @brief 指向缓冲池磁盘调度器的共享指针。
   *
   * 在把页写回磁盘时使用。
   */
  std::shared_ptr<DiskScheduler> disk_scheduler_;

  /**
   * @brief `ReadPageGuard` 的有效性标志。
   *
   * 因为我们必须允许构造无效的 page guard（见上面的文档），所以必须维护某种状态来表明本 guard 是否有效。
   * 注意默认构造函数会自动把这个字段设成 `false`。
   *
   * 如果没有这个标志，移动构造 / 移动赋值运算符就可能去析构或 `Drop()` 无效的成员，导致段错误。
   */
  bool is_valid_{false};

  /**
   * TODO(P1): 你可以在这下面添加任何你认为必要的字段。
   *
   * 如果你想拿点（并不存在的）风格分、想玩得花一点，可以去研究 `std::shared_lock` 类型，
   * 用它来做加锁机制，而不是手动调用 `lock` 和 `unlock`。
   */
};

/**
 * @brief 一个 RAII 对象，授予对某个页数据的线程安全**写**访问。
 *
 * BusTub 系统与缓冲池页数据打交道的**唯一**途径就是 page guard。因为 `WritePageGuard` 是 RAII 对象，
 * 系统永远不需要手动给页加锁 / 解锁。
 *
 * 持有 `WritePageGuard` 时，只有一个线程能独占这个页的数据。也就是说 `WritePageGuard` 的持有者
 * 可以随意修改页数据。反过来说，只要某个页上存在 `WritePageGuard`，同一时刻就不可能有别的
 * `WritePageGuard` 或任何 `ReadPageGuard` 存在于这个页上。
 */
class WritePageGuard {
  /** @brief 只有缓冲池管理器才能构造出有效的 `WritePageGuard`。 */
  friend class BufferPoolManager;

 public:
  /**
   * @brief `WritePageGuard` 的默认构造函数。
   *
   * 注意：我们**永远**不希望使用一个只经过默认构造的 guard。定义这个默认构造函数的唯一原因，
   * 是让我们能在栈上放一个「未初始化」的 guard，之后再用 `=` 移动赋值给它。
   *
   * **使用未初始化的 page guard 是未定义行为。**
   *
   * 换句话说，拿到有效 `WritePageGuard` 的唯一途径就是通过缓冲池管理器。
   */
  WritePageGuard() = default;

  WritePageGuard(const WritePageGuard &) = delete;
  auto operator=(const WritePageGuard &) -> WritePageGuard & = delete;
  WritePageGuard(WritePageGuard &&that) noexcept;
  auto operator=(WritePageGuard &&that) noexcept -> WritePageGuard &;
  auto GetPageId() const -> page_id_t;
  auto GetData() const -> const char *;
  template <class T>
  auto As() const -> const T * {
    return reinterpret_cast<const T *>(GetData());
  }
  auto GetDataMut() -> char *;
  template <class T>
  auto AsMut() -> T * {
    return reinterpret_cast<T *>(GetDataMut());
  }
  auto IsDirty() const -> bool;
  void Flush();
  void Drop();
  ~WritePageGuard();

 private:
  /** @brief 只有缓冲池管理器才能构造出有效的 `WritePageGuard`。 */
  explicit WritePageGuard(page_id_t page_id, std::shared_ptr<FrameHeader> frame, std::shared_ptr<ArcReplacer> replacer,
                          std::shared_ptr<std::mutex> bpm_latch, std::shared_ptr<DiskScheduler> disk_scheduler);

  /** @brief 本 guard 保护的那个页的 ID。 */
  page_id_t page_id_;

  /**
   * @brief 存放本 guard 所保护页的那个 frame。
   *
   * 这个 page guard 的几乎所有操作都应该通过这个指向 `FrameHeader` 的共享指针完成。
   */
  std::shared_ptr<FrameHeader> frame_;

  /**
   * @brief 指向缓冲池 replacer 的共享指针。
   *
   * 因为缓冲池无法知道这个 `WritePageGuard` 什么时候被析构，所以我们自己持有一个指向缓冲池 replacer 的指针，
   * 以便在析构时把这个 frame 重新设为可淘汰。
   */
  std::shared_ptr<ArcReplacer> replacer_;

  /**
   * @brief 指向缓冲池 latch 的共享指针。
   *
   * 因为缓冲池无法知道这个 `WritePageGuard` 什么时候被析构，所以我们自己持有一个指向缓冲池 latch 的指针，
   * 以便在需要更新 frame 在 replacer 里的淘汰状态时使用。
   */
  std::shared_ptr<std::mutex> bpm_latch_;

  /**
   * @brief 指向缓冲池磁盘调度器的共享指针。
   *
   * 在把页写回磁盘时使用。
   */
  std::shared_ptr<DiskScheduler> disk_scheduler_;

  /**
   * @brief `WritePageGuard` 的有效性标志。
   *
   * 因为我们必须允许构造无效的 page guard（见上面的文档），所以必须维护某种状态来表明本 guard 是否有效。
   * 注意默认构造函数会自动把这个字段设成 `false`。
   *
   * 如果没有这个标志，移动构造 / 移动赋值运算符就可能去析构或 `Drop()` 无效的成员，导致段错误。
   */
  bool is_valid_{false};

  /**
   * TODO(P1): 你可以在这下面添加任何你认为必要的字段。
   *
   * 如果你想拿点（并不存在的）风格分、想玩得花一点，可以去研究 `std::unique_lock` 类型，
   * 用它来做加锁机制，而不是手动调用 `lock` 和 `unlock`。
   */
};

}  // namespace bustub
