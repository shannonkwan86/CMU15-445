//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// limit_executor.cpp
//
// Identification: src/execution/limit_executor.cpp
//
// Copyright (c) 2015-2025, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#include <algorithm>

#include "common/macros.h"
#include "execution/executors/limit_executor.h"

namespace bustub {

/**
 * 构造一个新的 LimitExecutor 实例。
 * @param exec_ctx 执行器上下文（ExecutorContext）
 * @param plan 待执行的 Limit 计划节点
 * @param child_executor 提供输入 Tuple 的子执行器
 */
LimitExecutor::LimitExecutor(ExecutorContext *exec_ctx, const LimitPlanNode *plan,
                             std::unique_ptr<AbstractExecutor> &&child_executor)
    : AbstractExecutor(exec_ctx), plan_(plan), child_executor_(std::move(child_executor)) {}

/** 初始化 Limit 算子 */
void LimitExecutor::Init() {
  child_executor_->Init();
  count_ = 0;
}

/**
 * 从 Limit 算子中产出下一批 Tuple。
 * @param[out] tuple_batch Limit 算子产出的下一批 Tuple
 * @param[out] rid_batch Limit 算子产出的下一批 RID
 * @param batch_size 本批最多包含的 Tuple 数量（默认 BUSTUB_BATCH_SIZE）
 * @return 如果产出了 Tuple 返回 `true`，如果没有更多 Tuple 返回 `false`
 */
auto LimitExecutor::Next(std::vector<bustub::Tuple> *tuple_batch, std::vector<bustub::RID> *rid_batch,
                         size_t batch_size) -> bool {
  tuple_batch->clear();
  rid_batch->clear();

  if (count_ >= plan_->GetLimit()) {
    return false;
  }

  const size_t remaining = plan_->GetLimit() - count_;
  const size_t fetch_size = std::min(batch_size, remaining);
  if (fetch_size == 0 || !child_executor_->Next(tuple_batch, rid_batch, fetch_size)) {
    return false;
  }

  // 防御性截断：避免个别子执行器返回超过请求数量的数据
  if (tuple_batch->size() > remaining) {
    tuple_batch->resize(remaining);
    if (rid_batch->size() > remaining) {
      rid_batch->resize(remaining);
    }
  }

  count_ += tuple_batch->size();
  return !tuple_batch->empty();
}

}  // namespace bustub
