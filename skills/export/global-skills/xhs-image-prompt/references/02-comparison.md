# 02 - 概念对比图模板（左右两栏 + VS）

> **适用**：本期笔记中的"X vs Y / 软 vs 硬 / A vs B"型对比
> **核心公式**：双栏 + 3-3 标签 + 中间 VS + 底部金句

## 中文文字内容清单

```yaml
主标题:   "{对比主题}"                # 例：软 vs 硬 智能体
副标题:   "{来源/出处}"                # 例：蒲松涛：中国电子信息产业发展研究院
左栏标题: "{左主题}" + "（{左特征}）"   # 例：软智能体（不挑硬件）
右栏标题: "{右主题}" + "（{右特征}）"   # 例：硬智能体（要机器人/车）
左3标签:  ["{标签1}", "{标签2}", "{标签3}"]   # 例：["数字人", "智能客服", "AI 写作"]
右3标签:  ["{标签1}", "{标签2}", "{标签3}"]   # 例：["AI 汽车", "人形机器人", "工业机器人"]
中间词:   "VS"                        # 中国红
底部金句: "{金句 ≤ 20 字}"            # 例：软智能体先普及，硬智能体后跟上
底部水印: "Neural | 奇点 × AI 测评日记"
```

## 优化依据（1-3 行）

> 沿用 day11「左右对比」结构（参考 5 Agent 对比表，已验证 261 观看）
> "对比型"= 强收藏（用户反复对比阅读）
> 强化"署名 / 出处"（合规要求：必须明确致谢发言人）
> 配色：**左栏天空蓝 / 右栏焦糖橙**（双色对照易识别）

## 主体英文 Prompt

```text
Modern editorial comparison infographic, 3:4 vertical format.
Warm white background, dark grey ink lines, clean negative space.

[Top] Bold dark grey Chinese title: "{主标题}".
Below it, a small caramel orange italic subtitle: "{副标题}".

[Center] Two side-by-side vertical columns with a thin
dark grey divider in the middle. A big "VS" character
sits at the center of the divider in Chinese red, slightly tilted,
with small sparkles around it.

LEFT COLUMN (header in sky blue):
Title: "{左栏标题}"
3 product icons stacked vertically, each with a Chinese label below:
- Icon 1: "{左3标签[0]}"
- Icon 2: "{左3标签[1]}"
- Icon 3: "{左3标签[2]}"

RIGHT COLUMN (header in caramel orange):
Title: "{右栏标题}"
3 product icons stacked vertically, each with a Chinese label below:
- Icon 1: "{右3标签[0]}"
- Icon 2: "{右3标签[1]}"
- Icon 3: "{右3标签[2]}"

[Bottom] A big bold caramel orange Chinese sentence:
"{底部金句}".
Below it, a small grey watermark: "Neural | 奇点 × AI 测评日记".

[Decorations] Sky blue soft-cloud background tint on left,
caramel orange mechanical-gear pattern on right,
no cyberpunk, no neon colors, no robot face, no mascot animals,
museum atlas illustration aesthetic, clean editorial comparison style,
friendly and professional, 3:4 vertical composition.

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
[ ] {主标题} —— 重点核对（"vs"是英文小写）
[ ] {副标题} —— 完整人名 + 机构
[ ] {左栏标题} —— 括号注意中文全角
[ ] {右栏标题} —— 括号注意中文全角
[ ] 6 个标签 —— 一个不能错
[ ] {底部金句} —— 重点金句
[ ] 底部水印完整
[ ] 比例 3:4 竖版 ✓
[ ] 暖白底 + 蓝/橙双色对照 ✓
[ ] 无橘猫 / 无政府符号 ✓
```

## 范例参考

- [day15/prompts/15.3-软硬智能体对比.md](../../../xhs/内容/day15/prompts/15.3-软硬智能体对比.md)
