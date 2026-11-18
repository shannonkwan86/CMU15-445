//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// b_plus_tree_page.cpp
//
// Identification: src/storage/page/b_plus_tree_page.cpp
//
// Copyright (c) 2015-2025, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#include "storage/page/b_plus_tree_page.h"

namespace bustub {

/*
 * 读取 / 设置页类型的辅助函数。
 * 页类型的枚举类定义在 b_plus_tree_page.h 里。
 */
auto BPlusTreePage::IsLeafPage() const -> bool { return page_type_ == IndexPageType::LEAF_PAGE; }
void BPlusTreePage::SetPageType(IndexPageType page_type) { page_type_ = page_type; }

/*
 * 读取 / 设置 size（本页存储的键值对数量）的辅助函数。
 */
auto BPlusTreePage::GetSize() const -> int { return size_; }
void BPlusTreePage::SetSize(int size) { size_ = size; }
void BPlusTreePage::ChangeSizeBy(int amount) { size_ += amount; }

/*
 * 读取 / 设置 max size（本页容量）的辅助函数。
 */
auto BPlusTreePage::GetMaxSize() const -> int { return max_size_; }
void BPlusTreePage::SetMaxSize(int size) { max_size_ = size; }

/*
 * 读取最小页大小的辅助函数。
 * 一般来说，最小页大小 == 最大页大小 / 2。
 * 但究竟该取 ceil() 还是 floor()，取决于你自己的实现。
 */
auto BPlusTreePage::GetMinSize() const -> int { return max_size_ / 2; }

}  // namespace bustub
