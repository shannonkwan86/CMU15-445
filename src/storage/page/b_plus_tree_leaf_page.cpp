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

  // 插入把 [index, size_) 整体右移一格，墓碑下标要跟着修正
  for (size_t i = 0; i < num_tombstones_; i++) {
    if (static_cast<int>(tombstones_[i]) >= index) {
      tombstones_[i]++;
    }
  }
}

/**
 * @brief 把右半部分搬到 recipient，自己只保留左半。调用方负责分配并 Init recipient。
 */
FULL_INDEX_TEMPLATE_ARGUMENTS
void B_PLUS_TREE_LEAF_PAGE_TYPE::MoveHalfTo(BPlusTreeLeafPage *recipient) {
  int size = GetSize();
  int mid = size / 2;
  int n = size - mid;

  for (int i = 0; i < n; i++) {
    recipient->key_array_[i] = key_array_[mid + i];
    recipient->rid_array_[i] = rid_array_[mid + i];
  }
  recipient->SetSize(n);
  SetSize(mid);

  // 墓碑跟着条目走：下标 >= mid 的搬到新页并减去 mid，其余原地压实
  size_t kept = 0;
  recipient->num_tombstones_ = 0;
  for (size_t i = 0; i < num_tombstones_; i++) {
    if (static_cast<int>(tombstones_[i]) >= mid) {
      recipient->tombstones_[recipient->num_tombstones_++] = tombstones_[i] - mid;
    } else {
      tombstones_[kept++] = tombstones_[i];
    }
  }
  num_tombstones_ = kept;
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

FULL_INDEX_TEMPLATE_ARGUMENTS
void B_PLUS_TREE_LEAF_PAGE_TYPE::SetValueAt(int index, const ValueType &value) { rid_array_[index] = value; }

FULL_INDEX_TEMPLATE_ARGUMENTS
void B_PLUS_TREE_LEAF_PAGE_TYPE::RemoveAt(int index) {
  for (int i = index; i < GetSize() - 1; i++) {
    key_array_[i] = key_array_[i + 1];
    rid_array_[i] = rid_array_[i + 1];
  }
  ChangeSizeBy(-1);

  // 被删条目的墓碑作废，其余大于 index 的墓碑下标减一
  size_t kept = 0;
  for (size_t i = 0; i < num_tombstones_; i++) {
    int t = static_cast<int>(tombstones_[i]);
    if (t == index) {
      continue;
    }
    tombstones_[kept++] = static_cast<size_t>(t > index ? t - 1 : t);
  }
  num_tombstones_ = kept;
}

FULL_INDEX_TEMPLATE_ARGUMENTS
auto B_PLUS_TREE_LEAF_PAGE_TYPE::GetTombstoneCount() const -> int { return static_cast<int>(num_tombstones_); }

FULL_INDEX_TEMPLATE_ARGUMENTS
void B_PLUS_TREE_LEAF_PAGE_TYPE::AddTombstone(int index) {
  tombstones_[num_tombstones_++] = static_cast<size_t>(index);
}

FULL_INDEX_TEMPLATE_ARGUMENTS
auto B_PLUS_TREE_LEAF_PAGE_TYPE::RemoveTombstone(int index) -> bool {
  size_t kept = 0;
  bool found = false;
  for (size_t i = 0; i < num_tombstones_; i++) {
    if (static_cast<int>(tombstones_[i]) == index) {
      found = true;
      continue;
    }
    tombstones_[kept++] = tombstones_[i];
  }
  num_tombstones_ = kept;
  return found;
}

FULL_INDEX_TEMPLATE_ARGUMENTS
void B_PLUS_TREE_LEAF_PAGE_TYPE::ApplyOldestTombstone() {
  if (num_tombstones_ == 0) {
    return;
  }
  RemoveAt(static_cast<int>(tombstones_[0]));
}

FULL_INDEX_TEMPLATE_ARGUMENTS
void B_PLUS_TREE_LEAF_PAGE_TYPE::MakeTombstoneRoom(int incoming) {
  if (LEAF_PAGE_TOMB_CNT <= 0) {
    return;
  }
  while (static_cast<int>(num_tombstones_) + incoming > static_cast<int>(LEAF_PAGE_TOMB_CNT) && num_tombstones_ > 0) {
    ApplyOldestTombstone();
  }
}

FULL_INDEX_TEMPLATE_ARGUMENTS
void B_PLUS_TREE_LEAF_PAGE_TYPE::MoveAllTo(BPlusTreeLeafPage *recipient) {
  // 接收方已有的墓碑更旧，先处理它们，直到容得下本页的墓碑
  recipient->MakeTombstoneRoom(static_cast<int>(num_tombstones_));

  int base = recipient->GetSize();
  int n = GetSize();
  for (int i = 0; i < n; i++) {
    recipient->key_array_[base + i] = key_array_[i];
    recipient->rid_array_[base + i] = rid_array_[i];
  }
  for (size_t i = 0; i < num_tombstones_; i++) {
    recipient->tombstones_[recipient->num_tombstones_++] = tombstones_[i] + static_cast<size_t>(base);
  }
  recipient->SetSize(base + n);
  SetSize(0);
  num_tombstones_ = 0;
}

FULL_INDEX_TEMPLATE_ARGUMENTS
void B_PLUS_TREE_LEAF_PAGE_TYPE::MoveFirstToEndOf(BPlusTreeLeafPage *recipient) {
  // 先调整本页墓碑：0 号离开，其余下标减一
  bool moved_tombed = false;
  size_t kept = 0;
  for (size_t i = 0; i < num_tombstones_; i++) {
    int t = static_cast<int>(tombstones_[i]);
    if (t == 0) {
      moved_tombed = true;
    } else {
      tombstones_[kept++] = static_cast<size_t>(t - 1);
    }
  }
  num_tombstones_ = kept;

  recipient->MakeTombstoneRoom(moved_tombed ? 1 : 0);

  int base = recipient->GetSize();
  recipient->key_array_[base] = key_array_[0];
  recipient->rid_array_[base] = rid_array_[0];
  recipient->SetSize(base + 1);
  if (moved_tombed) {
    recipient->tombstones_[recipient->num_tombstones_++] = static_cast<size_t>(base);
  }

  for (int i = 0; i < GetSize() - 1; i++) {
    key_array_[i] = key_array_[i + 1];
    rid_array_[i] = rid_array_[i + 1];
  }
  ChangeSizeBy(-1);
}

FULL_INDEX_TEMPLATE_ARGUMENTS
void B_PLUS_TREE_LEAF_PAGE_TYPE::MoveLastToFrontOf(BPlusTreeLeafPage *recipient) {
  int last = GetSize() - 1;

  bool moved_tombed = false;
  size_t kept = 0;
  for (size_t i = 0; i < num_tombstones_; i++) {
    if (static_cast<int>(tombstones_[i]) == last) {
      moved_tombed = true;
    } else {
      tombstones_[kept++] = tombstones_[i];
    }
  }
  num_tombstones_ = kept;

  recipient->MakeTombstoneRoom(moved_tombed ? 1 : 0);

  int rsize = recipient->GetSize();
  for (int i = rsize; i > 0; i--) {
    recipient->key_array_[i] = recipient->key_array_[i - 1];
    recipient->rid_array_[i] = recipient->rid_array_[i - 1];
  }
  recipient->key_array_[0] = key_array_[last];
  recipient->rid_array_[0] = rid_array_[last];
  recipient->SetSize(rsize + 1);
  for (size_t i = 0; i < recipient->num_tombstones_; i++) {
    recipient->tombstones_[i]++;
  }
  if (moved_tombed) {
    recipient->tombstones_[recipient->num_tombstones_++] = 0;
  }

  ChangeSizeBy(-1);
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
