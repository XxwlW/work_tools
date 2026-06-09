"""Tag 业务逻辑。"""
from __future__ import annotations

from todolist.db import repository as repo
from todolist.models.tag import Tag


def list_all() -> list[Tag]:
    return repo.list_tags()


def get_or_create(name: str, color: str = "#6B7280") -> Tag:
    name = (name or "").strip()
    if not name:
        raise ValueError("标签名不能为空")
    return repo.get_or_create_tag(name, color)


def remove(tag_id: int) -> None:
    repo.delete_tag(tag_id)
