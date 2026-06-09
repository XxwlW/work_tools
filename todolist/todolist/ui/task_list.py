"""任务列表容器：可滚动，按过滤/排序结果渲染。"""
from __future__ import annotations

import customtkinter as ctk

from todolist.models.task import Task
from todolist.ui import theme
from todolist.ui.task_item import TaskItem


class TaskList(ctk.CTkScrollableFrame):
    def __init__(self, master, *, on_toggle, on_edit, on_delete):
        super().__init__(master, fg_color=theme.get()["bg"], corner_radius=0)
        self._on_toggle = on_toggle
        self._on_edit = on_edit
        self._on_delete = on_delete
        self._items: list[TaskItem] = []

    def render(self, tasks: list[Task]) -> None:
        # 清旧
        for it in self._items:
            it.destroy()
        self._items.clear()

        if not tasks:
            t = theme.get()
            placeholder = ctk.CTkLabel(
                self, text="（暂无任务）", text_color=t["fg_muted"],
                font=ctk.CTkFont(size=12)
            )
            placeholder.pack(pady=40)
            return

        for t in tasks:
            item = TaskItem(
                self, task=t,
                on_toggle=self._on_toggle,
                on_edit=self._on_edit,
                on_delete=self._on_delete,
            )
            item.pack(fill="x", padx=8, pady=2)
            self._items.append(item)
