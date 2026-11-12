# AGENTS.md

给在本仓库里工作的 AI agent 的规则。**优先级高于任何默认习惯。**

## 1. 硬性规则：默认只读

- **默认不修改任何文件。** 除非用户在当轮对话中**明确**要求写代码/改文件
  （例如「帮我实现」「把这行改掉」「创建 xxx 文件」），否则：
  - 不改 `src/`、`test/`、`tools/`、`CMakeLists.txt`、`build_support/` 里的任何文件
  - 不新增/删除文件（本文档自身除外，它是用户明确要求创建的）
  - 不执行 `git add` / `git commit` / `git checkout --` / `git reset` / `git stash`
  - 不改 `.gitignore`、不改 `build/` 之外的产物
- 用户问「这个怎么实现 / 为什么失败 / 这段什么意思」时：**只分析和讲解**，
  可以给伪代码、给关键片段、指出要改哪个函数，但**不要落盘**。
- 需要验证假设时，优先在 `/tmp` 下的副本里试，而不是改工作区。
- 若确实需要改文件才能验证，先说明「需要临时改 X，验证后还原」，得到同意再做，
  并在验证后立刻 `git checkout -- <file>` 还原。

## 2. 环境（已配置好，无需重装）

- 工具链（免 root 安装）：`~/.local/opt/bustub-toolchain/`，
  wrapper 在 `~/.local/bin/`：`clang-15`、`clang++-15`、`clang-format-15`、`clang-tidy-15`、`cmake`
- 版本：clang 15.0.7、cmake 3.28.3；Debug 模式默认开 AddressSanitizer + `-Wall -Wextra -Werror`
- 构建目录已存在：`build/`（已配置 `-DCMAKE_C_COMPILER=clang-15 -DCMAKE_CXX_COMPILER=clang++-15`）
- 详见 `/home/gsh/Code/Labs/bustub-2025fall-env.md`

```bash
cd build
cmake --build . --target <name>_test -j10   # 测试二进制在 build/test/<name>_test（不是 bin/）
./test/<name>_test                          # 模板测试多为 DISABLED_ 前缀
make check-tests                            # 全量（很慢）
make format && make check-format && make check-lint
./bin/bustub-sqllogictest ../test/sql/p3.01-seqscan.slt   # P3 的 SQL 逻辑测试
```

## 3. 作业结构（本仓库 = CMU 15-445 2025 Fall，tag v20251119-2025fall）

| Lab | 主题 | 主要文件 | 测试 |
| --- | --- | --- | --- |
| P0 | Count-min Sketch / SkipList | `src/primer/count_min_sketch.cpp`、`src/include/primer/count_min_sketch.h`、`src/primer/skiplist.cpp` | `test/primer/*` |
| P1 | Buffer Pool | `src/buffer/{arc_replacer,lru_k_replacer,buffer_pool_manager}.cpp`、`src/storage/disk/disk_scheduler.cpp`、`src/storage/page/page_guard.cpp` | `test/buffer/*`、`test/storage/{disk_scheduler,page_guard}_test.cpp` |
| P2 | B+Tree | `src/storage/page/b_plus_tree_*.cpp`、`src/storage/index/{b_plus_tree,index_iterator}.cpp`、`src/container/disk/hash/disk_extendible_hash_table.cpp` | `test/storage/b_plus_tree_*`、`test/container/disk/hash/*` |
| P3 | Query Execution | `src/execution/*_executor.cpp`、`src/execution/execution_common.cpp`、`src/optimizer/seqscan_as_indexscan.cpp` | `test/sql/p3.*.slt` |
| P4 | Concurrency Control | `src/concurrency/{transaction_manager,watermark}.cpp`、`src/include/execution/execution_common.h` | `test/concurrency/*.disabled` |

代码里用 `TODO(P0)` ~ `TODO(P4)` 标注要填的位置：`grep -rn "TODO(P1" src`。

## 4. 提交相关（不要自作主张动这些）

- 提交物由 `cd build && make submit-pN` 生成 `projectN-submission.zip`，
  只打包 `CMakeLists.txt` 里 `PN_FILES` 列出的文件（P0 只有 count-min sketch 的 `.h`/`.cpp`）。
- `AGENTS.md`、`build/`、`GRADESCOPE.md` 等**不在**打包列表里，不影响 OJ 评分。
- 不要为了「让提交干净」去删改用户的文件或 git 状态。

## 5. 教学口径

- 用户在自学这门课：**解释优先于给答案**。先讲清概念和坑（数据竞争、内存序、
  锁的粒度、move 语义等），再给方向；用户明确说「给我代码」时才给完整实现。
- 指出问题时给出定位（文件:行号）和原因，避免只说「错了」。
- 遵守学术诚信：不主动建议把实现公开、不帮助绕过 autograder。

## 6. Git 工作流：团队骨架 + 原子提交 + Conventional Commits

> 与第 1 节的关系：**默认仍然不执行任何 git 写操作**。
> 只有当用户明确说「提交 / commit / 建分支 / 合并」时才按本节执行，
> 并且**先给出计划与 commit message，等用户确认后再动手**。

### 6.1 分支模型（团队适配骨架）

```
upstream                ← 只读上游镜像（cmu-db/bustub 的 master），永不直接提交、永不 push
 └── dev                ← 集成分支：永远包含"所有已完成的 lab"；只接收 merge，不直接提交
      ├── lab/p0-primer       ← 每个 Lab 一条长分支，一律从 dev 拉出
      │    └── task/p0-t1-cms ← 可选：单个 Task 的短分支，用于试验/重构
      └── lab/p1-buffer-pool  ← 从 dev 拉出（P0 已合入 dev）
```

| 用途 | 命名 | 生命周期 |
| --- | --- | --- |
| 集成分支 | `dev`（本地分支，非 `master`/`main`） | 整个学期，只接收 merge |
| 单个 Lab 主线 | `lab/p<N>-<topic>`（如 `lab/p2-btree`） | 整个 lab 周期 |
| 单个 Task / 重构试验 | `task/p<N>-t<M>-<desc>`（如 `task/p1-t3-bpm-evict`） | 合回 lab 分支后删除 |
| 里程碑 | tag `p<N>-done`（如 `p1-done`，autograder 满分时打） | 永久 |

- 新 lab 分支一律从 `dev` 拉出：`git switch -c lab/p<N>-<topic> dev`。
- lab 完成后合回集成分支并打 tag：
  `git switch dev && git merge --no-ff lab/p<N>-<topic> && git tag -a p<N>-done -m "..."`。
  用 `--no-ff` 保留 lab 分支边界，里程碑 tag 打在 `dev` 上。
- `dev` 永不直接提交，只由 lab 分支 merge 前进。
- 同步上游：`git fetch origin && git merge origin/master`（在 `dev` 上做；如需要）。
- 短分支合回长分支用 `git merge --squash` 或 `rebase`，保持 lab 分支历史线性、可读。
- **禁止** `push` 到 `origin`（它指向公开仓库 cmu-db/bustub，既无权限也是学术诚信问题）；
  要备份就推到自己的**私有** remote（`git remote add private <私有仓库>`）。
- **禁止 fork**：public 仓库的 fork 强制 public 且无法转私有，等同公开发布实现（违反学术诚信）。

### 6.2 原子提交（Atomic Commits）

**定义**：一个 commit = 一个逻辑变更，且该 commit 单独 checkout 出来时
**能编译、相关测试能跑、格式检查能过**。

提交前自检：

- [ ] 只包含一个 task / 一个修复 / 一次重构，不夹带无关改动
- [ ] 没有顺手 `make format` 造成的其它文件改动（格式化单独一个 `style:` 提交）
- [ ] `cmake --build . --target <相关>_test -j10` 通过
- [ ] `make check-format check-lint` 通过
- [ ] 不含 `build/`、`*.zip`、`GRADESCOPE.md`、调试 `printf`、被注释掉的代码

反例（不要这样）：`feat(p1): arc + lru-k + bpm + 顺便格式化整个 src`
正例：拆成 `feat(p1/arc): implement eviction candidate selection`、
`feat(p1/bpm): add page eviction`、`style: run clang-format`

### 6.3 Conventional Commits（沟通语言）

格式：

```
<type>(<scope>): <subject>

<body: 为什么这么改，72 列换行>

<footer: BREAKING CHANGE / Refs #issue>
```

**type**（只用这些）：

| type | 用途 | 本仓库例子 |
| --- | --- | --- |
| `feat` | 实现作业要求的新功能 | `feat(p0/cms): implement count-min sketch` |
| `fix` | 修 bug（测试红了 → 绿） | `fix(p1/arc): correct ghost list demotion order` |
| `perf` | 只提升性能（不动语义） | `perf(p0/cms): use per-bucket atomics to cut contention` |
| `refactor` | 不改行为的结构调整 | `refactor(p2/btree): extract FindLeafPage helper` |
| `test` | 只改测试（取消 `DISABLED_`、加用例） | `test(p1): enable disabled replacer tests` |
| `style` | 格式化 / 空白 / lint 修复 | `style: apply clang-format to src/buffer` |
| `build` | CMake / 依赖 / 工具链 | `build: pin cmake toolchain to clang-15` |
| `docs` | 文档（含 AGENTS.md、注释） | `docs(agents): document git conventions` |
| `chore` | 杂务（签名、忽略文件、脚本） | `chore(sign): fill in Gradescope agreement info` |

**scope**：`<lab>/<模块>`，本仓库可用值：

```
p0/cms  p0/skiplist  p1/arc  p1/lruk  p1/bpm  p1/disk  p1/guard
p2/btree  p2/page  p2/iter  p2/hash  p3/exec  p3/agg  p3/join  p3/sort  p4/mvcc  p4/txn
```

也可用 `agents`、`deps`、`toolchain` 等非作业 scope。

**subject 规则**：祈使句（implement / fix / add，不用 implemented/added）、小写开头、
结尾不加句号、≤ 50 字符、说清「做了什么」而不是「改了哪个文件」。

**body 规则**：解释 **why**（约束、测试失败原因、取舍），不是复述 diff；
一行 72 列内；多条用 `-` 列表。

**footer**：破坏性变更写 `BREAKING CHANGE: ...`；关联 issue 写 `Refs #123`。

**本仓库常见错误对照**：

| 现状（不要） | 改成 |
| --- | --- |
| `add : 增加了个人信息` | `chore(sign): fill in Gradescope agreement info` |
| `update` / `fix bug` / `改了一下` | `fix(p1/bpm): release latch when eviction fails` |
| `p1 done!!!` | `feat(p1/arc): implement ARC replacer` + tag `p1-done` |
| 一个 commit 混 P1+P2 | 拆成两个 commit（`git add -p` 分批） |

### 6.4 提交前门禁（每次都要跑）

```bash
cd build
cmake --build . --target <相关>_test -j10 && ./test/<相关>_test
make check-format && make check-lint      # 失败就先 make format 再提交
```

### 6.5 Agent 执行边界

- 用户说「提交」时：先 `git status` + `git diff` 汇总 → **给出建议的 commit message 和要 stage 的文件清单** → 用户确认后才执行。
- **只 stage 明确相关的路径**（`git add <path>` 或 `git add -p`），**禁止** `git add -A` / `git add .`。
- 不 `push`、不 `push --force`、不改写已存在的提交（`rebase -i` / `commit --amend`）除非用户点名要求。
- 发现工作区里有用户自己的未完成改动（例如 `gradescope_sign.py`）时，**不要**顺手提交或还原，先问。
- 一次提交只解决一件事；用户要求「把所有东西提交了」时，先提议拆分方案。

### 6.6 可选：本地 commit-msg 校验钩子

`.git/hooks/commit-msg`（本地文件，不进仓库、不影响 OJ）：

```bash
#!/bin/sh
msg=$(head -1 "$1")
echo "$msg" | grep -qE '^(feat|fix|perf|refactor|test|style|build|docs|chore)(\([a-z0-9/_-]+\))?: [a-z].{0,49}$' \
  || { echo "✗ 不符合 Conventional Commits: <type>(<scope>): <subject>"; exit 1; }
```

需要时再安装（`chmod +x .git/hooks/commit-msg`），默认不启用。
