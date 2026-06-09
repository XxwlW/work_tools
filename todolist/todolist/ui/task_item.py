"""单条任务 UI：勾选框、优先级色条、标题、截止、删除按钮。"""
from __future__ import annotations

import customtkinter as ctk

from todolist.models.task import Task
from todolist.ui import theme
from todolist.utils.datetime import humanize_due


PRIORITY_COLOR_KEY = {"A": "p_a", "B": "p_b", "C": "p_c", "D": "p_d"}


class TaskItem(ctk.CTkFrame):
    def __init__(self, master, *, task: Task, on_toggle, on_edit, on_delete):
        super().__init__(master, fg_color="transparent", corner_radius=6)
        self.task = task
        self._on_toggle = on_toggle
        self._on_edit = on_edit
        self._on_delete = on_delete
        t = theme.get()

        # 左侧色条（按优先级）
        self._bar = ctk.CTkFrame(self, width=4, corner_radius=2,
                                 fg_color=t[PRIORITY_COLOR_KEY[task.priority]])
        self._bar.pack(side="left", fill="y", padx=(0, 8), pady=4)

        # 勾选
        self._check = ctk.CTkCheckBox(
            self, text="", width=22, command=self._handle_toggle
        )
        if task.status == "done":
            self._check.select()
        self._check.pack(side="left", padx=(4, 8), pady=8)

        # 中间：标题 + 副信息
        mid = ctk.CTkFrame(self, fg_color="transparent")
        mid.pack(side="left", fill="x", expand=True, pady=6)

        title_font = ctk.CTkFont(size=13, weight="normal",
                                 overstrike=task.status == "done")
        self._title = ctk.CTkLabel(
            mid, text=task.title, anchor="w",
            font=title_font, text_color=t["fg"]
        )
        self._title.pack(fill="x")

        sub = []
        if task.due_at:
            sub.append(f"⏰ {humanize_due(task.due_at)}")
        if task.description:
            sub.append(task.description[:60] + ("…" if len(task.description) > 60 else ""))
        if sub:
            self._sub = ctk.CTkLabel(
                mid, text="  ·  ".join(sub), anchor="w",
                font=ctk.CTkFont(size=11),
                text_color=t["fg_secondary"]
            )
            self._sub.pack(fill="x", pady=(2, 0))

        # 右侧：编辑 / 删除
        right = ctk.CTkFrame(self, fg_color="transparent")
        right.pack(side="right", padx=4)
        self._edit_btn = ctk.CTkButton(
            right, text="✎", width=28, height=24,
            fg_color="transparent", hover_color=t["bg_hover"],
            text_color=t["fg_secondary"], command=self._handle_edit
        )
        self._edit_btn.pack(side="left", padx=2)
        self._del_btn = ctk.CTkButton(
            right, text="✕", width=28, height=24,
            fg_color="transparent", hover_color=t["bg_hover"],
            text_color=t["danger"], command=self._handle_delete
        )
        self._del_btn.pack(side="left", padx=2)

        # 点击整行触发编辑
        for w in (self, self._title, mid):
            w.bind("<Double-Button-1>", lambda _e: self._handle_edit())

    def _handle_toggle(self) -> None:
        self._on_toggle(self.task)

    def _handle_edit(self) -> None:
        self._on_edit(self.task)

    def _handle_delete(self) -> None:
        self._on_delete(self.task)
