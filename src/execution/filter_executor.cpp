//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// filter_executor.cpp
//
// Identification: src/execution/filter_executor.cpp
//
// Copyright (c) 2015-2025, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#include "execution/executors/filter_executor.h"
#include "common/exception.h"
#include "type/value_factory.h"

namespace bustub {

/**
 * 构造一个新的 FilterExecutor 实例。
 * @param exec_ctx 执行器上下文（ExecutorContext）
 * @param plan 待执行的过滤计划节点
 * @param child_executor 为过滤算子提供数据的子执行器
 */
FilterExecutor::FilterExecutor(ExecutorContext *exec_ctx, const FilterPlanNode *plan,
                               std::unique_ptr<AbstractExecutor> &&child_executor)
    : AbstractExecutor(exec_ctx), plan_(plan), child_executor_(std::move(child_executor)) {}

/** 初始化过滤算子 */
void FilterExecutor::Init() {
  // 初始化子执行器
  child_executor_->Init();
}

/**
 * 从过滤算子中产出下一批 Tuple。
 * @param[out] tuple_batch 过滤算子产出的下一批 Tuple
 * @param[out] rid_batch 过滤算子产出的下一批 RID
 * @param batch_size 本批最多包含的 Tuple 数量（默认 BUSTUB_BATCH_SIZE）
 * @return 如果产出了 Tuple 返回 `true`，如果没有更多 Tuple 返回 `false`
 */
auto FilterExecutor::Next(std::vector<bustub::Tuple> *tuple_batch, std::vector<bustub::RID> *rid_batch,
                          size_t batch_size) -> bool {
  tuple_batch->clear();
  rid_batch->clear();

  auto filter_expr = plan_->GetPredicate();

  while (true) {
    // 如果 child_offset_ 不为 0，先处理上一次从子执行器拉取批次中剩余的 Tuple
    if (child_offset_ != 0) {
      for (size_t i = child_offset_; i < child_tuples_.size(); ++i) {
        auto &tuple = child_tuples_[i];
        auto &rid = child_rids_[i];
        // 计算过滤谓词
        auto value = filter_expr->Evaluate(&tuple, child_executor_->GetOutputSchema());
        if (filter_expr == nullptr || (!value.IsNull() && value.GetAs<bool>())) {
          tuple_batch->push_back(tuple);
          rid_batch->push_back(rid);
        }
      }
    }

    child_offset_ = 0;

    // 从子执行器获取下一批 Tuple
    const auto status = child_executor_->Next(&child_tuples_, &child_rids_, batch_size);

    // 如果没有更多 Tuple 且输出批次为空，返回 false
    if (!status && tuple_batch->empty()) {
      return false;
    }

    // 如果没有更多 Tuple 但输出批次非空，返回 true 先把剩余结果吐出
    if (!status && !tuple_batch->empty()) {
      return true;
    }

    for (size_t i = 0; i < child_tuples_.size(); ++i) {
      auto &tuple = child_tuples_[i];
      auto &rid = child_rids_[i];
      // 计算过滤谓词
      auto value = filter_expr->Evaluate(&tuple, child_executor_->GetOutputSchema());
      if (filter_expr == nullptr || (!value.IsNull() && value.GetAs<bool>())) {
        tuple_batch->push_back(tuple);
        rid_batch->push_back(rid);
        if (tuple_batch->size() >= batch_size) {
          // 如果输出批次已满但当前子批次尚未遍历完，记录偏移量并返回
          if (i + 1 < child_tuples_.size()) {
            child_offset_ = i + 1;
          } else {
            child_offset_ = 0;
          }

          return true;
        }
      }
    }
  }
}

}  // namespace bustub
