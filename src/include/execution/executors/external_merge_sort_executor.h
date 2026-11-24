//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// external_merge_sort_executor.h
//
// Identification: src/include/execution/executors/external_merge_sort_executor.h
//
// Copyright (c) 2015-2025, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#pragma once

#include <cstddef>
#include <memory>
#include <utility>
#include <vector>
#include "common/config.h"
#include "common/macros.h"
#include "execution/execution_common.h"
#include "execution/executors/abstract_executor.h"
#include "execution/plans/sort_plan.h"
#include "storage/page/intermediate_result_page.h"
#include "storage/table/tuple.h"

namespace bustub {

/**
 * 在外部归并排序期间，用于保存一段有序 Tuple（一个 run）的数据结构。
 * Tuple 可能分布在多个页面中，且页内和跨页均有序。
 */
class MergeSortRun {
 public:
  MergeSortRun() = default;
  MergeSortRun(std::vector<page_id_t> pages, BufferPoolManager *bpm) : pages_(std::move(pages)), bpm_(bpm) {}

  auto GetPageCount() -> size_t { return pages_.size(); }

  /** 用于遍历单个有序 run 中所有 Tuple 的迭代器。 */
  class Iterator {
    friend class MergeSortRun;

   public:
    Iterator() = default;

    /**
     * 将迭代器推进到下一条 Tuple。如果当前排序页已遍历完，则移动到下一个排序页。
     */
    auto operator++() -> Iterator & { UNIMPLEMENTED("TODO(P3): Add implementation."); }

    /**
     * 解引用迭代器，获取当前指向的有序 run 中的 Tuple。
     */
    auto operator*() -> Tuple { UNIMPLEMENTED("TODO(P3): Add implementation."); }

    /**
     * 检查两个迭代器是否指向同一个有序 run 中的同一条 Tuple。
     */
    auto operator==(const Iterator &other) const -> bool { UNIMPLEMENTED("TODO(P3): Add implementation."); }

    /**
     * 检查两个迭代器是否指向不同的 Tuple，或正在遍历不同的有序 run。
     */
    auto operator!=(const Iterator &other) const -> bool { UNIMPLEMENTED("TODO(P3): Add implementation."); }

   private:
    explicit Iterator(const MergeSortRun *run) : run_(run) {}

    /** 当前迭代器正在遍历的有序 run。 */
    [[maybe_unused]] const MergeSortRun *run_;

    /**
     * TODO(P3): 在此添加你的私有成员变量，例如记录当前在有序 run 中的位置。
     * 也可以按需添加额外的构造函数来初始化这些成员。
     */
  };

  /**
   * 获取指向有序 run 起始位置（第一条 Tuple）的迭代器。
   */
  auto Begin() -> Iterator { UNIMPLEMENTED("TODO(P3): Add implementation."); }

  /**
   * 获取指向有序 run 末尾位置（最后一条 Tuple 之后）的迭代器。
   */
  auto End() -> Iterator { UNIMPLEMENTED("TODO(P3): Add implementation."); }

 private:
  /** 存放有序 Tuple 的所有排序页的 page ID 列表。 */
  std::vector<page_id_t> pages_;
  /**
   * 用于读取排序页的 BufferPoolManager。当排序页不再需要时，需通过它删除页面。
   */
  [[maybe_unused]] BufferPoolManager *bpm_;
};

/**
 * ExternalMergeSortExecutor 执行外部归并排序（External Merge Sort）。
 *
 * 本学期仅要求实现二路外部归并排序（2-way external merge sort）。
 */
template <size_t K>
class ExternalMergeSortExecutor : public AbstractExecutor {
 public:
  ExternalMergeSortExecutor(ExecutorContext *exec_ctx, const SortPlanNode *plan,
                            std::unique_ptr<AbstractExecutor> &&child_executor);

  void Init() override;

  auto Next(std::vector<bustub::Tuple> *tuple_batch, std::vector<bustub::RID> *rid_batch, size_t batch_size)
      -> bool override;

  /** @return 外部归并排序的输出 Schema */
  auto GetOutputSchema() const -> const Schema & override { return plan_->OutputSchema(); }

 private:
  /** 待执行的排序计划节点 */
  const SortPlanNode *plan_;

  /** 根据 ORDER BY 子句比较 Tuple 的比较器 */
  TupleComparator cmp_;

  /** TODO(P3): 在此添加你自己的私有成员变量。 */
};

}  // namespace bustub
