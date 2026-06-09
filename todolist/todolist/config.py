"""全局配置：路径、常量。"""
from __future__ import annotations

import os
from pathlib import Path


# 应用元信息
APP_NAME = "Todolist"
APP_VERSION = "0.1.0"

# Windows 下 DB 存放位置：%APPDATA%/Todolist/todolist.db
# 其他平台：~/.local/share/Todolist/todolist.db
def get_data_dir() -> Path:
    if os.name == "nt":
        base = os.environ.get("APPDATA") or str(Path.home() / "AppData" / "Roaming")
        return Path(base) / "Todolist"
    return Path.home() / ".local" / "share" / "Todolist"


def get_db_path() -> Path:
    return get_data_dir() / "todolist.db"


# 业务常量
PRIORITY_LEVELS = ("A", "B", "C", "D")  # A 最高
STATUSES = ("pending", "in_progress", "done", "cancelled")
