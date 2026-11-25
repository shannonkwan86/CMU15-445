//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// update_executor.cpp
//
// Identification: src/execution/update_executor.cpp
//
// Copyright (c) 2015-2025, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#include <memory>
#include "common/macros.h"
#include "type/value_factory.h"

#include "execution/executors/update_executor.h"

namespace bustub {

/**
 * 构造一个新的 UpdateExecutor 实例。
 * @param exec_ctx 执行器上下文（ExecutorContext）
 * @param plan 待执行的更新计划节点
 * @param child_executor 为更新操作提供数据的子执行器
 */
UpdateExecutor::UpdateExecutor(ExecutorContext *exec_ctx, const UpdatePlanNode *plan,
                               std::unique_ptr<AbstractExecutor> &&child_executor)
    : AbstractExecutor(exec_ctx),
      plan_(plan),
      table_info_(exec_ctx->GetCatalog()->GetTable(plan->GetTableOid()).get()),
      child_executor_(std::move(child_executor)) {}

/** 初始化更新操作 */
void UpdateExecutor::Init() {
  child_executor_->Init();
  is_executed_ = false;
}

/**
 * 产出表中被更新的行数。
 * @param[out] tuple_batch 包含单个整数的 Tuple 批次，表示表中被更新的总行数
 * @param[out] rid_batch 更新操作产出的下一批 RID（忽略，不使用）
 * @param batch_size 本批最多包含的 Tuple 数量（默认 BUSTUB_BATCH_SIZE）
 * @return 如果产出了 Tuple 返回 `true`，如果没有更多 Tuple 返回 `false`
 *
 * NOTE: UpdateExecutor::Next() 不使用输出参数 `rid_batch`。
 * NOTE: UpdateExecutor::Next() 只会返回一次 `true` 并产出更新行数。
 */
auto UpdateExecutor::Next(std::vector<bustub::Tuple> *tuple_batch, std::vector<bustub::RID> *rid_batch,
                          size_t batch_size) -> bool {
  tuple_batch->clear();
  rid_batch->clear();

  if (is_executed_) {
    return false;
  }

  auto indexes = exec_ctx_->GetCatalog()->GetTableIndexes(table_info_->name_);
  int32_t updated_count = 0;
  std::vector<Tuple> child_tuples{};
  std::vector<RID> child_rids{};

  while (child_executor_->Next(&child_tuples, &child_rids, batch_size)) {
    for (size_t i = 0; i < child_tuples.size(); ++i) {
      const auto &old_tuple = child_tuples[i];
      const auto &old_rid = child_rids[i];

      // 1. 先逻辑删除旧行（设置 is_deleted_ = true）
      table_info_->table_->UpdateTupleMeta(TupleMeta{0, true}, old_rid);

      // 2. 按目标表达式逐列求值，构造更新后的新 Tuple
      std::vector<Value> new_values;
      new_values.reserve(plan_->target_expressions_.size());
      for (const auto &expr : plan_->target_expressions_) {
        new_values.push_back(expr->Evaluate(&old_tuple, child_executor_->GetOutputSchema()));
      }
      Tuple new_tuple{new_values, &table_info_->schema_};

      // 3. 将新 Tuple 插入堆表，拿到新分配的 RID
      auto new_rid = table_info_->table_->InsertTuple(TupleMeta{0, false}, new_tuple);
      if (!new_rid.has_value()) {
        continue;
      }

      // 4. 同步更新表上的所有索引：先删旧键，再插新键
      for (const auto &index_info : indexes) {
        auto old_key =
            old_tuple.KeyFromTuple(table_info_->schema_, index_info->key_schema_, index_info->index_->GetKeyAttrs());
        index_info->index_->DeleteEntry(old_key, old_rid, exec_ctx_->GetTransaction());

        auto new_key =
            new_tuple.KeyFromTuple(table_info_->schema_, index_info->key_schema_, index_info->index_->GetKeyAttrs());
        index_info->index_->InsertEntry(new_key, *new_rid, exec_ctx_->GetTransaction());
      }
      ++updated_count;
    }
  }

  tuple_batch->emplace_back(std::vector<Value>{ValueFactory::GetIntegerValue(updated_count)}, &GetOutputSchema());
  is_executed_ = true;
  return true;
}

}  // namespace bustub
