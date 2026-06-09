# 架构说明

## 1. 分层

```
┌──────────────────────────────────────────────────────┐
│ UI 层 (todolist/ui/)                                 │
│   - main_window / task_list / task_item / task_form  │
│   - category_sidebar / filter_bar                    │
│   - theme                                            │
│   ↳ 不直接访问 DB                                    │
└──────────────────────────────────────────────────────┘
                      ↓ 调用
┌──────────────────────────────────────────────────────┐
│ Services 层 (todolist/services/)                     │
│   - task_service / category_service / tag_service    │
│   - search_service  (filter / sort)                  │
│   - reminder_service (守护线程)                      │
│   ↳ 业务校验、组合操作                                │
└──────────────────────────────────────────────────────┘
                      ↓ 调用
┌──────────────────────────────────────────────────────┐
│ DB 层 (todolist/db/)                                 │
│   - connection  (单例 sqlite3.Connection)             │
│   - migrations  (建表 SQL)                           │
│   - repository (CRUD 封装)                           │
└──────────────────────────────────────────────────────┘
                      ↓
┌──────────────────────────────────────────────────────┐
│ Models (todolist/models/)                            │
│   - Task / Category / Tag (dataclass)                │
└──────────────────────────────────────────────────────┘
```

## 2. 关键文件职责

| 文件 | 职责 |
|---|---|
| [main.py](../main.py) | 入口（`from todolist.app import run`） |
| [todolist/app.py](../todolist/app.py) | 装配：DB init → migrations → 提醒线程 → 主窗 |
| [todolist/config.py](../todolist/config.py) | 路径/常量（DB 路径、优先级列表、状态列表） |
| [todolist/db/connection.py](../todolist/db/connection.py) | 全局单例 sqlite3 连接 + 锁 |
| [todolist/db/migrations.py](../todolist/db/migrations.py) | 建表 SQL（IF NOT EXISTS） |
| [todolist/db/repository.py](../todolist/db/repository.py) | tasks/categories/tags CRUD |
| [todolist/models/task.py](../todolist/models/task.py) | Task dataclass + `is_overdue()` |
| [todolist/models/category.py](../todolist/models/category.py) | Category dataclass |
| [todolist/models/tag.py](../todolist/models/tag.py) | Tag dataclass |
| [todolist/services/task_service.py](../todolist/services/task_service.py) | 任务业务逻辑（校验、组合） |
| [todolist/services/category_service.py](../todolist/services/category_service.py) | 分类业务逻辑 |
| [todolist/services/tag_service.py](../todolist/services/tag_service.py) | 标签业务逻辑（get_or_create） |
| [todolist/services/search_service.py](../todolist/services/search_service.py) | filter / sort 纯函数 |
| [todolist/services/reminder_service.py](../todolist/services/reminder_service.py) | 守护线程 + 系统通知 |
| [todolist/ui/main_window.py](../todolist/ui/main_window.py) | 主窗：装配侧栏/工具栏/列表/状态栏 |
| [todolist/ui/task_list.py](../todolist/ui/task_list.py) | 任务列表（CTkScrollableFrame） |
| [todolist/ui/task_item.py](../todolist/ui/task_item.py) | 单条任务（色条 + 勾选 + 标题 + 操作） |
| [todolist/ui/task_form.py](../todolist/ui/task_form.py) | 新建/编辑弹窗 |
| [todolist/ui/category_sidebar.py](../todolist/ui/category_sidebar.py) | 分类侧栏（含"全部"项 + 已归档区） |
| [todolist/ui/filter_bar.py](../todolist/ui/filter_bar.py) | 搜索 + 状态/优先级/排序选择 + 主题切换 |
| [todolist/ui/theme.py](../todolist/ui/theme.py) | 浅/深主题色板 |

## 3. 数据流

### 3.1 新建任务

```
用户点击"新建" → MainWindow._new_task
    → TaskForm (Toplevel)
        → user fills + 点击保存
        → TaskForm._save
            → tag_service.get_or_create (循环)
            → task_service.add_task (校验)
                → repository.create_task + task_tags
                    → sqlite3.execute
    → MainWindow.refresh
        → sidebar.render + list.render + status.configure
```

### 3.2 提醒触发

```
app.py 启动时
    → reminder_service.start
        → 后台 daemon 线程
            → 每 60 秒调 _scan_once
                → repository.find_due_reminders(now)
                → 对每条命中：
                    - _notify_windows (PowerShell NotifyIcon)
                    - repository.mark_reminded (置 reminded=1)
```

## 4. 状态/UI 同步模型

- **无显式订阅**：UI 直接调 `refresh()` 重渲染
- 优点：代码简单，零状态管理库依赖
- 缺点：数据规模 >1000 时会卡（个人 todolist 不会）
- Phase 2 可引入 zustand-lite 自写信号

## 5. 线程模型

- **主线程**：UI 渲染、用户交互
- **守护线程**：reminder_service（60 秒扫描）
- **DB 连接**：`check_same_thread=False`，跨线程安全（因为单写者：主线程 + 守护线程只读）

## 6. 扩展点（Phase 2+）

- `services/search_service.py` 新增 `aggregate_by_tag()` 等
- `ui/` 新增 `kanban_view.py`（看板视图）
- `db/migrations.py` 改为版本号管理，支持 schema 升级
- `services/reminder_service.py` 改用 `plyer`/`winotify` 跨平台通知
