"""CRUD 封装：tasks / categories / tags。"""
from __future__ import annotations

import sqlite3
from typing import Iterable

from todolist.db.connection import get_connection
from todolist.models.category import Category
from todolist.models.tag import Tag
from todolist.models.task import Task


# ---------------- Tasks ----------------

def create_task(task: Task) -> int:
    conn = get_connection()
    cur = conn.execute(
        """
        INSERT INTO tasks
          (title, description, status, priority, category_id,
           due_at, remind_at, sort_order)
        VALUES (?, ?, ?, ?, ?, ?, ?, ?)
        """,
        (
            task.title,
            task.description,
            task.status,
            task.priority,
            task.category_id,
            task.due_at,
            task.remind_at,
            task.sort_order,
        ),
    )
    new_id = cur.lastrowid
    if task.tag_ids:
        _replace_task_tags(conn, new_id, task.tag_ids)
    conn.commit()
    return new_id


def update_task(task: Task) -> None:
    if task.id is None:
        raise ValueError("update_task requires task.id")
    conn = get_connection()
    conn.execute(
        """
        UPDATE tasks SET
          title = ?, description = ?, status = ?, priority = ?,
          category_id = ?, due_at = ?, remind_at = ?,
          reminded = ?, sort_order = ?, updated_at = datetime('now'),
          completed_at = ?
        WHERE id = ?
        """,
        (
            task.title,
            task.description,
            task.status,
            task.priority,
            task.category_id,
            task.due_at,
            task.remind_at,
            task.reminded,
            task.sort_order,
            task.completed_at,
            task.id,
        ),
    )
    _replace_task_tags(conn, task.id, task.tag_ids)
    conn.commit()


def delete_task(task_id: int) -> None:
    conn = get_connection()
    conn.execute("DELETE FROM tasks WHERE id = ?", (task_id,))
    conn.commit()


def get_task(task_id: int) -> Task | None:
    conn = get_connection()
    row = conn.execute("SELECT * FROM tasks WHERE id = ?", (task_id,)).fetchone()
    if row is None:
        return None
    tag_ids = _get_task_tag_ids(conn, task_id)
    return Task.from_row(row, tag_ids=tag_ids)


def list_tasks(
    *,
    status: str | None = None,
    category_id: int | None = None,
    priority: str | None = None,
    tag_id: int | None = None,
) -> list[Task]:
    """通用任务列表。None 参数会被忽略，可叠加。"""
    conn = get_connection()
    where: list[str] = []
    params: list = []
    if status is not None:
        where.append("status = ?")
        params.append(status)
    if category_id is not None:
        where.append("category_id = ?")
        params.append(category_id)
    if priority is not None:
        where.append("priority = ?")
        params.append(priority)
    if tag_id is not None:
        where.append(
            "id IN (SELECT task_id FROM task_tags WHERE tag_id = ?)"
        )
        params.append(tag_id)
    sql = "SELECT * FROM tasks"
    if where:
        sql += " WHERE " + " AND ".join(where)
    sql += " ORDER BY sort_order ASC, id DESC"
    rows = conn.execute(sql, params).fetchall()
    if not rows:
        return []
    ids = [r["id"] for r in rows]
    tag_map = _get_task_tag_ids_batch(conn, ids)
    return [Task.from_row(r, tag_ids=tag_map.get(r["id"], [])) for r in rows]


def search_tasks(keyword: str) -> list[Task]:
    conn = get_connection()
    pattern = f"%{keyword}%"
    rows = conn.execute(
        "SELECT * FROM tasks WHERE title LIKE ? OR description LIKE ? "
        "ORDER BY sort_order ASC, id DESC",
        (pattern, pattern),
    ).fetchall()
    if not rows:
        return []
    ids = [r["id"] for r in rows]
    tag_map = _get_task_tag_ids_batch(conn, ids)
    return [Task.from_row(r, tag_ids=tag_map.get(r["id"], [])) for r in rows]


def mark_reminded(task_id: int) -> None:
    conn = get_connection()
    conn.execute("UPDATE tasks SET reminded = 1 WHERE id = ?", (task_id,))
    conn.commit()


def find_due_reminders(now_iso: str) -> list[Task]:
    """返回需要提醒的任务列表（remind_at <= now AND reminded = 0 AND status != done）。"""
    conn = get_connection()
    rows = conn.execute(
        """
        SELECT * FROM tasks
        WHERE remind_at IS NOT NULL
          AND remind_at <= ?
          AND reminded = 0
          AND status != 'done'
        """,
        (now_iso,),
    ).fetchall()
    return [Task.from_row(r, tag_ids=[]) for r in rows]


def _replace_task_tags(
    conn: sqlite3.Connection, task_id: int, tag_ids: Iterable[int]
) -> None:
    conn.execute("DELETE FROM task_tags WHERE task_id = ?", (task_id,))
    for tid in tag_ids:
        conn.execute(
            "INSERT OR IGNORE INTO task_tags(task_id, tag_id) VALUES (?, ?)",
            (task_id, tid),
        )


def _get_task_tag_ids(conn: sqlite3.Connection, task_id: int) -> list[int]:
    rows = conn.execute(
        "SELECT tag_id FROM task_tags WHERE task_id = ?", (task_id,)
    ).fetchall()
    return [r["tag_id"] for r in rows]


def _get_task_tag_ids_batch(
    conn: sqlite3.Connection, task_ids: list[int]
) -> dict[int, list[int]]:
    if not task_ids:
        return {}
    placeholders = ",".join("?" for _ in task_ids)
    rows = conn.execute(
        f"SELECT task_id, tag_id FROM task_tags WHERE task_id IN ({placeholders})",
        task_ids,
    ).fetchall()
    out: dict[int, list[int]] = {tid: [] for tid in task_ids}
    for r in rows:
        out.setdefault(r["task_id"], []).append(r["tag_id"])
    return out


# ---------------- Categories ----------------

def list_categories(include_archived: bool = False) -> list[Category]:
    conn = get_connection()
    if include_archived:
        rows = conn.execute(
            "SELECT * FROM categories ORDER BY sort_order, name"
        ).fetchall()
    else:
        rows = conn.execute(
            "SELECT * FROM categories WHERE archived = 0 ORDER BY sort_order, name"
        ).fetchall()
    return [Category.from_row(r) for r in rows]


def create_category(cat: Category) -> int:
    conn = get_connection()
    cur = conn.execute(
        "INSERT INTO categories(name, color, icon, sort_order) VALUES (?, ?, ?, ?)",
        (cat.name, cat.color, cat.icon, cat.sort_order),
    )
    conn.commit()
    return cur.lastrowid


def update_category(cat: Category) -> None:
    if cat.id is None:
        raise ValueError("update_category requires cat.id")
    conn = get_connection()
    conn.execute(
        "UPDATE categories SET name=?, color=?, icon=?, sort_order=?, "
        "archived=? WHERE id=?",
        (cat.name, cat.color, cat.icon, cat.sort_order, cat.archived, cat.id),
    )
    conn.commit()


def delete_category(cat_id: int) -> None:
    conn = get_connection()
    conn.execute("DELETE FROM categories WHERE id = ?", (cat_id,))
    conn.commit()


# ---------------- Tags ----------------

def list_tags() -> list[Tag]:
    conn = get_connection()
    rows = conn.execute("SELECT * FROM tags ORDER BY name").fetchall()
    return [Tag.from_row(r) for r in rows]


def get_or_create_tag(name: str, color: str = "#6B7280") -> Tag:
    name = name.strip()
    if not name:
        raise ValueError("tag name empty")
    conn = get_connection()
    row = conn.execute("SELECT * FROM tags WHERE name = ?", (name,)).fetchone()
    if row:
        return Tag.from_row(row)
    cur = conn.execute(
        "INSERT INTO tags(name, color) VALUES (?, ?)", (name, color)
    )
    conn.commit()
    return Tag(id=cur.lastrowid, name=name, color=color)


def delete_tag(tag_id: int) -> None:
    conn = get_connection()
    conn.execute("DELETE FROM tags WHERE id = ?", (tag_id,))
    conn.commit()
