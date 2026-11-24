//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// index_iterator.h
//
// Identification: src/include/storage/index/index_iterator.h
//
// Copyright (c) 2015-2025, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

/**
 * index_iterator.h
 * 用于 B+ 树的范围扫描
 */
#pragma once
#include <memory>
#include <optional>
#include <utility>
#include "buffer/traced_buffer_pool_manager.h"
#include "common/config.h"
#include "common/macros.h"
#include "storage/page/b_plus_tree_leaf_page.h"

namespace bustub {

#define INDEXITERATOR_TYPE IndexIterator<KeyType, ValueType, KeyComparator, NumTombs>
#define SHORT_INDEXITERATOR_TYPE IndexIterator<KeyType, ValueType, KeyComparator>

FULL_INDEX_TEMPLATE_ARGUMENTS_DEFN
class IndexIterator {
 public:
  // 你可以根据自己的成员变量自行定义构造函数
  IndexIterator();

  /**
   * @brief 接管已持有的叶子页读守卫，构造指向第 index 条的迭代器。
   *
   * 构造时会跳过墓碑（必要时顺延到后续叶子页），
   * 所以传进来的 index 不必正好落在有效条目上。
   */
  IndexIterator(std::shared_ptr<TracedBufferPoolManager> bpm, ReadPageGuard &&guard, int index);

  ~IndexIterator();  // NOLINT

  auto IsEnd() -> bool;

  auto operator*() -> std::pair<const KeyType &, const ValueType &>;

  auto operator++() -> IndexIterator &;

  auto operator==(const IndexIterator &itr) const -> bool { return page_id_ == itr.page_id_ && index_ == itr.index_; }

  auto operator!=(const IndexIterator &itr) const -> bool { return !(*this == itr); }

 private:
  /** 从当前位置向后找第一条有效条目；扫完所有叶子页则变成末尾哨兵。 */
  void AdvanceToValid();

  std::shared_ptr<TracedBufferPoolManager> bpm_;
  /** 当前页的读守卫；为空表示已经到末尾。 */
  std::optional<ReadPageGuard> guard_;
  page_id_t page_id_{INVALID_PAGE_ID};
  int index_{0};
};

}  // namespace bustub
