#!/usr/bin/env bash
# install-skills.sh — 一键导入 Claude Code skills
# 用法: bash install-skills.sh [--target DIR] [--global]
#   --target DIR  指定目标目录（默认: ~/.claude/skills）
#   --global      复制到全局 skills 目录
#   --project DIR 复制到项目级 .claude/skills

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
TARGET_DIR="$HOME/.claude/skills"
PROJECT_DIR=""
GLOBAL=false

while [[ $# -gt 0 ]]; do
  case "$1" in
    --target)   TARGET_DIR="$2"; shift 2 ;;
    --global)   GLOBAL=true; shift ;;
    --project)  PROJECT_DIR="$2"; shift 2 ;;
    -h|--help)
      echo "用法: bash install-skills.sh [--target DIR] [--global] [--project DIR]"
      exit 0
      ;;
    *) echo "未知参数: $1"; exit 1 ;;
  esac
done

# 探测源目录
SRC_GLOBAL="$SCRIPT_DIR/global-skills"
SRC_LOCAL="$SCRIPT_DIR/guizang-social-card-skill"

if [[ ! -d "$SRC_GLOBAL" ]]; then
  echo "❌ 错误: 找不到 global-skills 目录: $SRC_GLOBAL"
  exit 1
fi

# 创建目标目录
mkdir -p "$TARGET_DIR"

# 复制 global-skills 下的所有 skill
echo "📦 正在导入 global-skills 到: $TARGET_DIR"
count=0
for skill_dir in "$SRC_GLOBAL"/*/; do
  skill_name=$(basename "$skill_dir")
  dest="$TARGET_DIR/$skill_name"
  if [[ -d "$dest" ]]; then
    echo "  ⚠️  已存在: $skill_name（跳过；如需覆盖请先删除）"
  else
    cp -r "$skill_dir" "$dest"
    echo "  ✅ 安装: $skill_name"
    count=$((count + 1))
  fi
done

# 复制本地 skill（guizang-social-card-skill）
if [[ -d "$SRC_LOCAL" ]]; then
  dest="$TARGET_DIR/guizang-social-card-skill"
  if [[ -d "$dest" ]]; then
    echo "  ⚠️  已存在: guizang-social-card-skill（跳过）"
  else
    cp -r "$SRC_LOCAL" "$dest"
    echo "  ✅ 安装: guizang-social-card-skill"
    count=$((count + 1))
  fi
fi

echo ""
echo "✨ 完成！共安装 $count 个 skill。"
echo "📂 目标位置: $TARGET_DIR"
echo ""
echo "重启 Claude Code 让 skills 生效。"
