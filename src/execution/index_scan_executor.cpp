//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// index_scan_executor.cpp
//
// Identification: src/execution/index_scan_executor.cpp
//
// Copyright (c) 2015-2025, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#include <set>
#include <utility>

#include "common/macros.h"
#include "execution/executors/index_scan_executor.h"

namespace bustub {

/**
 * 创建一个新的索引扫描执行器。
 * @param exec_ctx 执行器上下文（ExecutorContext）
 * @param plan 待执行的索引扫描计划节点
 */
IndexScanExecutor::IndexScanExecutor(ExecutorContext *exec_ctx, const IndexScanPlanNode *plan)
    : AbstractExecutor(exec_ctx), plan_(plan) {}

void IndexScanExecutor::Init() {
  auto *catalog = exec_ctx_->GetCatalog();
  table_info_ = catalog->GetTable(plan_->table_oid_).get();
  auto index_info = catalog->GetIndex(plan_->GetIndexOid());
  auto *tree = dynamic_cast<BPlusTreeIndexForTwoIntegerColumn *>(index_info->index_.get());

  rids_.clear();
  rid_idx_ = 0;

  if (plan_->pred_keys_.empty()) {
    // 模式 A：有序全扫（例如 ORDER BY 索引列），从 B+ 树最左侧叶子开始收集所有 RID
    for (auto iter = tree->GetBeginIterator(); !iter.IsEnd(); ++iter) {
      rids_.push_back((*iter).second);
    }
    return;
  }

  // 模式 B：等值点查（例如 WHERE v1 = 2 或 4 = v1 OR v1 = 7），收集并去重所有命中的 RID
  std::set<std::pair<page_id_t, uint32_t>> seen_rids;
  for (const auto &key_expr : plan_->pred_keys_) {
    auto val = key_expr->Evaluate(nullptr, table_info_->schema_);
    Tuple key_tuple{std::vector<Value>{val}, &index_info->key_schema_};
    std::vector<RID> matched_rids;
    index_info->index_->ScanKey(key_tuple, &matched_rids, exec_ctx_->GetTransaction());
    for (const auto &rid : matched_rids) {
      if (seen_rids.emplace(rid.GetPageId(), rid.GetSlotNum()).second) {
        rids_.push_back(rid);
      }
    }
  }
}

auto IndexScanExecutor::Next(std::vector<bustub::Tuple> *tuple_batch, std::vector<bustub::RID> *rid_batch,
                             size_t batch_size) -> bool {
  tuple_batch->clear();
  rid_batch->clear();

  while (rid_idx_ < rids_.size() && tuple_batch->size() < batch_size) {
    auto rid = rids_[rid_idx_++];
    auto [meta, tuple] = table_info_->table_->GetTuple(rid);
    if (meta.is_deleted_) {
      continue;
    }
    if (plan_->filter_predicate_ != nullptr) {
      auto val = plan_->filter_predicate_->Evaluate(&tuple, GetOutputSchema());
      if (val.IsNull() || !val.GetAs<bool>()) {
        continue;
      }
    }

    tuple_batch->push_back(std::move(tuple));
    rid_batch->push_back(rid);
  }

  return !tuple_batch->empty();
}

}  // namespace bustub
