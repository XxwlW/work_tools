# MANIFEST — Skills 详细清单

> 导出时间：2026/06/07
> 源机器：Windows 11 / 用户 `Wang`
> Claude Code 用户目录：`C:\Users\Wang\.claude\`

## 一、全局 Skills（`~/.claude/skills/`）

### 1. agent-reach/
| 文件 | 大小 | 说明 |
|------|------|------|
| `SKILL.md` | 3.5 KB | 主入口 + 路由表 |
| `references/career.md` | 0.7 KB | LinkedIn 分类 |
| `references/dev.md` | 1.4 KB | GitHub 分类 |
| `references/search.md` | 0.8 KB | 搜索分类 |
| `references/social.md` | 6.5 KB | 小红书/抖音/微博/推特/B站/V2EX/Reddit |
| `references/video.md` | 2.7 KB | YouTube/B站/播客 |
| `references/web.md` | 1.9 KB | 网页/文章/公众号/RSS |

**触发词**：搜索/读取/操作任何支持的平台，URL 分享，web 搜索

### 2. cpp-large-file-refactoring/
| 文件 | 大小 | 说明 |
|------|------|------|
| `SKILL.md` | 7.5 KB | 触发器：>500KB C++ 文件、状态机、全局变量 |

### 3. cpp-park-out-refactoring/
| 文件 | 大小 | 说明 |
|------|------|------|
| `SKILL.md` | 6.3 KB | 触发器：`APAMap_ParkingOut*`、`g_park_out_status` |

### 4. find-skills/
| 文件 | 大小 | 说明 |
|------|------|------|
| `SKILL.md` | 5.3 KB | 帮助用户发现和安装 skills |

### 5. park-out-refactor-workflow/
| 文件 | 大小 | 说明 |
|------|------|------|
| `SKILL.md` | 3.8 KB | 泊出重构工作流编排 |

### 6. tsd-header-decoder/
| 文件 | 大小 | 说明 |
|------|------|------|
| `SKILL.md` | 3.6 KB | 触发器：`%TSD-Header-###%` 前缀 |

### 7. workflow-miner/
| 文件 | 大小 | 说明 |
|------|------|------|
| `SKILL.md` | 5.1 KB | 主入口 |
| `Prompt.md` | 3.5 KB | 提示词模板 |
| `README.md` | 7.2 KB | 中文说明 |
| `README-EN.md` | 7.0 KB | 英文说明 |
| `README.md.edtz` | 2.8 KB | TSD 压缩格式 |
| `README-EN.md.edtz` | 2.8 KB | TSD 压缩格式 |
| `agents/openai.yaml` | - | agent 配置 |
| `scripts/mine_patterns.py` | - | 模式挖掘脚本 |

### 8. xiaohongshu-science-notes/
| 文件 | 大小 | 说明 |
|------|------|------|
| `SKILL.md` | 3.8 KB | 小红书 AI 科普笔记 + 文案 prompt + 视觉设计 |

## 二、项目本地 Skills

### 9. guizang-social-card-skill/
**来源**：`./skills/guizang-social-card-skill-main/`

| 文件/目录 | 说明 |
|----------|------|
| `SKILL.md` | 23 KB 主入口 |
| `HANDOFF.md` | 31 KB 交付文档 |
| `PRODUCT.md` | 18 KB 产品说明 |
| `README.md` / `README.en.md` | 18 KB / 18 KB 双语说明 |
| `LICENSE` | 34 KB 许可证 |
| `package.json` / `package-lock.json` | Node 依赖（须在目标机运行 `npm install`） |
| `validate-social-deck.mjs` | 15 KB 验证脚本 |
| `agents/openai.yaml` | OpenAI agent 配置 |
| `assets/` | WebGL/HTML 模板 + 截图背景图 |
| `references/` | 12 个分类参考文档（background-systems, category-cookbook, components, content-planning, image-overlay, layout-recipes, map-component, platform-specs, portrait-fill, production-workflow, qa-checklist, screenshot-treatment, style-system, theme-presets, title-shortener） |

**注意**：使用此 skill 前需在 skill 目录运行 `npm install` 安装依赖。

## 三、统计

| 项目 | 数量 |
|------|------|
| 总 skill 数 | 9 |
| 含 references 子目录的 skill | 3 |
| 含 scripts 子目录的 skill | 1 |
| 含 agents 子目录的 skill | 2 |

## 四、未导出的内容

### A. 内置 Skills（无需导入）
以下 skills 来自 Claude Code 系统内置，不需要也无法手动复制：
- `agent-reach` 实际上在 `~/.claude/skills/`，已导出
- `claude-api`、`init`、`run`、`loop`、`review`、`simplify`、`code-review`、`verify`、`security-review`、`update-config`、`keybindings-help`、`fewer-permission-prompts`、`deep-research` — 这些在 Claude Code 二进制内自带

### B. 故意未导出
- `WorkNotes/personal/skills/gstack/` — gstack 是独立的开源工具项目（含 `node_modules`、~30+ 子命令），不是 Claude skill。如需可从 GitHub 单独克隆 `garrettwastaken/gstack`。
