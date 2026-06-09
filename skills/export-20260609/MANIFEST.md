# MANIFEST — Skills 详细清单

> 导出时间：2026/06/09
> 源机器：Windows 11 / 用户 `Wang`
> Claude Code 用户目录：`C:\Users\Wang\.claude\`（skills → `D:\ClaudeConfig\skills\`）
> Agent skills 目录：`C:\Users\Wang\.agents\skills\`（memory-management / remembering-conversations）
> 项目根目录：`e:\Wxj\LearnForPnc\Ubuntu_Share\my_project\`

---

## 一、全局 Skills（`global-skills/`）— 10 个

> 来源：`D:\ClaudeConfig\skills\` + `C:\Users\Wang\.agents\skills\`（已解除软链接 → 实际文件）

### 1. agent-reach/ — 36K
| 文件 | 说明 |
|------|------|
| `SKILL.md` | 主入口 + 路由表 |
| `references/career.md` | LinkedIn 分类 |
| `references/dev.md` | GitHub 分类 |
| `references/search.md` | 搜索分类 |
| `references/social.md` | 小红书/抖音/微博/推特/B站/V2EX/Reddit |
| `references/video.md` | YouTube/B站/播客 |
| `references/web.md` | 网页/文章/公众号/RSS |

**触发词**：搜索/读取/操作任何支持的平台，URL 分享，web 搜索

### 2. cpp-large-file-refactoring/ — 8.0K
| `SKILL.md` | 触发器：>500KB C++ 文件、状态机、全局变量 |

### 3. cpp-park-out-refactoring/ — 8.0K
| `SKILL.md` | 触发器：`APAMap_ParkingOut*`、`g_park_out_status` |

### 4. find-skills/ — 8.0K
| `SKILL.md` | 帮助用户发现和安装 skills |

### 5. memory-management/ — 12K ⭐ 新增（上次导出漏掉）
| `SKILL.md` | 两级记忆系统（CLAUDE.md 工作记忆 + memory/ 知识库），解码缩写/黑话 |

> 源位置是 `~/.claude/skills/memory-management` → `~/.agents/skills/memory-management`（软链接，已解除）

### 6. park-out-refactor-workflow/ — 4.0K
| `SKILL.md` | 泊出重构工作流编排 |

### 7. remembering-conversations/ — 8.0K ⭐ 新增（上次导出漏掉）
| `SKILL.md` | 回答前必先搜索历史对话，禁止"我不知道"或猜测 |
| `MCP-TOOLS.md` | 配套 MCP 工具说明 |

> 源位置是 `~/.claude/skills/remembering-conversations` → `~/.agents/skills/remembering-conversations`（软链接，已解除）

### 8. tsd-header-decoder/ — 4.0K
| `SKILL.md` | 触发器：`%TSD-Header-###%` 前缀 |

### 9. workflow-miner/ — 49K
| `SKILL.md` | 主入口 |
| `Prompt.md` | 提示词模板 |
| `README.md` / `README-EN.md` | 中/英文说明 |
| `README.md.edtz` / `README-EN.md.edtz` | TSD 压缩格式 |
| `agents/openai.yaml` | agent 配置 |
| `scripts/mine_patterns.py` | 模式挖掘脚本 |

### 10. xiaohongshu-science-notes/ — 4.0K
| `SKILL.md` | 小红书 AI 科普笔记 + 文案 prompt + 视觉设计 |

---

## 二、项目本地 Skills

### 11. guizang-social-card-skill-main/
**来源**：`./skills/guizang-social-card-skill-main/`

| 文件/目录 | 说明 |
|----------|------|
| `SKILL.md` | 23 KB 主入口 |
| `HANDOFF.md` | 31 KB 交付文档 |
| `PRODUCT.md` | 18 KB 产品说明 |
| `README.md` / `README.en.md` | 18 KB 双语说明 |
| `LICENSE` | 34 KB 许可证 |
| `package.json` / `package-lock.json` | Node 依赖（须在目标机运行 `npm install`） |
| `validate-social-deck.mjs` | 15 KB 验证脚本 |
| `agents/openai.yaml` | OpenAI agent 配置 |
| `assets/` | WebGL/HTML 模板 + 截图背景图（9 个 .webp） |
| `references/` | 15 个分类参考文档（background-systems, category-cookbook, components, content-planning, image-overlay, layout-recipes, map-component, platform-specs, portrait-fill, production-workflow, qa-checklist, screenshot-treatment, style-system, theme-presets, title-shortener） |

**注意**：使用此 skill 前需在 skill 目录运行 `npm install` 安装依赖。

---

## 三、统计

| 项目 | 数量 |
|------|------|
| 总 skill 数 | 11 |
| 全局 skill | 10 |
| 项目本地 skill | 1 |
| 含 references 子目录的 skill | 2（agent-reach, workflow-miner, guizang）→ 实际 3 |
| 含 scripts 子目录的 skill | 1（workflow-miner） |
| 含 agents 子目录的 skill | 2（workflow-miner, guizang） |
| 含 MCP-TOOLS 文档的 skill | 1（remembering-conversations） |

---

## 四、未导出的内容

### A. 内置 Skills（无需导入）
以下 skills 来自 Claude Code 系统内置，不需要也无法手动复制：

- `init` — 初始化项目
- `review` — 审查 PR
- `verify` — 验证代码改动
- `run` — 启动应用
- `code-review` — 审查 diff
- `simplify` — 应用修复
- `security-review` — 安全审查
- `claude-api` — Claude API 参考
- `loop` — 循环执行命令
- `update-config` — 配置 Claude Code
- `keybindings-help` — 自定义快捷键
- `fewer-permission-prompts` — 减少权限弹窗
- `deep-research` — 深度研究 harness

**如何确认**：这些 skills 在 `~/.claude/skills/` 下不存在，也不在 `~/.agents/skills/`，只在 Claude Code 二进制内自带。

### B. 故意未导出
- `WorkNotes/personal/skills/gstack/` — gstack 是独立的开源工具项目（含 `node_modules`、~30+ 子命令），不是 Claude skill。如需可从 GitHub 单独克隆 `garrettwastaken/gstack`。

### C. 上一版导出（2026/06/07）的差异
| 差异 | 2026/06/07 | 2026/06/09 |
|------|-----------|-----------|
| 总全局 skill | 8 | 10 |
| 缺失项 | `memory-management`, `remembering-conversations` | ✅ 已补齐 |
| 软链接 | 部分保留为软链接 | ✅ 全部解除为实体文件（可移植） |

---

## 五、安装方式（目标机）

### 安装全局 skills
```bash
# 复制到目标机的 Claude Code skills 目录
cp -r global-skills/* ~/.claude/skills/
```

### 安装 guizang 项目 skill
```bash
cp -r guizang-social-card-skill-main ~/.claude/skills/
cd ~/.claude/skills/guizang-social-card-skill-main
npm install
```

### 验证安装
```bash
ls ~/.claude/skills/   # 应看到 10 + 1 = 11 个 skill 目录
# 重启 Claude Code 即可在 /skills 列表中看到
```

---

## 六、变更日志

- **2026/06/09** v2 — 补齐 `memory-management` + `remembering-conversations`（解除软链接），完整覆盖 10 个全局 + 1 个项目本地 skill
- **2026/06/07** v1 — 初次导出 8 个全局 skill（漏了 2 个软链接指向 `.agents/skills/` 的）
