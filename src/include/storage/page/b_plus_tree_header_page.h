//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// b_plus_tree_header_page.h
//
// Identification: src/include/storage/page/b_plus_tree_header_page.h
//
// Copyright (c) 2015-2025, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#pragma once

#include "common/config.h"

namespace bustub {

/**
 * 头部页的唯一用途是保存根页面的 ID，这样在并发环境下读取根节点时
 * 就不会产生竞态。
 */
class BPlusTreeHeaderPage {
 public:
  // 删掉全部构造函数 / 析构函数，以保证内存安全
  BPlusTreeHeaderPage() = delete;
  BPlusTreeHeaderPage(const BPlusTreeHeaderPage &other) = delete;

  page_id_t root_page_id_;
};

}  // namespace bustub
