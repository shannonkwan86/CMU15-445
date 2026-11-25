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
#include <algorithm>
#include <vector>
#include "common/macros.h"
#include "execution/plans/sort_plan.h"

namespace bustub {

namespace {

constexpr size_t SORT_CHUNK_BYTES = BUSTUB_PAGE_SIZE * 4;

class RunWriter {
 public:
  explicit RunWriter(BufferPoolManager *bpm) : bpm_(bpm) {}

  void Append(const Tuple &tuple) {
    if (!cur_guard_.has_value()) {
      AllocatePage();
    }
    if (!cur_guard_->AsMut<IntermediateResultPage>()->Append(tuple)) {
      cur_guard_.reset();
      AllocatePage();
      BUSTUB_ASSERT(cur_guard_->AsMut<IntermediateResultPage>()->Append(tuple), "Tuple exceeds IntermediateResultPage");
    }
  }

  auto Finish() -> MergeSortRun {
    cur_guard_.reset();
    return {std::move(pages_), bpm_};
  }

 private:
  void AllocatePage() {
    auto pid = bpm_->NewPage();
    pages_.push_back(pid);
    cur_guard_.emplace(bpm_->WritePage(pid));
    cur_guard_->AsMut<IntermediateResultPage>()->Init();
  }

  BufferPoolManager *bpm_;
  std::vector<page_id_t> pages_;
  std::optional<WritePageGuard> cur_guard_;
};

auto MergeTwoRuns(MergeSortRun &run_a, MergeSortRun &run_b, const TupleComparator &cmp,
                  const std::vector<OrderBy> &order_bys, const Schema &schema, BufferPoolManager *bpm) -> MergeSortRun {
  RunWriter writer(bpm);
  auto it_a = run_a.Begin();
  auto end_a = run_a.End();
  auto it_b = run_b.Begin();
  auto end_b = run_b.End();

  std::optional<SortEntry> cur_a;
  std::optional<SortEntry> cur_b;
  if (it_a != end_a) {
    auto t = *it_a;
    cur_a.emplace(GenerateSortKey(t, order_bys, schema), std::move(t));
  }
  if (it_b != end_b) {
    auto t = *it_b;
    cur_b.emplace(GenerateSortKey(t, order_bys, schema), std::move(t));
  }

  while (cur_a.has_value() && cur_b.has_value()) {
    if (cmp(*cur_a, *cur_b)) {
      writer.Append(cur_a->second);
      ++it_a;
      if (it_a != end_a) {
        auto t = *it_a;
        cur_a.emplace(GenerateSortKey(t, order_bys, schema), std::move(t));
      } else {
        cur_a.reset();
      }
    } else {
      writer.Append(cur_b->second);
      ++it_b;
      if (it_b != end_b) {
        auto t = *it_b;
        cur_b.emplace(GenerateSortKey(t, order_bys, schema), std::move(t));
      } else {
        cur_b.reset();
      }
    }
  }

  while (cur_a.has_value()) {
    writer.Append(cur_a->second);
    ++it_a;
    if (it_a != end_a) {
      auto t = *it_a;
      cur_a.emplace(GenerateSortKey(t, order_bys, schema), std::move(t));
    } else {
      cur_a.reset();
    }
  }

  while (cur_b.has_value()) {
    writer.Append(cur_b->second);
    ++it_b;
    if (it_b != end_b) {
      auto t = *it_b;
      cur_b.emplace(GenerateSortKey(t, order_bys, schema), std::move(t));
    } else {
      cur_b.reset();
    }
  }

  return writer.Finish();
}

}  // namespace

template <size_t K>
ExternalMergeSortExecutor<K>::ExternalMergeSortExecutor(ExecutorContext *exec_ctx, const SortPlanNode *plan,
                                                        std::unique_ptr<AbstractExecutor> &&child_executor)
    : AbstractExecutor(exec_ctx), plan_(plan), cmp_(plan->GetOrderBy()), child_executor_(std::move(child_executor)) {}

/** 初始化外部归并排序 */
template <size_t K>
void ExternalMergeSortExecutor<K>::Init() {
  child_executor_->Init();
  if (sorted_run_.has_value()) {
    sorted_run_->Clear();
    sorted_run_.reset();
  }

  auto *bpm = exec_ctx_->GetBufferPoolManager();
  const auto &order_bys = plan_->GetOrderBy();
  const auto &schema = child_executor_->GetOutputSchema();

  // 1. 分块内排：每次攒满 SORT_CHUNK_BYTES 排好序后写入临时页，形成初始有序 run
  std::vector<MergeSortRun> runs;
  std::vector<SortEntry> entries;
  size_t chunk_bytes = 0;

  auto flush_entries = [&]() {
    if (entries.empty()) {
      return;
    }
    std::sort(entries.begin(), entries.end(), cmp_);
    RunWriter writer(bpm);
    for (const auto &entry : entries) {
      writer.Append(entry.second);
    }
    runs.push_back(writer.Finish());
    entries.clear();
    chunk_bytes = 0;
  };

  std::vector<Tuple> child_tuples{};
  std::vector<RID> child_rids{};
  while (child_executor_->Next(&child_tuples, &child_rids, BUSTUB_BATCH_SIZE)) {
    for (auto &tuple : child_tuples) {
      chunk_bytes += sizeof(int32_t) + tuple.GetLength();
      auto key = GenerateSortKey(tuple, order_bys, schema);
      entries.emplace_back(std::move(key), std::move(tuple));
      if (chunk_bytes >= SORT_CHUNK_BYTES) {
        flush_entries();
      }
    }
  }
  flush_entries();

  if (runs.empty()) {
    return;
  }

  // 2. 二路归并：两两合并有序 run 并回收旧临时页，直到只剩唯一有序 run
  while (runs.size() > 1) {
    std::vector<MergeSortRun> next_runs;
    next_runs.reserve((runs.size() + 1) / 2);
    for (size_t i = 0; i < runs.size(); i += 2) {
      if (i + 1 >= runs.size()) {
        next_runs.push_back(std::move(runs[i]));
      } else {
        next_runs.push_back(MergeTwoRuns(runs[i], runs[i + 1], cmp_, order_bys, schema, bpm));
        runs[i].Clear();
        runs[i + 1].Clear();
      }
    }
    runs = std::move(next_runs);
  }

  sorted_run_ = std::move(runs[0]);
  iter_ = sorted_run_->Begin();
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
  tuple_batch->clear();
  rid_batch->clear();

  if (!sorted_run_.has_value()) {
    return false;
  }

  auto end = sorted_run_->End();
  while (iter_ != end && tuple_batch->size() < batch_size) {
    tuple_batch->push_back(*iter_);
    rid_batch->emplace_back();
    ++iter_;
  }

  return !tuple_batch->empty();
}

template class ExternalMergeSortExecutor<2>;

}  // namespace bustub
