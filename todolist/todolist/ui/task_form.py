"""任务创建/编辑弹窗。

字段：标题、备注、优先级（下拉）、分类（下拉）、截止时间（文本）、提醒时间（文本，留空则不提醒）、标签（逗号分隔）。
"""
from __future__ import annotations

import customtkinter as ctk

from todolist.models.category import Category
from todolist.models.task import Task
from todolist.services import category_service, tag_service, task_service
from todolist.ui import theme
from todolist.utils.datetime import add_minutes_iso, format_for_input, parse_iso


class TaskForm(ctk.CTkToplevel):
    def __init__(self, master, *, task: Task | None = None,
                 categories: list[Category], on_submit):
        super().__init__(master)
        self.title("编辑任务" if task else "新建任务")
        self.geometry("520x540")
        self.resizable(False, False)
        self.transient(master)
        self.grab_set()

        self._task = task
        self._categories = categories
        self._on_submit = on_submit
        self._tag_ids: list[int] = []
        t = theme.get()

        # 标题
        ctk.CTkLabel(self, text="标题 *", anchor="w",
                     text_color=t["fg_secondary"]).pack(fill="x", padx=20, pady=(16, 2))
        self._title_var = ctk.StringVar(value=task.title if task else "")
        self._title_entry = ctk.CTkEntry(self, textvariable=self._title_var)
        self._title_entry.pack(fill="x", padx=20)

        # 备注
        ctk.CTkLabel(self, text="备注", anchor="w",
                     text_color=t["fg_secondary"]).pack(fill="x", padx=20, pady=(10, 2))
        self._desc = ctk.CTkTextbox(self, height=70)
        self._desc.pack(fill="x", padx=20)
        if task:
            self._desc.insert("1.0", task.description)

        # 优先级 + 分类（一行两列）
        row1 = ctk.CTkFrame(self, fg_color="transparent")
        row1.pack(fill="x", padx=20, pady=(10, 0))
        row1.grid_columnconfigure((0, 1), weight=1)

        ctk.CTkLabel(row1, text="优先级", anchor="w",
                     text_color=t["fg_secondary"]).grid(row=0, column=0, sticky="w")
        self._priority_var = ctk.StringVar(value=task.priority if task else "C")
        ctk.CTkOptionMenu(
            row1, values=["A", "B", "C", "D"],
            variable=self._priority_var, width=160
        ).grid(row=1, column=0, sticky="w", pady=(2, 0))

        ctk.CTkLabel(row1, text="分类", anchor="w",
                     text_color=t["fg_secondary"]).grid(row=0, column=1, sticky="w", padx=(12, 0))
        cat_names = ["（无）"] + [c.name for c in categories]
        self._cat_var = ctk.StringVar(value="（无）")
        if task and task.category_id:
            for c in categories:
                if c.id == task.category_id:
                    self._cat_var.set(c.name)
                    break
        ctk.CTkOptionMenu(
            row1, values=cat_names, variable=self._cat_var, width=160
        ).grid(row=1, column=1, sticky="w", padx=(12, 0), pady=(2, 0))

        # 截止时间
        ctk.CTkLabel(self, text="截止时间  (YYYY-MM-DD HH:MM，留空=无)",
                     anchor="w", text_color=t["fg_secondary"]).pack(
            fill="x", padx=20, pady=(10, 2))
        self._due_var = ctk.StringVar(
            value=format_for_input(parse_iso(task.due_at)) if task else ""
        )
        ctk.CTkEntry(self, textvariable=self._due_var,
                     placeholder_text="2026-06-15 18:00").pack(fill="x", padx=20)

        # 提醒时间
        ctk.CTkLabel(self, text="提醒时间  (YYYY-MM-DD HH:MM，留空=不提醒)",
                     anchor="w", text_color=t["fg_secondary"]).pack(
            fill="x", padx=20, pady=(10, 2))
        self._remind_var = ctk.StringVar(
            value=format_for_input(parse_iso(task.remind_at)) if task else ""
        )
        ctk.CTkEntry(self, textvariable=self._remind_var,
                     placeholder_text="2026-06-15 17:30").pack(fill="x", padx=20)

        # 标签
        ctk.CTkLabel(self, text="标签  (英文逗号分隔，留空=无)",
                     anchor="w", text_color=t["fg_secondary"]).pack(
            fill="x", padx=20, pady=(10, 2))
        self._tag_var = ctk.StringVar()
        ctk.CTkEntry(self, textvariable=self._tag_var,
                     placeholder_text="工作, 学习").pack(fill="x", padx=20)

        # 错误提示
        self._err = ctk.CTkLabel(self, text="", text_color=t["danger"],
                                 font=ctk.CTkFont(size=12))
        self._err.pack(fill="x", padx=20, pady=(8, 0))

        # 按钮
        btn_row = ctk.CTkFrame(self, fg_color="transparent")
        btn_row.pack(fill="x", padx=20, pady=(10, 16))
        ctk.CTkButton(btn_row, text="取消", width=80, fg_color="transparent",
                      border_width=1, text_color=t["fg"],
                      hover_color=t["bg_hover"],
                      command=self.destroy).pack(side="right", padx=(8, 0))
        ctk.CTkButton(btn_row, text="保存", width=100,
                      command=self._save).pack(side="right")

        # 回车保存
        self.bind("<Return>", lambda _e: self._save())
        self.bind("<Escape>", lambda _e: self.destroy())
        self._title_entry.focus_set()

    def _save(self) -> None:
        title = self._title_var.get().strip()
        description = self._desc.get("1.0", "end").strip()
        priority = self._priority_var.get()
        cat_name = self._cat_var.get()
        category_id = None
        for c in self._categories:
            if c.name == cat_name:
                category_id = c.id
                break
        due_text = self._due_var.get().strip()
        remind_text = self._remind_var.get().strip()
        due_iso = parse_iso(due_text).isoformat(sep=" ") if due_text else None
        remind_iso = parse_iso(remind_text).isoformat(sep=" ") if remind_text else None
        # 提醒时间晚于截止时间 → 自动回退到截止前 15 分钟
        if remind_iso and due_iso and parse_iso(remind_iso) > parse_iso(due_iso):
            remind_iso = add_minutes_iso(due_iso, -15)

        # 标签：解析 + 创建
        tag_ids: list[int] = []
        try:
            for raw in self._tag_var.get().split(","):
                name = raw.strip()
                if not name:
                    continue
                tag = tag_service.get_or_create(name)
                if tag.id is not None and tag.id not in tag_ids:
                    tag_ids.append(tag.id)
        except Exception as e:
            self._err.configure(text=f"标签失败：{e}")
            return

        try:
            if self._task is None:
                task_service.add_task(
                    title=title, description=description, priority=priority,
                    category_id=category_id, due_at=due_iso,
                    remind_at=remind_iso, tag_ids=tag_ids,
                )
            else:
                self._task.title = title
                self._task.description = description
                self._task.priority = priority
                self._task.category_id = category_id
                self._task.due_at = due_iso
                self._task.remind_at = remind_iso
                self._task.reminded = 0
                self._task.tag_ids = tag_ids
                task_service.edit_task(self._task)
        except ValueError as e:
            self._err.configure(text=str(e))
            return
        except Exception as e:
            self._err.configure(text=f"保存失败：{e}")
            return

        self._on_submit()
        self.destroy()
