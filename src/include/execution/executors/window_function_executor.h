//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// window_function_executor.h
//
// Identification: src/include/execution/executors/window_function_executor.h
//
// Copyright (c) 2015-2025, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#pragma once

#include <cstddef>
#include <memory>
#include <vector>

#include "execution/executor_context.h"
#include "execution/executors/abstract_executor.h"
#include "execution/plans/window_plan.h"
#include "storage/table/tuple.h"

namespace bustub {

/**
 * WindowFunctionExecutor 对包含窗口函数的列执行窗口函数计算。
 *
 * 窗口函数与普通聚合不同：它为每一条输入行都输出一行，并且可以与普通 SELECT 列组合输出。
 * WindowFunctionPlanNode 中的 columns 既包含普通列，也包含窗口函数列的占位符。
 *
 * 例如，对于如下查询：
 *    SELECT 0.1, 0.2, SUM(0.3) OVER (PARTITION BY 0.2 ORDER BY 0.3), SUM(0.4) OVER (PARTITION BY 0.1 ORDER BY 0.2,0.3)
 *      FROM table;
 *
 * WindowFunctionPlanNode 包含如下结构：
 *    columns: std::vector<AbstractExpressionRef>{0.1, 0.2, 0.-1(placeholder), 0.-1(placeholder)}
 *    window_functions_: {
 *      3: {
 *        partition_by: std::vector<AbstractExpressionRef>{0.2}
 *        order_by: std::vector<AbstractExpressionRef>{0.3}
 *        functions: std::vector<AbstractExpressionRef>{0.3}
 *        window_func_type: WindowFunctionType::SumAggregate
 *      }
 *      4: {
 *        partition_by: std::vector<AbstractExpressionRef>{0.1}
 *        order_by: std::vector<AbstractExpressionRef>{0.2,0.3}
 *        functions: std::vector<AbstractExpressionRef>{0.4}
 *        window_func_type: WindowFunctionType::SumAggregate
 *      }
 *    }
 *
 * 你的执行器应通过子执行器和 columns 中的表达式产出普通列，
 * 并利用 window_agg_indexes、partition_bys、order_bys、functions 和 window_agg_types 计算窗口函数列的结果。
 * 不允许直接对 columns 中的窗口函数占位符求值，因为其中包含无效的列 ID。
 *
 * WindowFunctionExecutor 不需要支持自定义窗口帧（例如 1 PRECEDING AND 1 FOLLOWING）。
 * 可以假设：当存在 ORDER BY 子句时窗口帧为 UNBOUNDED PRECEDING AND CURRENT ROW；
 * 当不存在 ORDER BY 子句时窗口帧为 UNBOUNDED PRECEDING AND UNBOUNDED FOLLOWING。
 */
class WindowFunctionExecutor : public AbstractExecutor {
 public:
  WindowFunctionExecutor(ExecutorContext *exec_ctx, const WindowFunctionPlanNode *plan,
                         std::unique_ptr<AbstractExecutor> &&child_executor);

  void Init() override;

  auto Next(std::vector<bustub::Tuple> *tuple_batch, std::vector<bustub::RID> *rid_batch, size_t batch_size)
      -> bool override;

  /** @return 窗口聚合计划节点的输出 Schema */
  auto GetOutputSchema() const -> const Schema & override { return plan_->OutputSchema(); }

 private:
  /** 待执行的窗口聚合计划节点 */
  const WindowFunctionPlanNode *plan_;

  /** 提供输入 Tuple 的子执行器 */
  std::unique_ptr<AbstractExecutor> child_executor_;

  /** 物化完成后的全部输出 Tuple 及当前输出游标 */
  std::vector<Tuple> output_tuples_;
  size_t cursor_{0};
};
}  // namespace bustub
