//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// count_min_sketch.cpp
//
// Identification: src/primer/count_min_sketch.cpp
//
// Copyright (c) 2015-2025, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#include "primer/count_min_sketch.h"

#include <algorithm>  // std::min
#include <cstdint>    // UINT32_MAX
#include <stdexcept>
#include <string>

namespace bustub {

/**
 * Constructor for the count-min sketch.
 *
 * @param width The width of the sketch matrix.
 * @param depth The depth of the sketch matrix.
 * @throws std::invalid_argument if width or depth are zero.
 */
template <typename KeyType>
CountMinSketch<KeyType>::CountMinSketch(uint32_t width, uint32_t depth)
    : width_(width), depth_(depth), counters_(static_cast<size_t>(width) * static_cast<size_t>(depth)) {
  if (width_ == 0 || depth_ == 0) {
    throw std::invalid_argument("CountMinSketch width and depth must be non-zero.");
  }
  // C++17: the default constructor of std::atomic does not guarantee zero-initialization.
  for (auto &counter : counters_) {
    counter.store(0, std::memory_order_relaxed);
  }

  /** @fall2025 PLEASE DO NOT MODIFY THE FOLLOWING */
  // Initialize seeded hash functions
  hash_functions_.reserve(depth_);
  for (size_t i = 0; i < depth_; i++) {
    hash_functions_.push_back(this->HashFunction(i));
  }
}

template <typename KeyType>
CountMinSketch<KeyType>::CountMinSketch(CountMinSketch &&other) noexcept
    : width_(other.width_), depth_(other.depth_), counters_(std::move(other.counters_)) {
  hash_functions_.reserve(depth_);
  for (size_t i = 0; i < depth_; i++) {
    hash_functions_.push_back(this->HashFunction(i));
  }
}

template <typename KeyType>
auto CountMinSketch<KeyType>::operator=(CountMinSketch &&other) noexcept -> CountMinSketch & {
  if (this != &other) {
    width_ = other.width_;
    depth_ = other.depth_;
    counters_ = std::move(other.counters_);

    hash_functions_.clear();
    hash_functions_.reserve(depth_);
    for (size_t i = 0; i < depth_; i++) {
      hash_functions_.push_back(this->HashFunction(i));
    }
  }
  return *this;
}

template <typename KeyType>
void CountMinSketch<KeyType>::Insert(const KeyType &item) {
  for (size_t i = 0; i < depth_; i++) {
    size_t col = hash_functions_[i](item);
    counters_[i * width_ + col].fetch_add(1, std::memory_order_relaxed);
  }
}

template <typename KeyType>
void CountMinSketch<KeyType>::Merge(const CountMinSketch<KeyType> &other) {
  if (width_ != other.width_ || depth_ != other.depth_) {
    throw std::invalid_argument("Incompatible CountMinSketch dimensions for merge.");
  }
  for (size_t i = 0; i < counters_.size(); i++) {
    counters_[i].fetch_add(other.counters_[i].load());
  }
}

template <typename KeyType>
auto CountMinSketch<KeyType>::Count(const KeyType &item) const -> uint32_t {
  uint32_t ans = UINT32_MAX;
  for (size_t i = 0; i < depth_; i++) {
    size_t col = hash_functions_[i](item);
    uint32_t loaded = counters_[i * width_ + col].load(std::memory_order_relaxed);
    ans = std::min(ans, loaded);
  }
  return ans;
}

template <typename KeyType>
void CountMinSketch<KeyType>::Clear() {
  for (auto &counter : counters_) {
    counter.store(0, std::memory_order_relaxed);
  }
}

template <typename KeyType>
auto CountMinSketch<KeyType>::TopK(uint16_t k, const std::vector<KeyType> &candidates)
    -> std::vector<std::pair<KeyType, uint32_t>> {
  std::vector<std::pair<KeyType, uint32_t>> result;
  result.reserve(candidates.size());
  for (const auto &item : candidates) {
    result.emplace_back(item, Count(item));
  }
  std::sort(result.begin(), result.end(), [](const auto &a, const auto &b) { return a.second > b.second; });
  if (result.size() > k) {
    result.resize(k);
  }
  return result;
}

// Explicit instantiations for all types used in tests
template class CountMinSketch<std::string>;
template class CountMinSketch<int64_t>;  // For int64_t tests
template class CountMinSketch<int>;      // This covers both int and int32_t
}  // namespace bustub
