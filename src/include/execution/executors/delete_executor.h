//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// delete_executor.h
//
// Identification: src/include/execution/executors/delete_executor.h
//
// Copyright (c) 2015-2025, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#pragma once

#include <memory>
#include <utility>
#include <vector>

#include "execution/executor_context.h"
#include "execution/executors/abstract_executor.h"
#include "execution/plans/delete_plan.h"
#include "storage/table/tuple.h"

namespace bustub {

/**
 * DeleteExecutor 对表执行删除操作。
 * 待删除的记录总是从子执行器拉取。
 */
class DeleteExecutor : public AbstractExecutor {
 public:
  DeleteExecutor(ExecutorContext *exec_ctx, const DeletePlanNode *plan,
                 std::unique_ptr<AbstractExecutor> &&child_executor);

  void Init() override;

  auto Next(std::vector<bustub::Tuple> *tuple_batch, std::vector<bustub::RID> *rid_batch, size_t batch_size)
      -> bool override;

  /** @return 删除操作的输出 Schema */
  auto GetOutputSchema() const -> const Schema & override { return plan_->OutputSchema(); };

 private:
  /** 待执行的删除计划节点 */
  const DeletePlanNode *plan_;

  /** 提供待删除 Tuple 的 RID 的子执行器 */
  std::unique_ptr<AbstractExecutor> child_executor_;

  /** 标记是否已执行过删除（Delete::Next 只产出一次结果） */
  bool is_executed_{false};
};
}  // namespace bustub
