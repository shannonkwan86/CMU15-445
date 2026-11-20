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
auto BPLUSTREE_TYPE::IsEmpty() const -> bool {
  ReadPageGuard guard = bpm_->ReadPage(header_page_id_);
  return guard.As<BPlusTreeHeaderPage>()->root_page_id_ == INVALID_PAGE_ID;
}

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
  page_id_t page_id = GetRootPageId();
  if (page_id == INVALID_PAGE_ID) {
    return false;
  }

  ReadPageGuard guard = bpm_->ReadPage(page_id);
  auto page = guard.As<BPlusTreePage>();

  while (!page->IsLeafPage()) {
    auto internal = guard.As<InternalPage>();
    page_id = internal->Lookup(key, comparator_);

    guard = bpm_->ReadPage(page_id);
    page = guard.As<BPlusTreePage>();
  }

  auto leaf = guard.As<LeafPage>();
  int index = leaf->Lookup(key, comparator_);
  if (index < 0 || leaf->IsTombstoned(index)) {
    return false;
  }

  result->push_back(leaf->ValueAt(index));
  return true;
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
auto BPLUSTREE_TYPE::InsertInto(page_id_t page_id, const KeyType &key, const ValueType &value, KeyType *up_key,
                                page_id_t *up_page) -> InsertResult {
  WritePageGuard guard = bpm_->WritePage(page_id);
  auto page = guard.AsMut<BPlusTreePage>();

  if (page->IsLeafPage()) {
    auto leaf = guard.AsMut<LeafPage>();
    int index = leaf->LowerBound(key, comparator_);
    if (index < leaf->GetSize() && comparator_(leaf->KeyAt(index), key) == 0) {
      if (!leaf->IsTombstoned(index)) {
        return InsertResult::kDuplicate;
      }
      // 这个槽位被墓碑标记过：直接复用，写入新 RID 并撤掉墓碑
      leaf->SetValueAt(index, value);
      leaf->RemoveTombstone(index);
      return InsertResult::kInserted;
    }

    leaf->InsertAt(index, key, value);
    if (leaf->GetSize() <= leaf->GetMaxSize()) {
      return InsertResult::kInserted;
    }

    page_id_t new_page_id = bpm_->NewPage();
    WritePageGuard new_guard = bpm_->WritePage(new_page_id);
    auto new_leaf = new_guard.AsMut<LeafPage>();
    new_leaf->Init(leaf_max_size_);
    leaf->MoveHalfTo(new_leaf);

    // 兄弟指针：新页接在原页后面
    new_leaf->SetNextPageId(leaf->GetNextPageId());
    leaf->SetNextPageId(new_page_id);

    *up_key = new_leaf->KeyAt(0);
    *up_page = new_page_id;
    return InsertResult::kSplit;
  }

  auto internal = guard.AsMut<InternalPage>();
  page_id_t child_page_id = internal->Lookup(key, comparator_);

  KeyType child_up_key;
  page_id_t child_up_page = INVALID_PAGE_ID;
  InsertResult child_result = InsertInto(child_page_id, key, value, &child_up_key, &child_up_page);

  if (child_result != InsertResult::kSplit) {
    return child_result;
  }

  int index = internal->ValueIndex(child_page_id);
  internal->InsertAt(index + 1, child_up_key, child_up_page);
  if (internal->GetSize() <= internal->GetMaxSize()) {
    return InsertResult::kInserted;
  }

  page_id_t new_page_id = bpm_->NewPage();
  WritePageGuard new_guard = bpm_->WritePage(new_page_id);
  auto new_internal = new_guard.AsMut<InternalPage>();
  new_internal->Init(internal_max_size_);
  internal->MoveHalfTo(new_internal, up_key);

  *up_page = new_page_id;
  return InsertResult::kSplit;
}

FULL_INDEX_TEMPLATE_ARGUMENTS
auto BPLUSTREE_TYPE::Insert(const KeyType &key, const ValueType &value) -> bool {
  WritePageGuard header_guard = bpm_->WritePage(header_page_id_);
  auto header = header_guard.AsMut<BPlusTreeHeaderPage>();

  page_id_t root_page_id = header->root_page_id_;
  if (root_page_id == INVALID_PAGE_ID) {
    page_id_t new_page_id = bpm_->NewPage();

    WritePageGuard leaf_guard = bpm_->WritePage(new_page_id);
    auto leaf = leaf_guard.AsMut<LeafPage>();
    leaf->Init(leaf_max_size_);
    leaf->InsertAt(0, key, value);

    header->root_page_id_ = new_page_id;
    return true;
  }

  KeyType up_key;
  page_id_t up_page = INVALID_PAGE_ID;
  InsertResult result = InsertInto(root_page_id, key, value, &up_key, &up_page);

  if (result == InsertResult::kDuplicate) {
    return false;
  }

  if (result == InsertResult::kSplit) {
    // 根分裂：新建一个内部页当根，旧根和新页都是它的孩子
    page_id_t new_root_id = bpm_->NewPage();
    WritePageGuard new_guard = bpm_->WritePage(new_root_id);
    auto new_root = new_guard.AsMut<InternalPage>();
    new_root->Init(internal_max_size_);
    new_root->SetValueAt(0, root_page_id);
    new_root->SetKeyAt(1, up_key);
    new_root->SetValueAt(1, up_page);
    new_root->SetSize(2);

    header->root_page_id_ = new_root_id;
  }

  return true;
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
void BPLUSTREE_TYPE::FixUnderflow(InternalPage *parent, WritePageGuard &&cur_guard) {
  if (parent->GetSize() < 2) {
    return;  // 没有兄弟可借可合，交给上层继续向上收敛
  }

  int index = parent->ValueIndex(cur_guard.GetPageId());
  if (index < 0) {
    return;
  }

  int sib_index = (index > 0) ? index - 1 : index + 1;
  page_id_t sib_id = parent->ValueAt(sib_index);
  WritePageGuard sib_guard = bpm_->WritePage(sib_id);

  bool cur_is_left = index < sib_index;
  auto cur_page = cur_guard.AsMut<BPlusTreePage>();

  if (cur_page->IsLeafPage()) {
    auto cur_leaf = cur_guard.AsMut<LeafPage>();
    auto sib_leaf = sib_guard.AsMut<LeafPage>();

    if (sib_leaf->GetSize() > sib_leaf->GetMinSize()) {
      if (cur_is_left) {
        sib_leaf->MoveFirstToEndOf(cur_leaf);
        parent->SetKeyAt(sib_index, sib_leaf->KeyAt(0));
      } else {
        sib_leaf->MoveLastToFrontOf(cur_leaf);
        parent->SetKeyAt(index, cur_leaf->KeyAt(0));
      }
      if (cur_leaf->GetSize() >= cur_leaf->GetMinSize()) {
        return;
      }
      // 借位时若应用了墓碑导致仍然不够，落到下面的合并
    }

    if (cur_leaf->GetSize() + sib_leaf->GetSize() > cur_leaf->GetMaxSize()) {
      return;  // 合并会溢出，只能保持欠满
    }

    if (cur_is_left) {
      sib_leaf->MoveAllTo(cur_leaf);
      cur_leaf->SetNextPageId(sib_leaf->GetNextPageId());
      parent->RemoveAt(sib_index);
    } else {
      cur_leaf->MoveAllTo(sib_leaf);
      sib_leaf->SetNextPageId(cur_leaf->GetNextPageId());
      parent->RemoveAt(index);
    }
    return;
  }

  auto cur_int = cur_guard.AsMut<InternalPage>();
  auto sib_int = sib_guard.AsMut<InternalPage>();

  if (sib_int->GetSize() > sib_int->GetMinSize()) {
    if (cur_is_left) {
      KeyType sep = parent->KeyAt(sib_index);
      sib_int->MoveFirstToEndOf(cur_int, &sep);
      parent->SetKeyAt(sib_index, sep);
    } else {
      KeyType sep = parent->KeyAt(index);
      sib_int->MoveLastToFrontOf(cur_int, &sep);
      parent->SetKeyAt(index, sep);
    }
    return;
  }

  if (cur_int->GetSize() + sib_int->GetSize() > cur_int->GetMaxSize()) {
    return;
  }

  if (cur_is_left) {
    KeyType sep = parent->KeyAt(sib_index);
    sib_int->MoveAllTo(cur_int, sep);
    parent->RemoveAt(sib_index);
  } else {
    KeyType sep = parent->KeyAt(index);
    cur_int->MoveAllTo(sib_int, sep);
    parent->RemoveAt(index);
  }
}

FULL_INDEX_TEMPLATE_ARGUMENTS
void BPLUSTREE_TYPE::Remove(const KeyType &key) {
  WritePageGuard header_guard = bpm_->WritePage(header_page_id_);
  auto header = header_guard.AsMut<BPlusTreeHeaderPage>();

  page_id_t root_id = header->root_page_id_;
  if (root_id == INVALID_PAGE_ID) {
    return;
  }

  // 向下走，把路径上的写守卫压栈（不含叶子页）
  std::deque<WritePageGuard> path;
  WritePageGuard guard = bpm_->WritePage(root_id);
  auto page = guard.AsMut<BPlusTreePage>();

  while (!page->IsLeafPage()) {
    auto internal = guard.AsMut<InternalPage>();
    page_id_t child_id = internal->Lookup(key, comparator_);
    path.push_back(std::move(guard));
    guard = bpm_->WritePage(child_id);
    page = guard.AsMut<BPlusTreePage>();
  }

  auto leaf = guard.AsMut<LeafPage>();

  // 墓碑缓冲区满时要先应用最老的删除，那会让条目左移，所以必须在定位之前做
  if (LEAF_PAGE_TOMB_CNT > 0) {
    leaf->MakeTombstoneRoom(1);
  }

  int index = leaf->Lookup(key, comparator_);
  if (index < 0 || leaf->IsTombstoned(index)) {
    return;
  }

  if (LEAF_PAGE_TOMB_CNT > 0) {
    leaf->AddTombstone(index);
  } else {
    leaf->RemoveAt(index);
  }

  if (leaf->GetSize() >= leaf->GetMinSize()) {
    return;
  }

  // 叶子页下溢，逐层向上修复
  while (!path.empty()) {
    WritePageGuard parent_guard = std::move(path.back());
    path.pop_back();
    auto parent = parent_guard.AsMut<InternalPage>();

    FixUnderflow(parent, std::move(guard));

    if (path.empty()) {
      // parent 就是根：内部页只剩一个孩子就降级
      if (parent->GetSize() == 1) {
        header->root_page_id_ = parent->ValueAt(0);
      }
      return;
    }

    if (parent->GetSize() >= parent->GetMinSize()) {
      return;
    }
    guard = std::move(parent_guard);
  }

  // 路径已空，guard 持有根
  auto root_page = guard.AsMut<BPlusTreePage>();
  if (root_page->IsLeafPage()) {
    auto root_leaf = guard.AsMut<LeafPage>();
    if (root_leaf->GetSize() == 0) {
      header->root_page_id_ = INVALID_PAGE_ID;
    }
  } else {
    auto root_internal = guard.AsMut<InternalPage>();
    if (root_internal->GetSize() == 1) {
      header->root_page_id_ = root_internal->ValueAt(0);
    }
  }
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
auto BPLUSTREE_TYPE::Begin() -> INDEXITERATOR_TYPE {
  page_id_t leaf_id = INVALID_PAGE_ID;

  {
    ReadPageGuard header_guard = bpm_->ReadPage(header_page_id_);
    page_id_t page_id = header_guard.As<BPlusTreeHeaderPage>()->root_page_id_;
    header_guard.Drop();

    if (page_id != INVALID_PAGE_ID) {
      ReadPageGuard page_guard = bpm_->ReadPage(page_id);
      const auto *page = page_guard.As<BPlusTreePage>();

      // 一路走最左孩子，直到叶子层
      while (!page->IsLeafPage()) {
        page_id = page_guard.As<InternalPage>()->ValueAt(0);
        page_guard = bpm_->ReadPage(page_id);
        page = page_guard.As<BPlusTreePage>();
      }
      leaf_id = page_id;
    }
  }  // page_guard 在此析构，避免和迭代器自己的守卫重复加读锁

  if (leaf_id == INVALID_PAGE_ID) {
    return End();
  }
  return INDEXITERATOR_TYPE(bpm_, leaf_id, 0);
}

/**
 * @brief 输入参数为下界键，先找到包含该键的叶子页，再构造索引迭代器
 * @return ：索引迭代器
 */
FULL_INDEX_TEMPLATE_ARGUMENTS
auto BPLUSTREE_TYPE::Begin(const KeyType &key) -> INDEXITERATOR_TYPE {
  page_id_t leaf_id = INVALID_PAGE_ID;
  int index = 0;

  {
    ReadPageGuard header_guard = bpm_->ReadPage(header_page_id_);
    page_id_t page_id = header_guard.As<BPlusTreeHeaderPage>()->root_page_id_;
    header_guard.Drop();

    if (page_id != INVALID_PAGE_ID) {
      ReadPageGuard page_guard = bpm_->ReadPage(page_id);
      const auto *page = page_guard.As<BPlusTreePage>();

      while (!page->IsLeafPage()) {
        page_id = page_guard.As<InternalPage>()->Lookup(key, comparator_);
        page_guard = bpm_->ReadPage(page_id);
        page = page_guard.As<BPlusTreePage>();
      }

      leaf_id = page_id;
      // 可能是 GetSize()，表示这一页全比 key 小；迭代器构造时会顺延到下一页
      index = page_guard.As<LeafPage>()->LowerBound(key, comparator_);
    }
  }

  if (leaf_id == INVALID_PAGE_ID) {
    return End();
  }
  return INDEXITERATOR_TYPE(bpm_, leaf_id, index);
}

/**
 * @brief 输入参数为 void，构造一个表示「叶子节点中键值对末尾」的索引迭代器
 * @return ：索引迭代器
 */
FULL_INDEX_TEMPLATE_ARGUMENTS
auto BPLUSTREE_TYPE::End() -> INDEXITERATOR_TYPE { return INDEXITERATOR_TYPE(); }

/**
 * @return 本树根节点的页 ID
 *
 * 你可能想在实现 Task #3 的时候实现本函数。
 */
FULL_INDEX_TEMPLATE_ARGUMENTS
auto BPLUSTREE_TYPE::GetRootPageId() -> page_id_t {
  ReadPageGuard guard = bpm_->ReadPage(header_page_id_);
  return guard.As<BPlusTreeHeaderPage>()->root_page_id_;
}

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
