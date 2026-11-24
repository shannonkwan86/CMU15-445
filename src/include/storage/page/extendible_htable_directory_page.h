//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// extendible_htable_directory_page.h
//
// Identification: src/include/storage/page/extendible_htable_directory_page.h
//
// Copyright (c) 2015-2025, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

/**
 * 目录页布局：
 *  --------------------------------------------------------------------------------------
 * | MaxDepth (4) | GlobalDepth (4) | LocalDepths (512) | BucketPageIds(2048) | Free(1528)
 *  --------------------------------------------------------------------------------------
 */

#pragma once

#include <cassert>
#include <climits>
#include <cstdlib>
#include <string>

#include "common/config.h"
#include "storage/index/generic_key.h"

namespace bustub {

static constexpr uint64_t HTABLE_DIRECTORY_PAGE_METADATA_SIZE = sizeof(uint32_t) * 2;

/**
 * HTABLE_DIRECTORY_ARRAY_SIZE 是可扩展哈希索引的目录页里能够容纳的 page_id 数量。
 * 取 512 是因为目录数组必须按 2 的幂增长，而 1024 个 page_id 就没地方放其他成员变量了。
 */
static constexpr uint64_t HTABLE_DIRECTORY_MAX_DEPTH = 9;
static constexpr uint64_t HTABLE_DIRECTORY_ARRAY_SIZE = 1 << HTABLE_DIRECTORY_MAX_DEPTH;

/**
 * 可扩展哈希表的目录页。
 */
class ExtendibleHTableDirectoryPage {
 public:
  // 删掉全部构造函数 / 析构函数，以保证内存安全
  ExtendibleHTableDirectoryPage() = delete;
  DISALLOW_COPY_AND_MOVE(ExtendibleHTableDirectoryPage);

  void Init(uint32_t max_depth = HTABLE_DIRECTORY_MAX_DEPTH);

  auto HashToBucketIndex(uint32_t hash) const -> uint32_t;

  auto GetBucketPageId(uint32_t bucket_idx) const -> page_id_t;

  void SetBucketPageId(uint32_t bucket_idx, page_id_t bucket_page_id);

  auto GetSplitImageIndex(uint32_t bucket_idx) const -> uint32_t;

  /**
   * GetGlobalDepthMask - 返回一个「低 global_depth 位为 1、其余为 0」的掩码。
   *
   * 在可扩展哈希中，我们用下面这个「哈希 + 掩码」函数把键映射到目录下标：
   *
   * DirectoryIndex = Hash(key) & GLOBAL_DEPTH_MASK
   *
   * 其中 GLOBAL_DEPTH_MASK 是一个从最低位开始、恰好有 GLOBAL_DEPTH 个 1 的掩码。
   * 例如全局深度为 3 时，在 32 位表示中对应的就是 0x00000007。
   *
   * @return 低 global_depth 位为 1、其余为 0 的掩码（1 从最低位开始向上）
   */
  auto GetGlobalDepthMask() const -> uint32_t;

  /**
   * GetLocalDepthMask - 与全局深度掩码相同，只不过用的是
   * 下标 bucket_idx 处那个桶的局部深度
   *
   * @param bucket_idx 用来查局部深度的下标
   * @return 低 local_depth 位为 1、其余为 0 的掩码（1 从最低位开始向上）
   */
  auto GetLocalDepthMask(uint32_t bucket_idx) const -> uint32_t;

  auto GetGlobalDepth() const -> uint32_t;

  auto GetMaxDepth() const -> uint32_t;

  void IncrGlobalDepth();

  void DecrGlobalDepth();

  auto CanShrink() -> bool;

  auto Size() const -> uint32_t;

  /**
   * @return 目录的最大大小
   */
  auto MaxSize() const -> uint32_t;

  auto GetLocalDepth(uint32_t bucket_idx) const -> uint32_t;

  void SetLocalDepth(uint32_t bucket_idx, uint8_t local_depth);

  void IncrLocalDepth(uint32_t bucket_idx);

  void DecrLocalDepth(uint32_t bucket_idx);

  /**
   * VerifyIntegrity
   *
   * 校验下面几条不变量：
   * (1) 所有 LD <= GD。
   * (2) 每个桶都恰好有 2^(GD - LD) 个指针指向它。
   * (3) 指向同一个 bucket_page_id 的那些下标，LD 必须相同。
   */
  void VerifyIntegrity() const;

  /**
   * 打印当前目录
   */
  void PrintDirectory() const;

 private:
  uint32_t max_depth_;
  uint32_t global_depth_;
  uint8_t local_depths_[HTABLE_DIRECTORY_ARRAY_SIZE];
  page_id_t bucket_page_ids_[HTABLE_DIRECTORY_ARRAY_SIZE];
};

static_assert(sizeof(page_id_t) == 4);

static_assert(sizeof(ExtendibleHTableDirectoryPage) <= BUSTUB_PAGE_SIZE);

}  // namespace bustub
