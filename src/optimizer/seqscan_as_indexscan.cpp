//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// seqscan_as_indexscan.cpp
//
// Identification: src/optimizer/seqscan_as_indexscan.cpp
//
// Copyright (c) 2015-2025, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#include <memory>
#include <optional>
#include <utility>
#include <vector>

#include "execution/expressions/column_value_expression.h"
#include "execution/expressions/comparison_expression.h"
#include "execution/expressions/constant_value_expression.h"
#include "execution/expressions/logic_expression.h"
#include "execution/plans/index_scan_plan.h"
#include "execution/plans/seq_scan_plan.h"
#include "optimizer/optimizer.h"

namespace bustub {

namespace {

struct IndexPredResult {
  uint32_t col_idx_;
  std::vector<AbstractExpressionRef> pred_keys_;
};

/**
 * 递归解析过滤谓词，提取单列等值（或同列等值的 OR 组合）中的列号与常量键。
 * 若表达式不符合单列等值点查形式，返回 std::nullopt。
 */
auto ExtractEqualPredicates(const AbstractExpressionRef &expr) -> std::optional<IndexPredResult> {
  if (const auto *comp_expr = dynamic_cast<const ComparisonExpression *>(expr.get()); comp_expr != nullptr) {
    if (comp_expr->comp_type_ != ComparisonType::Equal) {
      return std::nullopt;
    }

    auto left = comp_expr->GetChildAt(0);
    auto right = comp_expr->GetChildAt(1);

    // 情况 A：col = const（例如 v1 = 2）
    if (const auto *col_expr = dynamic_cast<const ColumnValueExpression *>(left.get()); col_expr != nullptr) {
      if (dynamic_cast<const ConstantValueExpression *>(right.get()) != nullptr) {
        return IndexPredResult{col_expr->GetColIdx(), {std::move(right)}};
      }
    }

    // 情况 B：const = col（例如 4 = v1）
    if (const auto *col_expr = dynamic_cast<const ColumnValueExpression *>(right.get()); col_expr != nullptr) {
      if (dynamic_cast<const ConstantValueExpression *>(left.get()) != nullptr) {
        return IndexPredResult{col_expr->GetColIdx(), {std::move(left)}};
      }
    }

    return std::nullopt;
  }

  // 情况 C：OR 组合（例如 4 = v1 OR v1 = 7），要求左右两边必须是同一列的等值条件
  if (const auto *logic_expr = dynamic_cast<const LogicExpression *>(expr.get()); logic_expr != nullptr) {
    if (logic_expr->logic_type_ != LogicType::Or) {
      return std::nullopt;
    }

    auto left_res = ExtractEqualPredicates(logic_expr->GetChildAt(0));
    auto right_res = ExtractEqualPredicates(logic_expr->GetChildAt(1));
    if (!left_res.has_value() || !right_res.has_value() || left_res->col_idx_ != right_res->col_idx_) {
      return std::nullopt;
    }

    left_res->pred_keys_.insert(left_res->pred_keys_.end(), std::make_move_iterator(right_res->pred_keys_.begin()),
                                std::make_move_iterator(right_res->pred_keys_.end()));
    return left_res;
  }

  return std::nullopt;
}

}  // namespace

/**
 * @brief 如果表上存在匹配的索引，将 SeqScan 优化为 IndexScan
 */
auto Optimizer::OptimizeSeqScanAsIndexScan(const bustub::AbstractPlanNodeRef &plan) -> AbstractPlanNodeRef {
  std::vector<AbstractPlanNodeRef> children;
  for (const auto &child : plan->GetChildren()) {
    children.emplace_back(OptimizeSeqScanAsIndexScan(child));
  }

  auto optimized_plan = plan->CloneWithChildren(std::move(children));

  // 第 1 关：必须是带过滤谓词的 SeqScan
  if (optimized_plan->GetType() != PlanType::SeqScan) {
    return optimized_plan;
  }
  const auto &seq_scan_plan = dynamic_cast<const SeqScanPlanNode &>(*optimized_plan);
  if (seq_scan_plan.filter_predicate_ == nullptr) {
    return optimized_plan;
  }

  // 第 2 关：提取单列等值（或同列 OR 等值）条件
  auto pred_res = ExtractEqualPredicates(seq_scan_plan.filter_predicate_);
  if (!pred_res.has_value()) {
    return optimized_plan;
  }

  // 第 3 关：查找该表上是否建有对应这一列的单列索引
  auto indexes = catalog_.GetTableIndexes(seq_scan_plan.table_name_);
  for (const auto &index_info : indexes) {
    const auto &key_attrs = index_info->index_->GetKeyAttrs();
    if (key_attrs.size() == 1 && key_attrs[0] == pred_res->col_idx_) {
      return std::make_shared<IndexScanPlanNode>(seq_scan_plan.output_schema_, seq_scan_plan.table_oid_,
                                                 index_info->index_oid_, seq_scan_plan.filter_predicate_,
                                                 std::move(pred_res->pred_keys_));
    }
  }

  return optimized_plan;
}

}  // namespace bustub
