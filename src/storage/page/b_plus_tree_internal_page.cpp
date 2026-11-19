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
// 内部页的 valuetype 应该是 page_id_t
template class BPlusTreeInternalPage<GenericKey<4>, page_id_t, GenericComparator<4>>;
template class BPlusTreeInternalPage<GenericKey<8>, page_id_t, GenericComparator<8>>;
template class BPlusTreeInternalPage<GenericKey<16>, page_id_t, GenericComparator<16>>;
template class BPlusTreeInternalPage<GenericKey<32>, page_id_t, GenericComparator<32>>;
template class BPlusTreeInternalPage<GenericKey<64>, page_id_t, GenericComparator<64>>;
}  // namespace bustub
