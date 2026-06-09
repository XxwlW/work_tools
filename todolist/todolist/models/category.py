"""Category 数据模型。"""
from __future__ import annotations

from dataclasses import dataclass
from typing import Any


@dataclass
class Category:
    id: int | None = None
    name: str = ""
    color: str = "#9CA3AF"
    icon: str | None = None
    sort_order: int = 0
    archived: int = 0
    created_at: str | None = None

    @classmethod
    def from_row(cls, row: Any) -> "Category":
        return cls(
            id=row["id"],
            name=row["name"],
            color=row["color"],
            icon=row["icon"],
            sort_order=row["sort_order"],
            archived=row["archived"],
            created_at=row["created_at"],
        )
