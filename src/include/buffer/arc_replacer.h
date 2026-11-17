
//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// arc_replacer.h
//
// Identification: src/include/buffer/arc_replacer.h
//
// Copyright (c) 2015-2025, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#pragma once

#include <list>
#include <memory>
#include <mutex>  // NOLINT
#include <optional>
#include <unordered_map>

#include "common/config.h"
#include "common/macros.h"

namespace bustub {

enum class AccessType { Unknown = 0, Lookup, Scan, Index };

enum class ArcStatus { MRU, MFU, MRU_GHOST, MFU_GHOST };

// TODO(student): 这个结构体的字段可以按你的需要增删改。
struct FrameStatus {
  page_id_t page_id_;
  frame_id_t frame_id_;
  bool evictable_;
  ArcStatus arc_status_;  // "这个页现在在哪张 list"，枚举（MRU / MFU / MRU_GHOST / MFU_GHOST）
  FrameStatus(page_id_t pid, frame_id_t fid, bool ev, ArcStatus st)
      : page_id_(pid), frame_id_(fid), evictable_(ev), arc_status_(st) {}
  std::list<frame_id_t>::iterator alive_it_;  // 活页在 mru_ / mfu_ 里的位置
  std::list<page_id_t>::iterator ghost_it_;   // ghost 在 mru_ghost_ / mfu_ghost_ 里的位置
};

/**
 * ArcReplacer 实现了 ARC 替换策略。
 */
class ArcReplacer {
 public:
  explicit ArcReplacer(size_t num_frames);

  DISALLOW_COPY_AND_MOVE(ArcReplacer);

  /**
   * @brief 销毁 `ArcReplacer`。
   */
  ~ArcReplacer() = default;

  auto Evict() -> std::optional<frame_id_t>;
  void RecordAccess(frame_id_t frame_id, page_id_t page_id, AccessType access_type = AccessType::Unknown);
  void SetEvictable(frame_id_t frame_id, bool set_evictable);
  void Remove(frame_id_t frame_id);
  auto Size() -> size_t;

 private:
  // TODO(student): 实现我！这些成员变量你可以随意替换或删除。
  std::list<frame_id_t> mru_;
  std::list<frame_id_t> mfu_;
  std::list<page_id_t> mru_ghost_;
  std::list<page_id_t> mfu_ghost_;

  /* 记录 mru_ 和 mfu_ 里的条目。
   * 这里用 frame_id_t 做 key，是为了保证同一个 frame 存活期间不会被重复记录 */
  std::unordered_map<frame_id_t, std::shared_ptr<FrameStatus>> alive_map_;
  /* 记录 mru_ghost_ 和 mfu_ghost_ 里的条目。
   * 这里用 page_id_t 而不是 frame_id_t，因为在 ghost list 里页号才是唯一标识 */
  std::unordered_map<page_id_t, std::shared_ptr<FrameStatus>> ghost_map_;

  /* 存活且可淘汰的条目数量 */
  [[maybe_unused]] size_t curr_size_{0};
  /* 原论文里的 p */
  [[maybe_unused]] size_t mru_target_size_{0};
  /* 原论文里的 c */
  [[maybe_unused]] size_t replacer_size_;
  std::mutex latch_;

  // TODO(student): 成员变量 / 成员函数可以按你的需要添加。
};

}  // namespace bustub
