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
void B_PLUS_TREE_LEAF_PAGE_TYPE::Init(int max_size) { UNIMPLEMENTED("TODO(P2): Add implementation."); }

/**
 * @brief 获取某个页的墓碑（tombstone）的辅助函数。
 * @return 本页中最近 `NumTombs` 个待删除的键，按新旧程度排序（最旧的排在最前面）。
 */
FULL_INDEX_TEMPLATE_ARGUMENTS
auto B_PLUS_TREE_LEAF_PAGE_TYPE::GetTombstones() const -> std::vector<KeyType> {
  UNIMPLEMENTED("TODO(P2): Add implementation.");
}

/**
 * 读取 / 设置下一页 ID 的辅助函数
 */
FULL_INDEX_TEMPLATE_ARGUMENTS
auto B_PLUS_TREE_LEAF_PAGE_TYPE::GetNextPageId() const -> page_id_t { UNIMPLEMENTED("TODO(P2): Add implementation."); }

FULL_INDEX_TEMPLATE_ARGUMENTS
void B_PLUS_TREE_LEAF_PAGE_TYPE::SetNextPageId(page_id_t next_page_id) {
  UNIMPLEMENTED("TODO(P2): Add implementation.");
}

/*
 * 找到并返回下标为 "index"（也就是数组偏移量）的那个键的辅助函数
 */
FULL_INDEX_TEMPLATE_ARGUMENTS
auto B_PLUS_TREE_LEAF_PAGE_TYPE::KeyAt(int index) const -> KeyType { UNIMPLEMENTED("TODO(P2): Add implementation."); }

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
