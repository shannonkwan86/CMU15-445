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
  using LeafPage = BPlusTreeLeafPage<KeyType, ValueType, KeyComparator>;

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
