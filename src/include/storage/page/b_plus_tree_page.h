//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// b_plus_tree_page.h
//
// Identification: src/include/storage/page/b_plus_tree_page.h
//
// Copyright (c) 2015-2025, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#pragma once

#include <cassert>
#include <climits>
#include <cstdlib>
#include <string>

#include "buffer/buffer_pool_manager.h"
#include "storage/index/generic_key.h"

namespace bustub {

#define MappingType std::pair<KeyType, ValueType>

#define FULL_INDEX_TEMPLATE_ARGUMENTS_DEFN \
  template <typename KeyType, typename ValueType, typename KeyComparator, ssize_t NumTombs = 0>
#define FULL_INDEX_TEMPLATE_ARGUMENTS \
  template <typename KeyType, typename ValueType, typename KeyComparator, ssize_t NumTombs>
#define INDEX_TEMPLATE_ARGUMENTS template <typename KeyType, typename ValueType, typename KeyComparator>

// 定义页类型枚举
enum class IndexPageType { INVALID_INDEX_PAGE = 0, LEAF_PAGE, INTERNAL_PAGE };

/**
 * 内部页和叶子页都继承自这个类。
 *
 * 它实际上是每个 B+ 树页面的「头部」，存放叶子页和内部页共有的信息。
 *
 * 头部布局（单位为字节，共 12 字节）：
 * ---------------------------------------------------------
 * | PageType (4) | CurrentSize (4) | MaxSize (4) |  ...   |
 * ---------------------------------------------------------
 */
class BPlusTreePage {
 public:
  // 删掉全部构造函数 / 析构函数，以保证内存安全
  BPlusTreePage() = delete;
  BPlusTreePage(const BPlusTreePage &other) = delete;
  ~BPlusTreePage() = delete;

  auto IsLeafPage() const -> bool;
  void SetPageType(IndexPageType page_type);

  auto GetSize() const -> int;
  void SetSize(int size);
  void ChangeSizeBy(int amount);

  auto GetMaxSize() const -> int;
  void SetMaxSize(int max_size);
  auto GetMinSize() const -> int;

  /*
   * TODO(P2): 如果你打算使用这些字段，就删掉 __attribute__((__unused__))。
   */
 private:
  // 成员变量：内部页和叶子页共有的属性
  IndexPageType page_type_;
  // 本页中键值对的数量
  int size_;
  // 本页能容纳的键值对数量上限
  int max_size_;
};

}  // namespace bustub
