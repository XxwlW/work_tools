# 笔记 11 精华版：野马不听缰？5 大翻车点治好它

> 借爆款公式："野马"比喻
> 数据已用 Anthropic 官方博客核验（2026-06-04）

---

## 📝 精华版正文

### 标题
`3 人 5 月百万行代码的"野马"5 大翻车点🐎`

### 封面文案
> 主文案："AI 野马不听缰？5 大翻车点"
> 副文案："Anthropic 官方警告"

### 正文

小红薯们，上篇讲了 Harness 5 大组件，姐妹问："我按 3 步做了，怎么还是翻车？"😂
今天大白话讲 **5 大翻车点**，全是 Anthropic 官方原话！

> 💡 **【Neural | 奇点 × AI 测评日记】Vol.09**

---

🐎 **【为什么 AI 像"野马不听缰"？】**
Anthropic 工程师原话："AI 像一个极其听话但缺背景知识的实习生"
你说得清，它就听话；你不说，它就**自己脑补**——脑补的迟早会爆。
下面 5 大翻车点，全是"野马跑偏"的具体表现。

---

🚨 **【① Token 失控】$9 vs $200 真相**
单 Agent 跑 20 分钟 $9 → 烂尾；Harness 6 小时 $200 → 能玩。
💬 治法：每 $10 / 30 分钟做一次 checkpoint。

---

🚨 **【② Context Anxiety】AI 写一半说"做完了"**
原话："begin wrapping up work prematurely"
💬 比喻：考前焦虑症——还剩 30 分钟，提前誊抄"答题要点"。
💬 治法：Context Reset + 交接。

---

🚨 **【③ One-shot 翻车】长任务不该一鼓作气**
原话："the agent tended to try to do too much at once"
💬 治法：Initializer Agent 写 feature_list.json（200+ 条全 false）。

---

🚨 **【④ 自我评分过高】让 AI 自己打分 = 考生自己批卷**
原话："agents tend to respond by confidently praising the work"
💬 治法：Generator + Evaluator 分离。

---

🚨 **【⑤ 循环死锁】能耗光耐心**
ghuntley 原文："Ralph will test you"
💬 治法：每次 loop 只准做一件事 + 读 fix_plan.md。

---

✅ **【10 条自检清单】**（长按保存）

1. feature_list.json ≥ 20 条全 false
2. 每轮硬性预算
3. git commit + progress note
4. Initializer → Coding → Evaluator
5. Evaluator 用 Playwright / curl
6. 禁止 AI 改 description
7. Context Reset + 交接
8. fix_plan.md 先读再写
9. 每次 loop 只准一件事
10. **不信 AI 说"做完了"**

---

📖 **金句**：
"一次写完 = 一次写砸。"
"$9 还是 $200，差的不只是钱。"

---

🎁 **关注**：

📌 关注【Neural | 奇点 × AI 测评日记】！

**Vol.08**：Harness 5 大组件（已发）
**Vol.09**（本篇）：5 大翻车点
**Vol.10**：OpenClaw 🦞（已发）
**Vol.11**：10 个玩法

👇 **扣 1 Vol.11！** 哪个坑？

`#Harness #AI干货 #Anthropic #AI翻车 #Neural奇点AI测评`
