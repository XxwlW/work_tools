"""顶部搜索 + 筛选/排序条。"""
from __future__ import annotations

import customtkinter as ctk

from todolist.config import PRIORITY_LEVELS, STATUSES
from todolist.models.category import Category
from todolist.ui import theme


STATUS_LABELS = {
    "pending": "待办",
    "in_progress": "进行中",
    "done": "已完成",
    "cancelled": "已取消",
}


class FilterBar(ctk.CTkFrame):
    def __init__(self, master, *, categories: list[Category], on_change):
        super().__init__(master, fg_color="transparent")
        self._on_change = on_change
        self._categories = categories
        t = theme.get()

        # 搜索框
        self._search_var = ctk.StringVar()
        self._search_var.trace_add("write", lambda *_: self._emit())
        self._search_entry = ctk.CTkEntry(
            self, textvariable=self._search_var,
            placeholder_text="🔍  搜索标题/备注", width=260
        )
        self._search_entry.pack(side="left", padx=(0, 8))

        # 状态多选
        ctk.CTkLabel(self, text="状态", text_color=t["fg_secondary"]).pack(
            side="left", padx=(8, 4))
        self._status_var = ctk.StringVar(value="待办,进行中")
        self._status_menu = ctk.CTkOptionMenu(
            self, values=["全部", "待办", "待办+进行中", "已完成", "已取消"],
            variable=self._status_var, width=120, command=lambda _v: self._emit()
        )
        self._status_menu.pack(side="left", padx=(0, 8))

        # 优先级多选
        ctk.CTkLabel(self, text="优先级", text_color=t["fg_secondary"]).pack(
            side="left", padx=(8, 4))
        self._priority_var = ctk.StringVar(value="全部")
        self._priority_menu = ctk.CTkOptionMenu(
            self, values=["全部"] + list(PRIORITY_LEVELS),
            variable=self._priority_var, width=80, command=lambda _v: self._emit()
        )
        self._priority_menu.pack(side="left", padx=(0, 8))

        # 排序
        ctk.CTkLabel(self, text="排序", text_color=t["fg_secondary"]).pack(
            side="left", padx=(8, 4))
        self._sort_var = ctk.StringVar(value="创建时间↓")
        self._sort_menu = ctk.CTkOptionMenu(
            self, values=["创建时间↓", "创建时间↑", "截止日期↓", "截止日期↑",
                          "优先级↓", "标题↓"],
            variable=self._sort_var, width=110, command=lambda _v: self._emit()
        )
        self._sort_menu.pack(side="left", padx=(0, 8))

        # 主题切换
        self._theme_btn = ctk.CTkButton(
            self, text="🌗", width=36, height=28, fg_color="transparent",
            hover_color=t["bg_hover"], text_color=t["fg"],
            command=self._toggle_theme
        )
        self._theme_btn.pack(side="right", padx=(8, 0))

    def _toggle_theme(self) -> None:
        from todolist.ui import theme as _t
        if _t.is_dark():
            _t.set_mode("light")
        else:
            _t.set_mode("dark")
        self._on_change()  # 触发主窗重渲染

    def _emit(self) -> None:
        kw = self._search_var.get()
        st = self._status_var.get()
        pri = self._priority_var.get()
        sort_label = self._sort_var.get()
        self._on_change({
            "keyword": kw,
            "status_label": st,
            "priority": pri,
            "sort_label": sort_label,
        })

    def current(self) -> dict:
        return {
            "keyword": self._search_var.get(),
            "status_label": self._status_var.get(),
            "priority": self._priority_var.get(),
            "sort_label": self._sort_var.get(),
        }

    def set_keyword(self, kw: str) -> None:
        self._search_var.set(kw)
