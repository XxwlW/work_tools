# Claude Code Skills 导出包

> 一键打包并导入 Claude Code 全部自定义 skills 到另一台电脑

## 📦 包内容

| 目录/文件 | 说明 |
|----------|------|
| `global-skills/` | 来自 `~/.claude/skills`（8 个 skill） |
| `guizang-social-card-skill/` | 来自项目本地（1 个 skill） |
| `install.sh` | macOS / Linux 导入脚本 |
| `install.bat` | Windows 导入脚本 |
| `MANIFEST.md` | 详细清单 |

## 📋 包含的 Skills（共 9 个）

### 🔍 搜索与研究
- **agent-reach** — 17 平台联网能力（小红书、抖音、微博、推特、B站、V2EX、Reddit、LinkedIn、GitHub、YouTube 等）
- **find-skills** — 帮助发现和安装 agent skills

### 🇨🇳 小红书相关
- **xiaohongshu-science-notes** — 创建小红书 AI 科普笔记
- **guizang-social-card-skill** — 鬼藏社交卡片生成（HTML + WebGL 背景图）

### 🅲++ 重构
- **cpp-large-file-refactoring** — 重构大型 C++ 文件
- **cpp-park-out-refactoring** — 重构 APA 泊出 C++ 代码
- **park-out-refactor-workflow** — 泊出重构工作流

### 🔧 工具类
- **tsd-header-decoder** — 解码 TSD-Header 编码文件
- **workflow-miner** — 工作流挖掘

## 🚀 使用方法

### 方式 A：解压后用脚本（推荐）

1. 把整个 `export/` 目录复制到目标电脑（zip / U 盘 / 飞书皆可）
2. 解压（如果是 zip）
3. 运行对应平台的脚本：

**Windows（PowerShell 或 cmd）**
```bat
cd E:\path\to\export
install.bat
```

**macOS / Linux / WSL**
```bash
cd /path/to/export
bash install.sh
```

### 方式 B：自定义目标目录

**Windows**
```bat
install.bat D:\my-skills
```

**Linux/macOS**
```bash
bash install.sh --target ~/my-skills
```

### 方式 C：手动复制

把 `global-skills/` 下的每个子目录和 `guizang-social-card-skill/` 整个目录，复制到：

```
~/.claude/skills/   (用户级，对所有项目生效)
```

或项目级：

```
<your-project>/.claude/skills/   (仅对当前项目生效)
```

## ⚠️ 注意事项

1. **重启 Claude Code**：安装后必须重启 Claude Code 才会加载新 skills
2. **避免覆盖**：脚本默认跳过已存在的同名 skill。如需覆盖，先手动删除对应目录
3. **gstack 故意未导出**：`WorkNotes/personal/skills/gstack` 是独立工具项目（含 `node_modules`），不是 Claude skill
4. **部分 skill 是 Claude Code 内置的**（如 `loop`、`claude-api`、`init`、`run` 等），无需手动导入

## 🔍 验证安装

重启 Claude Code 后，在对话中输入：

```
/agent-reach
```

如果能正常调用，说明安装成功。也可以运行 `find-skills` 来确认 skills 列表。

## 📂 目录结构

```
export/
├── README.md                       # 本文件
├── MANIFEST.md                     # 详细清单（含每个 skill 的文件列表）
├── install.sh                      # Linux/macOS 导入脚本
├── install.bat                     # Windows 导入脚本
├── global-skills/                  # 全局 skills
│   ├── agent-reach/
│   │   ├── SKILL.md
│   │   └── references/
│   │       ├── career.md
│   │       ├── dev.md
│   │       ├── search.md
│   │       ├── social.md
│   │       ├── video.md
│   │       └── web.md
│   ├── cpp-large-file-refactoring/SKILL.md
│   ├── cpp-park-out-refactoring/SKILL.md
│   ├── find-skills/SKILL.md
│   ├── park-out-refactor-workflow/SKILL.md
│   ├── tsd-header-decoder/SKILL.md
│   ├── workflow-miner/
│   │   ├── SKILL.md
│   │   ├── Prompt.md
│   │   ├── README.md
│   │   ├── README-EN.md
│   │   ├── README.md.edtz
│   │   ├── README-EN.md.edtz
│   │   ├── agents/openai.yaml
│   │   └── scripts/mine_patterns.py
│   └── xiaohongshu-science-notes/SKILL.md
└── guizang-social-card-skill/      # 项目本地 skill
    ├── SKILL.md
    ├── HANDOFF.md
    ├── README.md / README.en.md
    ├── PRODUCT.md
    ├── LICENSE
    ├── package.json / package-lock.json
    ├── validate-social-deck.mjs
    ├── agents/openai.yaml
    ├── assets/  (template-*.html, magazine-bg-webgl.js, screenshot-backgrounds/)
    └── references/  (12 个 .md 文档)
```

## 📤 导出方（本机）

如果你想重新生成此导出包，运行：

```bash
# 把所有全局 skills 汇总到 export/global-skills/
cp -r ~/.claude/skills/* export/global-skills/

# 复制项目本地 skill
cp -r ./skills/guizang-social-card-skill-main export/guizang-social-card-skill

# 可选：打包为 zip 便于传输
cd export && zip -r ../claude-skills-$(date +%Y%m%d).zip .
```

打包日期：2026/06/07
