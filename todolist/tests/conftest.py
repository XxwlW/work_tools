"""测试公共夹具：使用临时 DB 文件，避免污染真实数据。"""
from __future__ import annotations

import os
import tempfile
from pathlib import Path

import pytest


@pytest.fixture(autouse=True)
def isolated_db(monkeypatch):
    """每个测试都用临时目录作为 APPDATA，结束后清理。"""
    tmp = tempfile.mkdtemp(prefix="todolist-test-")
    monkeypatch.setenv("APPDATA", tmp)
    # 重置 connection 单例
    from todolist.db import connection
    connection.close_connection()
    from todolist.db.connection import get_connection
    from todolist.db.migrations import run_migrations
    run_migrations(get_connection())

    yield

    connection.close_connection()
