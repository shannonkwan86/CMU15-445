//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// delete_executor.cpp
//
// Identification: src/execution/delete_executor.cpp
//
// Copyright (c) 2015-2025, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#include <memory>
#include "common/macros.h"

#include "execution/executors/delete_executor.h"

namespace bustub {

/**
 * 构造一个新的 DeleteExecutor 实例。
 * @param exec_ctx 执行器上下文（ExecutorContext）
 * @param plan 待执行的删除计划节点
 * @param child_executor 为删除操作提供数据的子执行器
 */
DeleteExecutor::DeleteExecutor(ExecutorContext *exec_ctx, const DeletePlanNode *plan,
                               std::unique_ptr<AbstractExecutor> &&child_executor)
    : AbstractExecutor(exec_ctx) {
  UNIMPLEMENTED("TODO(P3): Add implementation.");
}

/** 初始化删除操作 */
void DeleteExecutor::Init() { UNIMPLEMENTED("TODO(P3): Add implementation."); }

/**
 * 产出从表中删除的行数。
 * @param[out] tuple_batch 包含单个整数的 Tuple 批次，表示从表中删除的总行数
 * @param[out] rid_batch 删除操作产出的下一批 RID（忽略，不使用）
 * @param batch_size 本批最多包含的 Tuple 数量（默认 BUSTUB_BATCH_SIZE）
 * @return 如果产出了 Tuple 返回 `true`，如果没有更多 Tuple 返回 `false`
 *
 * NOTE: DeleteExecutor::Next() 不使用输出参数 `rid_batch`。
 * NOTE: DeleteExecutor::Next() 只会返回一次 `true` 并产出删除行数。
 */
auto DeleteExecutor::Next(std::vector<bustub::Tuple> *tuple_batch, std::vector<bustub::RID> *rid_batch,
                          size_t batch_size) -> bool {
  UNIMPLEMENTED("TODO(P3): Add implementation.");
}

}  // namespace bustub
