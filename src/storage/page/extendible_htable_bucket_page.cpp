//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// extendible_htable_bucket_page.cpp
//
// Identification: src/storage/page/extendible_htable_bucket_page.cpp
//
// Copyright (c) 2015-2025, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#include <optional>
#include <utility>

#include "common/exception.h"
#include "storage/page/extendible_htable_bucket_page.h"

namespace bustub {

/**
 * 从缓冲池新建一个桶页之后，必须调用本初始化方法来设置默认值
 * @param max_size 桶数组的最大容量
 */
template <typename K, typename V, typename KC>
void ExtendibleHTableBucketPage<K, V, KC>::Init(uint32_t max_size) {
  throw NotImplementedException("ExtendibleHTableBucketPage not implemented");
}

/**
 * 查找一个键
 *
 * @param key 要查找的键
 * @param[out] value 查到的值
 * @param cmp 比较器
 * @return 如果键和值都存在则返回 true，找不到则返回 false。
 */
template <typename K, typename V, typename KC>
auto ExtendibleHTableBucketPage<K, V, KC>::Lookup(const K &key, V &value, const KC &cmp) const -> bool {
  return false;
}

/**
 * 尝试把一个键值对插入桶中。
 *
 * @param key 要插入的键
 * @param value 要插入的值
 * @param cmp 使用的比较器
 * @return 插入成功返回 true；桶已满或同一个键已存在则返回 false
 */
template <typename K, typename V, typename KC>
auto ExtendibleHTableBucketPage<K, V, KC>::Insert(const K &key, const V &value, const KC &cmp) -> bool {
  return false;
}

/**
 * 删除一个键值对。
 *
 * @return 删除成功返回 true，找不到则返回 false
 */
template <typename K, typename V, typename KC>
auto ExtendibleHTableBucketPage<K, V, KC>::Remove(const K &key, const KC &cmp) -> bool {
  return false;
}

template <typename K, typename V, typename KC>
void ExtendibleHTableBucketPage<K, V, KC>::RemoveAt(uint32_t bucket_idx) {
  throw NotImplementedException("ExtendibleHTableBucketPage not implemented");
}

/**
 * @brief 读取桶中某个下标处的键。
 *
 * @param bucket_idx 要读取键的桶内下标
 * @return 桶中下标 bucket_idx 处的键
 */
template <typename K, typename V, typename KC>
auto ExtendibleHTableBucketPage<K, V, KC>::KeyAt(uint32_t bucket_idx) const -> K {
  return {};
}

/**
 * 读取桶中某个下标处的值。
 *
 * @param bucket_idx 要读取值的桶内下标
 * @return 桶中下标 bucket_idx 处的值
 */
template <typename K, typename V, typename KC>
auto ExtendibleHTableBucketPage<K, V, KC>::ValueAt(uint32_t bucket_idx) const -> V {
  return {};
}

/**
 * 读取桶中某个下标处的条目。
 *
 * @param bucket_idx 要读取条目的桶内下标
 * @return 桶中下标 bucket_idx 处的条目
 */
template <typename K, typename V, typename KC>
auto ExtendibleHTableBucketPage<K, V, KC>::EntryAt(uint32_t bucket_idx) const -> const std::pair<K, V> & {
  return array_[0];
}

/**
 * @return 桶中条目的数量
 */
template <typename K, typename V, typename KC>
auto ExtendibleHTableBucketPage<K, V, KC>::Size() const -> uint32_t {
  return 0;
}

/**
 * @return 桶是否已满
 */
template <typename K, typename V, typename KC>
auto ExtendibleHTableBucketPage<K, V, KC>::IsFull() const -> bool {
  return false;
}

/**
 * @return 桶是否为空
 */
template <typename K, typename V, typename KC>
auto ExtendibleHTableBucketPage<K, V, KC>::IsEmpty() const -> bool {
  return false;
}

template class ExtendibleHTableBucketPage<int, int, IntComparator>;
template class ExtendibleHTableBucketPage<GenericKey<4>, RID, GenericComparator<4>>;
template class ExtendibleHTableBucketPage<GenericKey<8>, RID, GenericComparator<8>>;
template class ExtendibleHTableBucketPage<GenericKey<16>, RID, GenericComparator<16>>;
template class ExtendibleHTableBucketPage<GenericKey<32>, RID, GenericComparator<32>>;
template class ExtendibleHTableBucketPage<GenericKey<64>, RID, GenericComparator<64>>;

}  // namespace bustub
