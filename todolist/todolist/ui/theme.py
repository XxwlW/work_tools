"""主题：浅/深两套配色，统一切换。

MVP 不做用户偏好持久化，重启回到浅色。
"""
from __future__ import annotations

import customtkinter as ctk


THEME_LIGHT = {
    "bg": "#FFFFFF",
    "bg_secondary": "#F4F4F5",
    "bg_hover": "#E4E4E7",
    "fg": "#18181B",
    "fg_secondary": "#71717A",
    "fg_muted": "#A1A1AA",
    "border": "#E4E4E7",
    "accent": "#2563EB",
    "success": "#16A34A",
    "danger": "#DC2626",
    "warning": "#D97706",
    # 优先级色条
    "p_a": "#DC2626",
    "p_b": "#D97706",
    "p_c": "#2563EB",
    "p_d": "#71717A",
}

THEME_DARK = {
    "bg": "#0A0A0A",
    "bg_secondary": "#18181B",
    "bg_hover": "#27272A",
    "fg": "#FAFAFA",
    "fg_secondary": "#A1A1AA",
    "fg_muted": "#71717A",
    "border": "#27272A",
    "accent": "#3B82F6",
    "success": "#22C55E",
    "danger": "#EF4444",
    "warning": "#F59E0B",
    "p_a": "#EF4444",
    "p_b": "#F59E0B",
    "p_c": "#3B82F6",
    "p_d": "#A1A1AA",
}


_current: dict = THEME_LIGHT


def get() -> dict:
    return _current


def set_mode(mode: str) -> dict:
    """mode: 'light' | 'dark'。返回应用后的主题字典。"""
    global _current
    if mode == "dark":
        ctk.set_appearance_mode("dark")
        _current = THEME_DARK
    else:
        ctk.set_appearance_mode("light")
        _current = THEME_LIGHT
    return _current


def is_dark() -> bool:
    return _current is THEME_DARK
