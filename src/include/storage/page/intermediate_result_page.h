#pragma once

#include <cstddef>
#include <memory>
#include <utility>
#include <vector>

#include "storage/table/tuple.h"

namespace bustub {

/**
 * 用于存放外部归并排序（External Merge Sort）和哈希连接（Hash Join）中间结果的页面。
 * 支持变长 Tuple（例如包含 VARCHAR 的 Tuple）。
 */
class IntermediateResultPage {
 public:
  /**
   * TODO(P3): 定义并实现用于从排序/中间结果页读取和写入数据的方法，可按需添加其他辅助方法。
   */
 private:
  /**
   * TODO(P3): 定义私有成员变量。你可能需要 Actual Data 之前存放一些必要的页面元数据。
   */
};

}  // namespace bustub
