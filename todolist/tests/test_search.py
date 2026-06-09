"""搜索/筛选/排序 单元测试。"""
from __future__ import annotations

from todolist.services import task_service
from todolist.services.search_service import filter_tasks, sort_tasks


def _seed():
    task_service.add_task(title="写周报", priority="A", due_at="2026-12-31 18:00")
    task_service.add_task(title="买菜", priority="C", due_at="2026-06-10 18:00")
    task_service.add_task(title="回邮件", priority="B", due_at="2026-06-15 09:00")
    task_service.add_task(title="周报：整理 issue", priority="C")  # 描述含"周报"关键词


def test_keyword_in_title():
    _seed()
    res = task_service.search("周报")
    titles = [t.title for t in res]
    assert "写周报" in titles
    assert "买菜" not in titles


def test_keyword_in_description():
    task_service.add_task(title="任务X", description="这是一份周报")
    res = task_service.search("周报")
    titles = [t.title for t in res]
    assert "任务X" in titles


def test_filter_by_status():
    tid = task_service.add_task(title="完蛋")
    task_service.toggle_done(tid)
    all_tasks = task_service.list_all()
    pending = filter_tasks(all_tasks, statuses={"pending"})
    done = filter_tasks(all_tasks, statuses={"done"})
    assert any(t.title == "完蛋" for t in done)
    assert not any(t.title == "完蛋" for t in pending)


def test_filter_by_priority():
    _seed()
    all_tasks = task_service.list_all()
    a_only = filter_tasks(all_tasks, priorities={"A"})
    assert all(t.priority == "A" for t in a_only)
    assert any(t.title == "写周报" for t in a_only)


def test_sort_by_due_at_desc():
    _seed()
    res = sort_tasks(task_service.list_all(), key="due_at", descending=True)
    # 有值的排前面
    with_value = [t for t in res if t.due_at]
    assert with_value[0].due_at.startswith("2026-12-31")


def test_sort_by_priority_desc():
    _seed()
    res = sort_tasks(task_service.list_all(), key="priority", descending=True)
    # 降序优先级 = A 最小（最高）排前
    assert res[0].priority == "A"


def test_combined_filter_and_sort():
    _seed()
    all_tasks = task_service.list_all()
    filtered = filter_tasks(all_tasks, priorities={"A", "B"})
    sorted_res = sort_tasks(filtered, key="due_at", descending=True)
    assert all(t.priority in ("A", "B") for t in sorted_res)
