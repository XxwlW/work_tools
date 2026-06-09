# 04 - 数据引用图模板（大数字 + 周围支撑数据）

> **适用**：本期笔记的"X% 数字党 / 行业数据 / 报告引用"型
> **核心公式**：大数字 + 周围小气泡 + 底部金句 + 数据来源

## 中文文字内容清单

```yaml
主标题:   "{主题 + 数字}"        # 例：2027 年 70% 普及
中央大数字: "{数字}{单位}"        # 例：70%  /  78%  /  23%
中央含义:   "{数字含义 ≤ 10 字}"  # 例：AI 普及率
支撑数据1:  "{数字2}%{含义2}"      # 例：30% 教师岗位受影响
支撑数据2:  "{数字3}%{含义3}"
支撑数据3:  "{数字4}%{含义4}"
核心金句:   "{≤ 20 字金句}"        # 例：当 70% 人都用 AI，监管必须先行
数据来源:   "{出处}"               # 例：国务院 2025 印发
底部水印:   "Neural | 奇点 × AI 测评日记"
```

## 优化依据（1-3 行）

> 数字党公式（账号已验证 3 次：Token 3,069 / Skills 1,705 / 70 年进化 1,457）
> 大数字 = 视觉锚点（用户 3 秒内抓重点）
> 必须标注**数据来源**（合规 + 公信力）

## 主体英文 Prompt

```text
Modern editorial data infographic, 3:4 vertical format.
Warm white background, dark grey ink lines, clean negative space.

[Top] Bold dark grey Chinese title: "{主标题}".

[Center] A large caramel orange circle/bubble in center,
containing a HUGE bold dark grey number: "{中央大数字}".
Below the number, a small dark grey label: "{中央含义}".

Surrounding the central bubble, 3-4 small grey bubbles
with supporting data:
- Top-left: "{支撑数据1}"
- Top-right: "{支撑数据2}"
- Bottom-left: "{支撑数据3}"
- (optional Bottom-right: another data point)

[Bottom] Big bold Chinese red text: "{核心金句}".
[Bottom-middle] Tiny dark grey text: "数据来源: {数据来源}".
[Bottom-most] Small grey watermark: "Neural | 奇点 × AI 测评日记".

[Decorations] Subtle paper texture, gentle drop shadow on central bubble,
small star icons for emphasis, clean editorial style,
no mascot, no orange cat, no cyberpunk, no neon,
no robot face, museum atlas illustration aesthetic.

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
[ ] {主标题} —— 重点核对（含数字）
[ ] {中央大数字} —— 必须醒目（最大字号）
[ ] {中央含义} —— 简短说明
[ ] 3-4 个支撑数据 —— 数字必须对得上来源
[ ] {核心金句} —— 中国红，重点
[ ] {数据来源} —— 必填（合规）
[ ] 底部水印完整
[ ] 比例 3:4 竖版 ✓
[ ] 暖白底 + 焦糖橙中央气泡 ✓
[ ] 无橘猫 / 无政府符号 ✓
```

## 范例参考

- [day15/prompts/15.2-智能经济是什么.md](../../../xhs/内容/day15/prompts/15.2-智能经济是什么.md)
