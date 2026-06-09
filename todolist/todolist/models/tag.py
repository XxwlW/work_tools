"""Tag 数据模型。"""
from __future__ import annotations

from dataclasses import dataclass
from typing import Any


@dataclass
class Tag:
    id: int | None = None
    name: str = ""
    color: str = "#6B7280"
    created_at: str | None = None

    @classmethod
    def from_row(cls, row: Any) -> "Tag":
        return cls(
            id=row["id"],
            name=row["name"],
            color=row["color"],
            created_at=row["created_at"],
        )
