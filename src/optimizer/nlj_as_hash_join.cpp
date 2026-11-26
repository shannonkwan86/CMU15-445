//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// nlj_as_hash_join.cpp
//
// Identification: src/optimizer/nlj_as_hash_join.cpp
//
// Copyright (c) 2015-2025, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#include <algorithm>
#include <memory>
#include "catalog/column.h"
#include "catalog/schema.h"
#include "common/exception.h"
#include "common/macros.h"
#include "execution/expressions/column_value_expression.h"
#include "execution/expressions/comparison_expression.h"
#include "execution/expressions/constant_value_expression.h"
#include "execution/expressions/logic_expression.h"
#include "execution/plans/abstract_plan.h"
#include "execution/plans/filter_plan.h"
#include "execution/plans/hash_join_plan.h"
#include "execution/plans/nested_loop_join_plan.h"
#include "execution/plans/projection_plan.h"
#include "optimizer/optimizer.h"
#include "type/type_id.h"

namespace bustub {

namespace {

auto ExtractEquiJoinKeys(const AbstractExpressionRef &expr, std::vector<AbstractExpressionRef> *left_key_exprs,
                         std::vector<AbstractExpressionRef> *right_key_exprs) -> bool {
  if (const auto *logic_expr = dynamic_cast<const LogicExpression *>(expr.get()); logic_expr != nullptr) {
    if (logic_expr->logic_type_ != LogicType::And) {
      return false;
    }
    return ExtractEquiJoinKeys(logic_expr->GetChildAt(0), left_key_exprs, right_key_exprs) &&
           ExtractEquiJoinKeys(logic_expr->GetChildAt(1), left_key_exprs, right_key_exprs);
  }

  if (const auto *comp_expr = dynamic_cast<const ComparisonExpression *>(expr.get()); comp_expr != nullptr) {
    if (comp_expr->comp_type_ != ComparisonType::Equal) {
      return false;
    }
    const auto *left_col = dynamic_cast<const ColumnValueExpression *>(comp_expr->GetChildAt(0).get());
    const auto *right_col = dynamic_cast<const ColumnValueExpression *>(comp_expr->GetChildAt(1).get());
    if (left_col == nullptr || right_col == nullptr) {
      return false;
    }

    if (left_col->GetTupleIdx() == 0 && right_col->GetTupleIdx() == 1) {
      left_key_exprs->push_back(
          std::make_shared<ColumnValueExpression>(0, left_col->GetColIdx(), left_col->GetReturnType()));
      right_key_exprs->push_back(
          std::make_shared<ColumnValueExpression>(0, right_col->GetColIdx(), right_col->GetReturnType()));
      return true;
    }
    if (left_col->GetTupleIdx() == 1 && right_col->GetTupleIdx() == 0) {
      left_key_exprs->push_back(
          std::make_shared<ColumnValueExpression>(0, right_col->GetColIdx(), right_col->GetReturnType()));
      right_key_exprs->push_back(
          std::make_shared<ColumnValueExpression>(0, left_col->GetColIdx(), left_col->GetReturnType()));
      return true;
    }
  }

  return false;
}

}  // namespace

/**
 * @brief 将 NestedLoopJoin 优化为 HashJoin。
 * 初始框架仅考虑单个等值条件的 NLJ，你可以进一步支持多个等值条件（AND 连接）的 Join 优化。
 */
auto Optimizer::OptimizeNLJAsHashJoin(const AbstractPlanNodeRef &plan) -> AbstractPlanNodeRef {
  std::vector<AbstractPlanNodeRef> children;
  for (const auto &child : plan->GetChildren()) {
    children.emplace_back(OptimizeNLJAsHashJoin(child));
  }
  auto optimized_plan = plan->CloneWithChildren(std::move(children));

  if (optimized_plan->GetType() != PlanType::NestedLoopJoin) {
    return optimized_plan;
  }

  const auto &nlj_plan = dynamic_cast<const NestedLoopJoinPlanNode &>(*optimized_plan);
  if (nlj_plan.Predicate() == nullptr) {
    return optimized_plan;
  }

  std::vector<AbstractExpressionRef> left_key_exprs;
  std::vector<AbstractExpressionRef> right_key_exprs;
  if (ExtractEquiJoinKeys(nlj_plan.Predicate(), &left_key_exprs, &right_key_exprs) && !left_key_exprs.empty()) {
    return std::make_shared<HashJoinPlanNode>(nlj_plan.output_schema_, nlj_plan.GetLeftPlan(), nlj_plan.GetRightPlan(),
                                              std::move(left_key_exprs), std::move(right_key_exprs),
                                              nlj_plan.GetJoinType());
  }

  return optimized_plan;
}

}  // namespace bustub
