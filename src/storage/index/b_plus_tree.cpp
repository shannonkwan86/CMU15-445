//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// b_plus_tree.cpp
//
// Identification: src/storage/index/b_plus_tree.cpp
//
// Copyright (c) 2015-2025, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#include "storage/index/b_plus_tree.h"
#include "buffer/traced_buffer_pool_manager.h"
#include "storage/index/b_plus_tree_debug.h"

namespace bustub {

FULL_INDEX_TEMPLATE_ARGUMENTS
BPLUSTREE_TYPE::BPlusTree(std::string name, page_id_t header_page_id, BufferPoolManager *buffer_pool_manager,
                          const KeyComparator &comparator, int leaf_max_size, int internal_max_size)
    : bpm_(std::make_shared<TracedBufferPoolManager>(buffer_pool_manager)),
      index_name_(std::move(name)),
      comparator_(std::move(comparator)),
      leaf_max_size_(leaf_max_size),
      internal_max_size_(internal_max_size),
      header_page_id_(header_page_id) {
  WritePageGuard guard = bpm_->WritePage(header_page_id_);
  auto root_page = guard.AsMut<BPlusTreeHeaderPage>();
  root_page->root_page_id_ = INVALID_PAGE_ID;
}

/**
 * @brief 判断当前 B+ 树是否为空的辅助函数
 * @return 如果这棵 B+ 树里没有任何键值对，返回 true。
 */
FULL_INDEX_TEMPLATE_ARGUMENTS
auto BPLUSTREE_TYPE::IsEmpty() const -> bool { UNIMPLEMENTED("TODO(P2): Add implementation."); }

/*****************************************************************************
 * 查找
 *****************************************************************************/
/**
 * @brief 返回与输入键关联的那个唯一值
 *
 * 本方法用于点查询
 *
 * @param key 输入的键
 * @param[out] result 存放与输入键关联的那个唯一值（如果存在的话）的向量
 * @return ：true 表示该键存在
 */
FULL_INDEX_TEMPLATE_ARGUMENTS
auto BPLUSTREE_TYPE::GetValue(const KeyType &key, std::vector<ValueType> *result) -> bool {
  UNIMPLEMENTED("TODO(P2): Add implementation.");
  // Context 实例的声明。使用 Context 不是必须的，但建议使用。
  Context ctx;
}

/*****************************************************************************
 * 插入
 *****************************************************************************/
/**
 * @brief 把一个键值对插入 B+ 树
 *
 * 如果当前树是空的，就新建一棵树、更新根页 ID 并插入这条记录；
 * 否则插入到叶子页中。
 *
 * @param key 要插入的键
 * @param value 与键关联的值
 * @return：由于只支持唯一键，如果用户试图插入重复的键则返回 false，否则返回 true。
 */
FULL_INDEX_TEMPLATE_ARGUMENTS
auto BPLUSTREE_TYPE::Insert(const KeyType &key, const ValueType &value) -> bool {
  UNIMPLEMENTED("TODO(P2): Add implementation.");
  // Context 实例的声明。使用 Context 不是必须的，但建议使用。
  Context ctx;
}

/*****************************************************************************
 * 删除
 *****************************************************************************/
/**
 * @brief 删除与输入键关联的键值对
 * 如果当前树是空的，立即返回。
 * 否则，用户需要先找到正确的叶子页作为删除目标，然后从叶子页中删掉这条记录。
 * 记住在必要时处理重新分配（redistribute）或合并（merge）。
 *
 * @param key 输入的键
 */
FULL_INDEX_TEMPLATE_ARGUMENTS
void BPLUSTREE_TYPE::Remove(const KeyType &key) {
  // Context 实例的声明。
  Context ctx;
  UNIMPLEMENTED("TODO(P2): Add implementation.");
}

/*****************************************************************************
 * 索引迭代器
 *****************************************************************************/
/**
 * @brief 输入参数为 void，先找到最左边的叶子页，再构造索引迭代器
 *
 * 你可能想在实现 Task #3 的时候实现本函数。
 *
 * @return ：索引迭代器
 */
FULL_INDEX_TEMPLATE_ARGUMENTS
auto BPLUSTREE_TYPE::Begin() -> INDEXITERATOR_TYPE { UNIMPLEMENTED("TODO(P2): Add implementation."); }

/**
 * @brief 输入参数为下界键，先找到包含该键的叶子页，再构造索引迭代器
 * @return ：索引迭代器
 */
FULL_INDEX_TEMPLATE_ARGUMENTS
auto BPLUSTREE_TYPE::Begin(const KeyType &key) -> INDEXITERATOR_TYPE { UNIMPLEMENTED("TODO(P2): Add implementation."); }

/**
 * @brief 输入参数为 void，构造一个表示「叶子节点中键值对末尾」的索引迭代器
 * @return ：索引迭代器
 */
FULL_INDEX_TEMPLATE_ARGUMENTS
auto BPLUSTREE_TYPE::End() -> INDEXITERATOR_TYPE { UNIMPLEMENTED("TODO(P2): Add implementation."); }

/**
 * @return 本树根节点的页 ID
 *
 * 你可能想在实现 Task #3 的时候实现本函数。
 */
FULL_INDEX_TEMPLATE_ARGUMENTS
auto BPLUSTREE_TYPE::GetRootPageId() -> page_id_t { UNIMPLEMENTED("TODO(P2): Add implementation."); }

template class BPlusTree<GenericKey<4>, RID, GenericComparator<4>>;

template class BPlusTree<GenericKey<8>, RID, GenericComparator<8>>;
template class BPlusTree<GenericKey<8>, RID, GenericComparator<8>, 3>;
template class BPlusTree<GenericKey<8>, RID, GenericComparator<8>, 2>;
template class BPlusTree<GenericKey<8>, RID, GenericComparator<8>, 1>;
template class BPlusTree<GenericKey<8>, RID, GenericComparator<8>, -1>;

template class BPlusTree<GenericKey<16>, RID, GenericComparator<16>>;

template class BPlusTree<GenericKey<32>, RID, GenericComparator<32>>;

template class BPlusTree<GenericKey<64>, RID, GenericComparator<64>>;

}  // namespace bustub
