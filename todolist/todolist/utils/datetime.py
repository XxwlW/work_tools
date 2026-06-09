"""datetime 工具：统一使用 ISO8601 字符串存储与解析。"""
from __future__ import annotations

from datetime import datetime, timedelta


def now_iso() -> str:
    """当前本地时间 → ISO8601 字符串（无时区后缀）。"""
    return datetime.now().replace(microsecond=0).isoformat(sep=" ")


def parse_iso(value: str | None) -> datetime | None:
    """支持 'YYYY-MM-DD HH:MM:SS' / 'YYYY-MM-DDTHH:MM:SS' / 'YYYY-MM-DD HH:MM'。"""
    if not value:
        return None
    value = value.strip()
    for fmt in ("%Y-%m-%d %H:%M:%S", "%Y-%m-%dT%H:%M:%S", "%Y-%m-%d %H:%M", "%Y-%m-%dT%H:%M", "%Y-%m-%d"):
        try:
            return datetime.strptime(value, fmt)
        except ValueError:
            continue
    try:
        return datetime.fromisoformat(value)
    except ValueError:
        return None


def format_for_input(dt: datetime | None) -> str:
    """CTkEntry 显示用：'YYYY-MM-DD HH:MM'。"""
    if dt is None:
        return ""
    return dt.strftime("%Y-%m-%d %H:%M")


def add_minutes_iso(value: str | None, minutes: int) -> str | None:
    """将 ISO8601 字符串加 N 分钟，返回新的 ISO8601 字符串。"""
    dt = parse_iso(value)
    if dt is None:
        return None
    return (dt + timedelta(minutes=minutes)).replace(microsecond=0).isoformat(sep=" ")


def humanize_due(due: str | None, now: datetime | None = None) -> str:
    """把截止日期转成人类友好文案：'今天 18:00' / '明天 09:00' / '2026/7/1 18:00' / '已逾期 2 天'。"""
    if not due:
        return ""
    dt = parse_iso(due)
    if dt is None:
        return due
    now = now or datetime.now()
    delta_days = (dt.date() - now.date()).days
    if delta_days < 0:
        return f"已逾期 {-delta_days} 天"
    if delta_days == 0:
        return f"今天 {dt.strftime('%H:%M')}"
    if delta_days == 1:
        return f"明天 {dt.strftime('%H:%M')}"
    if delta_days < 7:
        weekdays = ["一", "二", "三", "四", "五", "六", "日"]
        return f"周{weekdays[dt.weekday()]} {dt.strftime('%H:%M')}"
    return dt.strftime("%Y/%m/%d %H:%M")
