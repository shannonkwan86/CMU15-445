//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// aggregation_executor.cpp
//
// Identification: src/execution/aggregation_executor.cpp
//
// Copyright (c) 2015-2025, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#include <memory>
#include "common/macros.h"

#include "execution/executors/aggregation_executor.h"

namespace bustub {

/**
 * 构造一个新的 AggregationExecutor 实例。
 * @param exec_ctx 执行器上下文（ExecutorContext）
 * @param plan 待执行的聚合计划节点
 * @param child_executor 提供输入 Tuple 的子执行器（可能为 `nullptr`）
 */
AggregationExecutor::AggregationExecutor(ExecutorContext *exec_ctx, const AggregationPlanNode *plan,
                                         std::unique_ptr<AbstractExecutor> &&child_executor)
    : AbstractExecutor(exec_ctx),
      plan_(plan),
      child_executor_(std::move(child_executor)),
      aht_(plan_->GetAggregates(), plan_->GetAggregateTypes()),
      aht_iterator_(aht_.Begin()) {}

/** 初始化聚合算子 */
void AggregationExecutor::Init() {
  child_executor_->Init();
  aht_.Clear();

  std::vector<Tuple> child_tuples{};
  std::vector<RID> child_rids{};
  while (child_executor_->Next(&child_tuples, &child_rids, BUSTUB_BATCH_SIZE)) {
    for (const auto &tuple : child_tuples) {
      aht_.InsertCombine(MakeAggregateKey(&tuple), MakeAggregateValue(&tuple));
    }
  }

  // 若输入为空且没有 GROUP BY，仍需输出 1 行全局初始聚合结果（COUNT(*) 为 0，其余为 NULL）
  if (aht_.Begin() == aht_.End() && plan_->GetGroupBys().empty()) {
    aht_.InsertDefault(AggregateKey{});
  }

  aht_iterator_ = aht_.Begin();
}

/**
 * 从聚合算子中产出下一批 Tuple。
 * @param[out] tuple_batch 聚合算子产出的下一批 Tuple
 * @param[out] rid_batch 聚合算子产出的下一批 RID
 * @param batch_size 本批最多包含的 Tuple 数量（默认 BUSTUB_BATCH_SIZE）
 * @return 如果产出了 Tuple 返回 `true`，如果没有更多 Tuple 返回 `false`
 */

auto AggregationExecutor::Next(std::vector<bustub::Tuple> *tuple_batch, std::vector<bustub::RID> *rid_batch,
                               size_t batch_size) -> bool {
  tuple_batch->clear();
  rid_batch->clear();

  while (aht_iterator_ != aht_.End() && tuple_batch->size() < batch_size) {
    std::vector<Value> values;
    const auto &group_bys = aht_iterator_.Key().group_bys_;
    const auto &aggregates = aht_iterator_.Val().aggregates_;
    values.reserve(group_bys.size() + aggregates.size());
    values.insert(values.end(), group_bys.begin(), group_bys.end());
    values.insert(values.end(), aggregates.begin(), aggregates.end());

    tuple_batch->emplace_back(std::move(values), &GetOutputSchema());
    rid_batch->emplace_back();
    ++aht_iterator_;
  }

  return !tuple_batch->empty();
}

/** 不要调用或删除此函数，否则将得零分。 */
auto AggregationExecutor::GetChildExecutor() const -> const AbstractExecutor * { return child_executor_.get(); }

}  // namespace bustub
