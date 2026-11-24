//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// column_pruning.cpp
//
// Identification: src/optimizer/column_pruning.cpp
//
// Copyright (c) 2015-2025, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#include "optimizer/optimizer.h"

namespace bustub {

/**
 * @brief 对紧跟在 Projection 计划节点之下的子计划执行列裁剪（column pruning）
 * @param plan 待优化的计划节点
 * @return 完成列裁剪后的新计划节点
 * @note 你可以在此函数中实现列裁剪优化。
 */
auto Optimizer::OptimizeColumnPruning(const bustub::AbstractPlanNodeRef &plan) -> AbstractPlanNodeRef {
  // 在此编写实现
  return plan;
}

}  // namespace bustub
