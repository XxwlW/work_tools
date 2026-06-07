---
name: xhs 项目目录结构
description: xhs/ 目录的完整结构（数据/、内容/、建议/、tools/）+ 关键路径 + 命名规范
metadata:
  type: project
---

# xhs/ 顶层目录

```
xhs/
├── 数据/
│   └── 笔记列表明细表.xlsx           ← 账号 15 篇笔记数据
├── 内容/                            ← 已发布 + 草稿（按 day 组织）
│   ├── 01-AI概念科普/
│   ├── 02-行业新闻/
│   ├── 03-Harness系列/
│   ├── 04-Agent系列/
│   ├── 05-打工人急救包/
│   ├── 06-政策观察/
│   ├── 07-工具配图库/
│   ├── day1/ ... day15/             ← 每日笔记（每个含完整版+精华版+prompts/）
│   └── xhs.md                       ← xhs 概览
├── xhs_配图_prompt.md
├── 小红书爆款内容生成器.md
├── 小红书账号主脑系统.docx
├── 知识猫 × 小红书内容工厂 V3.0.md
├── 知识猫图解视觉系统 V2.0.md
├── 头像.png / 头像2.png / 头像3.png
└── 建议/                            ← 用户给的反馈/建议（暂无）
```

# 关键路径速查

| 用途 | 路径 |
|------|------|
| **账号数据** | `xhs/数据/笔记列表明细表.xlsx` |
| **day 笔记** | `xhs/内容/dayN/` |
| **day Prompt** | `xhs/内容/dayN/prompts/` |
| **day 精华版** | `xhs/内容/dayN/NN-精华正文-800字.md` |
| **day 完整版** | `xhs/内容/dayN/NN-XXX-完整版.md` |
| **day 配图集** | `xhs/内容/dayN/NN-配图Prompt集.md` |
| **政策翻译** | `xhs/内容/06-政策观察/` |
| **Harness 系列** | `xhs/内容/03-Harness系列/` |
| **Agent 系列** | `xhs/内容/04-Agent系列/` |

# 命名规范

- **day 目录**：`dayN/`（N 从 1 开始）
- **完整版笔记**：`NN-主题-完整版.md`
- **精华版**：`NN-精华正文-800字.md`
- **配图 Prompt 集**：`NN-配图Prompt集.md`
- **单图 Prompt**：`prompts/NN.N-主题.md`（小数点编号）

# 之前的"建议" 概念

用户说"xhs 建议"时通常指：
- 之前对话中我对账号/笔记的具体建议
- 内容写作避坑（教程类、机会推荐类）
- 数据分析结论

**建议**是会话级反馈，已经编码到：
- `memory/no-tutorial-without-data.md`（教程类避坑）
- `memory/only-opinion-sharing.md`（机会推荐避坑）
- `memory/xiaohongshu-account-profile.md`（爆款公式）
