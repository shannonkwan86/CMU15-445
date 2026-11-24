//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// optimizer_custom_rules.cpp
//
// Identification: src/optimizer/optimizer_custom_rules.cpp
//
// Copyright (c) 2015-2025, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#include "execution/plans/abstract_plan.h"
#include "optimizer/optimizer.h"

// 你可以把自定义优化器规则的实现放到此文件中，并按需编排规则的应用顺序。
// 注意：部分测试用例会强制使用 starter 规则（`set force_optimizer_starter_rule=yes`），此时本文件中的配置不会生效。

namespace bustub {

auto Optimizer::OptimizeCustom(const AbstractPlanNodeRef &plan) -> AbstractPlanNodeRef {
  auto p = plan;
  p = OptimizeMergeProjection(p);
  p = OptimizeMergeFilterNLJ(p);
  p = OptimizeNLJAsHashJoin(p);
  p = OptimizeOrderByAsIndexScan(p);
  p = OptimizeSortLimitAsTopN(p);
  p = OptimizeMergeFilterScan(p);
  p = OptimizeSeqScanAsIndexScan(p);
  return p;
}

}  // namespace bustub
