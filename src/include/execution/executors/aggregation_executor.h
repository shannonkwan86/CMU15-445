//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// aggregation_executor.h
//
// Identification: src/include/execution/executors/aggregation_executor.h
//
// Copyright (c) 2015-2025, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#pragma once

#include <memory>
#include <unordered_map>
#include <utility>
#include <vector>

#include "common/macros.h"
#include "execution/executor_context.h"
#include "execution/executors/abstract_executor.h"
#include "execution/expressions/abstract_expression.h"
#include "execution/plans/aggregation_plan.h"
#include "storage/table/tuple.h"
#include "type/value_factory.h"

namespace bustub {

/**
 * 简化版哈希表，提供聚合运算所需的全部基础功能。
 */
class SimpleAggregationHashTable {
 public:
  /**
   * 构造一个新的 SimpleAggregationHashTable 实例。
   * @param agg_exprs 聚合表达式列表
   * @param agg_types 聚合函数类型列表
   */
  SimpleAggregationHashTable(const std::vector<AbstractExpressionRef> &agg_exprs,
                             const std::vector<AggregationType> &agg_types)
      : agg_exprs_{agg_exprs}, agg_types_{agg_types} {}

  /** @return 该聚合执行器的初始聚合值 */
  auto GenerateInitialAggregateValue() -> AggregateValue {
    std::vector<Value> values{};
    for (const auto &agg_type : agg_types_) {
      switch (agg_type) {
        case AggregationType::CountStarAggregate:
          // COUNT(*) 从 0 开始计数
          values.emplace_back(ValueFactory::GetIntegerValue(0));
          break;
        case AggregationType::CountAggregate:
        case AggregationType::SumAggregate:
        case AggregationType::MinAggregate:
        case AggregationType::MaxAggregate:
          // 其他聚合类型初始值为 NULL
          values.emplace_back(ValueFactory::GetNullValueByType(TypeId::INTEGER));
          break;
      }
    }
    return {values};
  }

  /**
   * 将输入值合并到当前的聚合结果中。
   * @param[out] result 输出的聚合结果
   * @param input 待合并的输入值
   */
  void CombineAggregateValues(AggregateValue *result, const AggregateValue &input) {
    for (uint32_t i = 0; i < agg_exprs_.size(); i++) {
      switch (agg_types_[i]) {
        case AggregationType::CountStarAggregate:
        case AggregationType::CountAggregate:
        case AggregationType::SumAggregate:
        case AggregationType::MinAggregate:
        case AggregationType::MaxAggregate:
          break;
      }
    }

    UNIMPLEMENTED("TODO(P3): Add implementation.");
  }

  /**
   * 将键值对插入哈希表，并与当前已有的聚合值合并。
   * @param agg_key 待插入的聚合键
   * @param agg_val 待插入的聚合值
   */
  void InsertCombine(const AggregateKey &agg_key, const AggregateValue &agg_val) {
    if (ht_.count(agg_key) == 0) {
      ht_.insert({agg_key, GenerateInitialAggregateValue()});
    }
    CombineAggregateValues(&ht_[agg_key], agg_val);
  }

  /**
   * 清空哈希表
   */
  void Clear() { ht_.clear(); }

  /** 遍历聚合哈希表的迭代器 */
  class Iterator {
   public:
    /** 为聚合哈希表创建迭代器 */
    explicit Iterator(std::unordered_map<AggregateKey, AggregateValue>::const_iterator iter) : iter_{iter} {}

    /** @return 迭代器当前指向的键 */
    auto Key() -> const AggregateKey & { return iter_->first; }

    /** @return 迭代器当前指向的值 */
    auto Val() -> const AggregateValue & { return iter_->second; }

    /** @return 递增后的迭代器引用 */
    auto operator++() -> Iterator & {
      ++iter_;
      return *this;
    }

    /** @return 两个迭代器指向同一位置时返回 `true` */
    auto operator==(const Iterator &other) -> bool { return this->iter_ == other.iter_; }

    /** @return 两个迭代器指向不同位置时返回 `true` */
    auto operator!=(const Iterator &other) -> bool { return this->iter_ != other.iter_; }

   private:
    /** 底层聚合哈希表迭代器 */
    std::unordered_map<AggregateKey, AggregateValue>::const_iterator iter_;
  };

  /** @return 指向哈希表起始位置的迭代器 */
  auto Begin() -> Iterator { return Iterator{ht_.cbegin()}; }

  /** @return 指向哈希表末尾位置的迭代器 */
  auto End() -> Iterator { return Iterator{ht_.cend()}; }

 private:
  /** 哈希表本质上是从 AggregateKey 到 AggregateValue 的映射 */
  std::unordered_map<AggregateKey, AggregateValue> ht_{};
  /** 聚合表达式列表 */
  const std::vector<AbstractExpressionRef> &agg_exprs_;
  /** 聚合类型列表 */
  const std::vector<AggregationType> &agg_types_;
};

/**
 * AggregationExecutor 对子执行器产出的 Tuple 执行聚合操作（如 COUNT、SUM、MIN、MAX）。
 */
class AggregationExecutor : public AbstractExecutor {
 public:
  AggregationExecutor(ExecutorContext *exec_ctx, const AggregationPlanNode *plan,
                      std::unique_ptr<AbstractExecutor> &&child_executor);

  void Init() override;

  auto Next(std::vector<bustub::Tuple> *tuple_batch, std::vector<bustub::RID> *rid_batch, size_t batch_size)
      -> bool override;

  /** @return 聚合操作的输出 Schema */
  auto GetOutputSchema() const -> const Schema & override { return plan_->OutputSchema(); };

  auto GetChildExecutor() const -> const AbstractExecutor *;

 private:
  /** @return 从 Tuple 提取出的 AggregateKey */
  auto MakeAggregateKey(const Tuple *tuple) -> AggregateKey {
    std::vector<Value> keys;
    for (const auto &expr : plan_->GetGroupBys()) {
      keys.emplace_back(expr->Evaluate(tuple, child_executor_->GetOutputSchema()));
    }
    return {keys};
  }

  /** @return 从 Tuple 提取出的 AggregateValue */
  auto MakeAggregateValue(const Tuple *tuple) -> AggregateValue {
    std::vector<Value> vals;
    for (const auto &expr : plan_->GetAggregates()) {
      vals.emplace_back(expr->Evaluate(tuple, child_executor_->GetOutputSchema()));
    }
    return {vals};
  }

 private:
  /** 待执行的聚合计划节点 */
  const AggregationPlanNode *plan_;

  /** 提供待聚合 Tuple 的子执行器 */
  std::unique_ptr<AbstractExecutor> child_executor_;

  /** 简化版聚合哈希表 */
  // TODO(Student): Uncomment SimpleAggregationHashTable aht_;

  /** 简化版聚合哈希表迭代器 */
  // TODO(Student): Uncomment SimpleAggregationHashTable::Iterator aht_iterator_;
};
}  // namespace bustub
