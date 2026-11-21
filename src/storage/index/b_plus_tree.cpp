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
auto BPLUSTREE_TYPE::TryInsertIntoLeaf(LeafPage *leaf, const KeyType &key, const ValueType &value) -> bool {
  int index = leaf->LowerBound(key, comparator_);
  if (index < leaf->GetSize() && comparator_(leaf->KeyAt(index), key) == 0) {
    if (!leaf->IsTombstoned(index)) {
      return false;  // 键已存在且有效，什么都不改
    }
    // 这个槽位被墓碑标记过：复用槽位，改 RID 并撤掉墓碑。size_ 不变——这不是插入，是复活。
    leaf->SetValueAt(index, value);
    leaf->RemoveTombstone(index);
    return true;
  }

  leaf->InsertAt(index, key, value);
  return true;
}

FULL_INDEX_TEMPLATE_ARGUMENTS
auto BPLUSTREE_TYPE::DescendReadOnly(page_id_t root_id, const KeyType &key) -> std::optional<ReadPath> {
  if (root_id == INVALID_PAGE_ID) {
    return std::nullopt;  // 空树
  }

  ReadPageGuard guard = bpm_->ReadPage(root_id);
  std::optional<ReadPageGuard> parent;

  while (!guard.As<BPlusTreePage>()->IsLeafPage()) {
    page_id_t child_id = guard.As<InternalPage>()->Lookup(key, comparator_);
    ReadPageGuard next = bpm_->ReadPage(child_id);

    if (next.As<BPlusTreePage>()->IsLeafPage()) {
      // 最后一层：父页读锁保留下来，它是「叶子被合并走」的唯一屏障
      parent = std::move(guard);
      guard = std::move(next);
      break;
    }

    guard = std::move(next);  // 中间层：孩子到手就放掉父页
  }

  return ReadPath{root_id, std::move(parent), std::move(guard)};
}

FULL_INDEX_TEMPLATE_ARGUMENTS
auto BPLUSTREE_TYPE::InsertInto(std::optional<WritePageGuard> guard, const KeyType &key, const ValueType &value,
                                KeyType *up_key, page_id_t *up_page) -> InsertResult {
  auto page = guard->AsMut<BPlusTreePage>();

  if (page->IsLeafPage()) {
    auto leaf = guard->AsMut<LeafPage>();
    if (!TryInsertIntoLeaf(leaf, key, value)) {
      return InsertResult::kDuplicate;
    }
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

  auto internal = guard->AsMut<InternalPage>();
  page_id_t child_page_id = internal->Lookup(key, comparator_);

  // crabbing：先抓孩子，再决定自己还能不能放
  WritePageGuard child_guard = bpm_->WritePage(child_page_id);

  // 判据 B：孩子装得下多一条 ⇒ 孩子不会分裂 ⇒ 本页不需要被修改。
  // 注意必须用孩子的 size/max，不是本页的。
  const auto *child_page = child_guard.As<BPlusTreePage>();
  const bool child_safe = child_page->GetSize() + 1 <= child_page->GetMaxSize();

  KeyType child_up_key;
  page_id_t child_up_page = INVALID_PAGE_ID;

  if (child_safe) {
    // 本页永远不会被修改，先放掉自己的锁再递归。
    // 保护孩子的不是本页的锁，而是孩子自己的写锁（在 child_guard 手里）：
    // 任何要搬走或分裂孩子的操作都需要孩子的写锁。
    guard.reset();

    InsertResult safe_result = InsertInto(std::move(child_guard), key, value, &child_up_key, &child_up_page);
    BUSTUB_ASSERT(safe_result != InsertResult::kSplit, "child judged safe but split");
    return safe_result;
  }

  InsertResult child_result = InsertInto(std::move(child_guard), key, value, &child_up_key, &child_up_page);

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
auto BPLUSTREE_TYPE::InsertSlow(const KeyType &key, const ValueType &value) -> bool {
  WritePageGuard header_guard = bpm_->WritePage(header_page_id_);
  page_id_t root_page_id = header_guard.AsMut<BPlusTreeHeaderPage>()->root_page_id_;

  if (root_page_id == INVALID_PAGE_ID) {
    // 空树：必须改 header，写锁已在手
    page_id_t new_page_id = bpm_->NewPage();

    WritePageGuard leaf_guard = bpm_->WritePage(new_page_id);
    auto leaf = leaf_guard.AsMut<LeafPage>();
    leaf->Init(leaf_max_size_);
    leaf->InsertAt(0, key, value);

    header_guard.AsMut<BPlusTreeHeaderPage>()->root_page_id_ = new_page_id;
    return true;
  }

  // crabbing：header 写 → 根 写，顺序自顶向下
  std::optional<WritePageGuard> root_guard{bpm_->WritePage(root_page_id)};

  // 判据 A：根装得下多一条 ⇒ 根不会分裂 ⇒ 没有任何路径能改 root_page_id。
  // 依据：改 root_page_id 必须先改根页本体（切开或搬走孩子），而根页写锁在我们手里。
  // 四条改根路径（空树建根、根分裂、根降级、根删空）全部需要根写锁。
  //
  // 因此可以提前放掉 header 写锁——它挡着所有快路径的读锁。
  // 约束：放开之后本函数绝不能再碰 header，否则会和「持 header 写锁、等根锁」的
  // 线程形成死锁环。下面的 kSplit 分支只在没放开时才会走到。
  const auto *root_page = root_guard->As<BPlusTreePage>();
  const bool root_safe = root_page->GetSize() + 1 <= root_page->GetMaxSize();
  if (root_safe) {
    header_guard.Drop();
  }

  KeyType up_key;
  page_id_t up_page = INVALID_PAGE_ID;
  InsertResult result = InsertInto(std::move(root_guard), key, value, &up_key, &up_page);

  if (result == InsertResult::kDuplicate) {
    return false;
  }

  if (result == InsertResult::kSplit) {
    // 根满了才会走到这里，此时 header 写锁从未被放开
    BUSTUB_ASSERT(!root_safe, "root judged safe but split");
    auto *header = header_guard.AsMut<BPlusTreeHeaderPage>();

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

FULL_INDEX_TEMPLATE_ARGUMENTS
auto BPLUSTREE_TYPE::InsertFast(const KeyType &key, const ValueType &value) -> FastInsertResult {
  // 快路径必须完成的重验：
  //   [1] 树身份稳定    —— 靠全程持有 header 读锁（root 不可能被换掉）
  //   [2] 父页读锁覆盖锁置换窗口 —— 第 ③ 步，第 ⑤ 步之前不许释放
  //   [3] LowerBound 重算 / [4] 查重 —— TryInsertIntoLeaf 内部
  //   [5] size + 1 <= max —— 第 ④ 步（写锁下）
  //
  // 加锁顺序必须严格自顶向下：header(读) → 根 → … → 父页(读) → 叶子(写)。
  // 绝不能在持有下层锁时回头请求 header，那和慢路径的「header 写 → parent 写」
  // 方向相反，会形成死锁环。
  //
  // 全函数只有一处显式 Drop（第 ③ 步放叶子读锁），其余守卫一律靠作用域析构。

  ReadPageGuard header_guard = bpm_->ReadPage(header_page_id_);
  page_id_t root_id = header_guard.As<BPlusTreeHeaderPage>()->root_page_id_;
  if (root_id == INVALID_PAGE_ID) {
    return FastInsertResult::kNeedSlowPath;  // 空树
  }

  auto path = DescendReadOnly(root_id, key);
  if (!path.has_value()) {
    return FastInsertResult::kNeedSlowPath;
  }

  // ② 读锁下预检：只用来避免无谓地拿写锁，不作数（窗口内可能被别人填满）
  const auto *leaf = path->leaf_.template As<LeafPage>();
  if (leaf->GetSize() + 1 > leaf->GetMaxSize()) {
    return FastInsertResult::kNeedSlowPath;
  }

  // ③ 锁置换：先放叶子读锁（shared_mutex 不可升级）。父页读锁正覆盖这个窗口，
  //    别的线程拿不到父页写锁，也就无法把这一页合并走，叶子不会变成孤儿。
  page_id_t leaf_id = path->leaf_.GetPageId();
  path->leaf_.Drop();
  WritePageGuard leaf_guard = bpm_->WritePage(leaf_id);

  // 叶子写锁已在手，谁也搬不走它，父页读锁可以提前放了（只影响并发度，不影响正确性）
  path->parent_.reset();

  // ④ 写锁下重验 + 插入
  auto *wleaf = leaf_guard.AsMut<LeafPage>();
  if (wleaf->GetSize() + 1 > wleaf->GetMaxSize()) {
    return FastInsertResult::kNeedSlowPath;  // 窗口内被别人填满了
  }
  if (!TryInsertIntoLeaf(wleaf, key, value)) {
    return FastInsertResult::kDuplicate;  // 键已存在
  }
  return FastInsertResult::kDone;
}

FULL_INDEX_TEMPLATE_ARGUMENTS
auto BPLUSTREE_TYPE::Insert(const KeyType &key, const ValueType &value) -> bool {
  // InsertFast 在独立函数里跑完，return 时它的守卫全部析构，
  // 所以走慢路径时树里一把锁都没有，不会和自己残留的读锁相撞。
  switch (InsertFast(key, value)) {
    case FastInsertResult::kDone:
      return true;
    case FastInsertResult::kDuplicate:
      return false;
    case FastInsertResult::kNeedSlowPath:
      break;
  }
  return InsertSlow(key, value);
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
auto BPLUSTREE_TYPE::CanShallowDelete(const LeafPage *leaf) const -> bool {
  if (LEAF_PAGE_TOMB_CNT > 0 && leaf->GetTombstoneCount() < static_cast<int>(LEAF_PAGE_TOMB_CNT)) {
    // 缓冲区还有空位：只追加一个墓碑，size 不变，不会下溢
    return true;
  }
  // 需要物理删掉一条（NumTombs == 0，或缓冲区已满、要先应用最老的）
  return leaf->GetSize() - 1 >= leaf->GetMinSize();
}

FULL_INDEX_TEMPLATE_ARGUMENTS
auto BPLUSTREE_TYPE::TryRemoveFromLeaf(LeafPage *leaf, const KeyType &key) -> bool {
  int index = leaf->Lookup(key, comparator_);
  if (index < 0 || leaf->IsTombstoned(index)) {
    return false;  // 不存在，未做任何修改
  }

  if (LEAF_PAGE_TOMB_CNT == 0) {
    leaf->RemoveAt(index);
    return true;
  }

  // 缓冲区满时要先应用最老的删除，那会让条目左移，所以必须在重新定位之前做
  leaf->MakeTombstoneRoom(1);
  index = leaf->Lookup(key, comparator_);
  if (index < 0 || leaf->IsTombstoned(index)) {
    // 腾位时恰好把这一条应用掉了：目标已经不在，也算删除成功
    return true;
  }
  leaf->AddTombstone(index);
  return true;
}

FULL_INDEX_TEMPLATE_ARGUMENTS
auto BPLUSTREE_TYPE::RemoveFast(const KeyType &key) -> FastRemoveResult {
  // 加锁顺序与 InsertFast 完全一致：header(读) 全程持有 → … → 父页(读) → 叶子(写)。
  // header 读锁在手 => root_page_id 不可能被改 => 不需要事后校验树身份。

  ReadPageGuard header_guard = bpm_->ReadPage(header_page_id_);
  page_id_t root_id = header_guard.As<BPlusTreeHeaderPage>()->root_page_id_;
  if (root_id == INVALID_PAGE_ID) {
    return FastRemoveResult::kNotFound;  // 空树，删什么都是「没有」
  }

  auto path = DescendReadOnly(root_id, key);
  if (!path.has_value()) {
    return FastRemoveResult::kNotFound;
  }

  // 读锁下预检：键不存在就直接返回（0 次写）；会下溢就交给慢路径
  const auto *leaf = path->leaf_.template As<LeafPage>();
  int index = leaf->Lookup(key, comparator_);
  if (index < 0 || leaf->IsTombstoned(index)) {
    return FastRemoveResult::kNotFound;
  }
  if (!CanShallowDelete(leaf)) {
    return FastRemoveResult::kNeedSlowPath;
  }

  // 锁置换：先放叶子读锁（shared_mutex 不可升级），父页读锁覆盖这个窗口
  page_id_t leaf_id = path->leaf_.GetPageId();
  path->leaf_.Drop();
  WritePageGuard leaf_guard = bpm_->WritePage(leaf_id);

  // 叶子写锁在手，谁也搬不走它，父页读锁可以提前放了
  path->parent_.reset();

  // 写锁下重验。注意：所有 return 都必须发生在 TryRemoveFromLeaf 之前——
  // 它会真的改页面（含 MakeTombstoneRoom 的物理删除），改完就退不回慢路径了。
  auto *wleaf = leaf_guard.AsMut<LeafPage>();
  if (!CanShallowDelete(wleaf)) {
    return FastRemoveResult::kNeedSlowPath;
  }
  if (!TryRemoveFromLeaf(wleaf, key)) {
    return FastRemoveResult::kNotFound;
  }
  return FastRemoveResult::kDone;
}

FULL_INDEX_TEMPLATE_ARGUMENTS
void BPLUSTREE_TYPE::Remove(const KeyType &key) {
  // RemoveFast 在独立函数里跑完，return 时它的守卫全部析构，
  // 所以走慢路径时树里一把锁都没有，不会和自己残留的读锁相撞。
  switch (RemoveFast(key)) {
    case FastRemoveResult::kDone:
    case FastRemoveResult::kNotFound:
      return;
    case FastRemoveResult::kNeedSlowPath:
      break;
  }
  RemoveSlow(key);
}

FULL_INDEX_TEMPLATE_ARGUMENTS
void BPLUSTREE_TYPE::RemoveSlow(const KeyType &key) {
  WritePageGuard header_guard = bpm_->WritePage(header_page_id_);
  page_id_t root_id = header_guard.AsMut<BPlusTreeHeaderPage>()->root_page_id_;
  if (root_id == INVALID_PAGE_ID) {
    return;
  }

  // crabbing：header 写 → 根 写，顺序自顶向下
  WritePageGuard root_guard = bpm_->WritePage(root_id);

  // 判据 A：根不会降级 ⇒ root_page_id 不可能变 ⇒ 可以提前放掉 header 写锁
  // （它挡着所有快路径的读锁）。
  //
  // 注意根**不受 GetMinSize() 的半满约束**，所以判据必须按类型分开：
  //   内部根：修复后 size == 1 才降级 → 要求 size - 1 >= 2
  //   叶子根：size == 0 才让整棵树变空 → 要求 size - 1 >= 1
  // 若用 min_size 代替前者，在 internal_min == 1 时会误判为安全（真会降级）。
  // 叶子根直接复用 CanShallowDelete，它已经处理了墓碑缓冲区（有空位时 size 不变）。
  //
  // 约束：放开之后本函数绝不能再碰 header，否则会和「持 header 写锁、等根锁」
  // 的线程形成死锁环。下面三处写 root_page_id 的代码都只在没放开时才会走到。
  const auto *root_page = root_guard.As<BPlusTreePage>();
  bool root_safe = false;
  if (root_page->IsLeafPage()) {
    root_safe = CanShallowDelete(root_guard.As<LeafPage>());
  } else {
    root_safe = root_page->GetSize() - 1 >= 2;
  }
  if (root_safe) {
    header_guard.Drop();
  }

  // 向下走，把路径上的写守卫压栈（不含叶子页）
  std::deque<WritePageGuard> path;
  WritePageGuard guard = std::move(root_guard);
  auto page = guard.AsMut<BPlusTreePage>();

  while (!page->IsLeafPage()) {
    auto internal = guard.AsMut<InternalPage>();
    page_id_t child_id = internal->Lookup(key, comparator_);
    path.push_back(std::move(guard));
    guard = bpm_->WritePage(child_id);
    page = guard.AsMut<BPlusTreePage>();

    // 判据 B：孩子删掉一条之后不会下溢 ⇒ 孩子不会被修改 ⇒ 祖先（都在 path 里）
    // 也不会被修改，可以全部放掉。保护孩子的是孩子自己的写锁，不是祖先的。
    //
    // 叶子页必须用 CanShallowDelete：墓碑缓冲区有空位时只追加墓碑、size 不变，
    // 那种情况本来就安全，用 size - 1 >= min 会漏掉这个机会。
    bool child_safe = false;
    if (page->IsLeafPage()) {
      child_safe = CanShallowDelete(guard.As<LeafPage>());
    } else {
      child_safe = page->GetSize() - 1 >= page->GetMinSize();
    }
    if (child_safe) {
      path.clear();
    }
  }

  auto leaf = guard.AsMut<LeafPage>();
  if (!TryRemoveFromLeaf(leaf, key)) {
    return;  // 键不存在或已被墓碑标记，什么都没改
  }

  if (leaf->GetSize() >= leaf->GetMinSize()) {
    return;
  }

  // 叶子被判成「不会下溢」却还是下溢了，path 早已被清空，下面会把它当成根来收缩，
  // 静默损坏结构。这条断言把「安全 ⇒ 不下溢」变成运行时检查。
  BUSTUB_ASSERT(!path.empty() || guard.GetPageId() == root_id, "leaf judged safe but underflowed");

  // 叶子页下溢，逐层向上修复
  while (!path.empty()) {
    WritePageGuard parent_guard = std::move(path.back());
    path.pop_back();
    auto parent = parent_guard.AsMut<InternalPage>();

    FixUnderflow(parent, std::move(guard));

    if (path.empty()) {
      // parent 就是根：内部页只剩一个孩子就降级
      if (parent->GetSize() == 1) {
        BUSTUB_ASSERT(!root_safe, "root judged safe but collapsed");
        header_guard.AsMut<BPlusTreeHeaderPage>()->root_page_id_ = parent->ValueAt(0);
      }
      return;
    }

    if (parent->GetSize() >= parent->GetMinSize()) {
      return;
    }
    guard = std::move(parent_guard);
  }

  // 路径已空，guard 持有根
  auto root_page_final = guard.AsMut<BPlusTreePage>();
  if (root_page_final->IsLeafPage()) {
    auto root_leaf = guard.AsMut<LeafPage>();
    if (root_leaf->GetSize() == 0) {
      BUSTUB_ASSERT(!root_safe, "root judged safe but emptied");
      header_guard.AsMut<BPlusTreeHeaderPage>()->root_page_id_ = INVALID_PAGE_ID;
    }
  } else {
    auto root_internal = guard.AsMut<InternalPage>();
    if (root_internal->GetSize() == 1) {
      BUSTUB_ASSERT(!root_safe, "root judged safe but collapsed");
      header_guard.AsMut<BPlusTreeHeaderPage>()->root_page_id_ = root_internal->ValueAt(0);
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
