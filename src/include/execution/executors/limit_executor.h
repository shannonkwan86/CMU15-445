//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// limit_executor.h
//
// Identification: src/include/execution/executors/limit_executor.h
//
// Copyright (c) 2015-2025, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#pragma once

#include <memory>
#include <utility>
#include <vector>

#include "execution/executors/abstract_executor.h"
#include "execution/plans/limit_plan.h"

namespace bustub {

/**
 * LimitExecutor 限制子算子产出的输出 Tuple 总数。
 */
class LimitExecutor : public AbstractExecutor {
 public:
  LimitExecutor(ExecutorContext *exec_ctx, const LimitPlanNode *plan,
                std::unique_ptr<AbstractExecutor> &&child_executor);

  void Init() override;

  auto Next(std::vector<bustub::Tuple> *tuple_batch, std::vector<bustub::RID> *rid_batch, size_t batch_size)
      -> bool override;

  /** @return Limit 算子的输出 Schema */
  auto GetOutputSchema() const -> const Schema & override { return plan_->OutputSchema(); };

 private:
  /** 待执行的 Limit 计划节点 */
  const LimitPlanNode *plan_;

  /** 提供输入 Tuple 的子执行器 */
  std::unique_ptr<AbstractExecutor> child_executor_;

  /** 已向下游产出的 Tuple 计数 */
  size_t count_{0};
};
}  // namespace bustub
