//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// nested_index_join_executor.cpp
//
// Identification: src/execution/nested_index_join_executor.cpp
//
// Copyright (c) 2015-2025, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#include "execution/executors/nested_index_join_executor.h"
#include "common/macros.h"

namespace bustub {

/**
 * 创建一个新的嵌套索引连接执行器。
 * @param exec_ctx 执行器上下文（ExecutorContext）
 * @param plan 待执行的嵌套索引连接计划节点
 * @param child_executor 外表（左表）对应的子执行器
 */
NestedIndexJoinExecutor::NestedIndexJoinExecutor(ExecutorContext *exec_ctx, const NestedIndexJoinPlanNode *plan,
                                                 std::unique_ptr<AbstractExecutor> &&child_executor)
    : AbstractExecutor(exec_ctx) {
  if (plan->GetJoinType() != JoinType::LEFT && plan->GetJoinType() != JoinType::INNER) {
    // 只需实现 LEFT JOIN 和 INNER JOIN
    throw bustub::NotImplementedException(fmt::format("join type {} not supported", plan->GetJoinType()));
  }
  UNIMPLEMENTED("TODO(P3): Add implementation.");
}

void NestedIndexJoinExecutor::Init() { UNIMPLEMENTED("TODO(P3): Add implementation."); }

auto NestedIndexJoinExecutor::Next(std::vector<bustub::Tuple> *tuple_batch, std::vector<bustub::RID> *rid_batch,
                                   size_t batch_size) -> bool {
  UNIMPLEMENTED("TODO(P3): Add implementation.");
}

}  // namespace bustub
