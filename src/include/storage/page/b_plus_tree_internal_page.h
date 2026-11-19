//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// b_plus_tree_internal_page.h
//
// Identification: src/include/storage/page/b_plus_tree_internal_page.h
//
// Copyright (c) 2015-2025, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#pragma once

#include <queue>
#include <string>

#include "storage/page/b_plus_tree_page.h"

namespace bustub {

#define B_PLUS_TREE_INTERNAL_PAGE_TYPE BPlusTreeInternalPage<KeyType, ValueType, KeyComparator>
#define INTERNAL_PAGE_HEADER_SIZE 12
#define INTERNAL_PAGE_SLOT_CNT \
  ((BUSTUB_PAGE_SIZE - INTERNAL_PAGE_HEADER_SIZE) / ((int)(sizeof(KeyType) + sizeof(ValueType))))  // NOLINT

/**
 * 在内部页里存放 `n` 个被索引的键和 `n + 1` 个子页指针（page_id）。
 * 指针 PAGE_ID(i) 指向一棵子树，该子树中所有键 K 都满足：
 * K(i) <= K < K(i+1)。
 * 注意：由于键的数量不等于子页指针的数量，key_array_ 里的第一个键永远保持无效。
 * 也就是说，任何查找 / 搜索都应该忽略第一个键。
 *
 * 内部页布局（键按递增顺序存放）：
 *  ---------
 * | HEADER |
 *  ---------
 *  ------------------------------------------
 * | KEY(1)(INVALID) | KEY(2) | ... | KEY(n) |
 *  ------------------------------------------
 *  ---------------------------------------------
 * | PAGE_ID(1) | PAGE_ID(2) | ... | PAGE_ID(n) |
 *  ---------------------------------------------
 */
INDEX_TEMPLATE_ARGUMENTS
class BPlusTreeInternalPage : public BPlusTreePage {
 public:
  // 删掉全部构造函数 / 析构函数，以保证内存安全
  BPlusTreeInternalPage() = delete;
  BPlusTreeInternalPage(const BPlusTreeInternalPage &other) = delete;

  void Init(int max_size = INTERNAL_PAGE_SLOT_CNT);

  auto KeyAt(int index) const -> KeyType;

  void SetKeyAt(int index, const KeyType &key);

  /**
   * @param value 要搜索的值
   * @return 与指定值对应的下标
   */
  auto ValueIndex(const ValueType &value) const -> int;

  auto ValueAt(int index) const -> ValueType;

  auto Lookup(const KeyType &key, const KeyComparator &cmp) const -> ValueType;

  /**
   * @brief 仅供测试使用。返回一个表示本内部页所有键的字符串，
   * 格式为 "(key1,key2,key3,...)"。
   *
   * @return 当前内部页中所有键的字符串表示
   */
  auto ToString() const -> std::string {
    std::string kstr = "(";
    bool first = true;

    // 内部页的第一个键永远是无效的
    for (int i = 1; i < GetSize(); i++) {
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
  // 存放页数据的数组。
  KeyType key_array_[INTERNAL_PAGE_SLOT_CNT];
  ValueType page_id_array_[INTERNAL_PAGE_SLOT_CNT];
  // （Spring 2025）如果需要，可以在下面自由添加更多字段和辅助函数
};

}  // namespace bustub
