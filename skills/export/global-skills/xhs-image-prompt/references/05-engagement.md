# 05 - 互动图模板（评论引导 + 系列预告）

> **适用**：每期 dayN 笔记的最后 1 张图（互动 + 钩子）
> **核心公式**：3 个评论气泡 + 评论区引导 + 3 行系列预告 + 关注引导

## 中文文字内容清单

```yaml
主标题:   "{互动主题}"              # 例：3 个观察 你咋看？
副标题:   "{互动副标题}"            # 例：评论区聊聊～
气泡1:    "{观察1标题}" / "{观察1引导问题}"   # 例：智能经济 ≠ AI 聊天 / AI 从说话升级到干活，你看到了吗？
气泡2:    "{观察2标题}" / "{观察2引导问题}"   # 例：6 大领域 + 70% / 2 年后多数人用 AI，你身边几个？
气泡3:    "{观察3标题}" / "{观察3引导问题}"   # 例：软 vs 硬 智能体 / 软先普及硬后跟上，你同意吗？
引导词:   "评论区见～"
预告1:    "Vol.{N+1}: {预告1主题}"  # 例：Vol.16：70% 普及那天你最想 AI 干啥？
预告2:    "Vol.{N+2}: {预告2主题}"  # 例：Vol.17：软 vs 硬 智能体 详细对比
预告3:    "Vol.{N+3}: {预告3主题}"  # 例：Vol.18：你身边 AI 用到啥程度了？
关注引导: "关注【{系列名}】每周{星期}更新"
底部水印: "Neural | 奇点 × AI 测评日记"
emoji:    ["✨", "?", "⭐"]
```

## 优化依据（1-3 行）

> 沿用 day11 9.9% CTR 互动图结构（已验证）
> 3 个**开放讨论**问题（不做评分/扣 X，让粉丝自由表达）
> 系列预告 = 关注引导的核心钩子（**最强涨粉杠杆**）
> 借鉴 5/22 Token 3,069 经验：互动图是流量承接点

## 主体英文 Prompt

```text
Modern editorial engagement illustration, 3:4 vertical format.
Warm white background, dark grey ink lines, clean negative space.

[Top] Bold dark grey Chinese title: "{主标题}".
Below it, a small caramel orange subtitle: "{副标题}".

[Center] A small simple line-art human character
(NO mascot animal, NO orange cat, just a thoughtful person)
holding a sign that says "评论区见～".
Around the character, 3 floating speech bubbles
in different rotations, each with observation + question:

Bubble 1 (top-left, caramel orange border):
"{观察1标题}" / "{观察1引导问题}"

Bubble 2 (top-right, Chinese red border):
"{观察2标题}" / "{观察2引导问题}"

Bubble 3 (bottom-center, mixed orange/red border):
"{观察3标题}" / "{观察3引导问题}"

[Center middle] A big downward red arrow with bold
Chinese text: "评论区见～".

[Bottom] A 3-line series preview in small dark grey text:
"Vol.{N+1}: {预告1主题}"
"Vol.{N+2}: {预告2主题}"
"Vol.{N+3}: {预告3主题}"

[Bottom-most] A horizontal strip with caramel orange text:
"关注【{系列名}】每周{星期}更新".
Final watermark: "Neural | 奇点 × AI 测评日记".

[Decorations] Floating sparkles, "?" x3, small 5-pointed stars,
subtle paper texture, no mascot, no orange cat, no cyberpunk,
no neon, no robot face, museum atlas aesthetic,
friendly and conversational vibe, warm and inviting.

--ar 3:4 --stylize 150 --niji 6
```

## 平台参数

| 平台 | 比例 | 备注 |
|---|---|---|
| **MJ** | `--ar 3:4 --stylize 150 --niji 6` | 保留全部参数 |
| **即梦/可灵** | 选 3:4 | 去掉参数 |
| **DALL-E 3** | 1024×1792 | 选该尺寸 |

## 中文校对 checklist

```
[ ] {主标题} —— 重点核对
[ ] {副标题} —— 互动金句
[ ] 3 个气泡：{观察标题} / {引导问题} —— 6 行不能错
[ ] "评论区见～" —— 引导金句
[ ] 3 个系列预告（注意 "Vol." 不是 "Vo1."）：
    - "Vol.{N+1}: ..."
    - "Vol.{N+2}: ..."
    - "Vol.{N+3}: ..."
[ ] "关注【{系列名}】每周{星期}更新" —— 关注引导
[ ] 底部水印完整
[ ] 比例 3:4 竖版 ✓
[ ] 暖白底 + 橙/红/混 三色气泡 ✓
[ ] 无橘猫 / 无政府符号 ✓
```

## 范例参考

- [day15/prompts/15.6-互动图-评论区扣1-5.md](../../../xhs/内容/day15/prompts/15.6-互动图-评论区扣1-5.md)
