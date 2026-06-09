"""Category 业务逻辑。"""
from __future__ import annotations

from todolist.db import repository as repo
from todolist.models.category import Category


def _validate(name: str) -> None:
    name = (name or "").strip()
    if not name:
        raise ValueError("分类名不能为空")
    if len(name) > 50:
        raise ValueError("分类名过长（>50 字符）")


def list_all(include_archived: bool = False) -> list[Category]:
    return repo.list_categories(include_archived=include_archived)


def add(name: str, color: str = "#9CA3AF", icon: str | None = None) -> int:
    _validate(name)
    return repo.create_category(Category(name=name.strip(), color=color, icon=icon))


def edit(cat: Category) -> None:
    _validate(cat.name)
    repo.update_category(cat)


def archive(cat_id: int) -> None:
    cat = next((c for c in repo.list_categories(include_archived=True) if c.id == cat_id), None)
    if cat is None:
        return
    cat.archived = 1
    repo.update_category(cat)


def remove(cat_id: int) -> None:
    repo.delete_category(cat_id)
