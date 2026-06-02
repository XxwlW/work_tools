/**
 * 日记画像视图脚本 (DiaryProfile.js) v3.0 - ViewQuery Dataset
 *
 * 功能：
 * 1. 扫描当前日记文件中的所有列表项。
 * 2. 自动解析 [Money, Emotion, Time] 向量。
 * 3. 展示：顶部关联文件条 -> 仪表盘 -> 双栏视图 (事件/账单)。
 */

// --- 1. 导入核心库 ---
const core = {};
await dv.view("11 scripts/Core/FinanceCore", core);
const { Utils: CoreUtils, Query, SourceResolver } = core;
const viewKit = {};
await dv.view("11 scripts/Core/ViewKit", viewKit);
const { ViewKit } = viewKit;
const viewQuery = {};
await dv.view("11 scripts/Core/ViewQuery", viewQuery);
const { ViewQuery } = viewQuery;

const Utils = { ...CoreUtils, ...ViewKit, parseValue: CoreUtils.parseValue };
const esc = value => ViewKit.escapeHtml(value);

// --- 样式注入 ---
const styles = `
.dp-container {
    --c-bg-card: var(--background-secondary);
    --c-bg-hover: var(--background-secondary-alt);
    --c-border: var(--background-modifier-border);
    --c-accent: var(--interactive-accent);
    --c-success: var(--color-green);
    --c-danger: var(--color-red);
    --c-text-muted: var(--text-muted);
    --c-text-normal: var(--text-normal);
    font-family: var(--font-interface);
    display: flex; flex-direction: column; gap: 16px;
    container-type: inline-size; container-name: view-container; max-width: 100%; overflow-x: hidden;
}

/* 顶部关联文件条 */
.dp-related-bar {
    background: var(--c-bg-card);
    border: 1px solid var(--c-border);
    border-radius: 8px;
    padding: 8px 12px;
    display: flex;
    align-items: center;
    gap: 10px;
    overflow-x: auto;
    white-space: nowrap;
}
.dp-related-label {
    font-size: 0.85em; font-weight: 600; color: var(--c-text-muted); flex-shrink: 0;
}
.dp-related-list {
    display: flex; gap: 8px;
}
.dp-related-item {
    font-size: 0.85em; color: var(--c-text-normal);
    background: var(--background-primary);
    padding: 2px 8px; border-radius: 4px;
    border: 1px solid var(--c-border);
    text-decoration: none;
    transition: all 0.2s;
}
.dp-related-item:hover {
    background: var(--c-bg-hover);
    color: var(--c-accent);
    border-color: var(--c-accent);
}

/* 仪表盘 */
.dp-dashboard {
    display: grid; grid-template-columns: repeat(5, 1fr); gap: 12px;
}
@container view-container (max-width: 750px) { .dp-dashboard { grid-template-columns: repeat(3, 1fr); } }
.dp-kpi-card {
    background: var(--c-bg-card); border: 1px solid var(--c-border); border-radius: 8px; padding: 12px; text-align: center;
    display: flex; flex-direction: column; justify-content: center; align-items: center;
    box-shadow: 0 2px 4px rgba(0,0,0,0.05);
}
.dp-kpi-icon { font-size: 1.4em; margin-bottom: 4px; opacity: 0.9; }
.dp-kpi-val { font-size: 1.2em; font-weight: 700; color: var(--c-text-normal); line-height: 1.2; }
.dp-kpi-label { font-size: 0.75em; color: var(--c-text-muted); margin-top: 2px; }

/* 双栏布局 */
.dp-main-grid {
    display: grid; grid-template-columns: 1fr 1fr; gap: 16px;
}
.dp-column {
    display: flex; flex-direction: column; gap: 10px;
}
.dp-column-header {
    font-size: 1.1em; font-weight: 600; color: var(--c-text-normal);
    padding-bottom: 8px; border-bottom: 2px solid var(--c-border);
    display: flex; align-items: center; gap: 6px;
}

/* 列表项卡片 */
.dp-item-card {
    background: var(--c-bg-card);
    border: 1px solid var(--c-border);
    border-radius: 8px;
    padding: 10px;
    display: flex; flex-direction: column; gap: 6px;
    transition: transform 0.2s;
}
.dp-item-card:hover {
    transform: translateY(-2px);
    box-shadow: 0 4px 8px rgba(0,0,0,0.1);
}
.dp-item-header {
    display: flex; justify-content: space-between; align-items: flex-start;
}
.dp-item-text {
    font-size: 0.95em; line-height: 1.4; color: var(--c-text-normal);
}
.dp-item-meta {
    display: flex; align-items: center; gap: 6px; flex-wrap: wrap; margin-top: 4px;
}
.dp-source-tag {
    font-size: 0.75em; color: var(--c-text-muted);
    background: var(--background-primary);
    padding: 1px 5px; border-radius: 3px;
    display: flex; align-items: center; gap: 3px;
}

.dp-badge {
    font-size: 0.75em; padding: 1px 5px; border-radius: 4px; font-weight: 600;
    display: inline-flex; align-items: center; gap: 3px;
}
.dp-badge-time { background: rgba(255, 152, 0, 0.1); color: var(--c-accent); }
.dp-badge-money { background: rgba(76, 175, 80, 0.1); color: var(--c-success); }
.dp-badge-money.neg { background: rgba(244, 67, 54, 0.1); color: var(--c-danger); }
.dp-badge-emotion { background: rgba(233, 30, 99, 0.1); color: #e91e63; }
.dp-container, .dp-container * { box-sizing: border-box; }
.dp-container { max-width: 100%; overflow-x: hidden; }
.dp-related-bar, .dp-dashboard, .dp-main-grid, .dp-column, .dp-item-card { min-width: 0; }
.dp-related-list { min-width: 0; overflow-x: auto; -webkit-overflow-scrolling: touch; }
.dp-related-item { max-width: 160px; overflow: hidden; text-overflow: ellipsis; white-space: nowrap; min-height: 28px; display: inline-flex; align-items: center; }
.dp-dashboard { grid-template-columns: repeat(auto-fit, minmax(116px, 1fr)); }
.dp-kpi-card { min-width: 0; }
.dp-kpi-val { max-width: 100%; overflow: hidden; text-overflow: ellipsis; }
.dp-item-header { gap: 8px; min-width: 0; }
.dp-item-text { min-width: 0; overflow-wrap: anywhere; }
.dp-source-tag, .dp-badge { min-height: 24px; }
.vk-display-link { overflow-wrap: anywhere; }
@container view-container (max-width: 760px) {
    .dp-main-grid { grid-template-columns: 1fr; }
    .dp-dashboard { grid-template-columns: repeat(3, minmax(0, 1fr)); }
    .dp-related-bar { align-items: flex-start; }
}
@container view-container (max-width: 520px) {
    .dp-container { gap: 10px; }
    .dp-related-bar { flex-direction: column; gap: 6px; white-space: normal; }
    .dp-related-list { width: 100%; flex-wrap: wrap; overflow-x: hidden; }
    .dp-related-item { max-width: 100%; min-height: 32px; }
    .dp-dashboard { grid-template-columns: repeat(2, minmax(0, 1fr)); gap: 8px; }
    .dp-kpi-card { padding: 9px 7px; }
    .dp-kpi-val { font-size: 1.05em; overflow-wrap: anywhere; }
    .dp-column-header { font-size: 1em; flex-wrap: wrap; }
    .dp-item-card { padding: 9px; }
    .dp-item-header { flex-direction: column; }
    .dp-item-meta { gap: 4px; }
}
`;
dv.container.innerHTML = `<style>${styles}</style>`;
const container = dv.container.createEl('div', { cls: 'dp-container' });

// --- 主逻辑 ---
const page = dv.current();
const diaryDate = page.file.day || page.file.ctime;
let items = [];
let relatedFiles = [];

function isTemplatePage(p) {
    return CoreUtils.hasObjectSupertag(p, "模板") || CoreUtils.resolveObjectType(p) === "模板";
}

// 1. 确定页面集合

// A. 深度解析集合 (Deep Parse): 用于提取 [事件/账单] 内容
// 包含: 当前日记 + 显式通过 "链接日记" 属性关联的文件
const deepPages = SourceResolver.resolve({ currentAndLinkedDiary: true });

// B. 广义关联集合 (Broad Relation): 用于顶部 "今日变动" 文件展示
// 包含: 深度集合(排除当前) + 反向链接 + 今日修改/创建
const relatedFilesMap = new Map(); // 使用 Map 去重: path -> page

// 1. 添加深度关联文件 (排除当前)
deepPages.forEach(p => {
    if (p.file.path !== page.file.path) {
        relatedFilesMap.set(p.file.path, p);
    }
});

// 2. 添加反向链接文件
const backlinkPages = (page.file.inlinks || []).map(link => dv.page(link.path)).filter(p => p && p.file);
for (let p of backlinkPages) {
    if (p.file.path !== page.file.path && !isTemplatePage(p)) {
        relatedFilesMap.set(p.file.path, p);
    }
}

// 3. 添加时间关联文件 (修改/创建)
if (diaryDate) {
    const timePages = SourceResolver.resolve({ allowGlobal: true }).filter(p => {
        if (p.file.path === page.file.path) return false;
        if (isTemplatePage(p)) return false;
        const sameDay = (date) => date && date.hasSame(diaryDate, 'day');
        return sameDay(p.file.mtime) || sameDay(p.file.ctime);
    });
    for (let p of timePages) {
        relatedFilesMap.set(p.file.path, p);
    }
}

// 生成 relatedFiles 数组
relatedFiles = Array.from(relatedFilesMap.values()).map(p => ({
    name: p.file.name,
    path: p.file.path,
    link: p.file.link
}));

function collectDiaryDataset(interaction = {}) {
    const dataset = ViewQuery.collect({
        Query,
        ViewKit,
        source: { querySources: { currentAndLinkedDiary: true } },
        consume: {
            entry(entry) {
                return {
                    money: entry?.vector?.money || 0,
                    emotion: entry?.vector?.emotion || 0,
                    time: entry?.vector?.time || 0,
                    entryType: entry?.type,
                };
            },
            include(consumption) {
                if (consumption.entryType === "journal") return true;
                return Boolean(consumption.money || consumption.emotion || consumption.time);
            },
        },
        interaction,
    });
    if (input && typeof input === "object") input.diaryDataset = dataset;
    return dataset;
}

function datasetToDiaryItems(dataset) {
    return (dataset?.visibleEntries || [])
        .map(entry => {
        const viewItem = CoreUtils.entryToViewItem(entry, { includeWallet: false });
        if (!viewItem) return null;
        return {
            ...viewItem,
            isJournal: entry.type === "journal",
            isRelatedNote: entry.sourcePath !== page.file.path,
            sourceName: entry.sourcePage?.file?.name || entry.sourcePath,
        };
    })
    .filter(Boolean);
}

// 2. 处理深度解析集合 (解析列表 + Frontmatter) -> 放入 items (时间轴 + 统计)
// 注意: 只有 currentAndLinkedDiary source 的内容会被解析到时间轴中
let currentDataset = collectDiaryDataset();
items = datasetToDiaryItems(currentDataset);

const lcDate = diaryDate ? new Date(diaryDate.ts || diaryDate) : new Date();
lcDate.setHours(0, 0, 0, 0);

// --- 渲染 1: 顶部关联文件 (Modified Files) ---
if (relatedFiles.length > 0) {
    const bar = container.createEl('div', { cls: 'dp-related-bar' });
    bar.createEl('div', { cls: 'dp-related-label', text: '📂 今日变动' });
    const list = bar.createEl('div', { cls: 'dp-related-list' });

    relatedFiles.forEach(f => {
        list.createEl('a', {
            cls: 'dp-related-item internal-link',
            text: f.name,
            href: f.path
        });
    });
}

// --- 渲染 2: FilterBar + 仪表盘 (Dashboard) ---
const filterHost = container.createEl('div');
const debugHost = input?.debug ? container.createEl('div') : null;
const dash = container.createEl('div', { cls: 'dp-dashboard' });
const timeCard = dash.createEl('div', { cls: 'dp-kpi-card' });
const emotionCard = dash.createEl('div', { cls: 'dp-kpi-card' });
const expenseCard = dash.createEl('div', { cls: 'dp-kpi-card' });
const netCard = dash.createEl('div', { cls: 'dp-kpi-card' });
const lcCard = dash.createEl('div', { cls: 'dp-kpi-card', attr: { title: '正在加载生活成本...' } });
await dv.view("11 scripts/Views/LivingCostWidget", { container: lcCard, asOfDate: lcDate });

// --- 渲染 3: 双栏布局 (Events vs Bills) ---
const mainGrid = container.createEl('div', { cls: 'dp-main-grid' });

function updateDashboard(data) {
    // tags 来自 StandardEntry，含父级继承标签且无 # 前缀
    const isTransfer = (i) => i.tags?.includes('转账');
    const totalTime = data.reduce((s, i) => s + i.vec[2], 0);
    const totalIncome = data.filter(i => i.vec[0] > 0 && !isTransfer(i)).reduce((s, i) => s + i.vec[0], 0);
    const totalExpense = data.filter(i => i.vec[0] < 0 && !isTransfer(i)).reduce((s, i) => s + i.vec[0], 0);
    const netMoney = totalIncome + totalExpense;
    const emotionItems = data.filter(i => i.vec[1] !== 0);
    const avgEmotion = emotionItems.length > 0 ? emotionItems.reduce((s, i) => s + i.vec[1], 0) / emotionItems.length : 0;

    timeCard.innerHTML = `
        <div class="dp-kpi-icon">⏳</div>
        <div class="dp-kpi-val">${totalTime.toFixed(1)}🍅</div>
        <div class="dp-kpi-label">专注投入</div>
    `;
    emotionCard.innerHTML = `
        <div class="dp-kpi-icon">❤</div>
        <div class="dp-kpi-val" style="color:${avgEmotion >= 0 ? 'var(--color-pink)' : 'var(--c-text-muted)'}">${avgEmotion > 0 ? '+' + avgEmotion.toFixed(1) : avgEmotion.toFixed(1)}</div>
        <div class="dp-kpi-label">情感指数</div>
    `;
    expenseCard.innerHTML = `
        <div class="dp-kpi-icon">💸</div>
        <div class="dp-kpi-val" style="color:var(--c-danger)">${Utils.fmtMoney(totalExpense)}</div>
        <div class="dp-kpi-label">今日支出</div>
    `;
    netCard.innerHTML = `
        <div class="dp-kpi-icon">💰</div>
        <div class="dp-kpi-val" style="color:${netMoney >= 0 ? 'var(--c-success)' : 'var(--c-danger)'}">${Utils.fmtMoney(netMoney)}</div>
        <div class="dp-kpi-label">净收支</div>
    `;
}

// 辅助渲染函数
function renderItem(container, item) {
    const card = container.createEl('div', { cls: 'dp-item-card' });

    let contentHtml = ViewKit.renderDisplayParts(item.displayParts, { fallback: item.displayText || item.text });
    if (item.isRelatedNote && item.text === item.sourceName) {
        // 如果是关联笔记本身（Frontmatter），直接显示链接
        contentHtml = `<a class="internal-link" href="${esc(item.path)}" style="text-decoration:none; color:var(--c-text-normal); font-weight:600;">${esc(item.text)}</a>`;
    }

    // Badges
    let badgesHtml = "";
    if (item.vec[2] > 0) badgesHtml += `<span class="dp-badge dp-badge-time">⏳${item.vec[2]}</span>`;
    if (item.vec[0] !== 0) badgesHtml += `<span class="dp-badge dp-badge-money ${item.vec[0] < 0 ? 'neg' : ''}">💰${item.vec[0]}</span>`;
    if (item.vec[1] !== 0) badgesHtml += `<span class="dp-badge dp-badge-emotion">❤${item.vec[1]}</span>`;

    // Source Tag (关联笔记来源)
    let sourceHtml = "";
    if (item.isRelatedNote) {
        sourceHtml = `<span class="dp-source-tag">📄 <a class="internal-link" href="${esc(item.path)}" style="color:inherit; text-decoration:none;">${esc(item.sourceName)}</a></span>`;
    }

    card.innerHTML = `
        <div class="dp-item-header">
            <div class="dp-item-text">${contentHtml}</div>
        </div>
        <div class="dp-item-meta">
            ${badgesHtml}
            ${sourceHtml}
        </div>
    `;
}

function renderColumns(data) {
    mainGrid.innerHTML = "";
    const eventItems = data.filter(i => !i.isJournal);
    const billItems = data.filter(i => i.isJournal);

    const leftCol = mainGrid.createEl('div', { cls: 'dp-column' });
    leftCol.createEl('div', { cls: 'dp-column-header' }).innerHTML = `<span>📅</span> 事件 / 记录`;
    if (eventItems.length > 0) {
        eventItems.forEach(item => renderItem(leftCol, item));
    } else {
        leftCol.createEl('div', { text: '暂无事件', style: 'color:var(--c-text-muted); font-style:italic;' });
    }

    const rightCol = mainGrid.createEl('div', { cls: 'dp-column' });
    rightCol.createEl('div', { cls: 'dp-column-header' }).innerHTML = `<span>💳</span> 账单 / 消费`;
    if (billItems.length > 0) {
        billItems.forEach(item => renderItem(rightCol, item));
    } else {
        rightCol.createEl('div', { text: '暂无账单', style: 'color:var(--c-text-muted); font-style:italic;' });
    }
}

function renderDiaryDebug(state = {}) {
    if (!debugHost) return;
    debugHost.innerHTML = "";
    ViewKit.renderDebugPanel(debugHost, {
        title: "Debug",
        dataset: currentDataset,
        interaction: state,
        rows: [
            ["view", "DiaryProfile"],
            ["target", page.file.path],
            ["source", "currentAndLinkedDiary"],
            ["source entries", currentDataset?.sourceEntries?.length || 0],
            ["consumed entries", currentDataset?.consumedEntries?.length || 0],
            ["visible entries", currentDataset?.visibleEntries?.length || 0],
            ["available tags", currentDataset?.availableTags?.length || 0],
            ["available links", currentDataset?.availableLinks?.length || 0],
            ["warnings", currentDataset?.warnings?.length || 0],
            ["interaction", state || {}],
            ["query metrics", currentDataset?.queryMetrics || {}],
        ],
    });
}

const filterBar = new ViewKit.FilterBar(filterHost, {
    controls: ['search', 'tags', 'links', 'sort'],
    availableTags: currentDataset.availableTags,
    availableLinks: currentDataset.availableLinks,
    sortFields: ViewKit.filterSortFields(['time', 'emotion', 'money']),
    initial: { sort: 'time', sortAsc: false },
    storageKey: `DiaryProfile:v2:${page.file.path}`,
    onFilter: (_filteredItems, state) => {
        currentDataset = collectDiaryDataset(state);
        items = datasetToDiaryItems(currentDataset);
        updateDashboard(items);
        renderColumns(items);
        renderDiaryDebug(state);
    },
});
filterBar.bind(currentDataset.filterItems);
