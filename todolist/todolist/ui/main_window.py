"""主窗装配：侧栏 + 顶部 + 任务列表 + 状态栏。

状态：当前选中分类、筛选/排序条件、当前主题。
"""
from __future__ import annotations

import customtkinter as ctk

from todolist.config import APP_NAME, APP_VERSION
from todolist.services import category_service, task_service
from todolist.services.search_service import filter_tasks, sort_tasks
from todolist.ui import theme
from todolist.ui.category_sidebar import CategorySidebar
from todolist.ui.filter_bar import FilterBar
from todolist.ui.task_form import TaskForm
from todolist.ui.task_list import TaskList


# 状态 → label
_STATUS_LABEL_TO_KEYS = {
    "全部": None,
    "待办": {"pending"},
    "待办+进行中": {"pending", "in_progress"},
    "已完成": {"done"},
    "已取消": {"cancelled"},
}

_SORT_LABEL_TO_KEY = {
    "创建时间↓": ("created_at", True),
    "创建时间↑": ("created_at", False),
    "截止日期↓": ("due_at", True),
    "截止日期↑": ("due_at", False),
    "优先级↓": ("priority", True),
    "标题↓": ("title", True),
}


class MainWindow(ctk.CTk):
    def __init__(self):
        super().__init__()
        self.title(f"{APP_NAME}  v{APP_VERSION}")
        self.geometry("1080x680")
        self.minsize(820, 540)
        theme.set_mode("light")
        self._build_layout()
        self._build_menu()
        self.refresh()

    def _build_menu(self) -> None:
        """绑快捷键：Ctrl+N 新建、F5 刷新、Ctrl+T 切换主题。"""
        self.bind("<Control-n>", lambda _e: self._new_task())
        self.bind("<Control-N>", lambda _e: self._new_task())
        self.bind("<F5>", lambda _e: self.refresh())
        self.bind("<Control-t>", lambda _e: self._toggle_theme())
        self.bind("<Control-T>", lambda _e: self._toggle_theme())

    def _toggle_theme(self) -> None:
        if theme.is_dark():
            theme.set_mode("light")
        else:
            theme.set_mode("dark")
        self.refresh()

    def _build_layout(self) -> None:
        t = theme.get()
        self.configure(fg_color=t["bg"])

        # 左侧分类
        self._sidebar = CategorySidebar(self, on_change=lambda _id: self.refresh())
        self._sidebar.pack(side="left", fill="y")

        # 右侧主区
        main = ctk.CTkFrame(self, fg_color=t["bg"], corner_radius=0)
        main.pack(side="left", fill="both", expand=True)

        # 顶部工具栏
        toolbar = ctk.CTkFrame(main, fg_color="transparent", height=44)
        toolbar.pack(fill="x", padx=12, pady=(10, 0))
        ctk.CTkButton(
            toolbar, text="➕ 新建任务  Ctrl+N", width=140,
            command=self._new_task,
        ).pack(side="left")
        ctk.CTkLabel(toolbar, text="", width=8).pack(side="left")  # spacer

        cats = category_service.list_all(include_archived=False)
        self._filter_bar = FilterBar(main, categories=cats, on_change=lambda *_: self.refresh())
        self._filter_bar.pack(fill="x", padx=12, pady=(8, 4))

        # 任务列表
        self._list = TaskList(
            main,
            on_toggle=self._toggle,
            on_edit=self._edit,
            on_delete=self._delete,
        )
        self._list.pack(fill="both", expand=True, padx=12, pady=8)

        # 状态栏
        self._status = ctk.CTkLabel(
            self, text="", anchor="w",
            text_color=t["fg_secondary"], font=ctk.CTkFont(size=11)
        )
        self._status.pack(side="bottom", fill="x", padx=12, pady=(0, 4))

    # ---------- actions ----------
    def _new_task(self) -> None:
        cats = category_service.list_all(include_archived=False)
        TaskForm(self, task=None, categories=cats, on_submit=self.refresh)

    def _edit(self, task) -> None:
        cats = category_service.list_all(include_archived=False)
        TaskForm(self, task=task, categories=cats, on_submit=self.refresh)

    def _toggle(self, task) -> None:
        task_service.toggle_done(task.id)
        self.refresh()

    def _delete(self, task) -> None:
        task_service.remove_task(task.id)
        self.refresh()

    # ---------- refresh ----------
    def refresh(self, *_) -> None:
        t = theme.get()
        self.configure(fg_color=t["bg"])
        self._status.configure(text_color=t["fg_secondary"])

        f = self._filter_bar.current()
        status_keys = _STATUS_LABEL_TO_KEYS.get(f["status_label"], None)
        priority = f["priority"]
        priority_arg = None if priority == "全部" else priority
        sort_key, sort_desc = _SORT_LABEL_TO_KEY.get(f["sort_label"], ("created_at", True))

        tasks = task_service.list_all()
        tasks = filter_tasks(
            tasks,
            keyword=f["keyword"],
            statuses=status_keys,
            priorities={priority_arg} if priority_arg else None,
            category_id=self._sidebar.selected_id(),
        )
        tasks = sort_tasks(tasks, key=sort_key, descending=sort_desc)

        self._list.render(tasks)
        total = len(tasks)
        all_total = len(task_service.list_all())
        cat_name = "全部"
        sel = self._sidebar.selected_id()
        if sel is not None:
            for c in category_service.list_all(include_archived=True):
                if c.id == sel:
                    cat_name = c.name
                    break
        self._status.configure(text=f"显示 {total} / 全部 {all_total}   ·   分类：{cat_name}")
