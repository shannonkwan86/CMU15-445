#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <utility>
#include <vector>

#include "common/config.h"
#include "storage/table/tuple.h"

namespace bustub {

/**
 * 用于存放外部归并排序（External Merge Sort）和哈希连接（Hash Join）中间结果的页面。
 * 支持变长 Tuple（例如包含 VARCHAR 的 Tuple）。
 */
class IntermediateResultPage {
 public:
  static constexpr uint32_t PAYLOAD_SIZE = BUSTUB_PAGE_SIZE - 2 * sizeof(uint32_t);

  static auto TupleStorageBytes(const Tuple &tuple) -> uint32_t {
    const uint32_t raw_bytes = sizeof(int32_t) + tuple.GetLength();
    return (raw_bytes + 3U) & ~3U;
  }

  void Init() {
    num_tuples_ = 0;
    free_end_ = PAYLOAD_SIZE;
  }

  auto Append(const Tuple &tuple) -> bool {
    const uint32_t tuple_bytes = TupleStorageBytes(tuple);
    const uint32_t dir_bytes = (num_tuples_ + 1) * sizeof(uint32_t);
    if (free_end_ < dir_bytes + tuple_bytes) {
      return false;
    }
    free_end_ -= tuple_bytes;
    tuple.SerializeTo(payload_ + free_end_);
    reinterpret_cast<uint32_t *>(payload_)[num_tuples_] = free_end_;
    ++num_tuples_;
    return true;
  }

  auto GetTuple(uint32_t idx) const -> Tuple {
    const uint32_t offset = reinterpret_cast<const uint32_t *>(payload_)[idx];
    Tuple tuple;
    tuple.DeserializeFrom(payload_ + offset);
    return tuple;
  }

  auto GetNumTuples() const -> uint32_t { return num_tuples_; }

 private:
  uint32_t num_tuples_;
  uint32_t free_end_;
  char payload_[PAYLOAD_SIZE];
};

static_assert(sizeof(IntermediateResultPage) == BUSTUB_PAGE_SIZE);

}  // namespace bustub
