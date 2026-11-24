//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// external_merge_sort_executor.cpp
//
// Identification: src/execution/external_merge_sort_executor.cpp
//
// Copyright (c) 2015-2025, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#include "execution/executors/external_merge_sort_executor.h"
#include <vector>
#include "common/macros.h"
#include "execution/plans/sort_plan.h"

namespace bustub {

template <size_t K>
ExternalMergeSortExecutor<K>::ExternalMergeSortExecutor(ExecutorContext *exec_ctx, const SortPlanNode *plan,
                                                        std::unique_ptr<AbstractExecutor> &&child_executor)
    : AbstractExecutor(exec_ctx), cmp_(plan->GetOrderBy()) {
  UNIMPLEMENTED("TODO(P3): Add implementation.");
}

/** 初始化外部归并排序 */
template <size_t K>
void ExternalMergeSortExecutor<K>::Init() {
  UNIMPLEMENTED("TODO(P3): Add implementation.");
}

/**
 * 从外部归并排序中产出下一批 Tuple。
 * @param[out] tuple_batch 外部归并排序产出的下一批 Tuple
 * @param[out] rid_batch 外部归并排序产出的下一批 RID
 * @param batch_size 本批最多包含的 Tuple 数量（默认 BUSTUB_BATCH_SIZE）
 * @return 如果产出了 Tuple 返回 `true`，如果没有更多 Tuple 返回 `false`
 */
template <size_t K>
auto ExternalMergeSortExecutor<K>::Next(std::vector<bustub::Tuple> *tuple_batch, std::vector<bustub::RID> *rid_batch,
                                        size_t batch_size) -> bool {
  UNIMPLEMENTED("TODO(P3): Add implementation.");
}

template class ExternalMergeSortExecutor<2>;

}  // namespace bustub
