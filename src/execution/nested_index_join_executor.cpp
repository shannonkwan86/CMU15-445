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
#include "type/value_factory.h"

namespace bustub {

/**
 * 创建一个新的嵌套索引连接执行器。
 * @param exec_ctx 执行器上下文（ExecutorContext）
 * @param plan 待执行的嵌套索引连接计划节点
 * @param child_executor 外表（左表）对应的子执行器
 */
NestedIndexJoinExecutor::NestedIndexJoinExecutor(ExecutorContext *exec_ctx, const NestedIndexJoinPlanNode *plan,
                                                 std::unique_ptr<AbstractExecutor> &&child_executor)
    : AbstractExecutor(exec_ctx), plan_(plan), child_executor_(std::move(child_executor)) {
  if (plan->GetJoinType() != JoinType::LEFT && plan->GetJoinType() != JoinType::INNER) {
    // 只需实现 LEFT JOIN 和 INNER JOIN
    throw bustub::NotImplementedException(fmt::format("join type {} not supported", plan->GetJoinType()));
  }
}

void NestedIndexJoinExecutor::Init() {
  child_executor_->Init();
  auto *catalog = exec_ctx_->GetCatalog();
  inner_table_info_ = catalog->GetTable(plan_->GetInnerTableOid()).get();
  index_info_ = catalog->GetIndex(plan_->GetIndexOid()).get();

  left_tuples_.clear();
  left_rids_.clear();
  left_idx_ = 0;
  matched_rids_.clear();
  matched_idx_ = 0;
  rids_fetched_ = false;
  left_matched_ = false;
}

auto NestedIndexJoinExecutor::MakeJoinedTuple(const Tuple &left_tuple, const Tuple *right_tuple) const -> Tuple {
  const auto &left_schema = child_executor_->GetOutputSchema();
  const auto &right_schema = plan_->InnerTableSchema();

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

auto NestedIndexJoinExecutor::Next(std::vector<bustub::Tuple> *tuple_batch, std::vector<bustub::RID> *rid_batch,
                                   size_t batch_size) -> bool {
  tuple_batch->clear();
  rid_batch->clear();

  while (tuple_batch->size() < batch_size) {
    // 1. 左表当前批次读完，拉取下一批
    if (left_idx_ >= left_tuples_.size()) {
      left_idx_ = 0;
      if (!child_executor_->Next(&left_tuples_, &left_rids_, batch_size)) {
        break;
      }
      continue;
    }

    const auto &left_tuple = left_tuples_[left_idx_];

    // 2. 针对当前左表行，计算连接键并去右表索引点查匹配的 RID
    if (!rids_fetched_) {
      matched_rids_.clear();
      matched_idx_ = 0;
      left_matched_ = false;
      rids_fetched_ = true;

      auto key_val = plan_->KeyPredicate()->Evaluate(&left_tuple, child_executor_->GetOutputSchema());
      if (!key_val.IsNull()) {
        Tuple key_tuple{std::vector<Value>{key_val}, &index_info_->key_schema_};
        index_info_->index_->ScanKey(key_tuple, &matched_rids_, exec_ctx_->GetTransaction());
      }
    }

    // 3. 当前左表行对应的所有右表 RID 已遍历完，切到下一条左表行
    if (matched_idx_ >= matched_rids_.size()) {
      const bool emit_left_null = (plan_->GetJoinType() == JoinType::LEFT && !left_matched_);
      ++left_idx_;
      rids_fetched_ = false;
      if (emit_left_null) {
        tuple_batch->push_back(MakeJoinedTuple(left_tuple, nullptr));
        rid_batch->emplace_back();
      }
      continue;
    }

    // 4. 回右表取整行，跳过逻辑删除行后拼装输出
    auto rid = matched_rids_[matched_idx_++];
    auto [meta, right_tuple] = inner_table_info_->table_->GetTuple(rid);
    if (meta.is_deleted_) {
      continue;
    }

    left_matched_ = true;
    tuple_batch->push_back(MakeJoinedTuple(left_tuple, &right_tuple));
    rid_batch->emplace_back();
  }

  return !tuple_batch->empty();
}

}  // namespace bustub
