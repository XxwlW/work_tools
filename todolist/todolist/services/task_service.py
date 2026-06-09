"""Task 业务逻辑：薄包装，主要做校验和组合操作。"""
from __future__ import annotations

from datetime import datetime

from todolist.config import PRIORITY_LEVELS, STATUSES
from todolist.db import repository as repo
from todolist.models.task import Task
from todolist.utils.datetime import now_iso, parse_iso


def _validate_title(title: str) -> None:
    title = (title or "").strip()
    if not title:
        raise ValueError("任务标题不能为空")
    if len(title) > 500:
        raise ValueError("任务标题过长（>500 字符）")


def _validate_priority(priority: str) -> None:
    if priority not in PRIORITY_LEVELS:
        raise ValueError(f"非法优先级：{priority}")


def _validate_status(status: str) -> None:
    if status not in STATUSES:
        raise ValueError(f"非法状态：{status}")


def _validate_due(due_at: str | None) -> None:
    if not due_at:
        return
    if parse_iso(due_at) is None:
        raise ValueError(f"无法解析截止日期：{due_at}")


def _validate_remind(remind_at: str | None) -> None:
    if not remind_at:
        return
    if parse_iso(remind_at) is None:
        raise ValueError(f"无法解析提醒时间：{remind_at}")


def add_task(
    *,
    title: str,
    description: str = "",
    priority: str = "C",
    category_id: int | None = None,
    due_at: str | None = None,
    remind_at: str | None = None,
    tag_ids: list[int] | None = None,
) -> int:
    _validate_title(title)
    _validate_priority(priority)
    _validate_due(due_at)
    _validate_remind(remind_at)
    task = Task(
        title=title.strip(),
        description=description,
        priority=priority,
        category_id=category_id,
        due_at=due_at,
        remind_at=remind_at,
        reminded=0,
        tag_ids=tag_ids or [],
    )
    return repo.create_task(task)


def edit_task(task: Task) -> None:
    _validate_title(task.title)
    _validate_priority(task.priority)
    _validate_status(task.status)
    _validate_due(task.due_at)
    _validate_remind(task.remind_at)
    if task.status == "done" and not task.completed_at:
        task.completed_at = now_iso()
    if task.status != "done":
        task.completed_at = None
    repo.update_task(task)


def toggle_done(task_id: int) -> None:
    task = repo.get_task(task_id)
    if task is None:
        return
    if task.status == "done":
        task.status = "pending"
        task.completed_at = None
    else:
        task.status = "done"
        task.completed_at = now_iso()
    repo.update_task(task)


def remove_task(task_id: int) -> None:
    repo.delete_task(task_id)


def get(task_id: int) -> Task | None:
    return repo.get_task(task_id)


def list_all() -> list[Task]:
    return repo.list_tasks()


def list_by_status(status: str) -> list[Task]:
    return repo.list_tasks(status=status)


def list_by_category(category_id: int) -> list[Task]:
    return repo.list_tasks(category_id=category_id)


def search(keyword: str) -> list[Task]:
    keyword = (keyword or "").strip()
    if not keyword:
        return list_all()
    return repo.search_tasks(keyword)
