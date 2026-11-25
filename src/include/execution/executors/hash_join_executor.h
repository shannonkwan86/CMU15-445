//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// hash_join_executor.h
//
// Identification: src/include/execution/executors/hash_join_executor.h
//
// Copyright (c) 2015-2025, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#pragma once

#include <memory>
#include <unordered_map>
#include <vector>

#include "common/util/hash_util.h"
#include "execution/executor_context.h"
#include "execution/executors/abstract_executor.h"
#include "execution/plans/hash_join_plan.h"
#include "storage/table/tuple.h"

namespace bustub {

struct HashJoinKey {
  std::vector<Value> keys_;

  auto operator==(const HashJoinKey &other) const -> bool {
    if (keys_.size() != other.keys_.size()) {
      return false;
    }
    for (size_t i = 0; i < keys_.size(); ++i) {
      if (keys_[i].CompareEquals(other.keys_[i]) != CmpBool::CmpTrue) {
        return false;
      }
    }
    return true;
  }
};

}  // namespace bustub

namespace std {
template <>
struct hash<bustub::HashJoinKey> {
  auto operator()(const bustub::HashJoinKey &join_key) const -> std::size_t {
    size_t curr_hash = 0;
    for (const auto &key : join_key.keys_) {
      if (!key.IsNull()) {
        curr_hash = bustub::HashUtil::CombineHashes(curr_hash, bustub::HashUtil::HashValue(&key));
      }
    }
    return curr_hash;
  }
};
}  // namespace std

namespace bustub {

/**
 * HashJoinExecutor 对两张表执行哈希连接（Hash Join）。
 */
class HashJoinExecutor : public AbstractExecutor {
 public:
  HashJoinExecutor(ExecutorContext *exec_ctx, const HashJoinPlanNode *plan,
                   std::unique_ptr<AbstractExecutor> &&left_child, std::unique_ptr<AbstractExecutor> &&right_child);

  void Init() override;

  auto Next(std::vector<bustub::Tuple> *tuple_batch, std::vector<bustub::RID> *rid_batch, size_t batch_size)
      -> bool override;

  /** @return 哈希连接的输出 Schema */
  auto GetOutputSchema() const -> const Schema & override { return plan_->OutputSchema(); };

 private:
  auto MakeJoinKey(const Tuple &tuple, const Schema &schema, const std::vector<AbstractExpressionRef> &exprs) const
      -> HashJoinKey;
  auto MakeJoinedTuple(const Tuple &left_tuple, const Tuple *right_tuple) const -> Tuple;

  /** 待执行的 HashJoin 计划节点 */
  const HashJoinPlanNode *plan_;
  /** 左右子执行器 */
  std::unique_ptr<AbstractExecutor> left_child_;
  std::unique_ptr<AbstractExecutor> right_child_;

  /** 右表构建的内存哈希表 */
  std::unordered_map<HashJoinKey, std::vector<Tuple>> ht_;

  /** 左表当前批次缓存与探测游标 */
  std::vector<Tuple> left_tuples_;
  std::vector<RID> left_rids_;
  size_t left_idx_{0};

  /** 当前左表行在哈希表中命中的右表行列表与遍历下标 */
  const std::vector<Tuple> *matched_right_tuples_{nullptr};
  size_t right_idx_{0};
};

}  // namespace bustub
