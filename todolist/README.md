# Todolist

> 一个适合个人的极简 Windows 桌面 todolist 工具，零云、零账号、数据全本地。

## 功能

- ✅ 任务增/删/改/标记完成
- 🏷️ 分类（带颜色）+ 标签（多对多）
- ⏰ 截止日期 + 提前提醒（系统通知）
- 🔍 搜索（标题 + 备注）/ 筛选（状态、优先级、分类）/ 排序
- 🌓 浅色 / 深色主题切换
- 💾 数据存本地 SQLite（路径：`%APPDATA%/Todolist/todolist.db`）

## 技术栈

- **语言**：Python 3.10+
- **GUI**：CustomTkinter（轻量、零外部 C 依赖、跨平台）
- **存储**：SQLite（标准库自带 `sqlite3`）
- **提醒**：守护线程 + Windows 通知

## 安装与运行

```bash
# 进入项目
cd todolist

# 创建虚拟环境
python -m venv .venv

# 激活（Git Bash on Windows）
source .venv/Scripts/activate

# 安装依赖
pip install -r requirements.txt

# 启动
python main.py
```

## 快捷键

| 快捷键 | 作用 |
|---|---|
| `Ctrl + N` | 新建任务 |
| `Ctrl + T` | 切换浅/深主题 |
| `F5` | 刷新 |
| `Enter` | 任务表单内保存 |
| `Esc` | 任务表单内取消 |
| 双击任务 | 编辑 |

## 目录结构

```
todolist/
├── main.py                 # 入口
├── requirements.txt
├── todolist/               # Python 包
│   ├── app.py              # 主程序装配
│   ├── config.py           # 路径/常量
│   ├── db/                 # 数据库（connection / migrations / repository）
│   ├── models/             # dataclass
│   ├── services/           # 业务逻辑
│   ├── ui/                 # CustomTkinter 视图
│   └── utils/              # 工具
├── tests/                  # 单元测试
└── docs/                   # 调研/架构文档
```

## 测试

```bash
pip install pytest
pytest tests/ -v
```

## 路线图（Phase 2+）

- [ ] 全局热键唤起浮层
- [ ] 系统托盘 + 关闭即隐藏
- [ ] 开机自启
- [ ] 重复任务 / 子任务
- [ ] 看板视图
- [ ] 数据导入/导出（todo.txt 协议）
- [ ] i18n（zh-CN / en-US）
- [ ] 打包为 .exe（PyInstaller / Nuitka）

## 数据位置

- Windows：`%APPDATA%/Todolist/todolist.db`
- 其他平台：`~/.local/share/Todolist/todolist.db`

卸载 = 删除 DB 文件即可。
