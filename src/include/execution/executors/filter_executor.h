//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// filter_executor.h
//
// Identification: src/include/execution/executors/filter_executor.h
//
// Copyright (c) 2015-2025, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#pragma once

#include <memory>
#include <vector>

#include "execution/executor_context.h"
#include "execution/executors/abstract_executor.h"
#include "execution/plans/filter_plan.h"
#include "execution/plans/seq_scan_plan.h"
#include "storage/table/tuple.h"

namespace bustub {

/**
 * FilterExecutor 执行过滤（filter）操作。
 */
class FilterExecutor : public AbstractExecutor {
 public:
  FilterExecutor(ExecutorContext *exec_ctx, const FilterPlanNode *plan,
                 std::unique_ptr<AbstractExecutor> &&child_executor);

  void Init() override;

  auto Next(std::vector<bustub::Tuple> *tuple_batch, std::vector<bustub::RID> *rid_batch, size_t batch_size)
      -> bool override;

  /** @return 过滤计划节点的输出 Schema */
  auto GetOutputSchema() const -> const Schema & override { return plan_->OutputSchema(); }

 private:
  /** 待执行的过滤计划节点 */
  const FilterPlanNode *plan_;

  /** 提供输入 Tuple 的子执行器 */
  std::unique_ptr<AbstractExecutor> child_executor_;

  /** 从子执行器拉取的 Tuple 批次和 RID 批次 */
  std::vector<Tuple> child_tuples_{};
  std::vector<RID> child_rids_{};

  /** 当前在子执行器批次中的偏移位置 */
  size_t child_offset_ = 0;
};
}  // namespace bustub
