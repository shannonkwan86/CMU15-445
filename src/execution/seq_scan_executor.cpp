//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// seq_scan_executor.cpp
//
// Identification: src/execution/seq_scan_executor.cpp
//
// Copyright (c) 2015-2025, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#include "execution/executors/seq_scan_executor.h"
#include "common/macros.h"

namespace bustub {

/**
 * 构造一个新的 SeqScanExecutor 实例。
 * @param exec_ctx 执行器上下文（ExecutorContext）
 * @param plan 待执行的顺序扫描计划节点
 */
SeqScanExecutor::SeqScanExecutor(ExecutorContext *exec_ctx, const SeqScanPlanNode *plan)
    : AbstractExecutor(exec_ctx), plan_(plan) {}

/** 初始化顺序扫描 */
void SeqScanExecutor::Init() {
  // 拿到系统目录（存放所有表和索引的元信息）
  auto *catalog = exec_ctx_->GetCatalog();
  // 从执行计划中取出要扫描的表编号
  auto table_oid = plan_->GetTableOid();
  // 用表编号查出对应的表信息（包含表结构和底层 TableHeap）
  auto table_info = catalog->GetTable(table_oid);
  // 创建指向表头的新迭代器，替换掉旧迭代器（支持从头重扫）
  iter_.emplace(table_info->table_->MakeIterator());
}

/**
 * 从顺序扫描中产出下一批 Tuple。
 * @param[out] tuple_batch 本次扫描产出的下一批 Tuple
 * @param[out] rid_batch 本次扫描产出的下一批 RID
 * @param batch_size 本批最多包含的 Tuple 数量（默认 BUSTUB_BATCH_SIZE）
 * @return 如果产出了 Tuple 返回 `true`，如果没有更多 Tuple 返回 `false`
 */
auto SeqScanExecutor::Next(std::vector<bustub::Tuple> *tuple_batch, std::vector<bustub::RID> *rid_batch,
                           size_t batch_size) -> bool {
  tuple_batch->clear();
  rid_batch->clear();

  while (!iter_->IsEnd() && tuple_batch->size() < batch_size) {
    auto [meta, tuple] = iter_->GetTuple();
    auto rid = iter_->GetRID();
    ++(*iter_);

    // 跳过已被逻辑删除的行
    if (meta.is_deleted_) {
      continue;
    }

    // 如果执行计划带有下推的过滤条件（WHERE），不满足则跳过
    if (plan_->filter_predicate_ != nullptr) {
      auto value = plan_->filter_predicate_->Evaluate(&tuple, GetOutputSchema());
      if (value.IsNull() || !value.GetAs<bool>()) {
        continue;
      }
    }

    tuple_batch->push_back(std::move(tuple));
    rid_batch->push_back(rid);
  }

  return !tuple_batch->empty();
}

}  // namespace bustub
