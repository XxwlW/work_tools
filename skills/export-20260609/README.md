# Skills 导出包 — 2026/06/09

## 📦 包含内容

| 目录 | 说明 |
|------|------|
| `global-skills/` | 10 个全局 skills（`~/.claude/skills/`） |
| `guizang-social-card-skill-main/` | 1 个项目本地 skill |
| `MANIFEST.md` | 详细清单 + 安装说明 + 变更日志 |

## 🚀 一键安装

### Windows
```cmd
:: 安装全局 skills
xcopy /E /I global-skills\* %USERPROFILE%\.claude\skills\

:: 安装 guizang 项目 skill
xcopy /E /I guizang-social-card-skill-main %USERPROFILE%\.claude\skills\guizang-social-card-skill-main
cd %USERPROFILE%\.claude\skills\guizang-social-card-skill-main
npm install
```

### Linux / macOS / WSL
```bash
# 安装全局 skills
cp -r global-skills/* ~/.claude/skills/

# 安装 guizang 项目 skill
cp -r guizang-social-card-skill-main ~/.claude/skills/
cd ~/.claude/skills/guizang-social-card-skill-main
npm install
```

## ✅ 验证

安装后重启 Claude Code，执行 `/skills` 命令应能看到 11 个 skill：
- agent-reach
- cpp-large-file-refactoring
- cpp-park-out-refactoring
- find-skills
- memory-management
- park-out-refactor-workflow
- remembering-conversations
- tsd-header-decoder
- workflow-miner
- xiaohongshu-science-notes
- guizang-social-card-skill

> ⚠️ Claude Code 还有 13 个**内置** skill（`init`/`review`/`verify` 等），无需也无法手动安装。

## 📝 详见
[MANIFEST.md](./MANIFEST.md)
