//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// b_plus_tree_leaf_page.cpp
//
// Identification: src/storage/page/b_plus_tree_leaf_page.cpp
//
// Copyright (c) 2015-2025, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#include <sstream>

#include "common/exception.h"
#include "common/rid.h"
#include "storage/page/b_plus_tree_leaf_page.h"

namespace bustub {

/*****************************************************************************
 * 辅助方法与工具函数
 *****************************************************************************/

/**
 * @brief 新建叶子页之后的初始化方法。
 *
 * 从缓冲池拿到一个新的叶子页之后，必须调用本方法来设置默认值：
 * 设置页类型、把当前 size 置零、设置页 ID / 父页 ID、
 * 设置下一页 ID，以及设置 max size。
 *
 * @param max_size 叶子节点的最大容量
 */
FULL_INDEX_TEMPLATE_ARGUMENTS
void B_PLUS_TREE_LEAF_PAGE_TYPE::Init(int max_size) {
  SetPageType(IndexPageType::LEAF_PAGE);
  SetSize(0);
  SetMaxSize(max_size);
  SetNextPageId(INVALID_PAGE_ID);
  num_tombstones_ = 0;
}

/**
 * @brief 获取某个页的墓碑（tombstone）的辅助函数。
 * @return 本页中最近 `NumTombs` 个待删除的键，按新旧程度排序（最旧的排在最前面）。
 */
FULL_INDEX_TEMPLATE_ARGUMENTS
auto B_PLUS_TREE_LEAF_PAGE_TYPE::GetTombstones() const -> std::vector<KeyType> {
  std::vector<KeyType> tombs;
  for (size_t i = 0; i < num_tombstones_; i++) {
    tombs.push_back(key_array_[tombstones_[i]]);
  }
  return tombs;
}

/**
 * 读取 / 设置下一页 ID 的辅助函数
 */
FULL_INDEX_TEMPLATE_ARGUMENTS
auto B_PLUS_TREE_LEAF_PAGE_TYPE::GetNextPageId() const -> page_id_t { return next_page_id_; }

FULL_INDEX_TEMPLATE_ARGUMENTS
void B_PLUS_TREE_LEAF_PAGE_TYPE::SetNextPageId(page_id_t next_page_id) { next_page_id_ = next_page_id; }

/*
 * 找到并返回下标为 "index"（也就是数组偏移量）的那个键的辅助函数
 */
FULL_INDEX_TEMPLATE_ARGUMENTS
auto B_PLUS_TREE_LEAF_PAGE_TYPE::KeyAt(int index) const -> KeyType { return key_array_[index]; }

FULL_INDEX_TEMPLATE_ARGUMENTS
auto B_PLUS_TREE_LEAF_PAGE_TYPE::ValueAt(int index) const -> ValueType { return rid_array_[index]; }

FULL_INDEX_TEMPLATE_ARGUMENTS
auto B_PLUS_TREE_LEAF_PAGE_TYPE::Lookup(const KeyType &key, const KeyComparator &cmp) const -> int {
  int lo = 0;
  int hi = GetSize() - 1;
  while (lo <= hi) {
    int mid = lo + (hi - lo) / 2;
    int c = cmp(key_array_[mid], key);
    if (c == 0) {
      return mid;
    }
    if (c < 0) {
      lo = mid + 1;
    } else {
      hi = mid - 1;
    }
  }
  return -1;
}

FULL_INDEX_TEMPLATE_ARGUMENTS
auto B_PLUS_TREE_LEAF_PAGE_TYPE::IsTombstoned(int index) const -> bool {
  for (size_t i = 0; i < num_tombstones_; i++) {
    if (static_cast<int>(tombstones_[i]) == index) {
      return true;
    }
  }
  return false;
}

FULL_INDEX_TEMPLATE_ARGUMENTS
void B_PLUS_TREE_LEAF_PAGE_TYPE::InsertAt(int index, const KeyType &key, const ValueType &value) {
  for (int i = GetSize(); i > index; i--) {
    key_array_[i] = key_array_[i - 1];
    rid_array_[i] = rid_array_[i - 1];
  }
  key_array_[index] = key;
  rid_array_[index] = value;
  ChangeSizeBy(1);
}

FULL_INDEX_TEMPLATE_ARGUMENTS
auto B_PLUS_TREE_LEAF_PAGE_TYPE::LowerBound(const KeyType &key, const KeyComparator &cmp) const -> int {
  int lo = 0;
  int hi = GetSize();
  while (lo < hi) {
    int mid = lo + (hi - lo) / 2;
    if (cmp(key_array_[mid], key) < 0) {
      lo = mid + 1;
    } else {
      hi = mid;
    }
  }
  return lo;
}
template class BPlusTreeLeafPage<GenericKey<4>, RID, GenericComparator<4>>;

template class BPlusTreeLeafPage<GenericKey<8>, RID, GenericComparator<8>>;
template class BPlusTreeLeafPage<GenericKey<8>, RID, GenericComparator<8>, 3>;
template class BPlusTreeLeafPage<GenericKey<8>, RID, GenericComparator<8>, 2>;
template class BPlusTreeLeafPage<GenericKey<8>, RID, GenericComparator<8>, 1>;
template class BPlusTreeLeafPage<GenericKey<8>, RID, GenericComparator<8>, -1>;

template class BPlusTreeLeafPage<GenericKey<16>, RID, GenericComparator<16>>;

template class BPlusTreeLeafPage<GenericKey<32>, RID, GenericComparator<32>>;

template class BPlusTreeLeafPage<GenericKey<64>, RID, GenericComparator<64>>;
}  // namespace bustub
