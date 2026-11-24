# Project #3 - 查询执行（Query Execution）中文文档

> 原文链接：https://15445.courses.cs.cmu.edu/fall2025/project3/  
> 原文更新日期：Nov 06, 2025  
> 适用学期：CMU 15-445/645 Fall 2025

---

## 概述（Overview）

至此，你已经实现了数据库管理系统的多个底层核心组件：Project #1 的 Buffer Pool Manager 和 Project #2 的 B+Tree 索引。在 Project #3 中，你将实现让 BusTub 执行 SQL 查询的核心组件——编写**算子执行器（operator executors）**以及用于变换查询计划的**优化器规则（optimizer rules）**。

本项目包含以下任务：

- **Task #1**：访问方法执行器（Access Method Executors）
- **Task #2**：聚合与连接执行器（Aggregation and Join Executors）
- **Task #3**：HashJoin 执行器与优化（HashJoin Executor and Optimization）
- **Task #4**：外部归并排序 + Limit 执行器 + 窗口函数（External Merge Sort + Limit Executors + Window Functions）
- **可选任务**：排行榜优化挑战（Optional Leaderboard Task）

---

## 背景知识（Background）

建议仔细阅读本节，因为你需要自己构造 SQL 查询来测试执行器实现。可以充分利用 `bustub-shell` 来帮助理解：

- 用 `EXPLAIN` 或 `EXPLAIN (o)` 查看原始计划和优化后的计划
- 理解计划节点中的参数含义（例如 `#0.0` 中第一个和第二个 `0` 分别代表什么）
- 阅读 `src/include/execution/plans/` 中几个简单的计划节点实现
- 留意 `src/include/execution/expressions/` 中的各类表达式类型

### 简介与 BusTub Shell

BusTub 仓库已经提供了完整的查询处理层前端。可以编译并运行 BusTub Shell 来执行 SQL：

```bash
cd build && make -j$(nproc) shell
./bin/bustub-shell
```

> 也可以在浏览器中使用官方的 BusTub Web Shell（内含完整参考实现）。

在 Shell 中可用 `\dt` 查看所有表。BusTub Shell 启动时会自动创建并预填充几张 mock 表（如 `__mock_table_1`、`__mock_table_2`、`__mock_table_3`），方便直接测试，重启后修改不会持久化。

注意事项：
- BusTub 仅支持 SQL 的一个小子集，具体支持的语法可参考 `test/sql/` 下的 SQLLogicTest 文件。
- 如果在 CLion 终端运行，需加上 `--disable-tty` 参数。
- 除内部命令外，所有 SQL 语句必须以 `;` 结尾。
- BusTub 仅支持 `INT` 和 `VARCHAR(n)` 类型；字符串必须用单引号，例如 `INSERT INTO table VALUES ('a')`。

### 查看 SQL 查询计划（`EXPLAIN`）

在任意查询前加 `EXPLAIN` 可打印查询处理流程：

```sql
EXPLAIN SELECT * FROM __mock_table_1;
```

输出分为三层：
1. **BINDER**：Parser + Binder 生成 AST（如 `BoundSelect`），自动展开 `*` 为具体列。
2. **PLANNER**：将 AST 转换为树形的初始查询计划，数据从叶子节点流向根节点。
3. **OPTIMIZER**：应用优化规则（例如消除冗余的 `Projection`）。

复杂查询示例：

```sql
EXPLAIN (o) SELECT colA, MAX(colB) FROM
  (SELECT * FROM __mock_table_1, __mock_table_3 WHERE colA = colE) GROUP BY colA;
```

`EXPLAIN` 是确认某条 SQL 是否命中了目标执行器的最有效工具。

### 示例执行器（已提供）

仓库中已提供若干示例执行器可供参考：

- **Projection**：对单个子节点做表达式计算，包含：
  - `ColumnValueExpression`：如 `#0.0` 表示第 0 个子节点的第 0 列（Join 中会出现 `#0.0 = #1.0`）
  - `ConstantExpression`：常量值
  - `ArithmeticExpression`：算术表达式树
- **Filter**：有一个子节点和一个谓词（predicate），过滤满足条件的元组。
- **Values**：直接产生用户给定的行数据，常用于 `INSERT INTO ... VALUES (...)`。

### 查询计划的输出 Schema

`EXPLAIN` 中每个计划节点后面的括号内容代表该节点的**输出 Schema**，例如：

```text
Projection { exprs=[#0.0, #0.1] } | (__mock_table_1.colA:INTEGER, __mock_table_1.colB:INTEGER)
```

你的执行器产出的元组必须**严格匹配**计划节点中定义的输出 Schema，否则测试会失败。

---

## 项目规范（Project Specification）

BusTub（Fall 2025）采用**基于行的向量化执行模型（row-based vectorization query processing model）**：

- 每个执行器实现 `Next` 方法，每次调用返回一批元组（最多 `BUSTUB_BATCH_SIZE` 条）以及对应的一批 `RID`，或者返回 `false` 表示没有更多元组。
- 执行器通过循环调用子执行器的 `Next` 方法，按批拉取并处理元组。
- 所有执行器在 `src/execution/executor_factory.cpp` 中由执行计划创建。
- 本项目所有测试用例都采用 SQLLogicTest 格式（位于 `test/sql/`）。

---

## Task #1：访问方法执行器（Access Method Executors）

Mock 表由 `MockScan` 生成，无法写入。本任务要实现真正读写存储层表的执行器，以及 `SeqScan -> IndexScan` 优化规则。

**涉及文件**：
- `src/include/execution/executors/seq_scan_executor.h` / `src/execution/seq_scan_executor.cpp`
- `src/include/execution/executors/insert_executor.h` / `src/execution/insert_executor.cpp`
- `src/include/execution/executors/update_executor.h` / `src/execution/update_executor.cpp`
- `src/include/execution/executors/delete_executor.h` / `src/execution/delete_executor.cpp`
- `src/include/execution/executors/index_scan_executor.h` / `src/execution/index_scan_executor.cpp`
- `src/optimizer/seqscan_as_indexscan.cpp`

### 1.1 SeqScan

遍历表并按批返回元组（`SELECT * FROM t1`）。

- **提示**：
  - 使用 `TableIterator` 时注意前置 `++` 和后置 `++` 的区别。
  - **不要**输出 `TableHeap` 中已删除的元组（检查对应 `TupleMeta` 的 `is_deleted_` 字段）。
  - 如果有下推的过滤谓词（`filter_predicate_`），需要在 `SeqScanExecutor` 中过滤。
  - 输出内容是匹配元组的拷贝及其原始 `RID`。
  - BusTub 不支持 `DROP TABLE` / `DROP INDEX`，需要清空表时重启 Shell 即可。

### 1.2 Insert

向表中插入元组并同步更新所有关联索引。有且仅有一个子节点提供待插入的值（Planner 保证其 Schema 与表一致）。

- **输出**：仅输出**单条**只含一个整数列的元组，表示插入的总行数。
- **提示**：
  - 通过 System Catalog（`Catalog::GetTable()`、`Catalog::GetTableIndexes()`）获取表和索引信息。
  - 通过 `TableHeap` 执行写入，并同步更新该表的所有索引。

### 1.3 Update

修改表中已有元组。有且仅有一个子节点提供待更新的记录（含原 `RID`）。

- **输出**：仅输出**单条**只含一个整数列的元组，表示更新的总行数。
- **提示**：
  - 实现方式：**先删除旧元组，再插入新元组**，并同步更新所有受影响的索引。

### 1.4 Delete

删除表中元组。有且仅有一个子节点提供待删除的记录。

- **输出**：仅输出**单条**只含一个整数列的元组，表示删除的总行数。
- **假设**：`DeleteExecutor` 总是位于查询计划的根节点，不应修改其结果集。
- **提示**：
  - 从子节点获取 `RID`，将对应 `TupleMeta` 的 `is_deleted_` 标记为 `true`，并同步更新所有受影响的索引。

### 1.5 IndexScan

利用 B+Tree 索引执行两类查询：

1. **点查（Point Lookup）**：`SELECT * FROM <table> WHERE <index_col> = <val>`（支持对同一索引的多个点查，如 `OR`）。
2. **有序扫描（Ordered Scan）**：`SELECT * FROM <table> ORDER BY <index_col>`（`ORDER BY` 转 `IndexScan` 的优化规则已提供；只需处理升序 `ASC` / 默认排序）。

- **关键说明**：
  - 本项目中索引对象类型固定为 `BPlusTreeIndexForTwoIntegerColumn`，可直接安全转换：
    ```cpp
    tree_ = dynamic_cast<BPlusTreeIndexForTwoIntegerColumn *>(index_info_->index_.get());
    ```
  - 仅需支持单个唯一整数列上的索引，测试不会向带索引的表插入重复键，因此每次点查最多返回一条元组。
  - 同样**不要**输出已删除的元组。
  - 点查使用 `ScanKey`，有序扫描使用 `IndexIterator`。
  - 建议先做下一节的优化器规则，再实现 `IndexScanExecutor`，便于理解它需要承接哪些查询。

### 1.6 优化规则：SeqScan 转 IndexScan

在 `src/optimizer/seqscan_as_indexscan.cpp` 中，将满足条件的 `SeqScanPlanNode` 转换为 `IndexScanPlanNode`：

1. **谓词下推到 SeqScan**：`MergeFilterScan`（`src/optimizer/merge_filter_scan.cpp`）已默认启用，你需要确保 `SeqScanExecutor` 支持处理其谓词。
2. **命中索引转换**：检查谓词中的过滤列，若存在对应索引则生成 `IndexScanPlanNode`。拿到满分需支持：
   - 单个等值条件：`WHERE v1 = 1`
   - 常量在左侧的翻转写法：`WHERE 1 = v1`
   - 同一索引列上的多个 `OR` 点查：`WHERE v1 = 1 OR v1 = 4`
   - 注意去重边界情况：`WHERE v1 = 1 OR v1 = 1`（可参考 `AggregateKey` 的思路）
   - `WHERE v1 = 1 AND v2 = 2` 这类跨列 `AND` **保持 SeqScan**，无需拆分谓词。
   - 只在点查和有序扫描两种场景下将 `SeqScan` 转为 `IndexScan`。

> 完成 Task #1 后，应通过 **SQLLogicTests #1 ~ #6**（`p3.01` ~ `p3.06`）。

---

## Task #2：聚合与连接执行器（Aggregation & Join Executors）

**涉及文件**：
- `src/include/execution/plans/aggregation_plan.h`
- `src/include/execution/executors/aggregation_executor.h` / `src/execution/aggregation_executor.cpp`
- `src/include/execution/executors/nested_loop_join_executor.h` / `src/execution/nested_loop_join_executor.cpp`
- `src/include/execution/executors/nested_index_join_executor.h` / `src/execution/nested_index_join_executor.cpp`

### 2.1 Aggregation

对每个分组计算聚合函数（支持 `GROUP BY`、全局聚合、`DISTINCT`；`HAVING` 会由 Planner 拆成 `Aggregation` 上方的 `Filter`，无需在聚合算子内处理）。

- **输出 Schema**：先是所有 `GROUP BY` 列，随后是所有聚合结果列。
- **内存假设**：假设聚合哈希表可以完全放入内存，无需多阶段分区，也不必使用 Buffer Pool 页。
- **已有辅助类**：`SimpleAggregationHashTable`（基于 `std::unordered_map`）及其 `Iterator`，你需要补全其中的 `CombineAggregateValues`。
- **关键坑点**：
  - 聚合是 **pipeline breaker**，仔细决定构建哈希表放在 `Init()` 还是 `Next()`。
  - 必须正确处理聚合列中的 `NULL` 值（参见测试用例预期行为）。
  - `GROUP BY` 列也可能包含 `NULL` 值，需要检查 `AggregateKey` 的相等比较逻辑（SQL 中 `NULL == NULL` 在分组时应视为同一组，而普通 `Value::CompareEquals` 返回 `CmpBool::CmpNull`）。
  - **空表聚合**（无 `GROUP BY` 时）：`CountStarAggregate` 返回 `0`，其他聚合类型返回 `integer_null`（这也解释了为什么 `GenerateInitialAggregateValue` 把大多数初始值设为 NULL）；有 `GROUP BY` 且输入为空时，不输出任何行。

### 2.2 NestedLoopJoin

默认用于所有 Join 操作，需支持 **Inner Join** 和 **Left Outer Join**，采用课程讲的简单嵌套循环连接算法。

- **输出 Schema**：左表（outer）所有列 + 右表（inner）所有列。
- **提示**：
  - 使用 `NestedLoopJoinPlanNode` 中的谓词，通过 `AbstractExpression::EvaluateJoin` 求值（注意返回值可能是 `true`、`false` 或 `NULL`，参考 `FilterExecutor` 的判断方式）。
  - Left Join 中左表某行若无任何右表匹配，需输出该左表行 + 右表各列的 `NULL` 值。

### 2.3 NestedIndexJoin

当 Join 条件是等值条件且右表在连接列上有索引时，优化器会自动把 `NestedLoopJoin` 转为 `NestedIndexJoin`（只需支持 Inner / Left Join）。

- **结构**：只有一个子节点（左表 / outer）；右表（inner）通过 Catalog 中的索引查找。
- **输出 Schema**：左表所有列 + 右表所有列。
- **实现流程**：从子节点取左表元组 -> 用 `key_predicate` 构造索引探测键 -> 调用索引 `ScanKey` 获取 `RID` -> 从右表 `TableHeap` 取对应元组并拼接输出。
- **说明**：Gradescope 测试按原样提供，不会测索引中包含 `NULL` 的极端情况。

> 完成 Task #2 后，应通过 **SQLLogicTests #7 ~ #13**（`p3.07` ~ `p3.13`）。

---

## Task #3：HashJoin 执行器与优化（HashJoin Executor and Optimization）

需先完成 Task #2 的 `NestedLoopJoinExecutor`。

**涉及文件**：
- `src/include/storage/page/intermediate_result_page.h`
- `src/include/execution/executors/hash_join_executor.h` / `src/execution/hash_join_executor.cpp`
- `src/optimizer/nlj_as_hash_join.cpp`

### 3.1 HashJoin（Grace Hash Join）

支持由多个 `AND` 连接的等值条件组成的 **Inner Join** 和 **Left Outer Join**。

- **输出 Schema**：左表所有列 + 右表所有列。
- **关键要求（Fall 2025 重点）**：
  - **探测哈希表可能放不进内存**（假设内存最多容纳约 4KB 元组的哈希表），因此必须实现课堂讲授的 **Grace Hash Join** 算法（先分区落盘到临时页，再逐分区构建内存哈希表并探测）。
  - 需要自行设计并实现 `IntermediateResultPage`（位于 `src/include/storage/page/intermediate_result_page.h`）的页布局和读写方法，用于存储中间分区结果。
  - **建议**：设计 `IntermediateResultPage` 前，先阅读 Task #4 的外部归并排序要求，决定两处是否复用同一种页布局。
  - 必须正确处理哈希冲突（左右两侧都可能有多个元组映射到同一桶或相同键）。
  - 使用 `HashJoinPlanNode` 的 `GetLeftJoinKey()` 和 `GetRightJoinKey()` 构造多列连接键（可参考 `SimpleAggregationHashTable` 的多属性哈希方式）。
  - HashJoin 的 build 阶段也是 pipeline breaker，合理安排 `Init()` 与 `Next()` 的职责。

### 3.2 优化规则：NestedLoopJoin 转 HashJoin

在 `src/optimizer/nlj_as_hash_join.cpp` 中，将满足条件的 `NestedLoopJoinPlanNode` 转换为 `HashJoinPlanNode`：

- 适用条件：Join 谓词是由**任意数量**的等值条件（两列相等）通过 `AND` 连接而成的合取式。
- 示例：
  - 优化前：`NestedLoopJoin { type=Inner, predicate=((#0.0=#1.0)and(#0.1=#1.2)) }`
  - 优化后：`HashJoin { type=Inner, left_key=[#0.0, #0.1], right_key=[#0.0, #0.2] }`
- **提示**：
  - 等值条件左右两侧的列可能来自任意一张表（例如右表列写在等号左边），使用 `ColumnValueExpression::GetTupleIdx()` 判断列属于左表（0）还是右表（1）。
  - 优化器规则的执行顺序很重要（需要在 Filter 与 NLJ 合并之后再做 NLJ -> HashJoin）。
  - 处理多个 `AND` 等值条件时，建议递归提取左右键，而不是硬编码多层 `if`。

> 完成 Task #3 后，应通过 **SQLLogicTests #14 ~ #15**（`p3.14` ~ `p3.15`）。

---

## Task #4：外部归并排序 + Limit + 窗口函数

需先完成 Task #1 的 `IndexScanExecutor`。

**涉及文件**：
- `src/include/storage/page/intermediate_result_page.h`
- `src/execution/execution_common.cpp`
- `src/include/execution/executors/external_merge_sort_executor.h` / `src/execution/external_merge_sort_executor.cpp`
- `src/include/execution/executors/limit_executor.h` / `src/execution/limit_executor.cpp`
- `src/include/execution/executors/window_function_executor.h` / `src/execution/window_function_executor.cpp`

> 提示：`LimitExecutor` 比 `ExternalMergeSortExecutor` 简单很多，卡住时可先写 Limit，但测试时两者一起测。  
> 假设：所有 `ORDER BY` 中每个排序键只出现一次（无需担心平局 tie），但**必须支持含 `NULL` 值的列排序**。

### 4.1 External Merge Sort

当 `ORDER BY` 无法由索引直接满足时，BusTub 使用 `SortPlanNode`（输出 Schema 与输入一致）。

- **排序规则**：
  - 未指定方向时默认为 `ASC`。
  - 未指定 `NULLS FIRST` / `NULLS LAST` 时，默认 `ASC` 为 `NULLS FIRST`，`DESC` 为 `NULLS LAST`。
  - 在 `src/execution/execution_common.cpp` 中实现辅助比较器 `TupleComparator`。
- **外部排序要求**：
  - **假设数据放不进内存**：必须将中间排序结果存入临时页（`IntermediateResultPage`），并基于上一轮的有序 run 递归归并。
  - **支持 `VARCHAR` 列**：单个含 `VARCHAR` 的元组不会超过一页大小（无需跨页存储，当前页放不下就申请新页）。
  - 允许对**单页内的元组**使用 `std::sort`，但**禁止**把所有元组放在内存里排序。
  - `IntermediateResultPage` 的布局应尽量紧凑，减少碎片。
  - **及时删除废弃临时页**：上一轮归并结束不再使用的页、以及整个排序完成后不再使用的页，都必须通过 Buffer Pool 删除，避免残留“僵尸页”。
  - 本学期仅测试**二路外部归并排序（2-way external merge sort）**（尽管类是模板化的），实现二路即可。
  - 合理利用 Project #1 实现的 Page Guard，Buffer Pool Manager 会自动处理 pin/unpin 与落盘。

### 4.2 Limit

限制子执行器输出的元组总数（`LIMIT N`），输出 Schema 与输入一致。

- 若子执行器输出总数小于 `limit`，原样输出全部元组。
- 无需支持 `OFFSET`。

### 4.3 Window Functions

窗口函数一般包含 `PARTITION BY`、`ORDER BY` 和 `window frame` 三部分，概念模型为：按 `PARTITION BY` 分区 -> 分区内按 `ORDER BY` 排序 -> 在每个窗口帧上计算窗口函数。

- **本项目简化范围**：
  - **不需要**支持自定义 window frame（如 `ROWS BETWEEN ...`），只需支持 `PARTITION BY` 和 `ORDER BY`：
    - 有 `ORDER BY`（无 frame）：每个分区内从第一行累计计算到当前行（running aggregate）。
    - 无 `ORDER BY`（无 frame）：每个分区内计算整个分区的聚合值，分区内每行输出相同结果。
    - 两者都省略 `OVER ()`：整个表作为一个分区。
  - BusTub 保证同一查询中所有窗口函数拥有相同的 `ORDER BY` 子句（不会出现多个不同 `ORDER BY` 混用的情况）。
  - 测试不检查输出行顺序，只检查每行内各列的值是否匹配。因此有 `ORDER BY` 时可先排序再计算；无 `ORDER BY` 时保持子节点原顺序即可。
- **推荐实现步骤**：
  1. 若有 `ORDER BY`，按要求对元组排序（可复用排序相关代码）。
  2. 为每个分区生成初始聚合值。
  3. 逐行合并各分区的值并记录每行结果（可复用聚合执行器相关代码）。
- **额外要求**：除已有聚合函数外，还需实现 `RANK` 窗口函数（Planner 保证出现 `RANK` 时 `ORDER BY` 非空；注意处理并列/ties 的排名跳过规则，例如 1, 1, 3，具体以测试用例为准）。

---

## 可选任务：排行榜优化（Optional Leaderboard Tasks）

> 本部分完全可选，不影响满分；且与之前项目不同，P3 排行榜测试中的 segfault 或超时**不会**扣分。但必须保证所有常规测试在开启自定义优化规则后依然全部通过（部分测试会用 `set force_optimizer_starter_rule=yes` 强制走初始规则）。

- **Query 1（Too Many Joins!）**：多表笛卡尔积 + `WHERE` 中混写连接条件与过滤条件。
  - 推荐优化：拆分过滤条件，提取 HashJoin 键，并将剩余过滤条件下推到 HashJoin 之下。
- **Query 2（The Mad Data Scientist）**：包含大量冗余列、重复聚合表达式和恒假过滤条件（`WHERE 1 = 2`）。
  - 推荐优化：列裁剪（已提供骨架 `src/optimizer/column_pruning.cpp`）、公共子表达式消除、将恒假过滤转为 0 行 `Values` 节点、谓词下推等。
- **Query 3（All that work, for a little thing?）**：两张大表等值内连接，但实际匹配行极少。
  - 推荐优化：在 HashJoin 的 build 阶段构建 **Bloom Filter** 加速探测过滤。

**排行榜合规规则**：TA 会人工审查代码并可能修改测试参数（如调小 Buffer Pool 大小），严禁硬编码答案、严禁通过 `#ifdef NDEBUG` 等方式探测排行榜测试。

---

## 补充组件说明（Additional Information）

1. **System Catalog**（`src/include/catalog/catalog.h`）：核心接口 `Catalog::GetTable()`、`Catalog::GetIndex()`、`Catalog::GetTableIndexes()`。
2. **索引更新**：`Insert` / `Update` / `Delete` 必须同步更新该表的所有索引，底层直接调用你在 Project #2 实现的 B+Tree 索引。
3. **优化器规则编写指南**：BusTub 是基于规则的优化器，通常采用**自底向上（bottom-up）**的方式——先递归优化子节点，再检查当前节点结构与属性，匹配则替换为目标计划节点。可参考仓库中已实现的规则。

---

## 测试、代码规范与提交（Instructions & Submission）

### 运行测试（SQLLogicTest）

本项目提供**全部**测试用例（无隐藏测试），位于 `test/sql/p3.*.slt`：

```bash
cd build
make -j$(nproc) sqllogictest
./bin/bustub-sqllogictest ../test/sql/p3.00-primer.slt --verbose
```

> 每次修改代码后记得重新编译 `sqllogictest`。

### 格式检查与静态分析（任一失败整个项目 0 分）

```bash
cd build
make format
make check-format
make check-lint
make check-clang-tidy-p3
```

### 内存泄漏检测

Debug 模式默认开启 ASAN + LSAN；若调试器受影响需要临时关闭 sanitizer：

```bash
cmake -DCMAKE_BUILD_TYPE=Debug -DBUSTUB_SANITIZER= ..
```

### 打包提交

```bash
cd build
make submit-p3
cd ..
python3 gradescope_sign.py
unzip -l project3-submission.zip | grep GRADESCOPE
```

> 评分维度：全部测试用例正确通过 + 无内存泄漏 + 代码格式/lint/clang-tidy 全通过。
