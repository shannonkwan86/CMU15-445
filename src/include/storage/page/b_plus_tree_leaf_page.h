//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// b_plus_tree_leaf_page.h
//
// Identification: src/include/storage/page/b_plus_tree_leaf_page.h
//
// Copyright (c) 2015-2025, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#pragma once

#include <string>
#include <utility>
#include <vector>

#include "storage/page/b_plus_tree_page.h"

namespace bustub {

#define B_PLUS_TREE_LEAF_PAGE_TYPE BPlusTreeLeafPage<KeyType, ValueType, KeyComparator, NumTombs>
#define LEAF_PAGE_HEADER_SIZE 16
#define LEAF_PAGE_DEFAULT_TOMB_CNT 0
#define LEAF_PAGE_TOMB_CNT ((NumTombs < 0) ? LEAF_PAGE_DEFAULT_TOMB_CNT : NumTombs)
#define LEAF_PAGE_SLOT_CNT                                                                               \
  ((BUSTUB_PAGE_SIZE - LEAF_PAGE_HEADER_SIZE - sizeof(size_t) - (LEAF_PAGE_TOMB_CNT * sizeof(size_t))) / \
   (sizeof(KeyType) + sizeof(ValueType)))  // NOLINT

/**
 * 在叶子页里同时存放被索引的键和记录 ID（记录 ID = 页 ID 加槽位号，
 * 具体实现见 include/common/rid.h）。只支持唯一键。
 *
 * 叶子页还包含一块固定大小的「墓碑」缓冲区，用来记录那些已经被删除的条目。
 *
 * 叶子页布局（键按顺序存放，墓碑的顺序由你决定）：
 *  --------------------
 * | HEADER | TOMB_SIZE |（其中 TOMB_SIZE 就是 num_tombstones_）
 *  --------------------
 *  -----------------------------------
 * | TOMB(0) | TOMB(1) | ... | TOMB(k) |
 *  -----------------------------------
 *  ---------------------------------
 * | KEY(1) | KEY(2) | ... | KEY(n) |
 *  ---------------------------------
 *  ---------------------------------
 * | RID(1) | RID(2) | ... | RID(n) |
 *  ---------------------------------
 *
 *  头部布局（单位为字节，共 16 字节）：
 *  -----------------------------------------------
 * | PageType (4) | CurrentSize (4) | MaxSize (4) |
 *  -----------------------------------------------
 *  -----------------
 * | NextPageId (4) |
 *  -----------------
 */
FULL_INDEX_TEMPLATE_ARGUMENTS_DEFN
class BPlusTreeLeafPage : public BPlusTreePage {
 public:
  // 删掉全部构造函数 / 析构函数，以保证内存安全
  BPlusTreeLeafPage() = delete;
  BPlusTreeLeafPage(const BPlusTreeLeafPage &other) = delete;

  void Init(int max_size = LEAF_PAGE_SLOT_CNT);

  auto GetTombstones() const -> std::vector<KeyType>;

  // 辅助函数
  auto GetNextPageId() const -> page_id_t;
  void SetNextPageId(page_id_t next_page_id);
  auto KeyAt(int index) const -> KeyType;
  auto ValueAt(int index) const -> ValueType;
  /** 借出页内数组元素的引用，供迭代器的 operator* 使用。只在守卫存活期间有效。 */
  auto KeyRefAt(int index) const -> const KeyType &;
  auto ValueRefAt(int index) const -> const ValueType &;
  auto Lookup(const KeyType &key, const KeyComparator &cmp) const -> int;
  auto IsTombstoned(int index) const -> bool;
  void InsertAt(int index, const KeyType &key, const ValueType &value);
  auto LowerBound(const KeyType &key, const KeyComparator &cmp) const -> int;
  void MoveHalfTo(BPlusTreeLeafPage *recipient);

  void SetValueAt(int index, const ValueType &value);

  /** 物理删除第 index 个条目，并修正墓碑下标。 */
  void RemoveAt(int index);

  auto GetTombstoneCount() const -> int;
  void AddTombstone(int index);
  /** 移除「标记第 index 个条目」的那个墓碑（条目本身保留）。 */
  auto RemoveTombstone(int index) -> bool;
  /** 缓冲区满时，真正删除最老的那次删除对应的条目。 */
  void ApplyOldestTombstone();
  /** 为即将进入的 incoming 个墓碑腾出空间，必要时先应用最老的。 */
  void MakeTombstoneRoom(int incoming);

  /** 把本页所有条目接到 recipient 末尾，自己变空。 */
  void MoveAllTo(BPlusTreeLeafPage *recipient);
  /** 把第一个条目搬到 recipient 末尾。 */
  void MoveFirstToEndOf(BPlusTreeLeafPage *recipient);
  /** 把最后一个条目搬到 recipient 最前面。 */
  void MoveLastToFrontOf(BPlusTreeLeafPage *recipient);

  /**
   * @brief 仅供测试使用。返回一个表示本叶子页所有键的字符串，
   * 格式为 "(tombkey1, tombkey2, ...|key1,key2,key3,...)"。
   *
   * @return std::string
   */
  auto ToString() const -> std::string {
    std::string kstr = "(";
    bool first = true;

    auto tombs = GetTombstones();
    for (size_t i = 0; i < tombs.size(); i++) {
      kstr.append(std::to_string(tombs[i].ToString()));
      if ((i + 1) < tombs.size()) {
        kstr.append(",");
      }
    }

    kstr.append("|");

    for (int i = 0; i < GetSize(); i++) {
      KeyType key = KeyAt(i);
      if (first) {
        first = false;
      } else {
        kstr.append(",");
      }

      kstr.append(std::to_string(key.ToString()));
    }
    kstr.append(")");

    return kstr;
  }

 private:
  page_id_t next_page_id_;
  size_t num_tombstones_;
  // 固定大小的墓碑缓冲区（存放的是 key_array_ / rid_array_ 的下标）。
  size_t tombstones_[LEAF_PAGE_TOMB_CNT];
  // 存放页数据的数组。
  KeyType key_array_[LEAF_PAGE_SLOT_CNT];
  ValueType rid_array_[LEAF_PAGE_SLOT_CNT];
  // （Spring 2025）如果需要，可以在下面自由添加更多字段和辅助函数
};

}  // namespace bustub
