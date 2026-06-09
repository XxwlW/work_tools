# 更新日志

## v0.1.0 — 2026-06-09

### 新增
- 任务 CRUD（创建、编辑、删除、勾选完成）
- 分类（颜色 + 名称 + 归档）
- 标签（多对多，逗号分隔输入）
- 搜索（标题 + 备注）
- 筛选（状态、优先级、分类）
- 排序（创建时间/截止日期/优先级/标题，升降）
- 浅/深主题切换
- 截止日期 + 提前提醒（系统通知）
- 快捷键：Ctrl+N 新建、Ctrl+T 主题、F5 刷新
- 单元测试：task_crud、search、reminder

### 数据
- SQLite 单文件，路径 `%APPDATA%/Todolist/todolist.db`
- 表：categories / tags / tasks / task_tags

### 技术栈
- Python 3.10+ / CustomTkinter / SQLite（标准库）

### 已知限制
- 不含全局热键（Phase 2）
- 不含系统托盘 / 关闭即隐藏（Phase 2）
- 不含重复任务 / 子任务 / 看板视图（Phase 2）
- 不打包成 .exe（Phase 2）
