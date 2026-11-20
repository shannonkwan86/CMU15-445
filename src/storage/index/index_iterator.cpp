//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// index_iterator.cpp
//
// Identification: src/storage/index/index_iterator.cpp
//
// Copyright (c) 2015-2025, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

/**
 * index_iterator.cpp
 */
#include <cassert>

#include "storage/index/index_iterator.h"

namespace bustub {

/**
 * @note 你可以在这里修改析构函数 / 构造函数，
 * 并设置自己的输入参数
 */
FULL_INDEX_TEMPLATE_ARGUMENTS
INDEXITERATOR_TYPE::IndexIterator() = default;

FULL_INDEX_TEMPLATE_ARGUMENTS
INDEXITERATOR_TYPE::IndexIterator(std::shared_ptr<TracedBufferPoolManager> bpm, page_id_t page_id, int index)
    : bpm_(std::move(bpm)), page_id_(page_id), index_(index) {
  if (page_id_ != INVALID_PAGE_ID) {
    guard_ = bpm_->ReadPage(page_id_);
    AdvanceToValid();
  }
}

FULL_INDEX_TEMPLATE_ARGUMENTS
INDEXITERATOR_TYPE::~IndexIterator() = default;  // NOLINT

FULL_INDEX_TEMPLATE_ARGUMENTS
void INDEXITERATOR_TYPE::AdvanceToValid() {
  while (guard_.has_value()) {
    const auto *leaf = guard_->template As<B_PLUS_TREE_LEAF_PAGE_TYPE>();

    // 在页内跳过被墓碑标记的条目
    while (index_ < leaf->GetSize() && leaf->IsTombstoned(index_)) {
      index_++;
    }
    if (index_ < leaf->GetSize()) {
      return;
    }

    // 这一页扫完了，顺着兄弟指针去下一页
    page_id_t next = leaf->GetNextPageId();
    if (next == INVALID_PAGE_ID) {
      guard_.reset();
      page_id_ = INVALID_PAGE_ID;
      index_ = 0;
      return;
    }
    guard_ = bpm_->ReadPage(next);  // 旧 guard 在此析构，unpin 当前页
    page_id_ = next;
    index_ = 0;
  }
}

FULL_INDEX_TEMPLATE_ARGUMENTS
auto INDEXITERATOR_TYPE::IsEnd() -> bool { return !guard_.has_value(); }

FULL_INDEX_TEMPLATE_ARGUMENTS
auto INDEXITERATOR_TYPE::operator*() -> std::pair<const KeyType &, const ValueType &> {
  const auto *leaf = guard_->template As<B_PLUS_TREE_LEAF_PAGE_TYPE>();
  return {leaf->KeyRefAt(index_), leaf->ValueRefAt(index_)};
}

FULL_INDEX_TEMPLATE_ARGUMENTS
auto INDEXITERATOR_TYPE::operator++() -> INDEXITERATOR_TYPE & {
  index_++;
  AdvanceToValid();
  return *this;
}

template class IndexIterator<GenericKey<4>, RID, GenericComparator<4>>;

template class IndexIterator<GenericKey<8>, RID, GenericComparator<8>>;
template class IndexIterator<GenericKey<8>, RID, GenericComparator<8>, 3>;
template class IndexIterator<GenericKey<8>, RID, GenericComparator<8>, 2>;
template class IndexIterator<GenericKey<8>, RID, GenericComparator<8>, 1>;
template class IndexIterator<GenericKey<8>, RID, GenericComparator<8>, -1>;

template class IndexIterator<GenericKey<16>, RID, GenericComparator<16>>;

template class IndexIterator<GenericKey<32>, RID, GenericComparator<32>>;

template class IndexIterator<GenericKey<64>, RID, GenericComparator<64>>;

}  // namespace bustub
