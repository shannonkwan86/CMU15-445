//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// b_plus_tree.h
//
// Identification: src/include/storage/index/b_plus_tree.h
//
// Copyright (c) 2015-2025, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

/**
 * b_plus_tree.h
 *
 * 一个简单 B+ 树数据结构的实现：内部页负责指引搜索方向，叶子页存放真正的数据。
 * (1) 只支持唯一键
 * (2) 支持插入与删除
 * (3) 结构应当能够动态地增长与收缩
 * (4) 实现用于范围扫描的索引迭代器
 */
#pragma once

#include <algorithm>
#include <deque>
#include <filesystem>
#include <iostream>
#include <memory>
#include <optional>
#include <queue>
#include <shared_mutex>
#include <string>
#include <vector>

#include "common/config.h"
#include "common/macros.h"
#include "storage/index/index_iterator.h"
#include "storage/page/b_plus_tree_header_page.h"
#include "storage/page/b_plus_tree_internal_page.h"
#include "storage/page/b_plus_tree_leaf_page.h"
#include "storage/page/page_guard.h"

namespace bustub {

struct PrintableBPlusTree;

/**
 * @brief Context 类的定义。
 *
 * 提示：这个类是为了帮你记录「正在修改或访问哪些页」而设计的。
 */
class Context {
 public:
  // 向 B+ 树插入 / 从 B+ 树删除时，把头部页的写 guard 存在这里。
  // 当你想把所有页都解锁时，记得把头部页的 guard 释放掉并置为 nullopt。
  std::optional<WritePageGuard> header_page_{std::nullopt};

  // 把根页面的 ID 存在这里，这样判断「当前页是不是根页」会更方便。
  page_id_t root_page_id_{INVALID_PAGE_ID};

  // 把正在修改的那些页的写 guard 存在这里。
  std::deque<WritePageGuard> write_set_;

  // 读取值时可能会用到它，但不是必须的。
  std::deque<ReadPageGuard> read_set_;

  auto IsRootPage(page_id_t page_id) -> bool { return page_id == root_page_id_; }
};

#define BPLUSTREE_TYPE BPlusTree<KeyType, ValueType, KeyComparator, NumTombs>

// 提供交互式 B+ 树 API 的主类。
FULL_INDEX_TEMPLATE_ARGUMENTS_DEFN
class BPlusTree {
  using InternalPage = BPlusTreeInternalPage<KeyType, page_id_t, KeyComparator>;
  using LeafPage = BPlusTreeLeafPage<KeyType, ValueType, KeyComparator, NumTombs>;

 public:
  explicit BPlusTree(std::string name, page_id_t header_page_id, BufferPoolManager *buffer_pool_manager,
                     const KeyComparator &comparator, int leaf_max_size = LEAF_PAGE_SLOT_CNT,
                     int internal_max_size = INTERNAL_PAGE_SLOT_CNT);

  // 如果这棵 B+ 树里没有任何键值对，返回 true。
  auto IsEmpty() const -> bool;

  // 向这棵 B+ 树插入一个键值对。
  auto Insert(const KeyType &key, const ValueType &value) -> bool;

  // 从这棵 B+ 树删除一个键及其对应的值。
  void Remove(const KeyType &key);

  // 返回与给定键关联的值
  auto GetValue(const KeyType &key, std::vector<ValueType> *result) -> bool;

  // 返回根节点的页 ID
  auto GetRootPageId() -> page_id_t;

  // 索引迭代器
  auto Begin() -> INDEXITERATOR_TYPE;

  auto End() -> INDEXITERATOR_TYPE;

  auto Begin(const KeyType &key) -> INDEXITERATOR_TYPE;

  void Print(BufferPoolManager *bpm);

  void Draw(BufferPoolManager *bpm, const std::filesystem::path &outf);

  auto DrawBPlusTree() -> std::string;

  // 从文件中读取数据并逐条插入
  void InsertFromFile(const std::filesystem::path &file_name);

  // 从文件中读取数据并逐条删除
  void RemoveFromFile(const std::filesystem::path &file_name);

  void BatchOpsFromFile(const std::filesystem::path &file_name);

  // 不要把这里改成 BufferPoolManager！
  std::shared_ptr<TracedBufferPoolManager> bpm_;

 private:
  /** 递归插入的三种结果。 */
  enum class InsertResult {
    kDuplicate,  // 键已存在，未插入
    kInserted,   // 插入成功，本页未分裂
    kSplit,      // 插入成功且本页分裂，需要往父页插一条
  };

  /** 快路径的三种结局。 */
  enum class FastInsertResult {
    kDone,          // 插入成功（含复用墓碑槽位）
    kDuplicate,     // 键已存在，未做任何修改
    kNeedSlowPath,  // 快路径处理不了（空树 / 会分裂 / 树被换掉），交回慢路径
  };

  /**
   * @brief 乐观快路径：只读锁下探到叶子，只在叶子上拿写锁。
   *
   * 成功时产生 1 次写（叶子页），失败时把所有守卫留在作用域里析构后返回，
   * 因此调用方可以直接接着走慢路径，不会和自己残留的读锁相撞。
   */
  auto InsertFast(const KeyType &key, const ValueType &value) -> FastInsertResult;

  /**
   * @brief 悲观慢路径：header 写锁 + 整条路径写锁，处理空树、分裂、根变化。
   *
   * 就是原先 Insert 的完整实现，未做任何并发改动。
   */
  auto InsertSlow(const KeyType &key, const ValueType &value) -> bool;

  /** 快路径删除的三种结局。 */
  enum class FastRemoveResult {
    kDone,          // 删除成功（物理删除或追加墓碑）
    kNotFound,      // 键不存在或已被墓碑标记，未做任何修改
    kNeedSlowPath,  // 会下溢，需要父页和兄弟页，交回慢路径
  };

  /**
   * @brief 预测「只在叶子上动手、不碰父页」是否安全。
   *
   * 必须在**修改任何数据之前**调用：一旦真的物理删掉一条，就没法再干净地退回慢路径。
   *
   * - 墓碑缓冲区还有空位：只追加一个墓碑，size 不变 → 安全
   * - NumTombs == 0 或缓冲区已满：要物理删一条 → 要求 size - 1 >= min_size
   */
  auto CanShallowDelete(const LeafPage *leaf) const -> bool;

  /**
   * @brief 从叶子页删掉一条记录，快慢两条删除路径共用。
   *
   * 先判存在（不改状态），再腾墓碑位置（这会物理删一条并让条目左移），
   * 然后重新定位，最后加墓碑或物理删除。
   *
   * @return false 表示键不存在或已被墓碑标记，此时未做任何修改。
   *
   * 注意：本函数**不判断**会不会下溢。调用方负责在它之前调 CanShallowDelete，
   * 或者在它之后自行修复下溢（慢路径就是这么做的）。
   */
  auto TryRemoveFromLeaf(LeafPage *leaf, const KeyType &key) -> bool;

  /**
   * @brief 乐观删除快路径：只读锁下探，只在叶子上拿写锁。
   *
   * 加锁顺序与 InsertFast 相同：header(读) 全程持有 → … → 父页(读) → 叶子(写)。
   */
  auto RemoveFast(const KeyType &key) -> FastRemoveResult;

  /** 悲观删除慢路径（原先 Remove 的完整实现，未做并发改动）。 */
  void RemoveSlow(const KeyType &key);

  /**
   * @brief 把一条记录写进叶子页：判重 → 命中墓碑则复活 → 否则按序插入。
   *
   * 三种情况：
   *   - 键已存在且有效      → 返回 false，不做任何修改
   *   - 键已存在但被墓碑标记 → 复用该槽位（改 RID、撤墓碑），size_ 不变，返回 true
   *   - 键不存在            → 按序插入，size_ 加一，返回 true
   *
   * 不检查容量：调用方必须保证「插入新键时不会溢出」，或者自己处理分裂。
   */
  auto TryInsertIntoLeaf(LeafPage *leaf, const KeyType &key, const ValueType &value) -> bool;

  /**
   * @brief 在 page_id 这棵子树里插入 key/value。
   *
   * 若本页因此分裂，通过 up_key / up_page 告诉调用者要往父页插什么。
   */
  auto InsertInto(page_id_t page_id, const KeyType &key, const ValueType &value, KeyType *up_key, page_id_t *up_page)
      -> InsertResult;

  /**
   * @brief 处理 cur 的下溢：与兄弟借一个条目，或者与之合并。parent 已持有写守卫。
   *
   * cur_guard 会被消费掉（合并时 cur 可能被清空）。
   */
  void FixUnderflow(InternalPage *parent, WritePageGuard &&cur_guard);

  /**
   * @brief 一次只读下探的产物。
   *
   * 只保留「叶子页」和「叶子页的父亲」这两级，中间层全部丢弃（锁在下降时已释放）。
   * 它不是完整路径，不能用来向上回溯。
   *
   * 契约：
   *   - parent_ 的读锁从 Lookup 之前就一直持有，直到本结构析构或被显式释放。因此
   *     leaf_ 仍然是 parent_ 的孩子——这是「放掉 leaf_ 的读锁、改拿写锁」这个窗口
   *     期间，防止叶子被别的线程合并走的唯一屏障。
   *   - 本结构**不校验树身份**。调用方必须从读到 root_id_ 的那一刻起，一直持有
   *     header 页的**读**锁，直到本次操作结束。否则中途树可能被换掉（根分裂、被
   *     删空），落点就会失效。
   *   - 不能在持有下层锁之后再回头去读 header：慢路径是「header 写 → parent
   *     写」，反向请求会形成死锁环。
   */
  struct ReadPath {
    /** 下探开始时读到的根页号。只是快照，不带锁。 */
    page_id_t root_id_{INVALID_PAGE_ID};
    /** 叶子页的父页读守卫；叶子就是根（树高 1）时为空。 */
    std::optional<ReadPageGuard> parent_;
    /** 目标叶子页的读守卫。 */
    ReadPageGuard leaf_;
  };

  /**
   * @brief 从 root_id 出发，以只读锁耦合自顶向下探到目标叶子页。
   *
   * 本函数**不访问 header 页**：root_id 由调用方提供，并且调用方必须一直持有
   * header 读锁（见 ReadPath 的契约）。
   *
   * 下降途中中间层是「拿到孩子的读锁就放掉父亲的读锁」，只有通向叶子的那一层例外：
   * 父亲读锁会保留到 ReadPath 析构。
   *
   * @return root_id 无效（空树）时返回 std::nullopt；否则返回 {root_id, 父页, 叶子}。
   */
  auto DescendReadOnly(page_id_t root_id, const KeyType &key) -> std::optional<ReadPath>;

  void ToGraph(page_id_t page_id, const BPlusTreePage *page, std::ofstream &out);

  void PrintTree(page_id_t page_id, const BPlusTreePage *page);

  auto ToPrintableBPlusTree(page_id_t root_id) -> PrintableBPlusTree;

  // 成员变量
  std::string index_name_;
  KeyComparator comparator_;
  std::vector<std::string> log;  // NOLINT
  int leaf_max_size_;
  int internal_max_size_;
  page_id_t header_page_id_;
};

/**
 * @brief 仅供测试使用。PrintableBPlusTree 是一棵可打印的 B+ 树。
 * 我们先把 B+ 树转换成一棵可打印的 B+ 树，然后再打印它。
 */
struct PrintableBPlusTree {
  int size_;
  std::string keys_;
  std::vector<PrintableBPlusTree> children_;

  /**
   * @brief 对一棵可打印的 B+ 树做 BFS 遍历，并输出到
   * out_buf 中
   *
   * @param out_buf
   */
  void Print(std::ostream &out_buf) {
    std::vector<PrintableBPlusTree *> que = {this};
    while (!que.empty()) {
      std::vector<PrintableBPlusTree *> new_que;

      for (auto &t : que) {
        int padding = (t->size_ - t->keys_.size()) / 2;
        out_buf << std::string(padding, ' ');
        out_buf << t->keys_;
        out_buf << std::string(padding, ' ');

        for (auto &c : t->children_) {
          new_que.push_back(&c);
        }
      }
      out_buf << "\n";
      que = new_que;
    }
  }
};

}  // namespace bustub
