//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// buffer_pool_manager.h
//
// Identification: src/include/buffer/buffer_pool_manager.h
//
// Copyright (c) 2015-2025, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#pragma once

#include <list>
#include <memory>
#include <optional>
#include <shared_mutex>
#include <unordered_map>
#include <vector>

#include "buffer/arc_replacer.h"
#include "common/config.h"
#include "recovery/log_manager.h"
#include "storage/disk/disk_scheduler.h"
#include "storage/page/page.h"
#include "storage/page/page_guard.h"

namespace bustub {

class BufferPoolManager;
class ReadPageGuard;
class WritePageGuard;

/**
 * @brief `BufferPoolManager` 的辅助类，负责管理一个内存 frame 及其相关的元数据。
 *
 * 本类表示 `BufferPoolManager` 用来存放页数据的那些内存 frame 的头部信息。
 * 注意：真正的 frame 内存并不直接存放在 `FrameHeader` 内部，`FrameHeader` 只是
 * 持有指向该 frame 的指针，两者是分开存储的。
 *
 * ---
 *
 * 你可能会（也可能不会）好奇：为什么字段 `data_` 是用 `std::vector` 在堆上动态
 * 分配的，而不是直接指向一块预先分配好的内存？
 *
 * 在传统的生产级缓冲池管理器中，缓冲池要管理的全部内存会先一次性分配成一大块
 * 连续数组（想象一次申请好几个 GB 的 `malloc`）。这块大内存随后被切分成若干个
 * 连续的 frame。换句话说，frame 由它相对数组基址的偏移来定义，每个间隔一个页
 * 大小（4 KB）。
 *
 * BusTub 的做法是让每个 frame 各自独立分配（通过 `std::vector<char>`），这样
 * 配合 AddressSanitizer 就能很容易地检测到缓冲区溢出。因为 C++ 本身没有任何
 * 内存安全的概念，如果所有页的数据都紧挨着放，那么把一个页的数据指针强转成某个
 * 大类型再往上写，就会悄无声息地覆盖掉别的页的数据。
 *
 * 如果你想为自己的缓冲池管理器尝试更高效的数据结构，完全可以。不过在后续的项目
 * 里（尤其是项目 2），能够检测到缓冲区溢出会给你带来巨大的好处。
 */
class FrameHeader {
  friend class BufferPoolManager;
  friend class ReadPageGuard;
  friend class WritePageGuard;

 public:
  explicit FrameHeader(frame_id_t frame_id);

 private:
  auto GetData() const -> const char *;
  auto GetDataMut() -> char *;
  void Reset();

  /** @brief 本头部所代表的 frame 的 ID / 下标。 */
  const frame_id_t frame_id_;

  /** @brief 本 frame 的读写锁。 */
  std::shared_mutex rwlatch_;

  /** @brief 本 frame 上的 pin 计数，用于把页保持在内存中。 */
  std::atomic<size_t> pin_count_;

  /** @brief 脏标记。 */
  bool is_dirty_;

  /**
   * @brief 指向本 frame 所持有的页数据的指针。
   *
   * 如果本 frame 没有存放任何页数据，则它的内容全是空字节。
   */
  std::vector<char> data_;

  /**
   * TODO(P1): 你可以在这里添加任何你认为必要的字段或辅助函数。
   *
   * 一个可行的优化是：额外存一个可选的页 ID，表示 `FrameHeader` 当前存放的是哪个页。
   * 这样也许能让你免去在缓冲池管理器别处搜索对应 (页 ID, frame ID) 映射的开销……
   */
  std::optional<page_id_t> page_id_{std::nullopt};
};

/**
 * @brief `BufferPoolManager` 类的声明。
 *
 * 正如实验说明中所写：缓冲池负责在主内存的缓冲区与持久化存储之间来回搬运物理页数据。
 * 它同时也是一个缓存 —— 把热点页留在内存里以便快速访问，把不再使用的冷页淘汰回存储。
 *
 * 在动手实现缓冲池管理器之前，请务必把实验说明完整读一遍。此外，你还需要先完成
 * `ArcReplacer` 和 `DiskManager` 两个类的实现。
 */
class BufferPoolManager {
 public:
  BufferPoolManager(size_t num_frames, DiskManager *disk_manager, LogManager *log_manager = nullptr);
  ~BufferPoolManager();

  auto Size() const -> size_t;
  auto NewPage() -> page_id_t;
  auto DeletePage(page_id_t page_id) -> bool;
  auto CheckedWritePage(page_id_t page_id, AccessType access_type = AccessType::Unknown)
      -> std::optional<WritePageGuard>;
  auto CheckedReadPage(page_id_t page_id, AccessType access_type = AccessType::Unknown) -> std::optional<ReadPageGuard>;
  auto WritePage(page_id_t page_id, AccessType access_type = AccessType::Unknown) -> WritePageGuard;
  auto ReadPage(page_id_t page_id, AccessType access_type = AccessType::Unknown) -> ReadPageGuard;
  auto FlushPageUnsafe(page_id_t page_id) -> bool;
  auto FlushPage(page_id_t page_id) -> bool;
  void FlushAllPagesUnsafe();
  void FlushAllPages();
  auto GetPinCount(page_id_t page_id) -> std::optional<size_t>;

 private:
  /** @brief 缓冲池中 frame 的数量。 */
  const size_t num_frames_;

  /** @brief 下一个将要分配的页 ID。 */
  std::atomic<page_id_t> next_page_id_;

  /**
   * @brief 保护缓冲池内部数据结构的锁。
   *
   * TODO(P1) 建议把这条注释替换成你自己对这个锁究竟保护了哪些内容的说明。
   */
  std::shared_ptr<std::mutex> bpm_latch_;

  /** @brief 本缓冲池所管理的各个 frame 的头部。 */
  std::vector<std::shared_ptr<FrameHeader>> frames_;

  /** @brief 页表，记录页与缓冲池 frame 之间的映射关系。 */
  std::unordered_map<page_id_t, frame_id_t> page_table_;

  /** @brief 一个空闲 frame 列表，其中不存放任何页的数据。 */
  std::list<frame_id_t> free_frames_;

  /** @brief 用于找出未被 pin 住、可被淘汰的候选页的替换器。 */
  std::shared_ptr<ArcReplacer> replacer_;

  /** @brief 指向磁盘调度器的指针。与 page guard 共享，用于刷盘。 */
  std::shared_ptr<DiskScheduler> disk_scheduler_;

  /**
   * @brief 指向日志管理器的指针。
   *
   * 注意：P1 请忽略它。
   */
  LogManager *log_manager_ __attribute__((__unused__));

  /**
   * TODO(P1): 如果你觉得有必要，可以添加额外的私有成员和辅助函数。
   *
   * 访问同一个页的不同方式之间，很可能存在大量重复代码。
   *
   * 我们建议你实现一个辅助函数：返回一个空闲的、里面什么都没存的 frame 的 ID。
   * 此外，你可能还想实现另一个辅助函数：它要么返回一个已经存有某页数据的
   * `FrameHeader` 的共享指针，要么返回该 `FrameHeader` 的下标。
   */

  auto GetUsableFrame() -> std::optional<frame_id_t>;

  auto FetchFrame(page_id_t page_id, AccessType access_type) -> std::shared_ptr<FrameHeader>;
  /**
   * @brief 把一个 frame 的数据写回磁盘，并在写完后清除脏标记。
   *
   * 如果 frame 不脏，则什么也不做。
   *
   * **调用者必须已经持有 `bpm_latch_`。** 正因为如此，本函数不能调 `FlushPageUnsafe`
   * （那会再拿一次同一把非递归锁，导致死锁）。
   *
   * @param frame 要写回的 frame。
   * @param page_id 该 frame 当前存放的页的 ID。
   */
  void FlushFrameLocked(const std::shared_ptr<FrameHeader> &frame, page_id_t page_id);
};
}  // namespace bustub
