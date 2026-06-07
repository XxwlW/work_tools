# 已安装的 Skills 列表

> 来源：Claude Code 当前会话系统上下文
> 生成日期：2026/06/07

## 🔍 搜索与研究

- **agent-reach** — 17 平台联网能力（小红书、抖音、微博、推特、B站、V2EX、Reddit、LinkedIn、GitHub、YouTube 等）
- **deep-research** — 深度研究框架，多源搜索 + 交叉验证 + 综合报告
- **find-skills** — 帮助发现和安装 agent skills

## 🇨🇳 小红书相关

- **xiaohongshu-science-notes** — 创建小红书 AI 科普笔记（含文案 prompt + 视觉设计）

## 🅲++ 重构

- **cpp-large-file-refactoring** — 重构大型 C++ 文件（>500KB），含状态机和全局变量
- **cpp-park-out-refactoring** — 重构 APA 泊出相关 C++ 代码
- **park-out-refactor-workflow** — 泊出重构工作流

## 🔧 工具类

- **tsd-header-decoder** — 解码 TSD-Header 编码文件（`%TSD-Header-###%` 前缀）
- **workflow-miner** — 工作流挖掘

## ⚙️ 配置与设置

- **update-config** — 配置 Claude Code harness（settings.json、hooks、权限等）
- **keybindings-help** — 自定义键盘快捷键（`~/.claude/keybindings.json`）
- **fewer-permission-prompts** — 扫描常用只读命令并加入白名单

## 🔁 循环与运行

- **loop** — 定时循环执行 prompt（`/loop 5m /foo`）
- **run** — 启动并驱动项目应用以验证改动

## ✅ 验证与审查

- **verify** — 验证代码改动是否真的有效
- **code-review** — 审查 diff 中的正确性 bug 和可简化/可复用/可优化点
- **simplify** — 审查改动以提升复用性、简洁性、效率
- **review** — 代码审查
- **security-review** — 安全审查

## 🤖 API 与初始化

- **claude-api** — Claude API / Anthropic SDK 参考（模型 ID、定价、参数、流式、工具使用等）
- **init** — 初始化项目

---

## 使用方式

直接在 Claude Code 中输入 `/<skill-name>` 即可调用，例如：

```
/agent-reach
/deep-research
/code-review
```

**统计**：共 21 个 skills，涵盖 8 大类目。
