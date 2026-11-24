//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// insert_executor.cpp
//
// Identification: src/execution/insert_executor.cpp
//
// Copyright (c) 2015-2025, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#include <memory>
#include "common/macros.h"

#include "execution/executors/insert_executor.h"

namespace bustub {

/**
 * 构造一个新的 InsertExecutor 实例。
 * @param exec_ctx 执行器上下文（ExecutorContext）
 * @param plan 待执行的插入计划节点
 * @param child_executor 提供待插入 Tuple 的子执行器
 */
InsertExecutor::InsertExecutor(ExecutorContext *exec_ctx, const InsertPlanNode *plan,
                               std::unique_ptr<AbstractExecutor> &&child_executor)
    : AbstractExecutor(exec_ctx) {
  UNIMPLEMENTED("TODO(P3): Add implementation.");
}

/** 初始化插入操作 */
void InsertExecutor::Init() { UNIMPLEMENTED("TODO(P3): Add implementation."); }

/**
 * 产出插入到表中的行数。
 * @param[out] tuple_batch 包含单个整数的 Tuple 批次，表示插入表的总行数
 * @param[out] rid_batch 插入操作产出的下一批 RID（忽略，不使用）
 * @param batch_size 本批最多包含的 Tuple 数量（默认 BUSTUB_BATCH_SIZE）
 * @return 如果产出了 Tuple 返回 `true`，如果没有更多 Tuple 返回 `false`
 *
 * NOTE: InsertExecutor::Next() 不使用输出参数 `rid_batch`。
 * NOTE: InsertExecutor::Next() 只会返回一次 `true` 并产出插入行数。
 */
auto InsertExecutor::Next(std::vector<bustub::Tuple> *tuple_batch, std::vector<bustub::RID> *rid_batch,
                          size_t batch_size) -> bool {
  UNIMPLEMENTED("TODO(P3): Add implementation.");
}

}  // namespace bustub
