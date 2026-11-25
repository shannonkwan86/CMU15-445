//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// hash_join_executor.cpp
//
// Identification: src/execution/hash_join_executor.cpp
//
// Copyright (c) 2015-2025, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#include "execution/executors/hash_join_executor.h"

#include <algorithm>
#include <utility>
#include <vector>

#include "common/macros.h"
#include "type/value_factory.h"

namespace bustub {

namespace {

auto HasNullKey(const HashJoinKey &join_key) -> bool {
  return std::any_of(join_key.keys_.begin(), join_key.keys_.end(), [](const Value &k) { return k.IsNull(); });
}

}  // namespace

/**
 * 构造一个新的 HashJoinExecutor 实例。
 * @param exec_ctx 执行器上下文（ExecutorContext）
 * @param plan 待执行的 HashJoin 计划节点
 * @param left_child 为连接左侧产出 Tuple 的子执行器
 * @param right_child 为连接右侧产出 Tuple 的子执行器
 */
HashJoinExecutor::HashJoinExecutor(ExecutorContext *exec_ctx, const HashJoinPlanNode *plan,
                                   std::unique_ptr<AbstractExecutor> &&left_child,
                                   std::unique_ptr<AbstractExecutor> &&right_child)
    : AbstractExecutor(exec_ctx),
      plan_(plan),
      left_child_(std::move(left_child)),
      right_child_(std::move(right_child)) {
  if (plan->GetJoinType() != JoinType::LEFT && plan->GetJoinType() != JoinType::INNER) {
    // 只需实现 LEFT JOIN 和 INNER JOIN
    throw bustub::NotImplementedException(fmt::format("join type {} not supported", plan->GetJoinType()));
  }
}

auto HashJoinExecutor::MakeJoinKey(const Tuple &tuple, const Schema &schema,
                                   const std::vector<AbstractExpressionRef> &exprs) const -> HashJoinKey {
  std::vector<Value> keys;
  keys.reserve(exprs.size());
  for (const auto &expr : exprs) {
    keys.push_back(expr->Evaluate(&tuple, schema));
  }
  return {std::move(keys)};
}

auto HashJoinExecutor::MakeJoinedTuple(const Tuple &left_tuple, const Tuple *right_tuple) const -> Tuple {
  const auto &left_schema = left_child_->GetOutputSchema();
  const auto &right_schema = right_child_->GetOutputSchema();

  std::vector<Value> values;
  values.reserve(left_schema.GetColumnCount() + right_schema.GetColumnCount());
  for (uint32_t i = 0; i < left_schema.GetColumnCount(); ++i) {
    values.push_back(left_tuple.GetValue(&left_schema, i));
  }
  for (uint32_t i = 0; i < right_schema.GetColumnCount(); ++i) {
    if (right_tuple != nullptr) {
      values.push_back(right_tuple->GetValue(&right_schema, i));
    } else {
      values.push_back(ValueFactory::GetNullValueByType(right_schema.GetColumn(i).GetType()));
    }
  }
  return {std::move(values), &GetOutputSchema()};
}

/** 初始化哈希连接 */
void HashJoinExecutor::Init() {
  left_child_->Init();
  right_child_->Init();
  ht_.clear();
  left_tuples_.clear();
  left_rids_.clear();
  left_idx_ = 0;
  matched_right_tuples_ = nullptr;
  right_idx_ = 0;

  // Build 阶段：将右表所有非 NULL 连接键的 Tuple 建入内存哈希表
  const auto &right_schema = right_child_->GetOutputSchema();
  std::vector<Tuple> right_tuples{};
  std::vector<RID> right_rids{};
  while (right_child_->Next(&right_tuples, &right_rids, BUSTUB_BATCH_SIZE)) {
    for (auto &tuple : right_tuples) {
      auto key = MakeJoinKey(tuple, right_schema, plan_->RightJoinKeyExpressions());
      if (!HasNullKey(key)) {
        ht_[std::move(key)].push_back(std::move(tuple));
      }
    }
  }
}

/**
 * 从哈希连接中产出下一批 Tuple。
 * @param[out] tuple_batch 哈希连接产出的下一批 Tuple
 * @param[out] rid_batch 哈希连接产出的下一批 RID
 * @param batch_size 本批最多包含的 Tuple 数量（默认 BUSTUB_BATCH_SIZE）
 * @return 如果产出了 Tuple 返回 `true`，如果没有更多 Tuple 返回 `false`
 */
auto HashJoinExecutor::Next(std::vector<bustub::Tuple> *tuple_batch, std::vector<bustub::RID> *rid_batch,
                            size_t batch_size) -> bool {
  tuple_batch->clear();
  rid_batch->clear();

  const auto &left_schema = left_child_->GetOutputSchema();

  // Probe 阶段：逐行读取左表并查哈希表配对
  while (tuple_batch->size() < batch_size) {
    if (matched_right_tuples_ != nullptr) {
      if (right_idx_ < matched_right_tuples_->size()) {
        tuple_batch->push_back(MakeJoinedTuple(left_tuples_[left_idx_], &(*matched_right_tuples_)[right_idx_++]));
        rid_batch->emplace_back();
        continue;
      }
      matched_right_tuples_ = nullptr;
      right_idx_ = 0;
      ++left_idx_;
    }

    if (left_idx_ >= left_tuples_.size()) {
      left_idx_ = 0;
      if (!left_child_->Next(&left_tuples_, &left_rids_, batch_size)) {
        break;
      }
      continue;
    }

    const auto &left_tuple = left_tuples_[left_idx_];
    auto left_key = MakeJoinKey(left_tuple, left_schema, plan_->LeftJoinKeyExpressions());
    auto it = HasNullKey(left_key) ? ht_.end() : ht_.find(left_key);

    if (it != ht_.end() && !it->second.empty()) {
      matched_right_tuples_ = &it->second;
      right_idx_ = 0;
      continue;
    }

    if (plan_->GetJoinType() == JoinType::LEFT) {
      tuple_batch->push_back(MakeJoinedTuple(left_tuple, nullptr));
      rid_batch->emplace_back();
    }
    ++left_idx_;
  }

  return !tuple_batch->empty();
}

}  // namespace bustub
