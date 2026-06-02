/**
 * 项目画像视图脚本 (ProjectProfile.js) v4.0 - ViewQuery Dataset
 *
 * 功能：
 * 1. 全面复刻 PersonProfile v3.4 的所有特性。
 * 2. 核心特性：
 *    - 视觉升级：时光轴、卡片仪表盘、胶囊标签。
 *    - 布局升级：自动分栏（收支分离）、滚动区域。
 *    - 功能升级：日期筛选、CSV 导出。
 * 3. 项目特有逻辑：
 *    - 进度追踪：基于 `期望努力值` 的进度条。
 *    - 链接匹配：支持全路径匹配。
 */

// --- 1. 导入核心库 ---
const core = {};
await dv.view("11 scripts/Core/FinanceCore", core);
const { Utils: CoreUtils, Query } = core;
const viewKit = {};
await dv.view("11 scripts/Core/ViewKit", viewKit);
const { ViewKit } = viewKit;
const viewQuery = {};
await dv.view("11 scripts/Core/ViewQuery", viewQuery);
const { ViewQuery } = viewQuery;

const Utils = { ...CoreUtils, ...ViewKit, parseValue: CoreUtils.parseValue };

// --- 样式注入 ---
const styles = `
.pp-container {
    --c-bg-card: var(--background-secondary);
    --c-bg-hover: var(--background-secondary-alt);
    --c-border: var(--background-modifier-border);
    --c-accent: var(--color-blue);
    --c-success: var(--color-green);
    --c-danger: var(--color-red);
    --c-text-muted: var(--text-muted);
    --c-text-normal: var(--text-normal);
    font-family: var(--font-interface);
    height: 100%; display: flex; flex-direction: column;
    container-type: inline-size; container-name: view-container; max-width: 100%; overflow-x: hidden;
}
.pp-header-bar {
    display: flex; justify-content: space-between; align-items: center; margin-bottom: 10px;
}
.pp-filter-info { font-size: 0.8em; color: var(--c-text-muted); background: var(--c-bg-card); padding: 2px 8px; border-radius: 4px; border: 1px solid var(--c-border); }
.pp-btn-export {
    font-size: 0.8em; padding: 2px 8px; border-radius: 4px; border: 1px solid var(--c-border);
    background: var(--c-bg-hover); cursor: pointer; color: var(--c-text-normal);
}
.pp-btn-export:hover { background: var(--c-border); }

/* 仪表盘 */
.pp-dashboard {
    display: grid; grid-template-columns: repeat(3, 1fr); gap: 8px; margin-bottom: 12px; flex-shrink: 0;
}
.pp-kpi-card {
    background: var(--c-bg-card); border: 1px solid var(--c-border); border-radius: 8px; padding: 8px; text-align: center;
    display: flex; flex-direction: column; justify-content: center; align-items: center;
}
.pp-kpi-icon { font-size: 1.2em; margin-bottom: 2px; opacity: 0.8; }
.pp-kpi-val { font-size: 1.1em; font-weight: 700; color: var(--c-text-normal); line-height: 1.2; }
.pp-kpi-label { font-size: 0.7em; color: var(--c-text-muted); margin-top: 1px; }

/* 进度条 (项目特有) */
.pp-progress-container {
    width: 100%; background-color: var(--background-modifier-border); border-radius: 4px; height: 4px; margin-top: 6px; overflow: hidden;
}
.pp-progress-bar {
    height: 100%; background-color: var(--c-accent); transition: width 0.3s ease;
}

/* 滚动区域 */
.pp-scroll-area {
    overflow-y: auto; padding-right: 4px; flex-grow: 1;
    max-height: clamp(320px, 55vh, 680px);
    border: 1px solid var(--c-border); border-radius: 8px; padding: 8px;
    background: var(--background-primary);
}
.pp-scroll-area::-webkit-scrollbar { width: 6px; }
.pp-scroll-area::-webkit-scrollbar-thumb { background-color: var(--c-border); border-radius: 3px; }

/* 时光轴 */
.pp-timeline { position: relative; padding-left: 16px; margin-top: 5px; }
.pp-timeline::before {
    content: ''; position: absolute; left: 5px; top: 0; bottom: 0; width: 2px; background: var(--c-border); border-radius: 1px;
}
.pp-timeline-item { position: relative; margin-bottom: 12px; padding-left: 12px; }
.pp-timeline-dot {
    position: absolute; left: -16px; top: 6px; width: 8px; height: 8px; border-radius: 50%;
    background: var(--c-bg-card); border: 2px solid var(--c-accent); z-index: 1;
}
.pp-timeline-content {
    background: var(--c-bg-card); border: 1px solid var(--c-border); border-radius: 6px; padding: 6px 10px;
    display: flex; justify-content: space-between; align-items: flex-start;
    transition: background 0.2s;
}
.pp-timeline-content:hover { background: var(--c-bg-hover); }
.pp-timeline-main { flex: 1; margin-right: 8px; overflow: hidden; }
.pp-timeline-date { font-size: 0.7em; color: var(--c-text-muted); margin-bottom: 1px; display: block; }
.pp-timeline-text { font-size: 0.9em; line-height: 1.3; color: var(--c-text-normal); white-space: nowrap; overflow: hidden; text-overflow: ellipsis; }
.pp-timeline-meta { display: flex; flex-direction: column; align-items: flex-end; min-width: 50px; }
.pp-badge {
    font-size: 0.7em; padding: 1px 4px; border-radius: 3px; font-weight: 600; margin-bottom: 1px;
    display: inline-flex; align-items: center; gap: 2px;
}
.pp-badge-time { background: rgba(33, 150, 243, 0.1); color: var(--c-accent); }
.pp-badge-emotion { background: rgba(233, 30, 99, 0.1); color: #e91e63; }

/* 表格 */
.pp-section-title {
    font-weight: 600; font-size: 0.9em; margin: 0 0 8px; padding-left: 6px; border-left: 3px solid var(--c-accent);
    color: var(--c-text-muted);
}
.tx-direction-host { margin: 0 0 8px; }
.tx-direction-bar { margin-bottom: 8px; background: color-mix(in srgb, var(--background-secondary-alt) 72%, transparent); }
.tx-section {
    display: flex; flex-direction: column; min-height: 0; flex: 1 1 auto;
    border: 1px solid color-mix(in srgb, var(--c-border) 78%, transparent);
    border-radius: 8px; overflow: hidden; background: var(--background-primary);
}
.tx-section-header {
    display: flex; justify-content: space-between; align-items: center; gap: 10px;
    padding: 10px 12px; border-left: 4px solid var(--c-accent);
    border-bottom: 1px solid var(--c-border);
    background: color-mix(in srgb, var(--background-secondary) 76%, transparent);
}
.tx-section-title-wrap { display: flex; flex-direction: column; gap: 3px; min-width: 0; }
.tx-section-title { font-weight: 700; color: var(--c-text-normal); }
.tx-section-meta { color: var(--c-text-muted); font-size: 0.78em; }
.tx-section-body {
    overflow-y: auto; max-height: clamp(320px, 55vh, 680px); padding: 8px;
    -webkit-overflow-scrolling: touch; overscroll-behavior-x: contain;
}
.tx-row-main, .tx-row-meta { display: flex; justify-content: space-between; align-items: center; gap: 8px; min-width: 0; }
.tx-row-text { display: flex; align-items: center; min-width: 0; flex: 1; font-weight: 500; color: var(--c-text-normal); }
.tx-row-text > span { white-space: nowrap; overflow: hidden; text-overflow: ellipsis; max-width: 100%; }
.tx-row-meta { margin-top: 2px; color: var(--c-text-muted); font-size: 0.82em; }
.tx-row-tags { display: flex; flex-wrap: wrap; align-items: center; gap: 4px; min-width: 0; }
.tx-amount { font-family: monospace; font-weight: 700; flex-shrink: 0; }
.tx-amount.income { color: var(--c-success); }
.tx-amount.expense { color: var(--c-danger); }
.tx-amount.neutral { color: var(--c-text-muted); }
.tx-amount-badge { display: inline-flex; align-items: center; min-height: 22px; padding: 0 7px; border-radius: 999px; border: 1px solid var(--c-border); font-size: 0.74em; line-height: 1; white-space: nowrap; }
.tx-amount-badge.income { color: var(--c-success); border-color: color-mix(in srgb, var(--c-success) 44%, transparent); background: color-mix(in srgb, var(--c-success) 10%, transparent); }
.tx-amount-badge.expense { color: var(--c-danger); border-color: color-mix(in srgb, var(--c-danger) 44%, transparent); background: color-mix(in srgb, var(--c-danger) 10%, transparent); }
.tx-amount-badge.neutral { color: var(--c-text-muted); background: var(--background-primary); }
.tx-empty { color: var(--c-text-muted); text-align: center; padding: 18px 8px; font-size: 0.9em; }
.pp-table { width: 100%; border-collapse: separate; border-spacing: 0 2px; font-size: 0.85em; }
.pp-table td {
    background: var(--c-bg-card); padding: 4px 8px; border-top: 1px solid var(--c-border); border-bottom: 1px solid var(--c-border);
}
.pp-table tr td:first-child { border-left: 1px solid var(--c-border); border-top-left-radius: 6px; border-bottom-left-radius: 6px; }
.pp-table tr td:last-child { border-right: 1px solid var(--c-border); border-top-right-radius: 6px; border-bottom-right-radius: 6px; }
.pp-tag-pill {
    display: inline-block; font-size: 0.7em; padding: 0 4px; border-radius: 8px;
    background: var(--background-primary); border: 1px solid var(--c-border); color: var(--c-text-muted); margin-right: 2px;
}
.pp-container, .pp-container * { box-sizing: border-box; }
.pp-header-bar { flex-wrap: wrap; gap: 8px; }
.pp-btn-export { min-height: 32px; }
.pp-dashboard { grid-template-columns: repeat(auto-fit, minmax(118px, 1fr)); }
.pp-kpi-card, .pp-scroll-area, .pp-timeline-main { min-width: 0; }
.pp-kpi-val { max-width: 100%; overflow: hidden; text-overflow: ellipsis; }
.pp-table { table-layout: fixed; }
.pp-table td { overflow: hidden; }
.pp-table td > div { min-width: 0; }
.pp-tag-pill { max-width: 120px; overflow: hidden; text-overflow: ellipsis; white-space: nowrap; vertical-align: middle; }
.pp-source-link { min-width: 24px; min-height: 24px; display: inline-flex; align-items: center; justify-content: center; }
.vk-display-link { overflow-wrap: anywhere; }
@container view-container (max-width: 760px) {
    .pp-dashboard { grid-template-columns: repeat(2, minmax(0, 1fr)); }
    .pp-scroll-area { max-height: clamp(300px, 52vh, 620px); overflow-x: hidden; padding-right: 8px; }
    .tx-section-body { max-height: clamp(300px, 52vh, 620px); }
}
@container view-container (max-width: 520px) {
    .pp-container { gap: 8px; overflow-x: hidden; }
    .pp-header-bar { align-items: stretch; }
    .pp-btn-export { width: auto; min-height: 40px; }
    .pp-dashboard { gap: 6px; }
    .pp-kpi-card { padding: 8px 6px; }
    .pp-timeline { padding-left: 12px; }
    .pp-timeline-item { padding-left: 10px; }
    .pp-timeline-content { flex-direction: column; gap: 6px; padding: 8px; }
    .pp-timeline-main { width: 100%; margin-right: 0; }
    .pp-timeline-text { white-space: normal; overflow: visible; text-overflow: clip; overflow-wrap: anywhere; }
    .pp-timeline-meta { flex-direction: row; flex-wrap: wrap; align-items: flex-start; min-width: 0; gap: 4px; }
    .pp-badge { min-height: 24px; align-items: center; }
    .pp-table, .pp-table tbody, .pp-table tr, .pp-table td { display: block; width: 100%; }
    .pp-table { border-spacing: 0; }
    .pp-table tr { margin-bottom: 8px; }
    .pp-table td { border: 1px solid var(--c-border); border-radius: 8px; padding: 8px; }
    .pp-table tr td:first-child, .pp-table tr td:last-child { border-radius: 8px; }
    .pp-table span[style*="white-space:nowrap"] { white-space: normal !important; overflow-wrap: anywhere; }
    .pp-tag-pill { max-width: 100%; margin-top: 3px; min-height: 22px; line-height: 20px; }
    .tx-section-header { align-items: flex-start; flex-direction: column; }
    .tx-section-body { max-height: clamp(320px, 60vh, 560px); overflow-x: hidden; }
    .tx-row-main, .tx-row-meta { align-items: flex-start; }
    .tx-row-meta { flex-direction: column; }
    .tx-row-text > span { white-space: normal; overflow-wrap: anywhere; }
    .tx-amount-badge { min-height: 28px; }
}
`;
dv.container.innerHTML = `<style>${styles}</style>`;
const container = dv.container.createEl('div', { cls: 'pp-container' });

// --- 主逻辑 ---
const targetFile = dv.current().file;
const targetName = targetFile.name;

// 兼容旧参数
let viewType = input.type || "event";
if (viewType === "log") viewType = "event";
if (viewType === "cost") viewType = "transaction";

const targetEffort = Number(targetFile.frontmatter["期望努力值"] || targetFile.frontmatter["预期"] || targetFile.frontmatter["target"]) || 0;

// 日期筛选
const fm = targetFile.frontmatter;
const PROFILE_EVENT_PAGE_SIZE = Math.max(1, Number(fm?.eventBatchSize || fm?.["事件批量"] || 30) || 30);
const PROFILE_TRANSACTION_BATCH_SIZE = Math.max(1, Number(fm?.transactionBatchSize || fm?.["交易批量"] || 30) || 30);
const startDateStr = input.startDate || fm["开始时间"];
const endDateStr = input.endDate || fm["结束时间"];

let items = [];

const queryRules = {
    type: viewType === "transaction" ? "journal" : "event",
    explicitTarget: true,
    ...(input.filters || {}),
};

function isCurrentProjectLink(link) {
    const key = ViewKit.normalizeFilterLink(link);
    const label = ViewKit.linkLabel(link);
    return CoreUtils.linkMatchesTarget(key, targetFile)
        || CoreUtils.linkMatchesTarget(label, targetFile);
}

function makeProjectProfileConsume() {
    return {
        types: [viewType === "transaction" ? "journal" : "event"],
        entry(entry) {
            return {
                money: entry?.type === "journal" ? (entry.vector?.money || 0) : 0,
                emotion: entry?.type === "event" ? (entry.vector?.emotion || 0) : 0,
                time: entry?.type === "event" ? (entry.vector?.time || 0) : 0,
                entryType: entry?.type,
            };
        },
    };
}

function collectProjectProfileDataset(interaction = {}) {
    const dataset = ViewQuery.collect({
        Query,
        ViewKit,
        source: { querySources: { linkedTo: true } },
        rules: queryRules,
        consume: makeProjectProfileConsume(),
        interaction,
        excludeLink: isCurrentProjectLink,
    });
    if (input && typeof input === "object") input.projectProfileDataset = dataset;
    return dataset;
}

function datasetToProfileItems(dataset) {
    return (dataset?.visibleEntries || [])
        .map(entry => CoreUtils.entryToViewItem(entry, { targetFile }))
        .filter(Boolean);
}

let currentDataset = collectProjectProfileDataset();
items = datasetToProfileItems(currentDataset);

items.sort((a, b) => b.ctime - a.ctime);
let visibleItems = items;

// --- 顶部控制栏 ---
const headerBar = container.createEl('div', { cls: 'pp-header-bar' });
const exportBtn = headerBar.createEl('button', { cls: 'pp-btn-export', text: '📤 导出 CSV' });
exportBtn.onclick = () => {
    const fileName = `${targetName}_${viewType}_${new Date().toISOString().split('T')[0]}.csv`;
    Utils.exportToCSV(visibleItems, fileName);
};
const directionHost = viewType === "transaction"
    ? container.createEl('div', { cls: 'tx-direction-host' })
    : null;
const filterHost = container.createEl('div');
const debugHost = input?.debug ? container.createEl('div') : null;

// --- 渲染逻辑 ---

function renderProfileDebug(state = {}) {
    if (!debugHost) return;
    debugHost.innerHTML = "";
    ViewKit.renderDebugPanel(debugHost, {
        title: "调试信息",
        dataset: currentDataset,
        interaction: state,
        rows: [
            ["view", `ProjectProfile:${viewType}`],
            ["target", targetFile.path],
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

if (viewType === "event") {
    const dash = container.createEl('div', { cls: 'pp-dashboard' });
    const scrollArea = container.createEl('div', { cls: 'pp-scroll-area' });

    function renderEventView(data) {
        visibleItems = data;
        dash.innerHTML = "";
        scrollArea.innerHTML = "";

        const totalTime = data.reduce((sum, i) => sum + i.vec[2], 0);
        const emotionRecords = data
            .filter(i => i.vec[1] !== 0)
            .map(i => ({ score: i.vec[1], date: i.ctime }));
        const weightedEmotion = CoreUtils.calculateEmotionScore(emotionRecords);
        const emotionLabel = CoreUtils.getEmotionLabel(weightedEmotion);
        const progress = targetEffort > 0 ? Math.min((totalTime / targetEffort) * 100, 100) : 0;

        const card1 = dash.createEl('div', { cls: 'pp-kpi-card' });
        if (targetEffort > 0) {
            card1.innerHTML = `
                <div style="display:flex; justify-content:space-between; width:100%; align-items:baseline;">
                    <span class="pp-kpi-val">${totalTime.toFixed(1)}🍅</span>
                    <span style="font-size:0.8em; color:var(--c-text-muted)">${progress.toFixed(0)}%</span>
                </div>
                <div class="pp-progress-container"><div class="pp-progress-bar" style="width:${progress}%"></div></div>
                <div class="pp-kpi-label">累计投入 (目标 ${targetEffort}🍅)</div>
            `;
        } else {
            card1.innerHTML = `<div class="pp-kpi-icon">⏳</div><div class="pp-kpi-val">${totalTime.toFixed(1)}🍅</div><div class="pp-kpi-label">累计投入</div>`;
        }
        dash.createEl('div', { cls: 'pp-kpi-card' }).innerHTML = `<div class="pp-kpi-icon">${emotionLabel.emoji}</div><div class="pp-kpi-val" style="color:${emotionLabel.color}">${emotionLabel.score}</div><div class="pp-kpi-label">${emotionLabel.label}</div>`;
        dash.createEl('div', { cls: 'pp-kpi-card' }).innerHTML = `<div class="pp-kpi-icon">📅</div><div class="pp-kpi-val">${data.length}</div><div class="pp-kpi-label">日志条目</div>`;
        ViewKit.renderTimeline(scrollArea, {
            items: data,
            progressive: true,
            pageSize: PROFILE_EVENT_PAGE_SIZE,
            loadMoreText: "加载更多",
        });
    }

    new ViewKit.FilterBar(filterHost, {
        controls: ['search', 'tags', 'links', 'dateRange', 'sort'],
        availableTags: currentDataset.availableTags,
        availableLinks: currentDataset.availableLinks,
        sortFields: ViewKit.filterSortFields(['date', 'time', 'emotion', 'money']),
        initial: { startDate: startDateStr || '', endDate: endDateStr || '', sort: 'date', sortAsc: false },
        storageKey: `ProjectProfile:v2:event:${targetFile.path}`,
        onFilter: (_filtered, state) => {
            currentDataset = collectProjectProfileDataset(state);
            items = datasetToProfileItems(currentDataset);
            renderEventView(items);
            renderProfileDebug(state);
        },
    }).bind(currentDataset.filterItems);

} else {
    let txDirection = "all";
    let txBaseItems = items;
    const txDirectionLabels = { all: "全部交易", income: "收入", expense: "支出" };

    const dash = container.createEl('div', { cls: 'pp-dashboard' });
    const txSection = container.createEl('section', { cls: 'tx-section' });
    const txHeader = txSection.createEl('div', { cls: 'tx-section-header' });
    const txTitleWrap = txHeader.createEl('div', { cls: 'tx-section-title-wrap' });
    const txTitle = txTitleWrap.createEl('div', { cls: 'tx-section-title', text: '全部交易' });
    const txMeta = txTitleWrap.createEl('div', { cls: 'tx-section-meta', text: '默认显示全部交易' });
    const txBody = txSection.createEl('div', { cls: 'tx-section-body' });

    const getAmount = item => Number(item?.vec?.[0] ?? item?.value ?? 0) || 0;
    const isTransfer = item => (item.tags || []).some(tag => ViewKit.normalizeFilterTag(tag) === "转账");
    const getFinanceItems = data => data.filter(item => !isTransfer(item));
    const getDirectionCounts = data => ({
        all: data.length,
        income: data.filter(item => getAmount(item) > 0).length,
        expense: data.filter(item => getAmount(item) < 0).length,
    });
    const applyDirection = (data, direction) => {
        if (direction === "income") return data.filter(item => getAmount(item) > 0);
        if (direction === "expense") return data.filter(item => getAmount(item) < 0);
        return data;
    };
    const getDirectionText = amount => amount > 0 ? "收入" : amount < 0 ? "支出" : "零额";

    function renderDirectionTabs(counts) {
        ViewKit.renderSegmentedControl(directionHost, {
            className: "tx-direction-bar",
            ariaLabel: "交易方向筛选",
            value: txDirection,
            options: [
                { key: "all", label: "全部", count: counts.all },
                { key: "income", label: "收入", count: counts.income },
                { key: "expense", label: "支出", count: counts.expense },
            ],
            onChange(next) {
                txDirection = next;
                renderTransactionView(txBaseItems);
            },
        });
    }

    function renderTransactionList(title, data, totalCount) {
        visibleItems = data;
        return ViewKit.renderTransactionList(txBody, {
            title,
            items: data,
            totalCount,
            direction: txDirection,
            pageSize: PROFILE_TRANSACTION_BATCH_SIZE,
            root: txBody,
            formatMoney: Utils.fmtMoney,
            getAmount,
            directionText: getDirectionText,
            setTitle: value => txTitle.setText(value),
            setMeta: value => txMeta.setText(value),
            sourceButtonOptions: {
                className: "pp-source-link",
                style: "text-decoration:none; color:var(--c-text-muted); font-size:0.85em; flex-shrink:0; margin-right:4px;",
            },
        });
    }

    function renderTransactionView(data) {
        txBaseItems = data;
        dash.innerHTML = "";

        const financeItems = getFinanceItems(data);
        const income = financeItems.filter(i => getAmount(i) > 0).reduce((s, i) => s + getAmount(i), 0);
        const expense = financeItems.filter(i => getAmount(i) < 0).reduce((s, i) => s + getAmount(i), 0);
        const net = income + expense;
        const counts = getDirectionCounts(financeItems);

        dash.innerHTML = `
            <div class="pp-kpi-card">
                <div class="pp-kpi-icon">💰</div>
                <div class="pp-kpi-val" style="color:${net >= 0 ? 'var(--c-success)' : 'var(--c-danger)'}">${Utils.fmtMoney(net)}</div>
                <div class="pp-kpi-label">净收支</div>
            </div>
            <div class="pp-kpi-card">
                <div class="pp-kpi-icon">📥</div>
                <div class="pp-kpi-val" style="color:var(--c-success)">+${Utils.fmtMoney(income)}</div>
                <div class="pp-kpi-label">预算/收入</div>
            </div>
            <div class="pp-kpi-card">
                <div class="pp-kpi-icon">📤</div>
                <div class="pp-kpi-val" style="color:var(--c-danger)">${Utils.fmtMoney(expense)}</div>
                <div class="pp-kpi-label">成本/支出</div>
            </div>
        `;

        renderDirectionTabs(counts);
        renderTransactionList(txDirectionLabels[txDirection] || txDirectionLabels.all, applyDirection(financeItems, txDirection), counts.all);
    }

    new ViewKit.FilterBar(filterHost, {
        controls: ['search', 'tags', 'links', 'dateRange', 'sort'],
        availableTags: currentDataset.availableTags,
        availableLinks: currentDataset.availableLinks,
        sortFields: ViewKit.filterSortFields(['date', 'money']),
        initial: { startDate: startDateStr || '', endDate: endDateStr || '', sort: 'date', sortAsc: false },
        storageKey: `ProjectProfile:v2:transaction:${targetFile.path}`,
        onFilter: (_filtered, state) => {
            currentDataset = collectProjectProfileDataset(state);
            items = datasetToProfileItems(currentDataset);
            renderTransactionView(items);
            renderProfileDebug(state);
        },
    }).bind(currentDataset.filterItems);
}
