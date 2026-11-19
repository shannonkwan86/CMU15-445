//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// b_plus_tree_internal_page.cpp
//
// Identification: src/storage/page/b_plus_tree_internal_page.cpp
//
// Copyright (c) 2015-2025, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#include <iostream>
#include <sstream>

#include "common/exception.h"
#include "storage/page/b_plus_tree_internal_page.h"

namespace bustub {
/*****************************************************************************
 * 辅助方法与工具函数
 *****************************************************************************/

/**
 * @brief 新建内部页之后的初始化方法。
 *
 * 把一个新建的页写成合法的 BPlusTreeInternalPage：写入必要的头部信息，
 * 包括设置页类型、当前 size、页 ID、父页 ID 以及页的最大容量。
 * 必须在创建新页之后调用，才能得到一个有效的内部页。
 *
 * @param max_size 页的最大容量
 */
INDEX_TEMPLATE_ARGUMENTS
void B_PLUS_TREE_INTERNAL_PAGE_TYPE::Init(int max_size) {
  SetPageType(IndexPageType::INTERNAL_PAGE);
  SetSize(0);
  SetMaxSize(max_size);
}

/**
 * @brief 读取 / 设置下标为 "index"（也就是数组偏移量）的那个键的辅助方法。
 *
 * @param index 要读取的键的下标。下标必须非零。
 * @return 该下标处的键
 */
INDEX_TEMPLATE_ARGUMENTS
auto B_PLUS_TREE_INTERNAL_PAGE_TYPE::KeyAt(int index) const -> KeyType { return key_array_[index]; }

/**
 * @brief 设置指定下标处的键。
 *
 * @param index 要设置的键的下标。下标必须非零。
 * @param key 新的键值
 */
INDEX_TEMPLATE_ARGUMENTS
void B_PLUS_TREE_INTERNAL_PAGE_TYPE::SetKeyAt(int index, const KeyType &key) { key_array_[index] = key; }

/**
 * @brief 读取下标为 "index"（也就是数组偏移量）的那个值的辅助方法。
 *
 * @param index 要读取的值的下标。
 * @return 该下标处的值
 */
INDEX_TEMPLATE_ARGUMENTS
auto B_PLUS_TREE_INTERNAL_PAGE_TYPE::ValueAt(int index) const -> ValueType { return page_id_array_[index]; }

/**
 * @brief 在内部页的键数组里二分，确定该走哪个孩子。
 * @return 目标键应该继续往下找的那个子页的 page_id
 */
INDEX_TEMPLATE_ARGUMENTS
auto B_PLUS_TREE_INTERNAL_PAGE_TYPE::Lookup(const KeyType &key, const KeyComparator &cmp) const -> ValueType {
  int child = 0;
  int lo = 1;
  int hi = GetSize() - 1;

  while (lo <= hi) {
    int mid = lo + (hi - lo) / 2;
    if (cmp(key_array_[mid], key) <= 0) {
      child = mid;
      lo = mid + 1;
    } else {
      hi = mid - 1;
    }
  }

  return page_id_array_[child];
}

INDEX_TEMPLATE_ARGUMENTS
void B_PLUS_TREE_INTERNAL_PAGE_TYPE::SetValueAt(int index, const ValueType &value) { page_id_array_[index] = value; }

/**
 * @brief 线性查找某个孩子在本页的下标，找不到返回 -1。
 *
 * 页号本身没有大小语义，所以只能找相等，不能二分。
 */
INDEX_TEMPLATE_ARGUMENTS
auto B_PLUS_TREE_INTERNAL_PAGE_TYPE::ValueIndex(const ValueType &value) const -> int {
  for (int i = 0; i < GetSize(); i++) {
    if (page_id_array_[i] == value) {
      return i;
    }
  }
  return -1;
}

INDEX_TEMPLATE_ARGUMENTS
void B_PLUS_TREE_INTERNAL_PAGE_TYPE::InsertAt(int index, const KeyType &key, const ValueType &value) {
  for (int i = GetSize(); i > index; i--) {
    key_array_[i] = key_array_[i - 1];
    page_id_array_[i] = page_id_array_[i - 1];
  }
  key_array_[index] = key;
  page_id_array_[index] = value;
  ChangeSizeBy(1);
}

INDEX_TEMPLATE_ARGUMENTS
void B_PLUS_TREE_INTERNAL_PAGE_TYPE::RemoveAt(int index) {
  for (int i = index; i < GetSize() - 1; i++) {
    key_array_[i] = key_array_[i + 1];
    page_id_array_[i] = page_id_array_[i + 1];
  }
  ChangeSizeBy(-1);
}

INDEX_TEMPLATE_ARGUMENTS
void B_PLUS_TREE_INTERNAL_PAGE_TYPE::MoveHalfTo(BPlusTreeInternalPage *recipient, KeyType *up_key) {
  int size = GetSize();
  int mid = size / 2;
  *up_key = key_array_[mid];

  int n = size - mid;  // 右半的孩子数
  for (int i = 0; i < n; i++) {
    recipient->page_id_array_[i] = page_id_array_[mid + i];
    if (i > 0) {
      recipient->key_array_[i] = key_array_[mid + i];  // 0 号键保持无效
    }
  }
  recipient->SetSize(n);
  SetSize(mid);
}

INDEX_TEMPLATE_ARGUMENTS
void B_PLUS_TREE_INTERNAL_PAGE_TYPE::MoveAllTo(BPlusTreeInternalPage *recipient, const KeyType &middle_key) {
  int base = recipient->GetSize();
  int n = GetSize();

  // 本页孩子 0 接到 recipient 末尾，接缝处的键是父页的分隔键
  recipient->key_array_[base] = middle_key;
  recipient->page_id_array_[base] = page_id_array_[0];
  for (int i = 1; i < n; i++) {
    recipient->key_array_[base + i] = key_array_[i];
    recipient->page_id_array_[base + i] = page_id_array_[i];
  }
  recipient->SetSize(base + n);
  SetSize(0);
}

INDEX_TEMPLATE_ARGUMENTS
void B_PLUS_TREE_INTERNAL_PAGE_TYPE::MoveFirstToEndOf(BPlusTreeInternalPage *recipient, KeyType *middle_key) {
  int base = recipient->GetSize();

  recipient->key_array_[base] = *middle_key;
  recipient->page_id_array_[base] = page_id_array_[0];
  recipient->SetSize(base + 1);

  // 本页 key[1] 成为新的分隔键
  *middle_key = key_array_[1];

  for (int i = 0; i < GetSize() - 1; i++) {
    page_id_array_[i] = page_id_array_[i + 1];
    key_array_[i] = key_array_[i + 1];
  }
  ChangeSizeBy(-1);
}

INDEX_TEMPLATE_ARGUMENTS
void B_PLUS_TREE_INTERNAL_PAGE_TYPE::MoveLastToFrontOf(BPlusTreeInternalPage *recipient, KeyType *middle_key) {
  int last = GetSize() - 1;
  int rsize = recipient->GetSize();

  // recipient 整体右移一格，空出孩子 0 的位置
  for (int i = rsize; i > 0; i--) {
    recipient->key_array_[i] = recipient->key_array_[i - 1];
    recipient->page_id_array_[i] = recipient->page_id_array_[i - 1];
  }
  recipient->page_id_array_[0] = page_id_array_[last];
  recipient->key_array_[1] = *middle_key;
  recipient->SetSize(rsize + 1);

  // 本页最后一个键成为新的分隔键
  *middle_key = key_array_[last];
  ChangeSizeBy(-1);
}

// 内部页的 valuetype 应该是 page_id_t
template class BPlusTreeInternalPage<GenericKey<4>, page_id_t, GenericComparator<4>>;
template class BPlusTreeInternalPage<GenericKey<8>, page_id_t, GenericComparator<8>>;
template class BPlusTreeInternalPage<GenericKey<16>, page_id_t, GenericComparator<16>>;
template class BPlusTreeInternalPage<GenericKey<32>, page_id_t, GenericComparator<32>>;
template class BPlusTreeInternalPage<GenericKey<64>, page_id_t, GenericComparator<64>>;
}  // namespace bustub
