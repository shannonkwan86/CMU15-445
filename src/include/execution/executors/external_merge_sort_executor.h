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
#include <optional>
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
    auto operator++() -> Iterator & {
      ++tuple_idx_;
      auto guard = run_->bpm_->ReadPage(run_->pages_[page_idx_]);
      if (tuple_idx_ >= guard.As<IntermediateResultPage>()->GetNumTuples()) {
        ++page_idx_;
        tuple_idx_ = 0;
      }
      return *this;
    }

    /**
     * 解引用迭代器，获取当前指向的有序 run 中的 Tuple。
     */
    auto operator*() -> Tuple {
      auto guard = run_->bpm_->ReadPage(run_->pages_[page_idx_]);
      return guard.As<IntermediateResultPage>()->GetTuple(tuple_idx_);
    }

    /**
     * 检查两个迭代器是否指向同一个有序 run 中的同一条 Tuple。
     */
    auto operator==(const Iterator &other) const -> bool {
      return run_ == other.run_ && page_idx_ == other.page_idx_ && tuple_idx_ == other.tuple_idx_;
    }

    /**
     * 检查两个迭代器是否指向不同的 Tuple，或正在遍历不同的有序 run。
     */
    auto operator!=(const Iterator &other) const -> bool { return !(*this == other); }

   private:
    explicit Iterator(const MergeSortRun *run, size_t page_idx = 0, uint32_t tuple_idx = 0)
        : run_(run), page_idx_(page_idx), tuple_idx_(tuple_idx) {}

    /** 当前迭代器正在遍历的有序 run。 */
    const MergeSortRun *run_{nullptr};
    size_t page_idx_{0};
    uint32_t tuple_idx_{0};
  };

  /**
   * 获取指向有序 run 起始位置（第一条 Tuple）的迭代器。
   */
  auto Begin() -> Iterator {
    if (pages_.empty()) {
      return End();
    }
    return Iterator{this, 0, 0};
  }

  /**
   * 获取指向有序 run 末尾位置（最后一条 Tuple 之后）的迭代器。
   */
  auto End() -> Iterator { return Iterator{this, pages_.size(), 0}; }

  /** 释放当前 run 占用的所有临时页面 */
  void Clear() {
    for (const auto pid : pages_) {
      bpm_->DeletePage(pid);
    }
    pages_.clear();
  }

 private:
  /** 存放有序 Tuple 的所有排序页的 page ID 列表。 */
  std::vector<page_id_t> pages_;
  /**
   * 用于读取排序页的 BufferPoolManager。当排序页不再需要时，需通过它删除页面。
   */
  BufferPoolManager *bpm_{nullptr};
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

  /** 提供待排序 Tuple 的子执行器 */
  std::unique_ptr<AbstractExecutor> child_executor_;

  /** 最终归并完成的唯一有序 run 及遍历迭代器 */
  std::optional<MergeSortRun> sorted_run_;
  MergeSortRun::Iterator iter_;
};

}  // namespace bustub
