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

#include "optimizer/optimizer.h"

namespace bustub {

/**
 * @brief 如果表上存在匹配的索引，将 SeqScan 优化为 IndexScan
 */
auto Optimizer::OptimizeSeqScanAsIndexScan(const bustub::AbstractPlanNodeRef &plan) -> AbstractPlanNodeRef {
  // TODO(P3): 实现带谓词的 SeqScan -> IndexScan 优化器规则
  // 在强制使用 starter 规则时，optimizer.cpp 中已为你启用了过滤谓词下推（MergeFilterScan）
  return plan;
}

}  // namespace bustub
