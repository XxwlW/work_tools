# 文档图面来源记录

本轮图面按教程正文重新制作，不再使用“无文字抽象概念图”。新版图面参考当前 `*-doc-*.png` 的信息结构，重新调用 Codex 内置 `image_gen` 生成每页专属 Obsidian 教学底图，再后期叠加准确的中文标注、Markdown 例子、箭头和解释卡片，保证新手能直接看懂。

## Image Gen 底图

- 生成方式：Codex 内置 `image_gen`
- 原始生成目录：

```text
C:\Users\keai3\.codex\generated_images\019e0b1b-dc50-75d1-994b-b67170241795
```

- 原始文件：

新版最终图使用 8 张专属 image gen 底图：

| 底图副本 | 原始 image gen 文件 | 用途 |
|---|---|---|
| `assets/source-imagegen-doc-00.png` | `ig_0d1d3995f0200da40169fec7da00248195a2999329345a8581.png` | 三步结构化：一句话 -> 链接 -> 值 |
| `assets/source-imagegen-doc-01.png` | `ig_0d1d3995f0200da40169fec8189b408195a064388fe867ed0a.png` | 日记收件箱和三类分发 |
| `assets/source-imagegen-doc-02.png` | `ig_0d1d3995f0200da40169fec84f56708195a22e60b5dc394d9f.png` | 普通文字和 wikilink 对照 |
| `assets/source-imagegen-doc-03.png` | `ig_0d1d3995f0200da40169fec88e0108819596de5df65abb556d.png` | 最小原子拆解 |
| `assets/source-imagegen-doc-04.png` | `ig_0d1d3995f0200da40169fec8c85a788195a9b7af8bfa7b9489.png` | 账单字段职责 |
| `assets/source-imagegen-doc-05.png` | `ig_0d1d3995f0200da40169fec90fd754819583e3df325f3185d0.png` | 严格写法和省略写法 |
| `assets/source-imagegen-doc-06.png` | `ig_0d1d3995f0200da40169fec95398b481958592eaa36afd3981.png` | 关键字后补 |
| `assets/source-imagegen-doc-07.png` | `ig_0d1d3995f0200da40169fec996572481958131082cc8a0b718.png` | 视图扫描当前页面 |

## 代表性底图提示词

```text
Use case: ui-mockup
Asset type: reusable background plate for Obsidian tutorial illustrations
Primary request: Create a clean, realistic Obsidian-like desktop workspace background that can be used as a base for overlaid Chinese labels and Markdown examples. The image should have a dark left sidebar, a top tab bar, and several large blank content panels/cards with plenty of empty space. It must not contain any readable text or fake characters.
Scene/backdrop: Knowledge-base note app on a desktop, dark sidebar, light main canvas, editor panel and preview/detail panel areas, subtle depth and clean shadows.
Style/medium: polished UI mockup, mature productivity tool, crisp raster image, restrained colors, suitable for technical documentation.
Composition/framing: 16:9 landscape, large blank central panels for later annotations; no complex abstract metaphors.
Text strategy: absolutely no readable words, no letters, no fake glyph paragraphs; panels should be blank or have very faint placeholder lines only.
Constraints: looks like an Obsidian/Markdown workspace; usable as a documentation figure background; not cartoonish.
Avoid: logos, branding, real data, dense text, garbled text, screenshots, flowchart-only design, toy 3D, cartoon style, phone numbers, emails, addresses, bank card numbers.
```

其余 8 张专属底图沿用同一策略：让 image 模型负责 Obsidian 场景、面板层次、箭头和视觉质感；所有中文、Markdown 例子和字段解释均由后期叠加。

## 最终图面清单

| 文件 | 插入页面 | 图面目的 |
|---|---|---|
| `assets/00-doc-loose-to-structured.png` | `00_先把心理压力降下来.md` | 用同一条记录展示“先写一句 -> 加链接 -> 补值”。 |
| `assets/01-doc-daily-inbox-example.png` | `01_从日记开始.md` | 展示日记收件箱里的三类记录会分别怎么被系统理解。 |
| `assets/02-doc-link-routing-comparison.png` | `02_写事情_让链接帮你归档.md` | 对比普通文字“小明”和链接 `[[小明]]` 的归档差异。 |
| `assets/03-doc-atom-breakdown.png` | `03_最小原子_系统到底在看什么.md` | 把一条 Markdown 记录拆成描述、链接和值。 |
| `assets/04-doc-bill-field-breakdown.png` | `04_写账单_钱包链接金额和标签.md` | 标出钱包归属、相关对象、`#记账` 和金额的职责。 |
| `assets/05-doc-strict-vs-shorthand.png` | `05_写法心法_严格写法和省略.md` | 对比严格写法和省略写法，并说明省略怎么借父级背景。 |
| `assets/06-doc-keywords-after-atom.png` | `06_关键字_等原子成立后再补充.md` | 展示账单先成立，再补 `BILL:`、`LIFE:`、`SOURCE:`。 |
| `assets/07-doc-view-scans-current-page.png` | `07_泛化_所有视图都在看相关原子.md` | 展示视图如何按当前页面链接扫描相关记录。 |

## 后期文字策略

- 所有中文说明和 Markdown 示例均由后期叠加，避免 image gen 生成乱码文字。
- 图面必须包含真实例子，而不是只有抽象图形。
- 图面强调“写法 -> 系统理解 -> 新手记住什么”，用于直接插入教程正文。
