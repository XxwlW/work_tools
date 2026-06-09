"""数据库连接管理。

单例 sqlite3.Connection，线程安全（check_same_thread=False + 自维护锁）。
"""
from __future__ import annotations

import sqlite3
import threading
from pathlib import Path

from todolist.config import get_db_path


_lock = threading.Lock()
_conn: sqlite3.Connection | None = None


def get_connection() -> sqlite3.Connection:
    """返回全局数据库连接；首次调用时创建文件目录。"""
    global _conn
    if _conn is not None:
        return _conn
    with _lock:
        if _conn is not None:
            return _conn
        path = get_db_path()
        path.parent.mkdir(parents=True, exist_ok=True)
        conn = sqlite3.connect(
            path,
            check_same_thread=False,
            detect_types=sqlite3.PARSE_DECLTYPES,
        )
        conn.row_factory = sqlite3.Row
        conn.execute("PRAGMA foreign_keys = ON")
        conn.execute("PRAGMA journal_mode = WAL")
        _conn = conn
        return conn


def close_connection() -> None:
    """关闭并清空全局连接（测试用）。"""
    global _conn
    with _lock:
        if _conn is not None:
            _conn.close()
            _conn = None
