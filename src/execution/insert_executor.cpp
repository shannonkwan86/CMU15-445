//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// insert_executor.cpp
//
// Identification: src/execution/insert_executor.cpp
//
// Copyright (c) 2015-2025, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#include <memory>
#include "common/macros.h"
#include "type/value_factory.h"

#include "execution/executors/insert_executor.h"

namespace bustub {

/**
 * 构造一个新的 InsertExecutor 实例。
 * @param exec_ctx 执行器上下文（ExecutorContext）
 * @param plan 待执行的插入计划节点
 * @param child_executor 提供待插入 Tuple 的子执行器
 */
InsertExecutor::InsertExecutor(ExecutorContext *exec_ctx, const InsertPlanNode *plan,
                               std::unique_ptr<AbstractExecutor> &&child_executor)
    : AbstractExecutor(exec_ctx), plan_(plan), child_executor_(std::move(child_executor)) {}

/** 初始化插入操作 */
void InsertExecutor::Init() {
  // 区别于 SeqScan：Insert 有子执行器，必须先初始化下级并重置完成标记
  child_executor_->Init();
  is_executed_ = false;
}

/**
 * 产出插入到表中的行数。
 * @param[out] tuple_batch 包含单个整数的 Tuple 批次，表示插入表的总行数
 * @param[out] rid_batch 插入操作产出的下一批 RID（忽略，不使用）
 * @param batch_size 本批最多包含的 Tuple 数量（默认 BUSTUB_BATCH_SIZE）
 * @return 如果产出了 Tuple 返回 `true`，如果没有更多 Tuple 返回 `false`
 *
 * NOTE: InsertExecutor::Next() 不使用输出参数 `rid_batch`。
 * NOTE: InsertExecutor::Next() 只会返回一次 `true` 并产出插入行数。
 */
auto InsertExecutor::Next(std::vector<bustub::Tuple> *tuple_batch, std::vector<bustub::RID> *rid_batch,
                          size_t batch_size) -> bool {
  tuple_batch->clear();
  rid_batch->clear();

  // 区别于 SeqScan：Insert 只在第 1 次调用时吐出总行数，第 2 次直接返回 false
  if (is_executed_) {
    return false;
  }

  auto *catalog = exec_ctx_->GetCatalog();
  auto table_info = catalog->GetTable(plan_->GetTableOid());
  auto indexes = catalog->GetTableIndexes(table_info->name_);

  int32_t inserted_count = 0;
  std::vector<Tuple> child_tuples{};
  std::vector<RID> child_rids{};

  // 区别于 SeqScan：不按 batch_size 截断，而是一口气把子执行器的所有待插入行全拉完
  while (child_executor_->Next(&child_tuples, &child_rids, batch_size)) {
    for (const auto &tuple : child_tuples) {
      // 1. 先插入堆表（TableHeap），拿到新分配的 RID
      auto new_rid = table_info->table_->InsertTuple(TupleMeta{0, false}, tuple);
      if (!new_rid.has_value()) {
        continue;
      }

      // 2. 区别于 SeqScan：写操作必须同步更新该表上的所有索引
      for (const auto &index_info : indexes) {
        auto key = tuple.KeyFromTuple(table_info->schema_, index_info->key_schema_, index_info->index_->GetKeyAttrs());
        index_info->index_->InsertEntry(key, *new_rid, exec_ctx_->GetTransaction());
      }
      ++inserted_count;
    }
  }

  // 3. 区别于 SeqScan：不吐出原始行数据，只产出 1 个包含总插入行数的单列整数 Tuple（rid_batch 留空）
  tuple_batch->emplace_back(std::vector<Value>{ValueFactory::GetIntegerValue(inserted_count)}, &GetOutputSchema());
  is_executed_ = true;
  return true;
}

}  // namespace bustub
