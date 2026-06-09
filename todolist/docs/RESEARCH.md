# 调研报告 — 桌面 Todolist 技术选型

> 调研时间：2026/6/9
> 目标：为个人项目选一个**轻量、单机、极简风**的桌面 todolist 方案

## 1. 选型结论

| 路线 | 结论 | 关键理由 |
|---|---|---|
| **Python + CustomTkinter** | **✅ 选定** | 极简风易实现、零外部 C 依赖、跨平台、CRUD 几行就够 |
| Tauri v2 | ★★★★★（被改用） | 桌面 todolist 事实标准，~10MB 包体、<50MB 内存 |
| Python + PySide6 | ★★★★ | Qt 生态最完整，但包体大、license 顾虑 |
| Electron | ★★★ | sleek ⭐1986 明星，但 todolist 不是主流，包体 100MB+ |
| Flutter Windows | ★★ | 桌面 todolist 生态空白 |
| Wails | ★ | 生态明显不成熟（个位数 star） |
| C++ Qt6 | ★★ | 性能强但 over-engineering |

## 2. 桌面 todolist 参考项目清单（截至 2026/6/8 star 数）

### 2.1 Tauri 系（最值得参考的生态）

| 仓库 | Star | 技术栈 | 借鉴点 |
|---|---:|---|---|
| [moyinglizi/personal-todo](https://github.com/moyinglizi/personal-todo) | 1 | Tauri v2 + Vanilla TS + SQLite (rusqlite) + 4 表 + 20+ IPC | **与本项目功能最匹配**：全局热键 + 托盘 + 提醒 + 浅深色 + i18n |
| [simcmoi/blinkdo](https://github.com/simcmoi/blinkdo) | 1 | Tauri 2.10 + React 19 + shadcn/ui + Tailwind + Zustand + Framer Motion + 本地 JSON | "Blink-fast overlay" 极简风、~10MB 包体、<50MB 内存 |
| [xtrinch/tauri-todo-sql](https://github.com/xtrinch/tauri-todo-sql) | 3 | Tauri + SQLite | SQLite 集成范本 |
| [anshulxyz/tauri-svelte-todo-app](https://github.com/anshulxyz/tauri-svelte-todo-app) | 6 | Tauri + Svelte + Rust | CRUD 范本 |
| [NotKeira/what-todo-next](https://github.com/NotKeira/what-todo-next) | 1 | Tauri + TS + SQLite | Win/Linux 跨平台 |
| [oxide-byte/todo-tauri](https://github.com/oxide-byte/todo-tauri) | 4 | Tauri + Leptos (Rust wasm) | 纯 Rust 前端尝试 |

### 2.2 Electron 系

| 仓库 | Star | 技术栈 | 借鉴点 |
|---|---:|---|---|
| [ransome1/sleek](https://github.com/ransome1/sleek) | **1986** | Electron + TypeScript | **杀手级功能来源**：todo.txt 协议 + 跨平台 + 文件监听 + 暗色 |
| [zonayedpca/DevTop](https://github.com/zonayedpca/DevTop) | 56 | Electron + JS | dev 工具集 |
| [wixiweb/googletasks-desktop](https://github.com/wixiweb/googletasks-desktop) | 23 | Electron + JS | Google Tasks 桌面版 |
| [Xmader/hydrogen](https://github.com/Xmader/hydrogen) | 22 | Electron + JS | Git Based Task，Linux/Windows |

**关键观察**：除了 sleek 是明星项目，Electron todolist 大多 star < 60，说明 Electron 在该场景已不是主流选择。

### 2.3 Flutter 系（移动端为主，桌面 todolist 几乎空白）

| 仓库 | Star | 备注 |
|---|---:|---|
| [iamEtornam/Tasky-Mobile-App](https://github.com/iamEtornam/Tasky-Mobile-App) | 291 | 移动端 + Firebase |
| [ErfanRht/Tasker](https://github.com/ErfanRht/Tasker) | 256 | 移动端 + Dart |
| [Kind-Unes/HabitNow](https://github.com/Kind-Unes/HabitNow) | 93 | 习惯/任务/重复，Hive 存储 |

**结论**：Flutter Windows 桌面 todolist 生态几乎空白。

### 2.4 Wails 系（Go + Rust + WebView）

| 仓库 | Star | 备注 |
|---|---:|---|
| [wailsapp/todo](https://github.com/wailsapp/todo) | 24 | 官方示例 |
| 其他 | < 15 | 生态明显不成熟 |

### 2.5 Rust 终端/TUI（数据模型参考）

| 仓库 | Star | 备注 |
|---|---:|---|
| [tsoding/todo-rs](https://github.com/tsoding/todo-rs) | 125 | Simple Interactive Terminal Todo App in Rust |
| [Gnarus-G/mynd](https://github.com/Gnarus-G/mynd) | 90 | "A todo app, in the terminal, or with a GUI. Simple and Frictionless." |
| [duanebester/gpui-todos](https://github.com/duanebester/gpui-todos) | 70 | GPUI (Zed 团队) |

## 3. 杀手级功能（基于 sleek 总结，业内复用度高）

| 功能 | 实现难度 | MVP |
|---|---|---|
| 优先级（A/B/C/D） | 极低 | ✅ |
| 截止日期 + 提前提醒 | 低 | ✅ |
| 上下文/标签（@tag、+tag、key:value） | 低 | ✅ |
| 分类（颜色 + 名称） | 低 | ✅ |
| 全文搜索 | 中 | ✅（LIKE 即可） |
| 排序（多字段 + 升降） | 低 | ✅ |
| 浅/深主题 | 低 | ✅ |
| 完成任务归档 | 低 | ✅（status=done 即可） |
| 重复任务 | 中 | Phase 2 |
| 子任务 | 中 | Phase 2 |
| 全局热键唤起 | 中 | Phase 2 |
| 系统托盘 + 关闭即隐藏 | 中 | Phase 2 |
| 看板视图 | 中 | Phase 2 |
| FTS5 全文搜索 | 中 | Phase 2 |
| i18n 多语言 | 中 | Phase 2 |
| Markdown 备注 | 高 | Phase 3 |

## 4. 极简风 UI 案例

- **blinkdo**（Tauri + shadcn）：圆角 + 克制配色 + 暗/亮模式自动切换
- **moyinglizi/personal-todo**：左侧彩色色条 + Things/TickTick 风格
- **sleek**（Electron）：todo.txt 文本驱动 + 列表风
- **Gnarus-G/mynd**：纯色 + 极简

本项目参考 blinkdo 的"克制配色 + 暗/亮模式"思想，色条方案参考 moyinglizi。

## 5. 数据模型参考

参考 moyinglizi/personal-todo 的 4 表范式 + sleek 的 todo.txt 字段：

### tasks
- 标题、备注、状态（pending/in_progress/done/cancelled）
- 优先级（A/B/C/D）
- 分类（外键）
- 截止日期、提醒时间、是否已提醒
- 重复规则（Phase 2）
- 子任务（Phase 2）
- 创建/更新/完成时间

### categories
- 名称、颜色、图标、排序、归档标记

### tags
- 名称、颜色

### task_tags（多对多）

## 6. 决策记录

| 日期 | 决策 | 理由 |
|---|---|---|
| 2026/6/9 | 改用 Python + CustomTkinter（弃 Tauri） | 用户选择 Python/C++ 路线后选 CustomTkinter 平衡开发速度与极简风 |
| 2026/6/9 | MVP 不含全局热键/托盘/打包 | 用户明确"只做 MVP 最小可用" |
| 2026/6/9 | 提醒用守护线程而非 win10toast | 减少依赖 + PowerShell NotifyIcon 兜底 |
| 2026/6/9 | DB 放 `%APPDATA%/Todolist/` | Windows 标准数据目录，避免项目目录被误删 |

## 7. 风险点

1. **CustomTkinter 高 DPI 缩放**：在 Win11 高 DPI 屏可能糊。已设置 `set_widget_scaling(1.0)` + `set_window_scaling(1.0)` 兜底。
2. **PowerShell 通知在 Win11 偶尔失效**：失败时降级到 `tkinter.messagebox`。
3. **DB 锁**：单连接 + `check_same_thread=False` + daemon 线程保护，无并发问题。
4. **中文路径**：Windows 10+ 默认 UTF-8，无虞。
