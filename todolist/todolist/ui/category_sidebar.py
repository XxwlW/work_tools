"""左侧分类侧栏：列表 + 新建/编辑/归档/删除。"""
from __future__ import annotations

import customtkinter as ctk

from todolist.models.category import Category
from todolist.services import category_service
from todolist.ui import theme


class CategorySidebar(ctk.CTkFrame):
    def __init__(self, master, *, on_change):
        super().__init__(master, width=200, corner_radius=0,
                         fg_color=theme.get()["bg_secondary"])
        self._on_change = on_change
        self._selected_id: int | None = None  # None=全部
        self._render()

    def _render(self) -> None:
        for w in self.winfo_children():
            w.destroy()
        t = theme.get()

        # 顶部
        header = ctk.CTkFrame(self, fg_color="transparent")
        header.pack(fill="x", padx=8, pady=(12, 4))
        ctk.CTkLabel(header, text="📂 分类", anchor="w",
                     font=ctk.CTkFont(size=13, weight="bold"),
                     text_color=t["fg"]).pack(side="left")
        ctk.CTkButton(
            header, text="+", width=24, height=24,
            fg_color="transparent", hover_color=t["bg_hover"],
            text_color=t["fg_secondary"], command=lambda: self._open_form(),
        ).pack(side="right")

        # "全部" 项
        self._render_item(None, "全部", t["accent"], count=None)
        ctk.CTkFrame(self, height=1, fg_color=t["border"]).pack(fill="x", padx=8, pady=4)

        # 分类列表
        cats = category_service.list_all(include_archived=False)
        for c in cats:
            self._render_item(c.id, c.name, c.color, count=None)

        ctk.CTkFrame(self, height=1, fg_color=t["border"]).pack(fill="x", padx=8, pady=8)

        # 已归档
        archived = category_service.list_all(include_archived=True)
        archived = [c for c in archived if c.archived]
        if archived:
            ctk.CTkLabel(self, text="已归档", anchor="w",
                         text_color=t["fg_muted"],
                         font=ctk.CTkFont(size=11)).pack(fill="x", padx=12, pady=(0, 4))
            for c in archived:
                self._render_item(c.id, c.name, c.color, count=None, archived=True)

    def _render_item(self, cat_id: int | None, name: str, color: str,
                     count: int | None, archived: bool = False) -> None:
        t = theme.get()
        is_selected = cat_id == self._selected_id
        bg = t["bg_hover"] if is_selected else "transparent"

        row = ctk.CTkFrame(self, fg_color=bg, corner_radius=6)
        row.pack(fill="x", padx=6, pady=1)

        ctk.CTkFrame(row, width=6, height=18, corner_radius=3,
                     fg_color=color if not archived else t["fg_muted"]).pack(
            side="left", padx=(8, 6), pady=8)

        label = ctk.CTkLabel(row, text=name, anchor="w",
                             text_color=t["fg"] if not archived else t["fg_muted"],
                             font=ctk.CTkFont(size=12))
        label.pack(side="left", fill="x", expand=True, pady=6)

        if not archived and cat_id is not None:
            ctk.CTkButton(
                row, text="✎", width=22, height=22,
                fg_color="transparent", hover_color=t["bg_hover"],
                text_color=t["fg_secondary"],
                command=lambda: self._open_form(Category(id=cat_id, name=name, color=color)),
            ).pack(side="right", padx=2)
        elif archived and cat_id is not None:
            ctk.CTkButton(
                row, text="↺", width=22, height=22,  # 恢复
                fg_color="transparent", hover_color=t["bg_hover"],
                text_color=t["accent"],
                command=lambda cid=cat_id: self._restore(cid),
            ).pack(side="right", padx=2)

        # 选中
        for w in (row, label):
            w.bind("<Button-1>", lambda _e, cid=cat_id: self._select(cid))

    def _select(self, cat_id: int | None) -> None:
        self._selected_id = cat_id
        self._render()
        self._on_change(cat_id)

    def _open_form(self, existing: Category | None = None) -> None:
        CategoryDialog(self, existing=existing, on_saved=self._on_saved)

    def _on_saved(self) -> None:
        self._on_change(self._selected_id)
        self._render()

    def _restore(self, cat_id: int) -> None:
        cat = next((c for c in category_service.list_all(include_archived=True)
                    if c.id == cat_id), None)
        if cat is None:
            return
        cat.archived = 0
        category_service.edit(cat)
        self._on_saved()

    def selected_id(self) -> int | None:
        return self._selected_id


class CategoryDialog(ctk.CTkToplevel):
    """新建/编辑分类的小弹窗。"""
    def __init__(self, master, *, existing: Category | None, on_saved):
        super().__init__(master)
        self.title("编辑分类" if existing else "新建分类")
        self.geometry("320x200")
        self.resizable(False, False)
        self.transient(master)
        self.grab_set()
        self._existing = existing
        self._on_saved = on_saved
        t = theme.get()

        ctk.CTkLabel(self, text="名称", anchor="w",
                     text_color=t["fg_secondary"]).pack(fill="x", padx=20, pady=(16, 2))
        self._name_var = ctk.StringVar(value=existing.name if existing else "")
        ctk.CTkEntry(self, textvariable=self._name_var).pack(fill="x", padx=20)

        ctk.CTkLabel(self, text="颜色 (HEX)", anchor="w",
                     text_color=t["fg_secondary"]).pack(fill="x", padx=20, pady=(10, 2))
        self._color_var = ctk.StringVar(value=existing.color if existing else "#9CA3AF")
        ctk.CTkEntry(self, textvariable=self._color_var).pack(fill="x", padx=20)

        self._err = ctk.CTkLabel(self, text="", text_color=t["danger"],
                                 font=ctk.CTkFont(size=12))
        self._err.pack(fill="x", padx=20, pady=(8, 0))

        btn = ctk.CTkFrame(self, fg_color="transparent")
        btn.pack(fill="x", padx=20, pady=(10, 16))
        if existing:
            ctk.CTkButton(btn, text="归档", width=70, fg_color="transparent",
                          border_width=1, text_color=t["warning"],
                          hover_color=t["bg_hover"],
                          command=self._archive).pack(side="left")
            ctk.CTkButton(btn, text="删除", width=70, fg_color="transparent",
                          border_width=1, text_color=t["danger"],
                          hover_color=t["bg_hover"],
                          command=self._delete).pack(side="left", padx=(8, 0))
        ctk.CTkButton(btn, text="取消", width=70, fg_color="transparent",
                      border_width=1, text_color=t["fg"],
                      hover_color=t["bg_hover"],
                      command=self.destroy).pack(side="right", padx=(8, 0))
        ctk.CTkButton(btn, text="保存", width=80, command=self._save).pack(side="right")

    def _save(self) -> None:
        name = self._name_var.get().strip()
        color = self._color_var.get().strip() or "#9CA3AF"
        try:
            if self._existing is None:
                category_service.add(name=name, color=color)
            else:
                self._existing.name = name
                self._existing.color = color
                category_service.edit(self._existing)
        except ValueError as e:
            self._err.configure(text=str(e))
            return
        except Exception as e:
            self._err.configure(text=f"失败：{e}")
            return
        self._on_saved()
        self.destroy()

    def _archive(self) -> None:
        if self._existing and self._existing.id is not None:
            category_service.archive(self._existing.id)
            self._on_saved()
            self.destroy()

    def _delete(self) -> None:
        if self._existing and self._existing.id is not None:
            category_service.remove(self._existing.id)
            self._on_saved()
            self.destroy()
