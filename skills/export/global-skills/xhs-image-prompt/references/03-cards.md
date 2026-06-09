# 03 - 观察卡片图模板（3-4 个并列卡片）

> **适用**：本期笔记的"3 个观察 / 4 个观点 / N 个步骤"型
> **核心公式**：编号 + 卡片标题 + 2-3 行描述 + 小图标

## 中文文字内容清单

```yaml
主标题: "{主题}"                # 例：3 个新观察
卡片1编号: "1"
卡片1标题: "{标题 ≤ 10 字}"     # 例：智能经济 ≠ AI 聊天
卡片1描述: "{2-3 行描述}"        # 例：AI 从说话升级到干活，国家定调
卡片2编号: "2"
卡片2标题: "{标题 ≤ 10 字}"     # 例：6 大领域 + 70%
卡片2描述: "{2-3 行描述}"
卡片3编号: "3"
卡片3标题: "{标题 ≤ 10 字}"     # 例：软 vs 硬 智能体
卡片3描述: "{2-3 行描述}"
底部水印: "Neural | 奇点 × AI 测评日记"
```

## 优化依据（1-3 行）

> 沿用 day11「卡片 + 编号」结构（参考 5 Agent 卡片，账号已验证）
> 卡片化 = 易扫读（用户 5 秒内抓重点）
> 编号 + 焦糖橙顶边 = 视觉锚点

## 主体英文 Prompt

```text
Modern editorial card infographic, 3:4 vertical format.
Warm white background, dark grey ink lines, clean negative space.

[Top] Bold dark grey Chinese title: "{主标题}".
Below it, a small caramel orange subtitle: "个人观察 · 仅供讨论".

[Center] 3 horizontal cards stacked vertically (or 3 vertical cards in row
based on content length), each with a thin caramel orange top-border
and subtle shadow.

CARD 1:
- Caramel orange number badge (top-left): "{卡片1编号}"
- Bold dark grey title: "{卡片1标题}"
- 2-3 line description in regular grey text: "{卡片1描述}"
- Small icon on right side related to topic

CARD 2:
- Caramel orange number badge: "{卡片2编号}"
- Bold dark grey title: "{卡片2标题}"
- 2-3 line description: "{卡片2描述}"

CARD 3:
- Caramel orange number badge: "{卡片3编号}"
- Bold dark grey title: "{卡片3标题}"
- 2-3 line description: "{卡片3描述}"

[Bottom] Small grey watermark: "Neural | 奇点 × AI 测评日记".
[Bottom-most] Tiny attribution in light grey: "以上为博主个人观察".

[Decorations] Subtle paper texture, sticky-note feel,
no mascot animals, no orange cat, no cyberpunk, no neon,
no robot face, clean museum atlas aesthetic,
friendly professional tone.

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
[ ] 3 个卡片编号 / 标题 / 描述 —— 一个不能错
[ ] "个人观察 · 仅供讨论" 副标题
[ ] 底部水印完整
[ ] "以上为博主个人观察" 致谢声明
[ ] 比例 3:4 竖版 ✓
[ ] 暖白底 + 焦糖橙顶边 ✓
[ ] 无橘猫 / 无萌系 IP ✓
```

## 范例参考

- [day15/prompts/15.4-3个新观察卡片.md](../../../xhs/内容/day15/prompts/15.4-3个新观察卡片.md)
