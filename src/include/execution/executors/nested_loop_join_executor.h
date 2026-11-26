//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// nested_loop_join_executor.h
//
// Identification: src/include/execution/executors/nested_loop_join_executor.h
//
// Copyright (c) 2015-2025, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#pragma once

#include <memory>
#include <vector>

#include "execution/executor_context.h"
#include "execution/executors/abstract_executor.h"
#include "execution/plans/nested_loop_join_plan.h"
#include "storage/table/tuple.h"

namespace bustub {

/**
 * NestedLoopJoinExecutor 对两张表执行嵌套循环连接（Nested-Loop Join）。
 */
class NestedLoopJoinExecutor : public AbstractExecutor {
 public:
  NestedLoopJoinExecutor(ExecutorContext *exec_ctx, const NestedLoopJoinPlanNode *plan,
                         std::unique_ptr<AbstractExecutor> &&left_executor,
                         std::unique_ptr<AbstractExecutor> &&right_executor);

  void Init() override;

  auto Next(std::vector<bustub::Tuple> *tuple_batch, std::vector<bustub::RID> *rid_batch, size_t batch_size)
      -> bool override;

  /** @return 嵌套循环连接的输出 Schema */
  auto GetOutputSchema() const -> const Schema & override { return plan_->OutputSchema(); };

 private:
  /** 构造连接后的输出 Tuple；若 right_tuple 为 nullptr（LEFT JOIN 未匹配），右侧列填 NULL */
  auto MakeJoinedTuple(const Tuple &left_tuple, const Tuple *right_tuple) const -> Tuple;

  /** 待执行的 NestedLoopJoin 计划节点 */
  const NestedLoopJoinPlanNode *plan_;
  /** 左侧（外表）与右侧（内表）子执行器 */
  std::unique_ptr<AbstractExecutor> left_executor_;
  std::unique_ptr<AbstractExecutor> right_executor_;

  /** 左表当前批次缓存与游标 */
  std::vector<Tuple> left_tuples_;
  std::vector<RID> left_rids_;
  size_t left_idx_{0};

  /** 右表当前批次缓存与游标 */
  std::vector<Tuple> right_tuples_;
  std::vector<RID> right_rids_;
  size_t right_idx_{0};

  /** 当前左表行是否已初始化右表扫描 / 是否至少匹配过一次右表行 */
  bool right_initialized_{false};
  bool left_matched_{false};
};

}  // namespace bustub
