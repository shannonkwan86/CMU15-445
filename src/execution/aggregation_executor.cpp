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
    : AbstractExecutor(exec_ctx) {
  UNIMPLEMENTED("TODO(P3): Add implementation.");
}

/** 初始化聚合算子 */
void AggregationExecutor::Init() { UNIMPLEMENTED("TODO(P3): Add implementation."); }

/**
 * 从聚合算子中产出下一批 Tuple。
 * @param[out] tuple_batch 聚合算子产出的下一批 Tuple
 * @param[out] rid_batch 聚合算子产出的下一批 RID
 * @param batch_size 本批最多包含的 Tuple 数量（默认 BUSTUB_BATCH_SIZE）
 * @return 如果产出了 Tuple 返回 `true`，如果没有更多 Tuple 返回 `false`
 */

auto AggregationExecutor::Next(std::vector<bustub::Tuple> *tuple_batch, std::vector<bustub::RID> *rid_batch,
                               size_t batch_size) -> bool {
  UNIMPLEMENTED("TODO(P3): Add implementation.");
}

/** 不要调用或删除此函数，否则将得零分。 */
auto AggregationExecutor::GetChildExecutor() const -> const AbstractExecutor * { return child_executor_.get(); }

}  // namespace bustub
