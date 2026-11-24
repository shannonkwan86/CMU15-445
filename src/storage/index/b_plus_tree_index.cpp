//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// b_plus_tree_index.cpp
//
// Identification: src/storage/index/b_plus_tree_index.cpp
//
// Copyright (c) 2015-2025, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#include "storage/index/b_plus_tree_index.h"

namespace bustub {
/**
 * 构造函数
 */
INDEX_TEMPLATE_ARGUMENTS
BPLUSTREE_INDEX_TYPE::BPlusTreeIndex(std::unique_ptr<IndexMetadata> &&metadata, BufferPoolManager *buffer_pool_manager)
    : Index(std::move(metadata)), comparator_(GetMetadata()->GetKeySchema()) {
  page_id_t header_page_id = buffer_pool_manager->NewPage();
  container_ = std::make_shared<BPlusTree<KeyType, ValueType, KeyComparator>>(GetMetadata()->GetName(), header_page_id,
                                                                              buffer_pool_manager, comparator_);
}

INDEX_TEMPLATE_ARGUMENTS
auto BPLUSTREE_INDEX_TYPE::InsertEntry(const Tuple &key, RID rid, Transaction *transaction) -> bool {
  // 构造要插入的索引键
  KeyType index_key;
  index_key.SetFromKey(key);

  return container_->Insert(index_key, rid);
}

INDEX_TEMPLATE_ARGUMENTS
void BPLUSTREE_INDEX_TYPE::DeleteEntry(const Tuple &key, RID rid, Transaction *transaction) {
  // 构造要删除的索引键
  KeyType index_key;
  index_key.SetFromKey(key);

  container_->Remove(index_key);
}

INDEX_TEMPLATE_ARGUMENTS
void BPLUSTREE_INDEX_TYPE::ScanKey(const Tuple &key, std::vector<RID> *result, Transaction *transaction) {
  // 构造要扫描的索引键
  KeyType index_key;
  index_key.SetFromKey(key);

  container_->GetValue(index_key, result);
}

INDEX_TEMPLATE_ARGUMENTS
auto BPLUSTREE_INDEX_TYPE::GetBeginIterator() -> SHORT_INDEXITERATOR_TYPE { return container_->Begin(); }

INDEX_TEMPLATE_ARGUMENTS
auto BPLUSTREE_INDEX_TYPE::GetBeginIterator(const KeyType &key) -> SHORT_INDEXITERATOR_TYPE {
  return container_->Begin(key);
}

INDEX_TEMPLATE_ARGUMENTS
auto BPLUSTREE_INDEX_TYPE::GetEndIterator() -> SHORT_INDEXITERATOR_TYPE { return container_->End(); }

template class BPlusTreeIndex<GenericKey<4>, RID, GenericComparator<4>>;
template class BPlusTreeIndex<GenericKey<8>, RID, GenericComparator<8>>;
template class BPlusTreeIndex<GenericKey<16>, RID, GenericComparator<16>>;
template class BPlusTreeIndex<GenericKey<32>, RID, GenericComparator<32>>;
template class BPlusTreeIndex<GenericKey<64>, RID, GenericComparator<64>>;

}  // namespace bustub
