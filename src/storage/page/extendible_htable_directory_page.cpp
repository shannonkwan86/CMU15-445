//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// extendible_htable_directory_page.cpp
//
// Identification: src/storage/page/extendible_htable_directory_page.cpp
//
// Copyright (c) 2015-2025, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#include "storage/page/extendible_htable_directory_page.h"

#include <algorithm>
#include <unordered_map>

#include "common/config.h"
#include "common/logger.h"

namespace bustub {

/**
 * 从缓冲池新建一个目录页之后，必须调用本初始化方法来设置默认值
 * @param max_depth 目录页的最大深度
 */
void ExtendibleHTableDirectoryPage::Init(uint32_t max_depth) {
  throw NotImplementedException("ExtendibleHTableDirectoryPage is not implemented");
}

/**
 * 得到键经哈希后落入的桶下标
 *
 * @param hash 键的哈希值
 * @return 当前键经哈希后落入的桶下标
 */
auto ExtendibleHTableDirectoryPage::HashToBucketIndex(uint32_t hash) const -> uint32_t { return 0; }

/**
 * 通过目录下标查找对应的桶页
 *
 * @param bucket_idx 要查找的目录下标
 * @return 与 bucket_idx 对应的桶页 ID
 */
auto ExtendibleHTableDirectoryPage::GetBucketPageId(uint32_t bucket_idx) const -> page_id_t { return INVALID_PAGE_ID; }

/**
 * 用桶下标和页 ID 更新目录中的映射
 *
 * @param bucket_idx 要写入 page_id 的目录下标
 * @param bucket_page_id 要写入的页 ID
 */
void ExtendibleHTableDirectoryPage::SetBucketPageId(uint32_t bucket_idx, page_id_t bucket_page_id) {
  throw NotImplementedException("ExtendibleHTableDirectoryPage is not implemented");
}

/**
 * 得到某个下标的「分裂镜像」下标
 *
 * @param bucket_idx 要查找分裂镜像的目录下标
 * @return 分裂镜像的目录下标
 **/
auto ExtendibleHTableDirectoryPage::GetSplitImageIndex(uint32_t bucket_idx) const -> uint32_t { return 0; }

/**
 * 得到哈希表目录的全局深度
 *
 * @return 目录的全局深度
 */
auto ExtendibleHTableDirectoryPage::GetGlobalDepth() const -> uint32_t { return 0; }

/**
 * 把目录的全局深度加一
 */
void ExtendibleHTableDirectoryPage::IncrGlobalDepth() {
  throw NotImplementedException("ExtendibleHTableDirectoryPage is not implemented");
}

/**
 * 把目录的全局深度减一
 */
void ExtendibleHTableDirectoryPage::DecrGlobalDepth() {
  throw NotImplementedException("ExtendibleHTableDirectoryPage is not implemented");
}

/**
 * @return 如果目录可以收缩则返回 true
 */
auto ExtendibleHTableDirectoryPage::CanShrink() -> bool { return false; }

/**
 * @return 当前目录的大小
 */
auto ExtendibleHTableDirectoryPage::Size() const -> uint32_t { return 0; }

/**
 * 得到 bucket_idx 处桶的局部深度
 *
 * @param bucket_idx 要查找的桶下标
 * @return bucket_idx 处桶的局部深度
 */
auto ExtendibleHTableDirectoryPage::GetLocalDepth(uint32_t bucket_idx) const -> uint32_t { return 0; }

/**
 * 把 bucket_idx 处桶的局部深度设为 local_depth
 *
 * @param bucket_idx 要更新的桶下标
 * @param local_depth 新的局部深度
 */
void ExtendibleHTableDirectoryPage::SetLocalDepth(uint32_t bucket_idx, uint8_t local_depth) {
  throw NotImplementedException("ExtendibleHTableDirectoryPage is not implemented");
}

/**
 * 把 bucket_idx 处桶的局部深度加一
 * @param bucket_idx 要自增的桶下标
 */
void ExtendibleHTableDirectoryPage::IncrLocalDepth(uint32_t bucket_idx) {
  throw NotImplementedException("ExtendibleHTableDirectoryPage is not implemented");
}

/**
 * 把 bucket_idx 处桶的局部深度减一
 * @param bucket_idx 要自减的桶下标
 */
void ExtendibleHTableDirectoryPage::DecrLocalDepth(uint32_t bucket_idx) {
  throw NotImplementedException("ExtendibleHTableDirectoryPage is not implemented");
}

}  // namespace bustub
