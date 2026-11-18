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
auto BPlusTreePage::IsLeafPage() const -> bool { UNIMPLEMENTED("TODO(P2): Add implementation."); }
void BPlusTreePage::SetPageType(IndexPageType page_type) { UNIMPLEMENTED("TODO(P2): Add implementation."); }

/*
 * 读取 / 设置 size（本页存储的键值对数量）的辅助函数。
 */
auto BPlusTreePage::GetSize() const -> int { UNIMPLEMENTED("TODO(P2): Add implementation."); }
void BPlusTreePage::SetSize(int size) { UNIMPLEMENTED("TODO(P2): Add implementation."); }
void BPlusTreePage::ChangeSizeBy(int amount) { UNIMPLEMENTED("TODO(P2): Add implementation."); }

/*
 * 读取 / 设置 max size（本页容量）的辅助函数。
 */
auto BPlusTreePage::GetMaxSize() const -> int { UNIMPLEMENTED("TODO(P2): Add implementation."); }
void BPlusTreePage::SetMaxSize(int size) { UNIMPLEMENTED("TODO(P2): Add implementation."); }

/*
 * 读取最小页大小的辅助函数。
 * 一般来说，最小页大小 == 最大页大小 / 2。
 * 但究竟该取 ceil() 还是 floor()，取决于你自己的实现。
 */
auto BPlusTreePage::GetMinSize() const -> int { UNIMPLEMENTED("TODO(P2): Add implementation."); }

}  // namespace bustub
