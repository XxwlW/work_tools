---
node_type: system
tags: [系统, Obsidian, 图谱, 说明书]
aliases: [xhs-Obsidian化说明书, Obsidian-关系图谱, xhs图谱]
created: 2026-06-09
---

# xhs/ Obsidian 关系图谱使用说明书

> 99 个 md 已全部加上 Obsidian frontmatter（节点类型 / 标签 / 别名 / 关联 / 标题 / 创建日期）。
> 在 Obsidian 中打开 `xhs/` 目录即可看到完整的关系图谱。

## 🎯 节点类型（6 类，共 99 个）

| node_type | 数量 | 含义 | 例子 |
|---|---|---|---|
| **note** | 79 | 已发布/待发布的小红书笔记 | day1 / day2 / day19 / Vol.16-20 清单 |
| **series** | 7 | 系列 README（按主题归类）| 01-AI概念科普 / 02-行业新闻 / ... |
| **pool** | 6 | 选题库（5 选题库 + 1 总目录）| 01-AI算力产业链 / 02-AI影响12行业 |
| **report** | 1 | Gemini 主研报 | 2026全球人工智能产业深度演进... |
| **system** | 5 | 系统/方法论文件 | xhs.md / 内容工厂 V3.0 / 视觉系统 V2.0 |
| **index** | 1 | 总目录 | xhs/内容/README.md |

## 🏷️ 标签体系（25+ 标签）

### 主题类
- `#AI-概念` `#AI-算力` `#行业-影响` `#跨圈层`
- `#政策` `#智能经济` `#打工人` `#工具`

### 内容形态
- `#笔记` `#系列` `#教程` `#科普` `#热点`
- `#Agent` `#Harness` `#横评` `#时间线`

### 节点角色
- `#选题库` `#研报` `#系统` `#配图` `#Prompt`

### day 索引（每个 day 都有专属标签）
- `#day1` ... `#day19`（用于按时间筛选）

## 🔗 关系类型（自动通过 wikilink 生成）

### 类型 1：同系列内（强关系）
- `[[day1]]` → `[[day2]]` → `[[day3]]`（按时间线串）
- `01-AI概念科普/README` → `[[day1]]` `[[day2]]` `[[day9]]`

### 类型 2：跨系列（主题相关）
- `[[Vol.19]]` → `[[xhs-diagnosis-2026-06-08]]` → `[[Vol.16-20涨粉型选题清单]]`
- `[[Gemini主研报]]` → `[[01-AI算力产业链]]` → `[[04-AI投资板块]]`

### 类型 3：素材 → 笔记
- `[[01-AI算力产业链]]` → `[[Vol.21 HBM]]`（数据被笔记引用）

### 类型 4：笔记 → 预告
- `[[Vol.19]]` → `[[Vol.20]]`（下期预告）

## 🗺️ 推荐图谱视图设置

### 视图 1：完整图谱（全节点）
- 在 Obsidian 打开 `xhs/` 根目录
- 点击左侧「Graph view」
- 设置：
  - **Filter**: `#笔记 OR #选题库 OR #研报`
  - **Depth**: 2
  - **Node size**: by backlinks
  - **Color**: by node_type

### 视图 2：Vol 系列（聚焦未来 5 篇）
- Filter: `#Vol-排期 OR #day19 OR #day15`
- Depth: 1
- 显示 Vol.16-20 排期 + Vol.19 + 上下游

### 视图 3：选题库全图（聚焦素材库）
- Filter: `#选题库 OR #研报`
- Depth: 2
- 显示 5 选题库 + 主研报 + 跨文件互链

### 视图 4：政策翻译官系列
- Filter: `#政策 OR #智能经济`
- Depth: 1
- 显示 day15 + day19 + 02-行业新闻

## 🛠️ 实用操作

### 1. 找某篇笔记的所有引用
- 打开 `[[day11]]`
- 右侧「Backlinks」面板：显示所有提到 day11 的文件
- 包括：xhs-diagnosis.md / Vol.16-20 清单 / day19-精华版

### 2. 看 Vol.19 的所有素材
- 打开 `day19/19-精华版.md`
- 看「related」：自动列出 7 张配图 Prompt
- 看「Backlinks」：所有引用 Vol.19 的文件

### 3. 找所有 Agent 相关内容
- 左侧 Search：`#Agent`
- 结果：day6 / day11 / day12 / 04-Agent系列/README / xhs诊断

### 4. 用 Alias 快速跳转
- 输入 `Vol.19` 即可跳到 `day19/19-精华版.md`（不用记路径）
- 输入 `HBM` 跳到 `01-AI算力产业链.md`
- 输入 `主研报` 跳到 Gemini 长文

## 📊 数字统计

| 维度 | 数量 |
|---|---|
| **md 文件** | 99 |
| **frontmatter 覆盖** | 100% |
| **节点类型** | 6 |
| **唯一标签** | 25+ |
| **自动 related 链接** | 200+（每文件 0-8 条）|
| **wikilink（正文内）** | 50+ |
| **最大节点（中心）** | Vol.19（被引用 4+ 次）|

## 🎨 视觉化建议

### 在 graph view 中给节点上色
在 Obsidian Graph Settings 里加 CSS：
```css
/* node_type 颜色 */
.graph-view .node[data-tags*="node_type-note"] { color: #4A90E2; }
.graph-view .node[data-tags*="node_type-pool"] { color: #FF8C42; }
.graph-view .node[data-tags*="node_type-report"] { color: #9D4EDD; }
.graph-view .node[data-tags*="node_type-series"] { color: #6BCB77; }
.graph-view .node[data-tags*="node_type-system"] { color: #495057; }
```

## 🔄 维护规则

### 新加笔记时
1. 在 frontmatter 加 `node_type: note`
2. 加 2-3 个 tags（如 `['笔记', '政策', 'day20']`）
3. 加 2-3 个 aliases（`['day20', 'Vol.20', '第20期']`）
4. 在文末加 related（指向素材库 + 上下期）
5. 跑 `obsidianize_v2.py` 重新生成（如有需要）

### 新加选题时
1. `node_type: pool`
2. tags 加 `['选题库', 'AI-XXX']`
3. 在 `00-总目录.md` 同步更新索引
4. aliases 加短名方便搜索

## 📂 节点类型快速跳转

| 想看什么 | 节点类型 | 例子 |
|---|---|---|
| 已发笔记 | `#node_type-note` + `#dayN` | day1-day19 |
| 待发排期 | `#node_type-note` + `#Vol-排期` | Vol.16-20 清单 / day19 |
| 素材库 | `#node_type-pool` | 5 选题库 |
| 主研报 | `#node_type-report` | Gemini 长文 |
| 系列导航 | `#node_type-series` | 01-07 系列 |
| 系统方法 | `#node_type-system` | 内容工厂 / 视觉系统 |

## 📁 备份

- 改造前完整备份：`xhs.backup_obsidian/`
- 脚本：`obsidianize_v2.py`（项目根目录）
- 备份在确认无误后可手动删除

**Why:** 把 99 个 md 升级为 Obsidian 原生格式，可直接生成关系图谱
**How to apply:** 用 Obsidian 打开 `xhs/` → 左侧 Graph → 按上面"视图"设置过滤
