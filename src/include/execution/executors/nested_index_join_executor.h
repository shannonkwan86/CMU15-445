//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// nested_index_join_executor.h
//
// Identification: src/include/execution/executors/nested_index_join_executor.h
//
// Copyright (c) 2015-2025, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#pragma once

#include <memory>
#include <vector>

#include "execution/executor_context.h"
#include "execution/executors/abstract_executor.h"
#include "execution/plans/nested_index_join_plan.h"
#include "storage/table/tuple.h"

namespace bustub {

/**
 * NestedIndexJoinExecutor 执行基于索引的嵌套循环连接（Nested Index Join）。
 */
class NestedIndexJoinExecutor : public AbstractExecutor {
 public:
  NestedIndexJoinExecutor(ExecutorContext *exec_ctx, const NestedIndexJoinPlanNode *plan,
                          std::unique_ptr<AbstractExecutor> &&child_executor);

  /** @return 嵌套索引连接的输出 Schema */
  auto GetOutputSchema() const -> const Schema & override { return plan_->OutputSchema(); }

  void Init() override;

  auto Next(std::vector<bustub::Tuple> *tuple_batch, std::vector<bustub::RID> *rid_batch, size_t batch_size)
      -> bool override;

 private:
  /** 构造连接后的输出 Tuple；若 right_tuple 为 nullptr（LEFT JOIN 未匹配），右侧列填 NULL */
  auto MakeJoinedTuple(const Tuple &left_tuple, const Tuple *right_tuple) const -> Tuple;

  /** 待执行的嵌套索引连接计划节点 */
  const NestedIndexJoinPlanNode *plan_;
  /** 外表（左表）子执行器 */
  std::unique_ptr<AbstractExecutor> child_executor_;
  /** 内表（右表）与索引元信息 */
  const TableInfo *inner_table_info_{nullptr};
  const IndexInfo *index_info_{nullptr};

  /** 左表当前批次缓存与游标 */
  std::vector<Tuple> left_tuples_;
  std::vector<RID> left_rids_;
  size_t left_idx_{0};

  /** 当前左表行在右表索引中查到的 RID 列表与遍历游标 */
  std::vector<RID> matched_rids_;
  size_t matched_idx_{0};
  bool rids_fetched_{false};
  bool left_matched_{false};
};
}  // namespace bustub
