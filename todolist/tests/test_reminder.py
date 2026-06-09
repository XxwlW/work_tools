"""提醒逻辑单元测试（不触发真实通知）。"""
from __future__ import annotations

import sys
from datetime import datetime, timedelta
from unittest.mock import patch

from todolist.services import task_service, reminder_service


def test_find_due_reminders_picks_past():
    """过去时间 + 未提醒 + 未完成 → 应被命中。"""
    past = (datetime.now() - timedelta(minutes=5)).replace(microsecond=0)
    past_iso = past.isoformat(sep=" ")
    task_service.add_task(title="过期任务", remind_at=past_iso)
    hits = task_service.list_all()
    # 直接调 repository 验证
    from todolist.db import repository as repo
    from todolist.utils.datetime import now_iso
    found = repo.find_due_reminders(now_iso())
    assert any(t.title == "过期任务" for t in found)


def test_done_task_not_picked():
    past = (datetime.now() - timedelta(minutes=5)).replace(microsecond=0)
    past_iso = past.isoformat(sep=" ")
    tid = task_service.add_task(title="已完成", remind_at=past_iso)
    task_service.toggle_done(tid)
    from todolist.db import repository as repo
    from todolist.utils.datetime import now_iso
    found = repo.find_due_reminders(now_iso())
    assert all(t.id != tid for t in found)


def test_future_reminder_not_picked():
    future = (datetime.now() + timedelta(hours=2)).replace(microsecond=0)
    future_iso = future.isoformat(sep=" ")
    task_service.add_task(title="未来", remind_at=future_iso)
    from todolist.db import repository as repo
    from todolist.utils.datetime import now_iso
    found = repo.find_due_reminders(now_iso())
    assert all(t.title != "未来" for t in found)


def test_scan_now_marks_reminded():
    past = (datetime.now() - timedelta(minutes=5)).replace(microsecond=0)
    past_iso = past.isoformat(sep=" ")
    tid = task_service.add_task(title="再触发一次", remind_at=past_iso)

    # 屏蔽真实通知
    with patch.object(reminder_service, "_notify_windows"):
        fired = reminder_service.scan_now()
    assert fired == 1
    # 第二次扫不到
    with patch.object(reminder_service, "_notify_windows"):
        fired2 = reminder_service.scan_now()
    assert fired2 == 0
