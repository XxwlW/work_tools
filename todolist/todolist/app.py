"""应用入口：初始化 DB、migration、提醒线程，再启动主窗。"""
from __future__ import annotations

import customtkinter as ctk

from todolist.config import APP_NAME
from todolist.db.connection import get_connection
from todolist.db.migrations import run_migrations
from todolist.services import reminder_service
from todolist.ui.main_window import MainWindow
from todolist.ui.theme import set_mode


def run() -> None:
    ctk.set_appearance_mode("light")
    ctk.set_default_color_theme("blue")
    ctk.set_widget_scaling(1.0)
    ctk.set_window_scaling(1.0)

    # 初始化 DB（连接 + 建表）
    conn = get_connection()
    run_migrations(conn)

    # 启动提醒守护线程
    reminder_service.start()

    # 启动主窗
    app = MainWindow()
    app.mainloop()

    # 退出时停提醒线程
    reminder_service.stop()
