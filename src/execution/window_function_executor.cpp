//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// window_function_executor.cpp
//
// Identification: src/execution/window_function_executor.cpp
//
// Copyright (c) 2015-2025, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#include "execution/executors/window_function_executor.h"

#include <algorithm>
#include <cstdint>
#include <unordered_map>
#include <utility>
#include <vector>

#include "execution/execution_common.h"
#include "execution/plans/aggregation_plan.h"
#include "execution/plans/window_plan.h"
#include "storage/table/tuple.h"
#include "type/type_id.h"
#include "type/value_factory.h"

namespace bustub {

namespace {

struct RankState {
  int32_t row_count_{0};
  int32_t cur_rank_{0};
  std::vector<Value> prev_order_vals_;
};

auto MakePartitionKey(const Tuple &tuple, const Schema &schema, const std::vector<AbstractExpressionRef> &partition_by)
    -> AggregateKey {
  std::vector<Value> keys;
  keys.reserve(partition_by.size());
  for (const auto &expr : partition_by) {
    keys.push_back(expr->Evaluate(&tuple, schema));
  }
  return {std::move(keys)};
}

auto MakeInitialWindowValue(WindowFunctionType type) -> Value {
  if (type == WindowFunctionType::CountStarAggregate) {
    return ValueFactory::GetIntegerValue(0);
  }
  return ValueFactory::GetNullValueByType(TypeId::INTEGER);
}

void CombineWindowValue(Value *result, const Value &input, WindowFunctionType type) {
  if (type == WindowFunctionType::CountStarAggregate) {
    *result = result->Add(ValueFactory::GetIntegerValue(1));
    return;
  }
  if (input.IsNull()) {
    return;
  }
  if (type == WindowFunctionType::CountAggregate) {
    *result = result->IsNull() ? ValueFactory::GetIntegerValue(1) : result->Add(ValueFactory::GetIntegerValue(1));
    return;
  }
  if (result->IsNull()) {
    *result = input;
    return;
  }
  switch (type) {
    case WindowFunctionType::SumAggregate:
      *result = result->Add(input);
      break;
    case WindowFunctionType::MinAggregate:
      *result = result->Min(input);
      break;
    case WindowFunctionType::MaxAggregate:
      *result = result->Max(input);
      break;
    default:
      break;
  }
}

auto ValuesEqualWithNull(const std::vector<Value> &lhs, const std::vector<Value> &rhs) -> bool {
  if (lhs.size() != rhs.size()) {
    return false;
  }
  for (size_t i = 0; i < lhs.size(); ++i) {
    const bool lhs_null = lhs[i].IsNull();
    const bool rhs_null = rhs[i].IsNull();
    if (lhs_null && rhs_null) {
      continue;
    }
    if (lhs_null != rhs_null || lhs[i].CompareEquals(rhs[i]) != CmpBool::CmpTrue) {
      return false;
    }
  }
  return true;
}

}  // namespace

/**
 * 构造一个新的 WindowFunctionExecutor 实例。
 * @param exec_ctx 执行器上下文（ExecutorContext）
 * @param plan 待执行的窗口聚合计划节点
 */
WindowFunctionExecutor::WindowFunctionExecutor(ExecutorContext *exec_ctx, const WindowFunctionPlanNode *plan,
                                               std::unique_ptr<AbstractExecutor> &&child_executor)
    : AbstractExecutor(exec_ctx), plan_(plan), child_executor_(std::move(child_executor)) {}

/** 初始化窗口聚合算子 */
void WindowFunctionExecutor::Init() {
  child_executor_->Init();
  output_tuples_.clear();
  cursor_ = 0;

  const auto &child_schema = child_executor_->GetOutputSchema();
  std::vector<Tuple> tuples;
  std::vector<Tuple> batch_tuples;
  std::vector<RID> batch_rids;
  while (child_executor_->Next(&batch_tuples, &batch_rids, BUSTUB_BATCH_SIZE)) {
    for (auto &tuple : batch_tuples) {
      tuples.push_back(std::move(tuple));
    }
  }

  if (tuples.empty()) {
    return;
  }

  // Planner 保证同一查询内所有窗口函数的 ORDER BY 子句完全一致（要么全为空，要么全相同）
  const std::vector<OrderBy> *global_order_by = nullptr;
  if (!plan_->window_functions_.empty() && !plan_->window_functions_.begin()->second.order_by_.empty()) {
    global_order_by = &plan_->window_functions_.begin()->second.order_by_;
  }

  if (global_order_by != nullptr) {
    std::vector<SortEntry> sort_entries;
    sort_entries.reserve(tuples.size());
    for (auto &tuple : tuples) {
      auto key = GenerateSortKey(tuple, *global_order_by, child_schema);
      sort_entries.emplace_back(std::move(key), std::move(tuple));
    }
    TupleComparator cmp(*global_order_by);
    std::stable_sort(sort_entries.begin(), sort_entries.end(), cmp);
    tuples.clear();
    tuples.reserve(sort_entries.size());
    for (auto &entry : sort_entries) {
      tuples.push_back(std::move(entry.second));
    }
  }

  const size_t num_rows = tuples.size();
  const size_t num_cols = plan_->columns_.size();
  std::vector<std::vector<Value>> result_grid(num_rows, std::vector<Value>(num_cols));

  // 1. 计算所有普通列
  for (size_t col_idx = 0; col_idx < num_cols; ++col_idx) {
    if (plan_->window_functions_.find(static_cast<uint32_t>(col_idx)) != plan_->window_functions_.end()) {
      continue;
    }
    for (size_t row_idx = 0; row_idx < num_rows; ++row_idx) {
      result_grid[row_idx][col_idx] = plan_->columns_[col_idx]->Evaluate(&tuples[row_idx], child_schema);
    }
  }

  // 2. 分别计算每个窗口函数列
  for (const auto &[col_idx, wf] : plan_->window_functions_) {
    if (wf.order_by_.empty()) {
      // 无 ORDER BY：窗口帧为整个分区（UNBOUNDED PRECEDING AND UNBOUNDED FOLLOWING）
      std::unordered_map<AggregateKey, Value> part_agg;
      std::vector<AggregateKey> row_keys;
      row_keys.reserve(num_rows);
      for (size_t row_idx = 0; row_idx < num_rows; ++row_idx) {
        auto key = MakePartitionKey(tuples[row_idx], child_schema, wf.partition_by_);
        auto [it, inserted] = part_agg.try_emplace(key, MakeInitialWindowValue(wf.type_));
        Value arg_val = wf.type_ == WindowFunctionType::CountStarAggregate
                            ? ValueFactory::GetIntegerValue(1)
                            : wf.function_->Evaluate(&tuples[row_idx], child_schema);
        CombineWindowValue(&it->second, arg_val, wf.type_);
        row_keys.push_back(std::move(key));
      }
      for (size_t row_idx = 0; row_idx < num_rows; ++row_idx) {
        result_grid[row_idx][col_idx] = part_agg.at(row_keys[row_idx]);
      }
      continue;
    }

    if (wf.type_ == WindowFunctionType::Rank) {
      std::unordered_map<AggregateKey, RankState> part_rank;
      for (size_t row_idx = 0; row_idx < num_rows; ++row_idx) {
        auto key = MakePartitionKey(tuples[row_idx], child_schema, wf.partition_by_);
        auto &state = part_rank[key];
        ++state.row_count_;
        auto order_vals = GenerateSortKey(tuples[row_idx], wf.order_by_, child_schema);
        if (!ValuesEqualWithNull(order_vals, state.prev_order_vals_)) {
          state.cur_rank_ = state.row_count_;
          state.prev_order_vals_ = std::move(order_vals);
        }
        result_grid[row_idx][col_idx] = ValueFactory::GetIntegerValue(state.cur_rank_);
      }
      continue;
    }

    // 有 ORDER BY 的常规聚合：按分区逐行累计（UNBOUNDED PRECEDING AND CURRENT ROW）
    std::unordered_map<AggregateKey, Value> part_agg;
    for (size_t row_idx = 0; row_idx < num_rows; ++row_idx) {
      auto key = MakePartitionKey(tuples[row_idx], child_schema, wf.partition_by_);
      auto [it, inserted] = part_agg.try_emplace(key, MakeInitialWindowValue(wf.type_));
      Value arg_val = wf.type_ == WindowFunctionType::CountStarAggregate
                          ? ValueFactory::GetIntegerValue(1)
                          : wf.function_->Evaluate(&tuples[row_idx], child_schema);
      CombineWindowValue(&it->second, arg_val, wf.type_);
      result_grid[row_idx][col_idx] = it->second;
    }
  }

  output_tuples_.reserve(num_rows);
  for (size_t row_idx = 0; row_idx < num_rows; ++row_idx) {
    output_tuples_.emplace_back(std::move(result_grid[row_idx]), &GetOutputSchema());
  }
}

/**
 * 从窗口聚合算子中产出下一批 Tuple。
 * @param[out] tuple_batch 窗口聚合算子产出的下一批 Tuple
 * @param[out] rid_batch 窗口聚合算子产出的下一批 RID
 * @param batch_size 本批最多包含的 Tuple 数量（默认 BUSTUB_BATCH_SIZE）
 * @return 如果产出了 Tuple 返回 `true`，如果没有更多 Tuple 返回 `false`
 */
auto WindowFunctionExecutor::Next(std::vector<bustub::Tuple> *tuple_batch, std::vector<bustub::RID> *rid_batch,
                                  size_t batch_size) -> bool {
  tuple_batch->clear();
  rid_batch->clear();

  while (cursor_ < output_tuples_.size() && tuple_batch->size() < batch_size) {
    tuple_batch->push_back(output_tuples_[cursor_]);
    rid_batch->emplace_back();
    ++cursor_;
  }

  return !tuple_batch->empty();
}
}  // namespace bustub
