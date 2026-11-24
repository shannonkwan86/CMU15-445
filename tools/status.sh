#!/bin/sh
# 项目当前状态。供人或 agent 在断言「现在是什么状态」之前执行，
# 把状态判断从「回忆上一次读到的内容」变成「读一次命令输出」。
#
# 用法：tools/status.sh

set -e
cd "$(git rev-parse --show-toplevel)"

echo "=== git ==="
git status --short --branch

echo
echo "=== 最近提交 ==="
git log --oneline -3

echo
echo "=== 未实现的 TODO(P2) ==="
grep -rn 'UNIMPLEMENTED("TODO(P2' src/ || echo "(none)"

echo
echo "=== 改动文件的格式检查 ==="
files=$(git diff --name-only HEAD -- '*.cpp' '*.h')
if [ -z "$files" ]; then
  echo "(no changed files)"
else
  # 这里故意不加引号：需要按空白拆成多个文件参数。
  if clang-format-15 --dry-run --Werror $files; then
    echo "format: OK"
  else
    echo "format: FAILED"
    exit 1
  fi
fi
