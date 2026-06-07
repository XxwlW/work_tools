# 笔记 11：Vol.09《Harness 5 大翻车点》——Anthropic 亲口说

> 主题：5 大常见翻车点（按用户痛点排序）+ 自检清单
> 风格延续 v4 标准：极简封面 + IP 标识 + 系列预告
> ✅ **所有数据已用 Anthropic 官方博客核验（2026-06-04）**

---

## 📝 笔记正文

### 标题（5 选 1）
- `Anthropic 亲口说：让 AI 一次写完 = 必翻车🚨` ← 推荐
- `AI 写着写着突然说"做完了"？这是病，叫 Context Anxiety😱`
- `$9 还是 $200 差的不只是钱，是产品能不能跑起来💰`
- `5 个翻车点让 Harness 续作变灾难❌90% 的人会踩`
- `让 AI 给自己打分 = 让考生自己批卷📝`

### 封面文案（v4 极简）
> 主文案："5 大翻车点 90% 的人会踩"
> 副文案："Anthropic 官方警告"
> 视觉锤：医学生警告 + 大字

### 正文

📚 **姐妹们，上篇 Vol.08《Harness 续篇》讲了 3 步让 AI 当 24h 数字员工**
**好多姐妹留言："我按你说的做了，怎么还是翻车了？"**
**今天大白话讲 5 大翻车点，全是 Anthropic 2025-2026 官方博客原话**
**（Vol.09 · Neural | 奇点 × AI 测评日记）**

---

> 💡 **【Neural | 奇点 × AI 测评日记】Vol.09** ← IP 标识
> 系列定位：每篇一个 AI 神器 / 工具的"小白能懂的深度测评"

---

🚨 **【翻车点 ①】Token 失控——$9 vs $200 的真相**

**具体场景**：让 AI 做一个 2D 游戏编辑器。
Anthropic 自己实测：
- 单 Agent 跑 20 分钟 → **$9** → 实体存在但"对任何操作都没反应"
- Harness 跑 6 小时 → **$200** → 真正能玩

**为什么翻车**：没人拦着，模型把"自我评估 + 修改 + 再评估"当免费午餐。每个循环都在烧 token。

**怎么治**：给每一轮**硬性预算**——每 $10 / 每 30 分钟做一次 checkpoint，看产出值不值继续烧。

📖 **数据来源**：Anthropic 2026-03-24 博客

---

🚨 **【翻车点 ②】Context Anxiety（上下文焦虑）——AI 写一半说"做完了"**

**具体场景**：模型写到一半突然开始"收尾"，列个 to-do 就 stop，其实活儿才做了一半。

**Anthropic 原话**：
> "Some models also begin wrapping up work prematurely as they believe they approach their context limit."

**医学比喻** ≈ **考前焦虑症**：考生还剩 30 分钟能做完整张卷子，但他觉得时间不够，**提前 10 分钟就开始誊抄"答题要点"**，把后面大题直接空着。

**怎么治**：用 **Context Reset**（清空窗口 + 用结构化交接文件喂给下一棒），不要只靠 compaction。

---

🚨 **【翻车点 ③】一上来就 One-shot——长任务不该一鼓作气**

**具体场景**：用户说"帮我做一个 claude.ai 克隆"，模型立刻开始堆代码，写到一半 context 满了，留给下一棒的是**半成品 + 没文档**。

**Anthropic 2025-11-26 博客原话**：
> "the agent tended to try to do too much at once—essentially to attempt to one-shot the app. Often, this led to the model running out of context in the middle of its implementation."

**怎么治**：第一棒必须是 **Initializer Agent**——先写一个 **feature_list.json**（200+ 条功能，全标 `passes: false`），后续 Agent 一次只挑一条，改完才能 flip 那一行。

---

🚨 **【翻车点 ④】自我评分过高——让 AI 给自己打分 = 让考生自己批卷**

**具体场景**：Agent 跑了一阵，回过头看一眼 repo，"看着像做完了"，直接宣布胜利。

**Anthropic 原话**：
> "agents tend to respond by confidently praising the work—even when, to a human observer, the quality is obviously mediocre"

**怎么治**：**Generator + Evaluator 分离**。让另一个 Agent 拿 Playwright / MCP 实际点一遍，跑端到端测试，每条契约都打分。

---

🚨 **【翻车点 ⑤】循环死锁 / 半路换方向——能耗光你的耐心**

**具体场景**：让 AI 写完整功能，结果它循环 20 次写同样一段代码；或者跑到一半突然加奇怪"AI 增强"功能。

**ghuntley 原文（被 Anthropic 官方点名引用）**：
> "Ralph will test you. Every time Ralph has taken a wrong direction... Ralph gets tuned—like a guitar."

**怎么治**：
- 每次 loop 只准做**一件事**（ghuntley 原话）
- prompt 里加"先读 fix_plan.md 再动手"
- 准备一个 **fix_plan.md** 让 AI 写前必须读

---

✅ **【10 条自检清单】**（直接打印照做）

- [ ] 给任务写了 **feature_list.json**（≥20 条，全 false）
- [ ] 每一轮有 **硬性预算**（$ / 时间 / 最大循环次数）
- [ ] 每轮结束有 **git commit + progress note**
- [ ] 有 **Initializer → Coding → Evaluator** 三段
- [ ] Evaluator 用 **真实工具**（Playwright / curl / 单测）点过 / 跑过
- [ ] 关键功能列表写在 JSON 里、**禁止 AI 改 description**
- [ ] 上下文快满时主动 **reset + 交接**
- [ ] 给 AI 准备了 **fix_plan.md**，强制先读再写
- [ ] 每次 loop 只准做**一件事**
- [ ] **不信 AI 说"做完了"**，必须看到 passes=true + 测试截图

---

📖 **【金句 5 连发】**

1. "**一次写完 = 一次写砸**。长任务不拆，AI 必烂尾。"
2. "**让 AI 给自己打分 = 让考生自己批卷**。高分低能是常态。"
3. "**$9 还是 $200，差的不只是钱**——是产品能不能跑起来。"
4. "**AI 写到一半突然说做完了**，不是它偷懒，是它在'context anxiety'。"
5. "**给 AI 一个 while true 循环 = 给自己一个无底洞**。一次只准做一件事。"

---

🎁 **【关注钩子 + 系列预告】**

📌 **关注【Neural | 奇点 × AI 测评日记】，不迷路！**

**Vol.08**：Harness 续篇（已发）
**Vol.09**（本篇）：5 大翻车点
**Vol.10**（已发）：OpenClaw 🦞 单篇
**Vol.11 预告**：《OpenClaw 10 个超神玩法》

👇 **扣 1 解锁 Vol.11！**
**你用 Harness 踩过哪个坑？评论区对号入座！**

`#Harness #AI干货 #打工人必备 #数字员工 #Agent #AI提效 #Anthropic #AI翻车 #Neural奇点AI测评`

---

# 🎨 配图 Prompt（v4 中英混合版，5 张，直接生成含中文）

> **v4 极简封面 + IP 标识**（与 day10 / day12 标准一致）

## 11.1 封面（5 大翻车点 + Anthropic 警告）

```
Hand-drawn bullet journal style science illustration, 3:4 vertical format.
Beige aged kraft paper background, dark brown pen line art,
low-saturation watercolor wash. SIMPLIFIED composition,
ONE main warning visual, BIG bold typography.

[CENTER - HUGE title] Massive bold Chinese text in dark brown
taking up 1/3 of the canvas, centered:
"5 大翻车点"
Below it in smaller but still large text:
"90% 的人会踩"
Below that small text: "Anthropic 官方警告"

[CENTER BELOW TITLE] ONE large warning icon (red triangle with !)
with a small Anthropic logo above, and a big "❌" red X mark
on a crashed robot. Speech bubble from robot:
Chinese text "我以为我做完了"

[Bottom-left corner] ONE red badge with bold Chinese text:
"5 大翻车点"

[Bottom-right corner] ONE red badge with bold Chinese text:
"下期：OpenClaw 玩法"

[FOUR CORNERS - watermark style] Small Chinese text in each corner:
Top-left: "Neural | 奇点 × AI 测评日记"
Top-right: "Neural | 奇点 × AI 测评日记"
Bottom (faint): "Neural | 奇点 × AI 测评日记"

[REMOVED] NO multiple sticky notes, NO complex icons

[Decorations] Just ONE warning triangle, ONE crashed robot,
red X mark, "Anthropic" badge, clean white space around title.

--ar 3:4 --stylize 150 --niji 6
```

## 11.2 翻车点 ① + ②（$9 vs $200 + Context Anxiety）

```
Hand-drawn bullet journal style science illustration, 3:4 vertical format.
Beige aged kraft paper background, dark brown pen line art,
low-saturation watercolor wash.

[Top] White sticky note with bold Chinese text:
"🚨 翻车点 ① + ②". Small subtitle: "Token 失控 + 上下文焦虑".

[Center left] "Token 失控" red box with Chinese text:
"$9 烂尾" (red sad face) vs "$200 能跑" (green happy face)
Big arrow between them, with Chinese text "差的不只是钱"

[Center right] "Context Anxiety" red box with Chinese text:
"AI 写到一半突然说'做完了'"
Sub-caption: "考前焦虑症 - 提前收尾"

[Bottom-left] Red warning box: Chinese text
"硬性预算：每 $10 / 30 分钟做 checkpoint"

[Bottom-right] Green success box: Chinese text
"Context Reset = 清空窗口 + 交接文件"

[Decorations] Dollar signs, clock icons, "!" warnings,
"passes=false" red tag, sparkles, "Anthropic 2026" stamp.

--ar 3:4 --stylize 150 --niji 6
```

## 11.3 翻车点 ③ + ④（One-shot + 自我评分）

```
Hand-drawn bullet journal style science illustration, 3:4 vertical format.
Beige aged kraft paper background, dark brown pen line art,
low-saturation watercolor wash.

[Top] White sticky note with bold Chinese text:
"🚨 翻车点 ③ + ④". Small subtitle: "一次写完 + 自我评分".

[Center left] "One-shot 翻车" red box with Chinese text:
"长任务不该一鼓作气"
Sub-caption: "写到一半 context 满了 = 半成品 + 没文档"

[Center right] "自我评分过高" red box with Chinese text:
"让 AI 给自己打分 = 让考生自己批卷"
Sub-caption: "高分低能是常态"

[Bottom-left] Green success box: Chinese text
"Initializer Agent 写 feature_list.json"

[Bottom-right] Green success box: Chinese text
"Generator + Evaluator 分离"

[Decorations] "JSON" file icon, "200+ features" tag,
"Evaluator" badge, red X marks, green checkmarks,
"Generator + Evaluator" split diagram.

--ar 3:4 --stylize 150 --niji 6
```

## 11.4 翻车点 ⑤（循环死锁 / 半路换方向）

```
Hand-drawn bullet journal style science illustration, 3:4 vertical format.
Beige aged kraft paper background, dark brown pen line art,
low-saturation watercolor wash.

[Top] White sticky note with bold Chinese text:
"🚨 翻车点 ⑤". Small subtitle: "循环死锁 + 半路换方向".

[Center main] A cartoon robot stuck in a "while true" loop,
circling endlessly with the same code block, with a
small "❌" and a confused face. Speech bubble:
Chinese text "我跑到一半忘了原本要做啥"

[Right side] Three small "wrong direction" arrows
showing the robot changing goals mid-run:
"做游戏" -> "做 AI 增强" -> "做天气预报"

[Bottom-left] Green success box: Chinese text
"每次 loop 只准做一件事"

[Bottom-right] Green success box: Chinese text
"先读 fix_plan.md 再动手"

[Decorations] "while true" code snippet, loop arrows,
"fix_plan.md" file icon, sparkles, warning signs.

--ar 3:4 --stylize 150 --niji 6
```

## 11.5 10 条自检清单 + 互动图

```
Hand-drawn bullet journal style science illustration, 3:4 vertical format.
Beige aged kraft paper background, dark brown pen line art,
low-saturation watercolor wash.

[Top] White sticky note with bold Chinese text:
"✅ 10 条自检清单". Small subtitle: "长按保存照着做".

[Center main] A clipboard with 10 checkboxes,
each with a small Chinese label, all unchecked:

"□ feature_list.json ≥20 条全 false"
"□ 每轮硬性预算（$ / 时间）"
"□ git commit + progress note"
"□ Initializer → Coding → Evaluator"
"□ Evaluator 用 Playwright / curl"
"□ 禁止 AI 改 description"
"□ Context Reset + 交接"
"□ fix_plan.md 先读再写"
"□ 每次 loop 只准一件事"
"□ 不信 AI 说做完了"

[Bottom-left] A small "关注" badge with Chinese text:
"关注不迷路 + 扣 1 解锁 Vol.11"

[Bottom-right] A big red downward arrow with bold Chinese text:
"你踩过哪个坑？评论区对号入座！"

[Decorations] Clipboard, checkboxes, "Anthropic" stamp,
heart icon, "follow" badge, sparkles.

--ar 3:4 --stylize 150 --niji 6
```

---

# 📋 发布清单

## 推荐配置


**项**  **推荐**

🔸 **标题** | `Anthropic 亲口说：让 AI 一次写完 = 必翻车🚨`
🔸 **封面** | 11.1 极简版（5 大翻车点 + Anthropic 警告）
🔸 **配图** | 11.1 + 11.2 + 11.3 + 11.4 + 11.5（5 张）
🔸 **发布时间** | **20:30**
🔸 **标签** | `#Harness #AI干货 #打工人必备 #数字员工 #Agent #AI提效 #Anthropic #AI翻车 #Neural奇点AI测评`


## 置顶评论

```
"姐妹们，5 大翻车点 90% 的人都踩过 💔
我自己就在第 ④ 条上栽过——让 AI 自己测代码，
结果全跑通但线上崩了 😂

📌 Vol.10 已经发了 OpenClaw 🦞 单篇
下一篇 Vol.11《OpenClaw 10 个超神玩法》

👇 你踩过哪个坑？评论区对号入座！
扣 1 解锁 Vol.11 优先看！"
```

---

# ✅ 数据核验记录（2026-06-04）


**数据点**  **状态**  **来源**

🔸 **$9 vs $200 实测** | ✅ 精确 | Anthropic 2026-03-24 博客
🔸 **"Context anxiety" 原话** | ✅ 确认 | Anthropic 2026-03-24 博客
🔸 **"Self-evaluation bias" 原话** | ✅ 确认 | Anthropic 2026-03-24 博客
🔸 **"Premature completion" 原话** | ✅ 确认 | Anthropic 2025-11-26 博客
🔸 **One-shot 翻车原话** | ✅ 确认 | Anthropic 2025-11-26 博客
🔸 **feature_list.json 200+ 模式** | ✅ 确认 | Anthropic 2025-11-26 博客
🔸 **Ralph / "One item per loop"** | ✅ 确认 | ghuntley 2025-07-14
🔸 **Anthropic 引用 Ralph** | ✅ 确认 | Anthropic 2026-03-24 博客


## ❌ 修正旧版错误


**错误**  **修正**

🔸 **"Anthropic 2026 发了 2 篇"** | **2025-11-26 + 2026-03-24**（一篇 2025）
🔸 **"AI 加一堆没用功能" 具体数字** | 未核实，**删除具体数字**


---

# 🗂️ 相关文件

- [xhs/day13/prompts/](prompts/) - 5 个独立 Prompt
- [xhs/day13/出图指南.md](出图指南.md) - 5 平台出图步骤
- [xhs/day12/10-OpenClaw单篇.md](../day12/10-OpenClaw单篇.md) - Vol.10
- [xhs/day10/08-Harness续篇.md](../day10/08-Harness续篇.md) - Vol.08
