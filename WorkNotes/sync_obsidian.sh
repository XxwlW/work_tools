#!/bin/bash

# --- 核心配置 ---
LOCAL_DIR="/media/sf_ShareFile/Wxj/5_Note/WorkNotes/"
REMOTE_DIR="gdrive:WorkNotes"
PROXY="http://127.0.0.1:7890"

# 设置代理
export http_proxy=$PROXY
export https_proxy=$PROXY

echo "=========================================="
echo "  Rclone 双向同步助手 (支持同步删除) - $(date)"
echo "=========================================="

# 检查是否存在同步缓存/数据库
# 如果没有数据库，或者你希望强制以本地为准进行一次大清理，可以手动运行带 --resync 的命令
if [ ! -d "$HOME/.cache/rclone/bisync" ]; then
    echo "[FIRST RUN] 检测到首次运行或缓存丢失，正在执行初始化同步..."
    # --resync 会确保云端和本地完全一致（本地没有的，云端必删）
    rclone bisync "$LOCAL_DIR" "$REMOTE_DIR" \
        --resync \
        --verbose \
        --conflict-resolve newer \
        --delete-header \
        --exclude ".obsidian/workspace.json" \
        --exclude ".obsidian/cache/**"
else
    echo "[RUN] 执行增量同步（包含删除同步）..."
    # --force 参数允许 rclone 执行删除操作
    # --conflict-resolve newer 确保在冲突时以较新的文件为准
    rclone bisync "$LOCAL_DIR" "$REMOTE_DIR" \
        --verbose \
        --conflict-resolve newer \
        --force \
        --exclude ".obsidian/workspace.json" \
        --exclude ".obsidian/cache/**"
fi

# 检查执行结果
if [ $? -eq 0 ]; then
    echo "------------------------------------------"
    echo "✅ 同步成功！本地的删除已同步到云端。"
else
    echo "------------------------------------------"
    echo "❌ 同步遇到冲突或失败！"
    echo "建议：如果删除未生效，请手动执行一次：rclone bisync $LOCAL_DIR $REMOTE_DIR --resync"
fi

echo "=========================================="
read -p "按回车键退出..."