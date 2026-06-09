"""Task 数据模型。"""
from __future__ import annotations

from dataclasses import dataclass, field
from datetime import datetime
from typing import Any


@dataclass
class Task:
    id: int | None = None
    title: str = ""
    description: str = ""
    status: str = "pending"            # pending/in_progress/done/cancelled
    priority: str = "C"                # A/B/C/D
    category_id: int | None = None
    due_at: str | None = None          # ISO8601
    remind_at: str | None = None       # ISO8601
    reminded: int = 0
    sort_order: int = 0
    created_at: str | None = None
    updated_at: str | None = None
    completed_at: str | None = None
    tag_ids: list[int] = field(default_factory=list)

    def to_dict(self) -> dict[str, Any]:
        return {
            "id": self.id,
            "title": self.title,
            "description": self.description,
            "status": self.status,
            "priority": self.priority,
            "category_id": self.category_id,
            "due_at": self.due_at,
            "remind_at": self.remind_at,
            "reminded": self.reminded,
            "sort_order": self.sort_order,
            "created_at": self.created_at,
            "updated_at": self.updated_at,
            "completed_at": self.completed_at,
            "tag_ids": list(self.tag_ids),
        }

    @classmethod
    def from_row(cls, row: Any, tag_ids: list[int] | None = None) -> "Task":
        return cls(
            id=row["id"],
            title=row["title"],
            description=row["description"],
            status=row["status"],
            priority=row["priority"],
            category_id=row["category_id"],
            due_at=row["due_at"],
            remind_at=row["remind_at"],
            reminded=row["reminded"],
            sort_order=row["sort_order"],
            created_at=row["created_at"],
            updated_at=row["updated_at"],
            completed_at=row["completed_at"],
            tag_ids=tag_ids or [],
        )

    def is_done(self) -> bool:
        return self.status == "done"

    def is_overdue(self, now: datetime | None = None) -> bool:
        if not self.due_at or self.is_done():
            return False
        try:
            due = datetime.fromisoformat(self.due_at)
        except ValueError:
            return False
        return due < (now or datetime.now())
