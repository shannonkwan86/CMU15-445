//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// buffer_pool_manager.cpp
//
// Identification: src/buffer/buffer_pool_manager.cpp
//
// Copyright (c) 2015-2025, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#include "buffer/buffer_pool_manager.h"
#include "buffer/arc_replacer.h"
#include "common/config.h"
#include "common/macros.h"
namespace bustub {

/**
 * @brief `FrameHeader` 的构造函数，把所有字段都初始化为默认值。
 *
 * 更多信息见 "buffer/buffer_pool_manager.h" 里 `FrameHeader` 的文档。
 *
 * @param frame_id 我们要为其创建 header 的 frame 的 ID / 下标。
 */
FrameHeader::FrameHeader(frame_id_t frame_id) : frame_id_(frame_id), data_(BUSTUB_PAGE_SIZE, 0) { Reset(); }

/**
 * @brief 获取一个指向 frame 数据的裸 `const` 指针。
 *
 * @return const char* 指向 frame 所存数据的不可变指针。
 */
auto FrameHeader::GetData() const -> const char * { return data_.data(); }

/**
 * @brief 获取一个指向 frame 数据的裸可写指针。
 *
 * @return char* 指向 frame 所存数据的可写指针。
 */
auto FrameHeader::GetDataMut() -> char * { return data_.data(); }

/**
 * @brief 重置 `FrameHeader` 的各个成员字段。
 */
void FrameHeader::Reset() {
  std::fill(data_.begin(), data_.end(), 0);
  pin_count_.store(0);
  is_dirty_ = false;
  page_id_ = std::nullopt;
  state_ = FrameState::EMPTY;
}

/**
 * @brief 创建一个新的 `BufferPoolManager` 实例并初始化所有字段。
 *
 * 更多信息见 "buffer/buffer_pool_manager.h" 里 `BufferPoolManager` 的文档。
 *
 * ### 实现
 *
 * 我们已经按参考解的思路把构造函数实现好了。如果它和你的实现不搭，你可以随意修改。
 *
 * 不过要提醒一句：如果你偏离我们的引导太远，我们帮你会难得多。我们的建议是先用我们提供的
 * 垫脚石把缓冲池管理器实现出来。
 *
 * 等你有了一个完全能跑的实现（所有 Gradescope 测试用例都通过），再去玩点更有意思的东西！
 *
 * @param num_frames 缓冲池的大小。
 * @param disk_manager 磁盘管理器。
 * @param log_manager 日志管理器。P1 阶段请忽略它。
 */
BufferPoolManager::BufferPoolManager(size_t num_frames, DiskManager *disk_manager, LogManager *log_manager)
    : num_frames_(num_frames),
      next_page_id_(0),
      bpm_latch_(std::make_shared<std::mutex>()),
      replacer_(std::make_shared<ArcReplacer>(num_frames)),
      disk_scheduler_(std::make_shared<DiskScheduler>(disk_manager)),
      log_manager_(log_manager) {
  // 严格来说不是必须的……
  std::scoped_lock latch(*bpm_latch_);

  // 把单调递增的计数器初始化成 0。
  next_page_id_.store(0);

  // 一次性把所有内存中的 frame 都分配好。
  frames_.reserve(num_frames_);

  // 页表应该正好有 `num_frames_` 个槽位，一一对应 `num_frames_` 个 frame。
  page_table_.reserve(num_frames_);

  // 初始化所有 frame header，并用所有可能的 frame ID 填满空闲 frame 链表
  // （因为一开始所有 frame 都是空闲的）。
  for (size_t i = 0; i < num_frames_; i++) {
    frames_.push_back(std::make_shared<FrameHeader>(i));
    free_frames_.push_back(static_cast<int>(i));
  }
}

/**
 * @brief 销毁 `BufferPoolManager`，释放缓冲池用过的所有内存。
 */
BufferPoolManager::~BufferPoolManager() = default;

/**
 * @brief 返回这个缓冲池管理多少个 frame。
 */
auto BufferPoolManager::Size() const -> size_t { return num_frames_; }

/**
 * @brief 取一个可用的 frame，把它的 `pin_count_` 置 1 作为占位。
 *
 * 返回时帧处于 `EMPTY` 状态、`pin_count_ == 1`、不在 `page_table_` 里。
 * 如果这个帧是从替换器淘汰来的，脏页的写回已经在本函数内部**在锁外**完成。
 *
 * **调用者不得持有 `bpm_latch_`。** 本函数内部自行加锁，返回时已释放。
 */
auto BufferPoolManager::AcquireUsableFrame() -> std::optional<frame_id_t> {
  std::unique_lock lock(*bpm_latch_);

  // 情况 1：还有空闲帧，直接拿走。空闲帧不在页表、不在替换器里，没人能看见它。
  if (!free_frames_.empty()) {
    auto fid = free_frames_.back();
    free_frames_.pop_back();
    frames_[fid]->pin_count_ = 1;  // 占位：I/O 期间不许别人动它
    return fid;
  }

  // 情况 2：淘汰一个 victim。`Evict()` 内部已经把它从存活链表和 `alive_map_` 里摘除，
  // 所以即使放锁，别的线程也不会再选中它。
  auto victim = replacer_->Evict();
  if (!victim.has_value()) {
    return std::nullopt;  // 所有 frame 都被 pin 住了
  }

  auto fid = victim.value();
  auto frame = frames_[fid];
  auto old_pid = frame->page_id_.value();

  // 占位 + 标记「正在写回」。
  // 注意：此时页表里旧页的映射**必须保留**。若提前摘掉，别的线程访问旧页会重新从磁盘
  // 加载，而磁盘上还是旧版本 —— 等于丢更新。他们应该看到 `FLUSHING` 然后睡等。
  frame->pin_count_ = 1;
  frame->state_ = FrameState::FLUSHING;
  const bool need_flush = frame->is_dirty_;

  lock.unlock();
  if (need_flush) {
    auto promise = disk_scheduler_->CreatePromise();
    auto future = promise.get_future();
    std::vector<DiskRequest> requests;
    requests.push_back({true, frame->GetDataMut(), old_pid, std::move(promise)});
    disk_scheduler_->Schedule(requests);
    future.get();
  }
  lock.lock();

  // 收尾：现在才摘映射、清脏、把帧变回 `EMPTY`。
  page_table_.erase(old_pid);
  frame->is_dirty_ = false;
  frame->page_id_ = std::nullopt;
  frame->state_ = FrameState::EMPTY;
  // 唤醒在等旧页的人：他们醒来会重查页表，发现已经不在内存，于是从磁盘重新加载。
  cv_.notify_all();

  // `pin_count_` 保持 1，作为调用者的占位。
  return fid;
}

/**
 * @brief 把一个未被使用的帧还回 `free_frames_`。
 *
 * 用于「取到帧之后发现目标页已被别人加载」的路径。
 */
void BufferPoolManager::ReleaseUsableFrame(frame_id_t frame_id) {
  auto frame = frames_[frame_id];
  frame->pin_count_ = 0;
  frame->state_ = FrameState::EMPTY;
  frame->page_id_ = std::nullopt;
  free_frames_.push_back(frame_id);
}

/**
 * @brief 把指定的页弄进内存并 pin 住，返回承载它的 frame。
 *
 * 命中页表时只做 pin；未命中时先找可用 frame，再从磁盘读入。
 *
 * **磁盘 I/O 在 `bpm_latch_` 之外完成。** 靠一个显式状态机保证并发正确性：
 * 放锁之前先在页表里登记「页 X 在帧 F，`LOADING`」并 pin 住 F；别人看到 `LOADING`
 * 就在 `cv_` 上睡等，醒来后**重新查表**（不能复用等待前的记忆）。
 *
 * **调用者不得持有 `bpm_latch_`。** 本函数内部自行加锁，返回时已释放。
 *
 * 返回时 frame 的状态：`VALID`、`pin_count_` 已加 1、已设为不可淘汰、已向替换器
 * 记录本次访问。**页锁尚未加**，由调用者负责。
 *
 * @return 成功则返回该 frame；内存耗尽（所有 frame 都被 pin 住）返回 `nullptr`。
 */
auto BufferPoolManager::FetchFrame(page_id_t page_id, AccessType access_type) -> std::shared_ptr<FrameHeader> {
  std::unique_lock lock(*bpm_latch_);

  for (;;) {
    // ── ① 查表 ────────────────────────────────────────────────
    auto it = page_table_.find(page_id);
    if (it != page_table_.end()) {
      auto frame = frames_[it->second];

      if (frame->state_ != FrameState::VALID) {
        // 别人正在把这一页读进来（`LOADING`），或正在把它的旧内容写回去（`FLUSHING`）。
        // 睡到它不再是中间状态为止；醒来后必须重新查表，不能复用这个 frame。
        cv_.wait(lock,
                 [&frame] { return frame->state_ != FrameState::LOADING && frame->state_ != FrameState::FLUSHING; });
        continue;
      }

      // 命中：页已经在内存里，只需要 pin 住并通知替换器。
      frame->pin_count_++;
      replacer_->RecordAccess(frame->frame_id_, page_id, access_type);
      replacer_->SetEvictable(frame->frame_id_, false);
      return frame;
    }

    // ── ② 未命中：取一个可用 frame（可能要先写回脏页，所以必须在锁外做）──
    lock.unlock();
    auto fid_opt = AcquireUsableFrame();
    lock.lock();

    if (!fid_opt.has_value()) {
      return nullptr;  // 内存耗尽
    }

    auto fid = fid_opt.value();
    auto frame = frames_[fid];

    // 取帧的过程中放过锁，别的线程可能已经把这一页加载好了。必须重查，
    // 否则同一页会被登记两次（两份副本 + 页表被覆盖 + 孤儿帧）。
    if (page_table_.find(page_id) != page_table_.end()) {
      ReleaseUsableFrame(fid);
      continue;
    }

    // ── ③ 放锁之前先登记：映射 + `LOADING` + pin 占位 ──────────
    page_table_[page_id] = fid;
    frame->page_id_ = page_id;
    frame->state_ = FrameState::LOADING;
    frame->pin_count_ = 1;

    // ── ④ 放锁读盘 ──────────────────────────────────────────
    lock.unlock();
    auto promise = disk_scheduler_->CreatePromise();
    auto future = promise.get_future();
    std::vector<DiskRequest> requests;
    requests.push_back({false, frame->GetDataMut(), page_id, std::move(promise)});
    disk_scheduler_->Schedule(requests);
    future.get();
    lock.lock();

    // ── ⑤ 收尾 + 唤醒等待者 ──────────────────────────────────
    // 注：`DiskScheduler` 的工作线程没有捕获异常，读盘失败会直接终止进程，所以这里
    // 不需要「撤映射 + 唤醒等待者」的恢复路径。若将来改成可恢复，必须在这里
    // `page_table_.erase(page_id)` 并 `cv_.notify_all()`。
    frame->state_ = FrameState::VALID;
    replacer_->RecordAccess(fid, page_id, access_type);
    replacer_->SetEvictable(fid, false);
    cv_.notify_all();

    return frame;
  }
}

/**
 * @brief 在磁盘上分配一个新页。
 *
 * ### 实现
 *
 * 你需要维护一个线程安全、单调递增的计数器，类型是 `std::atomic<page_id_t>`。
 * 详情见 [atomics](https://en.cppreference.com/w/cpp/atomic/atomic) 的文档。
 *
 * @return 新分配出来的页的 ID。
 */
auto BufferPoolManager::NewPage() -> page_id_t { return next_page_id_.fetch_add(1); }

/**
 * @brief 从数据库里删掉一个页，磁盘上和内存里都要删。
 *
 * 如果这个页在缓冲池里被 pin 住了，本函数什么也不做，返回 `false`。否则本函数把这个页
 * 从磁盘和内存里都删掉（如果它还在缓冲池里），返回 `true`。
 *
 * ### 实现
 *
 * 想一想一个页、以及这个页的元数据都可能存在于哪些地方，用这个来指导你的实现。
 * 你大概会想先实现完 `CheckedReadPage` 和 `CheckedWritePage`，再来实现本函数。
 *
 * 你应该调用磁盘调度器的 `DeallocatePage`，把空间腾出来给新页用。
 *
 * @param page_id 要删除的页的 ID。
 * @return 页存在但删不掉时返回 `false`；页本来不存在、或删除成功时返回 `true`。
 */
auto BufferPoolManager::DeletePage(page_id_t page_id) -> bool {
  std::scoped_lock lock(*bpm_latch_);
  auto it = page_table_.find(page_id);

  if (it == page_table_.end()) {
    disk_scheduler_->DeallocatePage(page_id);
    return true;
  }

  auto fid = it->second;
  if (frames_[fid]->pin_count_.load() > 0) {
    return false;
  }

  page_table_.erase(page_id);
  replacer_->SetEvictable(fid, true);
  replacer_->Remove(fid);
  frames_[fid]->Reset();
  free_frames_.push_back(fid);
  disk_scheduler_->DeallocatePage(page_id);
  return true;
}

/**
 * @brief 获取一个可选的、加了写锁的 page guard。需要的话用户可以指定 `AccessType`。
 *
 * 如果没法把页数据弄进内存，本函数返回 `std::nullopt`。
 *
 * 页数据**只能**通过 page guard 访问。`BufferPoolManager` 的使用者应该根据自己的访问方式
 * 选择获取 `ReadPageGuard` 还是 `WritePageGuard`，这样才能保证任何数据访问都是线程安全的。
 *
 * 同一时刻只能有 1 个 `WritePageGuard` 在读 / 写某个页。这让数据访问既能只读又能可写：
 * 持有 `WritePageGuard` 的线程可以随意操作页数据。如果用户希望多个线程同时读同一个页，
 * 就必须改用 `CheckedReadPage` 去获取 `ReadPageGuard`。
 *
 * ### 实现
 *
 * 你需要实现三种主要情况。前两种相对简单：一种是有充足的空闲内存，另一种是我们其实
 * 不需要做任何额外的 I/O。想一想这两种情况到底意味着什么。
 *
 * 第三种最麻烦：我们手上没有**现成**可用的内存。缓冲池的任务是用你之前实现的替换算法
 * 找出可以淘汰的候选 frame，从而腾出内存把目标页弄进来。
 *
 * 一旦缓冲池确定了要淘汰哪个 frame，可能还需要若干次 I/O 才能把我们要的页弄进这个 frame。
 *
 * 这里大概率会和 `CheckedReadPage` 有大量重复代码，所以抽几个 helper 函数可能会很方便。
 *
 * 这两个函数是本项目的核心，所以我们不会再给更多提示了。祝你好运！
 *
 * @param page_id 我们要写的页的 ID。
 * @param access_type 页访问的类型。
 * @return std::optional<WritePageGuard> 一个可选的 latch guard：如果没有空闲 frame 了（内存耗尽）
 * 就返回 `std::nullopt`；否则返回一个保证对页数据独占、可写访问的 `WritePageGuard`。
 */
auto BufferPoolManager::CheckedWritePage(page_id_t page_id, AccessType access_type) -> std::optional<WritePageGuard> {
  // 把页弄进内存并 pin 住。`FetchFrame` 返回时已释放 `bpm_latch_`。
  auto frame = FetchFrame(page_id, access_type);
  if (frame == nullptr) {
    return std::nullopt;
  }

  // 页锁必须在 `bpm_latch_` 之外拿：`Drop()` 的顺序是“先放页锁、再拿 bpm_latch_”，
  // 反着来两边会互等。
  frame->rwlatch_.lock();
  return WritePageGuard(page_id, frame, replacer_, bpm_latch_, disk_scheduler_);
}

/**
 * @brief 获取一个可选的、加了读锁的 page guard。需要的话用户可以指定 `AccessType`。
 *
 * 如果没法把页数据弄进内存，本函数返回 `std::nullopt`。
 *
 * 页数据**只能**通过 page guard 访问。`BufferPoolManager` 的使用者应该根据自己的访问方式
 * 选择获取 `ReadPageGuard` 还是 `WritePageGuard`，这样才能保证任何数据访问都是线程安全的。
 *
 * 不同线程可以同时持有任意多个 `ReadPageGuard` 来读同一个页的数据。但所有访问都必须是只读的。
 * 如果用户想修改页数据，就必须改用 `CheckedWritePage` 去获取 `WritePageGuard`。
 *
 * ### 实现
 *
 * 见 `CheckedWritePage` 的实现说明。
 *
 * @param page_id 我们要读的页的 ID。
 * @param access_type 页访问的类型。
 * @return std::optional<ReadPageGuard> 一个可选的 latch guard：如果没有空闲 frame 了（内存耗尽）
 * 就返回 `std::nullopt`；否则返回一个保证对页数据共享、只读访问的 `ReadPageGuard`。
 */
auto BufferPoolManager::CheckedReadPage(page_id_t page_id, AccessType access_type) -> std::optional<ReadPageGuard> {
  // 把页弄进内存并 pin 住。`FetchFrame` 返回时已释放 `bpm_latch_`。
  auto frame = FetchFrame(page_id, access_type);
  if (frame == nullptr) {
    return std::nullopt;
  }

  // 页锁必须在 `bpm_latch_` 之外拿，理由同 `CheckedWritePage`。
  // 读锁是共享的，多个 `ReadPageGuard` 可以同时持有。
  frame->rwlatch_.lock_shared();
  return ReadPageGuard(page_id, frame, replacer_, bpm_latch_, disk_scheduler_);
}

/**
 * @brief `CheckedWritePage` 的包装，存在内层值时把它解出来。
 *
 * 如果 `CheckedWritePage` 返回 `std::nullopt`，**本函数会终止整个进程。**
 *
 * 本函数**只**应该为了测试和顺手而使用。如果缓冲池管理器有可能内存耗尽，请改用
 * `CheckedWritePage`，这样你才能处理那种情况。
 *
 * 实现相关的更多信息见 `CheckedWritePage` 的文档。
 *
 * @param page_id 我们要读的页的 ID。
 * @param access_type 页访问的类型。
 * @return WritePageGuard 一个保证对页数据独占、可写访问的 page guard。
 */
auto BufferPoolManager::WritePage(page_id_t page_id, AccessType access_type) -> WritePageGuard {
  auto guard_opt = CheckedWritePage(page_id, access_type);

  if (!guard_opt.has_value()) {
    fmt::println(stderr, "\n`CheckedWritePage` failed to bring in page {}\n", page_id);
    std::abort();
  }

  return std::move(guard_opt).value();
}

/**
 * @brief `CheckedReadPage` 的包装，存在内层值时把它解出来。
 *
 * 如果 `CheckedReadPage` 返回 `std::nullopt`，**本函数会终止整个进程。**
 *
 * 本函数**只**应该为了测试和顺手而使用。如果缓冲池管理器有可能内存耗尽，请改用
 * `CheckedWritePage`，这样你才能处理那种情况。
 *
 * 实现相关的更多信息见 `CheckedReadPage` 的文档。
 *
 * @param page_id 我们要读的页的 ID。
 * @param access_type 页访问的类型。
 * @return ReadPageGuard 一个保证对页数据共享、只读访问的 page guard。
 */
auto BufferPoolManager::ReadPage(page_id_t page_id, AccessType access_type) -> ReadPageGuard {
  auto guard_opt = CheckedReadPage(page_id, access_type);

  if (!guard_opt.has_value()) {
    fmt::println(stderr, "\n`CheckedReadPage` failed to bring in page {}\n", page_id);
    std::abort();
  }

  return std::move(guard_opt).value();
}

/**
 * @brief 把一个 frame 的数据写回磁盘，并在写完后清除脏标记。
 *
 * 如果 frame 不脏，则什么也不做。
 *
 * **调用者必须已经持有 `bpm_latch_`。**
 */
void BufferPoolManager::FlushFrameLocked(const std::shared_ptr<FrameHeader> &frame, page_id_t page_id) {
  if (!frame->is_dirty_) {
    return;
  }

  auto promise = disk_scheduler_->CreatePromise();
  auto future = promise.get_future();

  std::vector<DiskRequest> requests;
  requests.push_back({true, frame->GetDataMut(), page_id, std::move(promise)});
  disk_scheduler_->Schedule(requests);

  future.get();
  frame->is_dirty_ = false;
}

/**
 * @brief **不安全地**把一个页的数据写回磁盘。
 *
 * 如果页被修改过，本函数会把它的数据写到磁盘上。如果给定的页不在内存里，本函数返回 `false`。
 *
 * 本函数里**不应该**给页加锁。也就是说，你要仔细考虑什么时候去翻转 `is_dirty_` 标志位。
 *
 * ### 实现
 *
 * 你大概会想等 `CheckedReadPage` 和 `CheckedWritePage` 都实现完再来做这个函数，
 * 因为那时候该做什么会清楚得多。
 *
 * @param page_id 要写回的页的 ID。
 * @return 页表里找不到这个页时返回 `false`；否则返回 `true`。
 */
auto BufferPoolManager::FlushPageUnsafe(page_id_t page_id) -> bool {
  std::scoped_lock lock(*bpm_latch_);

  auto it = page_table_.find(page_id);
  if (it == page_table_.end()) {
    return false;
  }

  auto frame = frames_[it->second];

  // 页还在搬（`LOADING` / `FLUSHING`）时 `data_` 不是一份稳定快照。
  // 本函数是「不安全」变体，不引入等待，直接报告失败。
  if (frame->state_ != FrameState::VALID) {
    return false;
  }

  FlushFrameLocked(frame, page_id);
  return true;
}

/**
 * @brief **安全地**把一个页的数据写回磁盘。
 *
 * 如果页被修改过，本函数会把它的数据写到磁盘上。如果给定的页不在内存里，本函数返回 `false`。
 *
 * 本函数里应该给页加锁，以保证写回磁盘的是一份一致的状态。
 *
 * ### 实现
 *
 * 你大概会想等 `CheckedReadPage`、`CheckedWritePage` 以及 page guard 里的 `Flush` 都实现完
 * 再来做这个函数，因为那时候该做什么会清楚得多。
 *
 * @param page_id 要写回的页的 ID。
 * @return 页表里找不到这个页时返回 `false`；否则返回 `true`。
 */
auto BufferPoolManager::FlushPage(page_id_t page_id) -> bool {
  std::shared_ptr<FrameHeader> frame;
  frame_id_t fid = INVALID_FRAME_ID;

  // ── ① 锁内：等到这个页就绪，然后 pin 住它 ──────────────────
  {
    std::unique_lock lock(*bpm_latch_);

    for (;;) {
      auto it = page_table_.find(page_id);
      if (it == page_table_.end()) {
        return false;
      }

      frame = frames_[it->second];
      fid = frame->frame_id_;

      if (frame->state_ != FrameState::VALID) {
        // 页还在搬（`LOADING` / `FLUSHING`）：等它完成。
        // 醒来必须重查：这期间页可能被淘汰，也可能换了别的页。
        cv_.wait(lock, [&] {
          return frame->state_ == FrameState::VALID || page_table_.find(page_id) == page_table_.end() ||
                 page_table_.at(page_id) != fid;
        });
        continue;
      }

      // pin 住，保证出锁后到拿页锁的这段时间里它不会被淘汰。
      frame->pin_count_++;
      replacer_->SetEvictable(fid, false);
      break;
    }
  }

  // ── ② 拿页的写锁，保证写回的是一份稳定快照 ──────────────────
  // 注意锁顺序：先页锁、再 `bpm_latch_`（与 `PageGuard::Drop` 一致），
  // 绝不反过来。拿页锁时不能持有 `bpm_latch_`，否则两边互等。
  std::scoped_lock page_lock(frame->rwlatch_);

  // 拿到页锁之后重新读一次脏标记：上面那次 pin 到这次拿到页锁之间，
  // 可能有写者已经改过这个页并把它置脏了。
  bool dirty = false;
  {
    std::scoped_lock lock(*bpm_latch_);
    dirty = frame->is_dirty_;
  }

  // ── ③ 锁外写回 ──────────────────────────────────────────
  if (dirty) {
    auto promise = disk_scheduler_->CreatePromise();
    auto future = promise.get_future();
    std::vector<DiskRequest> requests;
    requests.push_back({true, frame->GetDataMut(), page_id, std::move(promise)});
    disk_scheduler_->Schedule(requests);
    future.get();
  }

  // ── ④ 锁内收尾：清脏 + 减 pin ─────────────────────────────
  {
    std::scoped_lock lock(*bpm_latch_);
    if (dirty) {
      frame->is_dirty_ = false;
    }
    frame->pin_count_--;
    if (frame->pin_count_ == 0) {
      replacer_->SetEvictable(fid, true);
    }
  }

  return true;
}

/**
 * @brief **不安全地**把内存里所有页数据都写回磁盘。
 *
 * 本函数里**不应该**给页加锁。也就是说，你要仔细考虑什么时候去翻转 `is_dirty_` 标志位。
 *
 * ### 实现
 *
 * 你大概会想等 `CheckedReadPage`、`CheckedWritePage` 和 `FlushPage` 都实现完再来做这个函数，
 * 因为那时候该做什么会清楚得多。
 */
void BufferPoolManager::FlushAllPagesUnsafe() {
  std::scoped_lock lock(*bpm_latch_);

  // 已经持有 `bpm_latch_` 了，所以只能调 `FlushFrameLocked`，
  // 不能调 `FlushPageUnsafe`（那会再拿一次同一把非递归锁）。
  // 跳过还在搬的帧（`LOADING` / `FLUSHING`）：它们的 `data_` 不是稳定快照。
  for (const auto &frame : frames_) {
    if (frame->page_id_.has_value() && frame->state_ == FrameState::VALID) {
      FlushFrameLocked(frame, frame->page_id_.value());
    }
  }
}

/**
 * @brief **安全地**把内存里所有页数据都写回磁盘。
 *
 * 本函数里应该给页加锁，以保证写回磁盘的是一份一致的状态。
 *
 * ### 实现
 *
 * 你大概会想等 `CheckedReadPage`、`CheckedWritePage` 和 `FlushPage` 都实现完再来做这个函数，
 * 因为那时候该做什么会清楚得多。
 */
void BufferPoolManager::FlushAllPages() {
  // 先在锁内把页号拷出来，再在锁外逐页写回。
  // 直接在锁内遍历 `page_table_` 并调 `FlushPage` 会死锁。
  std::vector<page_id_t> page_ids;
  {
    std::scoped_lock lock(*bpm_latch_);
    page_ids.reserve(page_table_.size());
    for (const auto &entry : page_table_) {
      page_ids.push_back(entry.first);
    }
  }

  for (auto page_id : page_ids) {
    FlushPage(page_id);
  }
}

/**
 * @brief 取出某个页的 pin count。如果页不在内存里，返回 `std::nullopt`。
 *
 * 本函数是线程安全的。在多线程环境下多个线程访问同一个页时，调用方可以放心调用它。
 *
 * 本函数是给测试用的。如果实现错了，肯定会导致测试套件和 autograder 出问题。
 *
 * # 实现
 *
 * 我们会用这个函数来测试你的缓冲池管理器是否正确地维护 pin count。因为 `FrameHeader` 里的
 * `pin_count_` 字段是原子类型，你不需要去拿承载该页的那个 frame 的 latch，
 * 直接用原子的 `load` 安全地读出它的值即可。不过你仍然需要拿缓冲池的 latch。
 *
 * 再提醒一次，如果你对原子类型不熟，见 C++ 官方文档
 * [这里](https://en.cppreference.com/w/cpp/atomic/atomic)。
 *
 * @param page_id 我们要取 pin count 的页的 ID。
 * @return std::optional<size_t> 页存在时返回它的 pin count；否则返回 `std::nullopt`。
 */
auto BufferPoolManager::GetPinCount(page_id_t page_id) -> std::optional<size_t> {
  std::scoped_lock lock(*bpm_latch_);

  auto it = page_table_.find(page_id);
  if (it == page_table_.end()) {
    return std::nullopt;
  }

  auto frame_id = it->second;
  auto frame = frames_[frame_id];
  return frame->pin_count_.load();
}

}  // namespace bustub
