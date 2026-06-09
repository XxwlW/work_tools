"""搜索/筛选/排序 — 纯函数，UI 层调用。"""
from __future__ import annotations

from typing import Iterable, Literal

from todolist.models.task import Task


SortKey = Literal["due_at", "priority", "created_at", "title"]


# 越大 = 越重要：降序时 A 排前
_PRIORITY_ORDER = {"A": 4, "B": 3, "C": 2, "D": 1}


def filter_tasks(
    tasks: Iterable[Task],
    *,
    keyword: str = "",
    statuses: set[str] | None = None,
    priorities: set[str] | None = None,
    category_id: int | None = None,
    tag_id: int | None = None,
    only_pending: bool = False,
) -> list[Task]:
    """通用过滤：所有条件都满足才返回。"""
    keyword = (keyword or "").strip().lower()
    out: list[Task] = []
    for t in tasks:
        if keyword and keyword not in t.title.lower() and keyword not in t.description.lower():
            continue
        if only_pending and t.status == "done":
            continue
        if statuses and t.status not in statuses:
            continue
        if priorities and t.priority not in priorities:
            continue
        if category_id is not None and t.category_id != category_id:
            continue
        if tag_id is not None and tag_id not in t.tag_ids:
            continue
        out.append(t)
    return out


def sort_tasks(
    tasks: Iterable[Task],
    *,
    key: SortKey = "created_at",
    descending: bool = False,
) -> list[Task]:
    """稳定排序；空值排到末尾（不论升降）。"""
    items = list(tasks)

    def sort_value(t: Task):
        v = getattr(t, key, None)
        if key == "priority":
            return _PRIORITY_ORDER.get(v or "C", 99)
        return v or ""

    items.sort(key=sort_value, reverse=descending)

    # 把空值强制排到末尾
    def has_value(t: Task) -> bool:
        v = getattr(t, key, None)
        if key == "priority":
            return bool(v)
        return bool(v)

    with_value = [t for t in items if has_value(t)]
    without_value = [t for t in items if not has_value(t)]
    if descending:
        return with_value + without_value
    return with_value + without_value
