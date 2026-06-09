# 01 - 封面图模板（数字党 + 反常识 + IP 编号）

> **适用**：每期 dayN 笔记的第 1 张图（封面）
> **核心公式**：IP 编号 + 主标题 ≤ 8 字 + 副标题 ≤ 14 字 + 3 个数字标签

## 中文文字内容清单（出图后逐字核对）

```yaml
顶部小字: "【{系列名} Vol.{NN}】"   # 例：【政策翻译官 Vol.01】
主标题:   "{≤8字}"                  # 例：两会新词 智能经济
副标题:   "{≤14字}"                 # 例：3 个新观察
数字标签1: "{数字1}"                 # 例：2027
数字标签2: "{数字2}"                 # 例：70%
数字标签3: "{数字3}"                 # 例：3 观察
装饰词:   ["{口语词1}", "{口语词2}", "{口语词3}"]  # 例：["国家定调", "智能经济", "3 个新观察"]
底部水印: "Neural | 奇点 × AI 测评日记"
emoji:    ["🔥", "⭐", "✨"]         # 选 1-2 个
```

## 优化依据（1-3 行）

> 沿用 day11「中文文字 + 英文 Prompt + 平台参数」三段式结构（已验证 0 错字、好用）
> 画风 = **暖白 + 焦糖橙 + 深灰黑**（账号统一调性）
> 强化数字钩子：3 个具体数字 = 数字党公式（CTR 提升 30%+）

## 主体英文 Prompt

```text
Modern editorial poster style, 3:4 vertical format.
Warm white background, {主强调色}, dark grey ink lines, clean negative space.

[Top] A small dark grey title bar with white text: "【{系列名} Vol.{NN}】".

[Center top] A friendly small character in casual hoodie
(simple line art, NO mascot animal, NO orange cat, NO robot face,
just a thoughtful human figure) holding a laptop,
beside a large speech bubble with abstract AI circuit pattern
and floating data icons (no government or official symbols).

[Center] Big bold dark grey Chinese text: "{主标题}"
(this is the most important text, very large).
Below it, a small caramel orange subtitle: "{副标题}".

[Center middle] 3 numerical badges in a horizontal row:
- Caramel orange circle: "{数字1}"
- Chinese red circle: "{数字2}"
- Dark grey circle: "{数字3}"

[Right side] A speech bubble from the mascot with Chinese text:
"{装饰词1}" (or emoji 🔥).

[Bottom-left] {emoji 数量} small Chinese labels:
"{装饰词2}" / "{装饰词3}".

[Bottom-right] A subtle watermark in light grey:
"Neural | 奇点 × AI 测评日记".

[Decorations] Subtle AI circuit pattern in background,
small 5-pointed star icons, subtle paper texture,
no government symbols, no flags, no emblems, no official icons,
no cyberpunk, no neon colors, no robot face, no mascot animals,
authoritative but friendly and approachable vibe,
museum atlas illustration aesthetic, clean negative space.

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
[ ] {顶部小字} —— 重点核对（IP 编号格式）
[ ] {主标题} —— 重点核对（≤ 8 字）
[ ] {副标题} —— 重点核对（≤ 14 字）
[ ] {3 个数字标签} —— 数字必须醒目
[ ] {装饰词} —— 口语词/术语
[ ] {emoji} —— 是否正确显示
[ ] 底部水印 "Neural | 奇点 × AI 测评日记" —— 完整
[ ] 比例 3:4 竖版 ✓
[ ] 暖白底 + 焦糖橙 + 深灰黑调性 ✓
[ ] 无橘猫 / 无萌系 IP / 无政府符号 ✓
```

## 范例参考

- [day15/prompts/15.1-封面-两会新词智能经济.md](../../../xhs/内容/day15/prompts/15.1-封面-两会新词智能经济.md)
- [day11 5 个 AI Agent 封面](../../../xhs/内容/day11/prompts/)（参考对比）
