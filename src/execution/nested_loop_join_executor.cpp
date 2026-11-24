//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// nested_loop_join_executor.cpp
//
// Identification: src/execution/nested_loop_join_executor.cpp
//
// Copyright (c) 2015-2025, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#include "execution/executors/nested_loop_join_executor.h"
#include "binder/table_ref/bound_join_ref.h"
#include "common/exception.h"
#include "common/macros.h"

namespace bustub {

/**
 * 构造一个新的 NestedLoopJoinExecutor 实例。
 * @param exec_ctx 执行器上下文（ExecutorContext）
 * @param plan 待执行的嵌套循环连接计划节点
 * @param left_executor 为连接左侧（外表）产出 Tuple 的子执行器
 * @param right_executor 为连接右侧（内表）产出 Tuple 的子执行器
 */
NestedLoopJoinExecutor::NestedLoopJoinExecutor(ExecutorContext *exec_ctx, const NestedLoopJoinPlanNode *plan,
                                               std::unique_ptr<AbstractExecutor> &&left_executor,
                                               std::unique_ptr<AbstractExecutor> &&right_executor)
    : AbstractExecutor(exec_ctx) {
  if (plan->GetJoinType() != JoinType::LEFT && plan->GetJoinType() != JoinType::INNER) {
    // 只需实现 LEFT JOIN 和 INNER JOIN
    throw bustub::NotImplementedException(fmt::format("join type {} not supported", plan->GetJoinType()));
  }
  UNIMPLEMENTED("TODO(P3): Add implementation.");
}

/** 初始化连接算子 */
void NestedLoopJoinExecutor::Init() { UNIMPLEMENTED("TODO(P3): Add implementation."); }

/**
 * 从嵌套循环连接中产出下一批 Tuple。
 * @param[out] tuple_batch 连接算子产出的下一批 Tuple
 * @param[out] rid_batch 连接算子产出的下一批 RID
 * @param batch_size 本批最多包含的 Tuple 数量（默认 BUSTUB_BATCH_SIZE）
 * @return 如果产出了 Tuple 返回 `true`，如果没有更多 Tuple 返回 `false`
 */
auto NestedLoopJoinExecutor::Next(std::vector<bustub::Tuple> *tuple_batch, std::vector<bustub::RID> *rid_batch,
                                  size_t batch_size) -> bool {
  UNIMPLEMENTED("TODO(P3): Add implementation.");
}

}  // namespace bustub
