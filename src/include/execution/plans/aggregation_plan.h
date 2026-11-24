//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// aggregation_plan.h
//
// Identification: src/include/execution/plans/aggregation_plan.h
//
// Copyright (c) 2015-2025, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#pragma once

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "common/util/hash_util.h"
#include "execution/expressions/abstract_expression.h"
#include "execution/plans/abstract_plan.h"
#include "fmt/format.h"
#include "storage/table/tuple.h"

namespace bustub {

/** AggregationType 枚举系统中支持的所有聚合函数类型 */
enum class AggregationType { CountStarAggregate, CountAggregate, SumAggregate, MinAggregate, MaxAggregate };

/**
 * AggregationPlanNode 表示各种 SQL 聚合函数，例如 COUNT()、SUM()、MIN() 和 MAX()。
 *
 * NOTE: 为简化本项目，AggregationPlanNode 始终有且仅有一个子节点。
 */
class AggregationPlanNode : public AbstractPlanNode {
 public:
  /**
   * 构造一个新的 AggregationPlanNode。
   * @param output_schema 该计划节点的输出 Schema
   * @param child 提供待聚合数据的子计划节点
   * @param group_bys 聚合的 GROUP BY 表达式列表
   * @param aggregates 待聚合的表达式列表
   * @param agg_types 对应的聚合函数类型列表
   */
  AggregationPlanNode(SchemaRef output_schema, AbstractPlanNodeRef child, std::vector<AbstractExpressionRef> group_bys,
                      std::vector<AbstractExpressionRef> aggregates, std::vector<AggregationType> agg_types)
      : AbstractPlanNode(std::move(output_schema), {std::move(child)}),
        group_bys_(std::move(group_bys)),
        aggregates_(std::move(aggregates)),
        agg_types_(std::move(agg_types)) {}

  /** @return 计划节点类型 */
  auto GetType() const -> PlanType override { return PlanType::Aggregation; }

  /** @return 该聚合计划节点的子节点 */
  auto GetChildPlan() const -> AbstractPlanNodeRef {
    BUSTUB_ASSERT(GetChildren().size() == 1, "Aggregation expected to only have one child.");
    return GetChildAt(0);
  }

  /** @return 第 idx 个 GROUP BY 表达式 */
  auto GetGroupByAt(uint32_t idx) const -> const AbstractExpressionRef & { return group_bys_[idx]; }

  /** @return 所有 GROUP BY 表达式 */
  auto GetGroupBys() const -> const std::vector<AbstractExpressionRef> & { return group_bys_; }

  /** @return 第 idx 个聚合表达式 */
  auto GetAggregateAt(uint32_t idx) const -> const AbstractExpressionRef & { return aggregates_[idx]; }

  /** @return 所有聚合表达式 */
  auto GetAggregates() const -> const std::vector<AbstractExpressionRef> & { return aggregates_; }

  /** @return 所有聚合类型 */
  auto GetAggregateTypes() const -> const std::vector<AggregationType> & { return agg_types_; }

  static auto InferAggSchema(const std::vector<AbstractExpressionRef> &group_bys,
                             const std::vector<AbstractExpressionRef> &aggregates,
                             const std::vector<AggregationType> &agg_types) -> Schema;

  BUSTUB_PLAN_NODE_CLONE_WITH_CHILDREN(AggregationPlanNode);

  /** GROUP BY 表达式列表 */
  std::vector<AbstractExpressionRef> group_bys_;
  /** 聚合表达式列表 */
  std::vector<AbstractExpressionRef> aggregates_;
  /** 聚合类型列表 */
  std::vector<AggregationType> agg_types_;

 protected:
  auto PlanNodeToString() const -> std::string override;
};

/** AggregateKey 表示聚合操作中的分组键 */
struct AggregateKey {
  /** GROUP BY 各列的值 */
  std::vector<Value> group_bys_;

  /**
   * 比较两个 AggregateKey 是否相等。TODO(p3): 你可能需要修改这里以正确处理 NULL。
   * @param other 用于比较的另一个 AggregateKey
   * @return 若两个 AggregateKey 的所有分组值等价则返回 `true`，否则返回 `false`
   */
  auto operator==(const AggregateKey &other) const -> bool {
    for (uint32_t i = 0; i < other.group_bys_.size(); i++) {
      if (group_bys_[i].CompareEquals(other.group_bys_[i]) != CmpBool::CmpTrue) {
        return false;
      }
    }
    return true;
  }
};

/** AggregateValue 表示各聚合函数的当前累计结果 */
struct AggregateValue {
  /** 各聚合值列表 */
  std::vector<Value> aggregates_;
};

}  // namespace bustub

namespace std {

/** 为 AggregateKey 实现 std::hash */
template <>
struct hash<bustub::AggregateKey> {
  auto operator()(const bustub::AggregateKey &agg_key) const -> std::size_t {
    size_t curr_hash = 0;
    for (const auto &key : agg_key.group_bys_) {
      if (!key.IsNull()) {
        curr_hash = bustub::HashUtil::CombineHashes(curr_hash, bustub::HashUtil::HashValue(&key));
      }
    }
    return curr_hash;
  }
};

}  // namespace std

template <>
struct fmt::formatter<bustub::AggregationType> : formatter<std::string> {
  template <typename FormatContext>
  auto format(bustub::AggregationType c, FormatContext &ctx) const {
    using bustub::AggregationType;
    std::string name = "unknown";
    switch (c) {
      case AggregationType::CountStarAggregate:
        name = "count_star";
        break;
      case AggregationType::CountAggregate:
        name = "count";
        break;
      case AggregationType::SumAggregate:
        name = "sum";
        break;
      case AggregationType::MinAggregate:
        name = "min";
        break;
      case AggregationType::MaxAggregate:
        name = "max";
        break;
    }
    return formatter<std::string>::format(name, ctx);
  }
};
