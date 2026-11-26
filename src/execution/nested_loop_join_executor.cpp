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
#include "type/value_factory.h"

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
    : AbstractExecutor(exec_ctx),
      plan_(plan),
      left_executor_(std::move(left_executor)),
      right_executor_(std::move(right_executor)) {
  if (plan->GetJoinType() != JoinType::LEFT && plan->GetJoinType() != JoinType::INNER) {
    // 只需实现 LEFT JOIN 和 INNER JOIN
    throw bustub::NotImplementedException(fmt::format("join type {} not supported", plan->GetJoinType()));
  }
}

/** 初始化连接算子 */
void NestedLoopJoinExecutor::Init() {
  left_executor_->Init();
  left_tuples_.clear();
  left_rids_.clear();
  left_idx_ = 0;

  right_tuples_.clear();
  right_rids_.clear();
  right_idx_ = 0;
  right_initialized_ = false;
  left_matched_ = false;
}

auto NestedLoopJoinExecutor::MakeJoinedTuple(const Tuple &left_tuple, const Tuple *right_tuple) const -> Tuple {
  const auto &left_schema = left_executor_->GetOutputSchema();
  const auto &right_schema = right_executor_->GetOutputSchema();

  std::vector<Value> values;
  values.reserve(left_schema.GetColumnCount() + right_schema.GetColumnCount());
  for (uint32_t i = 0; i < left_schema.GetColumnCount(); ++i) {
    values.push_back(left_tuple.GetValue(&left_schema, i));
  }
  for (uint32_t i = 0; i < right_schema.GetColumnCount(); ++i) {
    if (right_tuple != nullptr) {
      values.push_back(right_tuple->GetValue(&right_schema, i));
    } else {
      values.push_back(ValueFactory::GetNullValueByType(right_schema.GetColumn(i).GetType()));
    }
  }
  return {std::move(values), &GetOutputSchema()};
}

/**
 * 从嵌套循环连接中产出下一批 Tuple。
 * @param[out] tuple_batch 连接算子产出的下一批 Tuple
 * @param[out] rid_batch 连接算子产出的下一批 RID
 * @param batch_size 本批最多包含的 Tuple 数量（默认 BUSTUB_BATCH_SIZE）
 * @return 如果产出了 Tuple 返回 `true`，如果没有更多 Tuple 返回 `false`
 */
auto NestedLoopJoinExecutor::Next(std::vector<bustub::Tuple> *tuple_batch, std::vector<bustub::RID> *rid_batch,
                                  size_t batch_size) -> bool {
  tuple_batch->clear();
  rid_batch->clear();

  const auto &left_schema = left_executor_->GetOutputSchema();
  const auto &right_schema = right_executor_->GetOutputSchema();
  const auto &predicate = plan_->Predicate();

  while (tuple_batch->size() < batch_size) {
    // 1. 当前左表批次已用完，向右拉取下一批左表数据
    if (left_idx_ >= left_tuples_.size()) {
      left_idx_ = 0;
      if (!left_executor_->Next(&left_tuples_, &left_rids_, batch_size)) {
        break;
      }
      continue;
    }

    // 2. 刚切到一条新的左表行，必须重新 Init() 右表从头扫描
    if (!right_initialized_) {
      right_executor_->Init();
      right_tuples_.clear();
      right_rids_.clear();
      right_idx_ = 0;
      right_initialized_ = true;
      left_matched_ = false;
    }

    const auto &left_tuple = left_tuples_[left_idx_];

    // 3. 右表当前批次已扫完，拉取右表下一批；若返回 false 表示右表整轮扫完
    if (right_idx_ >= right_tuples_.size()) {
      right_idx_ = 0;
      if (!right_executor_->Next(&right_tuples_, &right_rids_, batch_size)) {
        // 右表扫完一圈：LEFT JOIN 且当前左行未匹配过时，补 1 行右侧全 NULL 的结果
        const bool emit_left_null = (plan_->GetJoinType() == JoinType::LEFT && !left_matched_);
        ++left_idx_;
        right_initialized_ = false;
        if (emit_left_null) {
          tuple_batch->push_back(MakeJoinedTuple(left_tuple, nullptr));
          rid_batch->emplace_back();
        }
      }
      continue;
    }

    // 4. 左右两行都就绪，计算连接条件
    const auto &right_tuple = right_tuples_[right_idx_++];
    bool is_match = true;
    if (predicate != nullptr) {
      auto val = predicate->EvaluateJoin(&left_tuple, left_schema, &right_tuple, right_schema);
      is_match = !val.IsNull() && val.GetAs<bool>();
    }

    if (is_match) {
      left_matched_ = true;
      tuple_batch->push_back(MakeJoinedTuple(left_tuple, &right_tuple));
      rid_batch->emplace_back();
    }
  }

  return !tuple_batch->empty();
}

}  // namespace bustub
