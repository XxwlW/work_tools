"""提醒服务：守护线程定期扫描 + 弹通知。

MVP 用 tkinter.messagebox 兜底，不引入 win10toast 以减少依赖。
- 扫描频率：60 秒
- 命中条件：remind_at <= now AND reminded = 0 AND status != 'done'
- 触发动作：弹系统通知（Windows 走 PowerShell BurntToast/msg.exe 兜底）+ 标记 reminded = 1
"""
from __future__ import annotations

import subprocess
import sys
import threading
import tkinter.messagebox as messagebox
from datetime import datetime

from todolist.db import repository as repo
from todolist.models.task import Task
from todolist.utils.datetime import now_iso, parse_iso


# 60 秒扫一次
SCAN_INTERVAL = 60
# 全局守护线程单例
_thread: threading.Thread | None = None
_stop_event = threading.Event()


def _notify_windows(title: str, body: str) -> None:
    """Windows 下弹原生通知（用 PowerShell + Windows.UI.Notifications）。失败时降级到 messagebox。"""
    if sys.platform != "win32":
        # 非 Windows：直接 messagebox
        try:
            messagebox.showinfo(title, body)
        except Exception:
            pass
        return
    try:
        # 尝试用 BurntToast；缺失则降级到 msg.exe
        ps_script = (
            f"[reflection.assembly]::loadwithpartialname('System.Windows.Forms') | Out-Null;"
            f"[reflection.assembly]::loadwithpartialname('System.Drawing') | Out-Null;"
            f"$n = new-object system.windows.forms.notifyicon;"
            f"$n.icon = [System.Drawing.SystemIcons]::Information;"
            f"$n.visible = $true;"
            f"$n.showballoontip(5000, '{_escape(title)}', '{_escape(body)}', "
            f"[system.windows.forms.tooltipicon]::Info)"
        )
        subprocess.Popen(
            ["powershell", "-NoProfile", "-Command", ps_script],
            creationflags=getattr(subprocess, "CREATE_NO_WINDOW", 0),
            stdout=subprocess.DEVNULL,
            stderr=subprocess.DEVNULL,
        )
    except Exception:
        try:
            messagebox.showinfo(title, body)
        except Exception:
            pass


def _escape(s: str) -> str:
    return s.replace("'", "''")


def _scan_once() -> None:
    now = now_iso()
    try:
        due_tasks = repo.find_due_reminders(now)
    except Exception as e:
        # 静默失败，避免刷屏
        print(f"[reminder] scan error: {e}", file=sys.stderr)
        return
    for t in due_tasks:
        _fire(t)
        repo.mark_reminded(t.id)


def _fire(t: Task) -> None:
    body = f"截止：{t.due_at or '未设置'}    优先级：{t.priority}"
    _notify_windows(f"📌 待办提醒：{t.title}", body)


def _loop() -> None:
    while not _stop_event.is_set():
        try:
            _scan_once()
        except Exception as e:
            print(f"[reminder] loop error: {e}", file=sys.stderr)
        # wait_for 支持提前唤醒
        _stop_event.wait(SCAN_INTERVAL)


def start() -> None:
    """启动守护线程（幂等）。"""
    global _thread
    if _thread is not None and _thread.is_alive():
        return
    _stop_event.clear()
    _thread = threading.Thread(target=_loop, name="todolist-reminder", daemon=True)
    _thread.start()


def stop(timeout: float = 2.0) -> None:
    """停止守护线程。"""
    _stop_event.set()
    if _thread is not None:
        _thread.join(timeout=timeout)


def scan_now() -> int:
    """手动触发一次扫描，返回触发数量（测试用）。"""
    count_before = len(repo.find_due_reminders(now_iso()))
    _scan_once()
    return count_before
