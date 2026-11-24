//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// extendible_htable_header_page.cpp
//
// Identification: src/storage/page/extendible_htable_header_page.cpp
//
// Copyright (c) 2015-2025, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#include "storage/page/extendible_htable_header_page.h"

#include "common/exception.h"

namespace bustub {

/**
 * 从缓冲池新建一个头部页之后，必须调用本初始化方法来设置默认值
 * @param max_depth 头部页的最大深度
 */
void ExtendibleHTableHeaderPage::Init(uint32_t max_depth) {
  throw NotImplementedException("ExtendibleHTableHeaderPage is not implemented");
}

/**
 * 得到键经哈希后落入的目录下标
 *
 * @param hash 键的哈希值
 * @return 键经哈希后落入的目录下标
 */
auto ExtendibleHTableHeaderPage::HashToDirectoryIndex(uint32_t hash) const -> uint32_t { return 0; }

/**
 * 得到某个下标处的目录页 ID
 *
 * @param directory_idx 目录页 ID 数组中的下标
 * @return 该下标处的目录页 ID
 */
auto ExtendibleHTableHeaderPage::GetDirectoryPageId(uint32_t directory_idx) const -> page_id_t { return 0; }

/**
 * @brief 设置某个下标处的目录页 ID
 *
 * @param directory_idx 目录页 ID 数组中的下标
 * @param directory_page_id 目录页的页 ID
 */
void ExtendibleHTableHeaderPage::SetDirectoryPageId(uint32_t directory_idx, page_id_t directory_page_id) {
  throw NotImplementedException("ExtendibleHTableHeaderPage is not implemented");
}

/**
 * @brief 得到头部页能够容纳的目录页 ID 的最大数量
 */
auto ExtendibleHTableHeaderPage::MaxSize() const -> uint32_t { return 0; }

}  // namespace bustub
