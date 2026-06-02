/**
 * 钱包画像视图脚本 (WalletProfile.js) v10.5 - Event ViewQuery Dataset
 *
 * 功能：
 * 1. 提供统一的钱包数据解析、计算和渲染逻辑。
 * 2. [New v10.3] 新增 type: "event" 事件视图（时光轴、KPI 仪表盘、情感分析）。
 * 3. 原有 type: "transaction" 交易视图保持不变。
 * 4. 适配 dv.view 调用方式。
 */

// --- 1. 导入核心库 ---
const core = {};
await dv.view("11 scripts/Core/FinanceCore", core);
const { CONFIG, Utils, Wallet, Query } = core;
const CoreUtils = Utils;
const viewKit = {};
await dv.view("11 scripts/Core/ViewKit", viewKit);
const { ViewKit } = viewKit;
const viewQuery = {};
await dv.view("11 scripts/Core/ViewQuery", viewQuery);
const { ViewQuery } = viewQuery;

// --- 2. 视图类型 ---
const viewType = (input && input.type) || "transaction";

// --- 3. 通用工具 ---
const ViewUtils = { ...CoreUtils, ...ViewKit, parseValue: CoreUtils.parseValue };
const esc = value => ViewKit.escapeHtml(value);

// =============================================
// 事件视图 (Event View) — 与 PersonProfile / ProjectProfile 统一体验
// =============================================
if (viewType === "event") {

    // --- 事件视图样式 ---
    const evStyles = `
    .pp-container {
        --c-bg-card: var(--background-secondary);
        --c-bg-hover: var(--background-secondary-alt);
        --c-border: var(--background-modifier-border);
        --c-accent: var(--color-cyan);
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
    .pp-badge-time { background: rgba(38, 166, 154, 0.1); color: var(--c-accent); }
    .pp-badge-emotion { background: rgba(233, 30, 99, 0.1); color: #e91e63; }
    .pp-container, .pp-container * { box-sizing: border-box; }
    .pp-header-bar { flex-wrap: wrap; gap: 8px; }
    .pp-dashboard { grid-template-columns: repeat(auto-fit, minmax(118px, 1fr)); }
    .pp-kpi-card, .pp-scroll-area, .pp-timeline-main { min-width: 0; }
    .pp-kpi-val { max-width: 100%; overflow: hidden; text-overflow: ellipsis; }
    .pp-source-link { min-width: 24px; min-height: 24px; display: inline-flex; align-items: center; justify-content: center; }
    .vk-display-link { overflow-wrap: anywhere; }
    @container view-container (max-width: 760px) {
        .pp-dashboard { grid-template-columns: repeat(2, minmax(0, 1fr)); }
        .pp-scroll-area { max-height: clamp(300px, 52vh, 620px); overflow-x: hidden; padding-right: 8px; }
    }
    @container view-container (max-width: 520px) {
        .pp-container { gap: 8px; overflow-x: hidden; }
        .pp-timeline { padding-left: 12px; }
        .pp-timeline-item { padding-left: 10px; }
        .pp-timeline-content { flex-direction: column; gap: 6px; padding: 8px; }
        .pp-timeline-main { width: 100%; margin-right: 0; }
        .pp-timeline-text { white-space: normal; overflow: visible; text-overflow: clip; overflow-wrap: anywhere; }
        .pp-timeline-meta { flex-direction: row; flex-wrap: wrap; align-items: flex-start; min-width: 0; gap: 4px; }
        .pp-badge { min-height: 24px; align-items: center; }
        .pp-btn-export { min-height: 40px; }
    }
    `;
    dv.container.innerHTML = `<style>${evStyles}</style>`;
    const container = dv.container.createEl('div', { cls: 'pp-container' });

    // --- 数据抓取 ---
    const targetFile = dv.current().file;
    const targetName = targetFile.name;

    const fm = targetFile.frontmatter || {};
    const WALLET_EVENT_PAGE_SIZE = Math.max(1, Number(fm.eventBatchSize || fm["事件批量"] || 30) || 30);
    const startDateStr = (input && input.startDate) || fm["开始时间"];
    const endDateStr = (input && input.endDate) || fm["结束时间"];
    const queryRules = { type: "event", explicitTarget: true, ...((input && input.filters) || {}) };

    function isCurrentWalletLink(link) {
        const key = ViewKit.normalizeFilterLink(link);
        const label = ViewKit.linkLabel(link);
        return CoreUtils.linkMatchesTarget(key, targetFile)
            || CoreUtils.linkMatchesTarget(label, targetFile);
    }

    function collectWalletEventDataset(interaction = {}) {
        const dataset = ViewQuery.collect({
            Query,
            ViewKit,
            source: { querySources: { linkedTo: true } },
            rules: queryRules,
            consume: {
                types: ["event"],
                entry(entry) {
                    return {
                        emotion: entry?.vector?.emotion || 0,
                        time: entry?.vector?.time || 0,
                        entryType: entry?.type,
                    };
                },
            },
            interaction,
            excludeLink: isCurrentWalletLink,
        });
        if (input && typeof input === "object") input.walletEventDataset = dataset;
        return dataset;
    }

    function datasetToEventItems(dataset) {
        return (dataset?.visibleEntries || [])
            .map(entry => CoreUtils.entryToViewItem(entry, { targetFile, includeWallet: false }))
            .filter(Boolean);
    }

    let currentDataset = collectWalletEventDataset();
    let items = datasetToEventItems(currentDataset);

    items.sort((a, b) => b.ctime - a.ctime);
    let visibleItems = items;

    // --- 顶部控制栏 ---
    const headerBar = container.createEl('div', { cls: 'pp-header-bar' });
    const exportBtn = headerBar.createEl('button', { cls: 'pp-btn-export', text: '📤 导出 CSV' });
    exportBtn.onclick = () => {
        const fileName = `${targetName}_事件_${new Date().toISOString().split('T')[0]}.csv`;
        ViewUtils.exportToCSV(visibleItems, fileName);
    };
    const filterHost = container.createEl('div');
    const debugHost = input?.debug ? container.createEl('div') : null;

    // --- KPI 仪表盘 ---
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

        dash.createEl('div', { cls: 'pp-kpi-card' }).innerHTML = `
            <div class="pp-kpi-icon">⏳</div>
            <div class="pp-kpi-val">${totalTime.toFixed(1)}🍅</div>
            <div class="pp-kpi-label">累计投入</div>
        `;
        dash.createEl('div', { cls: 'pp-kpi-card' }).innerHTML = `
            <div class="pp-kpi-icon">${emotionLabel.emoji}</div>
            <div class="pp-kpi-val" style="color:${emotionLabel.color}">${emotionLabel.score}</div>
            <div class="pp-kpi-label">${emotionLabel.label}</div>
        `;
        dash.createEl('div', { cls: 'pp-kpi-card' }).innerHTML = `
            <div class="pp-kpi-icon">📅</div>
            <div class="pp-kpi-val">${data.length}</div>
            <div class="pp-kpi-label">事件记录</div>
        `;
        ViewKit.renderTimeline(scrollArea, {
            items: data,
            progressive: true,
            pageSize: WALLET_EVENT_PAGE_SIZE,
            loadMoreText: "加载更多",
        });
    }

    function renderWalletEventDebug(state = {}) {
        if (!debugHost) return;
        debugHost.innerHTML = "";
        ViewKit.renderDebugPanel(debugHost, {
            title: "Debug",
            dataset: currentDataset,
            interaction: state,
            rows: [
                ["view", "WalletProfile:event"],
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

    new ViewKit.FilterBar(filterHost, {
        controls: ['search', 'tags', 'links', 'dateRange', 'sort'],
        availableTags: currentDataset.availableTags,
        availableLinks: currentDataset.availableLinks,
        sortFields: ViewKit.filterSortFields(['date', 'time', 'emotion']),
        initial: { startDate: startDateStr || '', endDate: endDateStr || '', sort: 'date', sortAsc: false },
        storageKey: `WalletProfile:v2:event:${targetFile.path}`,
        onFilter: (_filtered, state) => {
            currentDataset = collectWalletEventDataset(state);
            items = datasetToEventItems(currentDataset);
            renderEventView(items);
            renderWalletEventDebug(state);
        },
    }).bind(currentDataset.filterItems);

} else {

// =============================================
// 交易视图 (Transaction View) — 原有逻辑完整保留
// =============================================

const styles = `
.fin-v9-container {
    --c-bg-card: var(--background-secondary); --c-bg-hover: var(--background-secondary-alt);
    --c-text-main: var(--text-normal); --c-text-muted: var(--text-muted);
    --c-accent: var(--color-cyan); --c-danger: var(--color-red); --c-success: var(--color-green);
    --c-warning: var(--color-orange); --c-border: var(--background-modifier-border);
    --r-card: 12px;
    --s-shadow: 0 4px 6px -1px rgba(0, 0, 0, 0.1), 0 2px 4px -1px rgba(0, 0, 0, 0.06);
    font-family: var(--font-interface); color: var(--c-text-main);
    container-type: inline-size; container-name: view-container; max-width: 100%; overflow-x: hidden;
}
.fin-v9-container, .fin-v9-container * { box-sizing: border-box; }
.fin-grid-row { display: grid; grid-template-columns: 4.5fr 5.5fr; gap: 0.8rem; align-items: start; }
.fin-col { display: flex; flex-direction: column; gap: 1.5rem; min-width: 0; }
.fin-card { background: var(--c-bg-card); border-radius: var(--r-card); padding: 1.2rem; box-shadow: var(--s-shadow); border: 1px solid var(--c-border); display: flex; flex-direction: column; min-width: 0; }
.fin-card-header { display: flex; justify-content: space-between; align-items: center; gap: 8px; margin-bottom: 1rem; padding-bottom: 0.5rem; border-bottom: 1px solid var(--c-border); flex-wrap: wrap; }
.fin-title { font-weight: 600; font-size: 1.1em; display: flex; align-items: center; gap: 8px; min-width: 0; }
.tx-direction-host { margin: -2px 0 8px; }
.tx-direction-bar { margin-bottom: 8px; background: color-mix(in srgb, var(--background-secondary-alt) 72%, transparent); }
.wallet-transaction-section { border-left: 3px solid color-mix(in srgb, var(--c-accent) 70%, var(--c-border)); }
.wallet-transaction-section .fin-card-header { padding: 8px 10px; border-radius: 6px; background: color-mix(in srgb, var(--background-primary) 82%, transparent); border-bottom-color: color-mix(in srgb, var(--c-accent) 28%, var(--c-border)); }
.tx-section-title-wrap { display: flex; flex-direction: column; gap: 3px; min-width: 0; }
.tx-section-meta { color: var(--c-text-muted); font-size: 0.78em; }
.tx-amount-badge { display: inline-flex; align-items: center; min-height: 22px; padding: 0 7px; border-radius: 999px; border: 1px solid var(--c-border); font-size: 0.74em; line-height: 1; white-space: nowrap; }
.tx-amount-badge.income { color: var(--c-success); border-color: color-mix(in srgb, var(--c-success) 44%, transparent); background: color-mix(in srgb, var(--c-success) 10%, transparent); }
.tx-amount-badge.expense { color: var(--c-danger); border-color: color-mix(in srgb, var(--c-danger) 44%, transparent); background: color-mix(in srgb, var(--c-danger) 10%, transparent); }
.tx-amount-badge.neutral { color: var(--c-text-muted); background: var(--background-primary); }
.tx-empty { color: var(--c-text-muted); text-align: center; padding: 18px 8px; font-size: 0.9em; }
.kpi-group { display: grid; grid-template-columns: repeat(auto-fit, minmax(140px, 1fr)); gap: 1rem; margin-bottom: 1.5rem; }
.kpi-card { background: var(--c-bg-card); padding: 1rem; border-radius: var(--r-card); border: 1px solid var(--c-border); position: relative; overflow: hidden; }
.kpi-label { font-size: 0.85em; color: var(--c-text-muted); margin-bottom: 4px; }
.kpi-value { font-size: 1.5em; font-weight: 700; letter-spacing: 0; overflow-wrap: anywhere; }
.kpi-sub { font-size: 0.8em; margin-top: 4px; display: flex; gap: 8px; flex-wrap: wrap; }
.kpi-icon { position: absolute; right: -10px; bottom: -10px; font-size: 4rem; opacity: 0.05; pointer-events: none; }
.scroll-container { height: clamp(360px, 58vh, 650px); overflow-y: auto; padding-right: 4px; -webkit-overflow-scrolling: touch; overscroll-behavior-x: contain; }
.scroll-container::-webkit-scrollbar { width: 4px; }
.scroll-container::-webkit-scrollbar-thumb { background: var(--c-border); border-radius: 2px; }
.timeline { display: flex; flex-direction: column; gap: 0; }
.timeline-item { display: flex; gap: 12px; padding: 6px 0 6px 16px; border-left: 2px solid var(--c-border); position: relative; }
.timeline-item::before { content: ''; position: absolute; left: -5px; top: 14px; width: 8px; height: 8px; border-radius: 50%; background: var(--c-border); transition: all 0.3s ease; }
.timeline-item.urgent::before { background: var(--c-danger); box-shadow: 0 0 0 3px rgba(239, 83, 80, 0.2); }
.timeline-item.warning::before { background: var(--c-warning); }
.timeline-item.future::before { background: var(--c-success); }
.tl-date { min-width: 60px; text-align: right; font-size: 0.85em; color: var(--c-text-muted); line-height: 1.4; }
.tl-content { flex: 1; min-width: 0; background: var(--c-bg-hover); padding: 6px 10px; border-radius: 6px; display: flex; justify-content: space-between; align-items: center; gap: 8px; }
.wallet-table { width: 100%; border-collapse: separate; border-spacing: 0; font-size: 0.82em; white-space: nowrap; table-layout: fixed; }
.wallet-table th { text-align: center; color: var(--c-text-muted); font-weight: normal; padding: 3px 4px; box-shadow: 0 1px 0 var(--c-border); border-bottom: none; font-size: 0.9em; position: sticky; top: 0; background: var(--c-bg-card); z-index: 10; }
.wallet-table th:first-child { text-align: left; }
.wallet-table td { padding: 1px 4px; border-bottom: 1px solid var(--c-border); vertical-align: middle; height: 36px; overflow: hidden; text-overflow: ellipsis; }
.num-cell { text-align: center; font-family: monospace; }
.fin-btn { min-height: 40px; color: var(--c-text-main); }
.badge { max-width: 120px; overflow: hidden; text-overflow: ellipsis; white-space: nowrap; }
.wallet-card-list { display: none; flex-direction: column; gap: 8px; }
.wallet-tx-card { border: 1px solid var(--c-border); border-radius: 8px; background: var(--background-primary); padding: 10px; display: flex; flex-direction: column; gap: 7px; min-width: 0; }
.wallet-tx-top { display: flex; justify-content: space-between; gap: 10px; align-items: flex-start; }
.wallet-tx-desc { min-width: 0; font-weight: 600; line-height: 1.35; overflow-wrap: anywhere; }
.wallet-tx-amount { flex: 0 0 auto; font-family: monospace; font-weight: 700; text-align: right; }
.wallet-tx-meta { display: flex; flex-wrap: wrap; gap: 6px; align-items: center; color: var(--c-text-muted); font-size: 0.82em; }
.wallet-tx-tags { display: flex; flex-wrap: wrap; gap: 4px; }
.text-success { color: var(--c-success); }
.text-danger { color: var(--c-danger); }
.text-muted { color: var(--c-text-muted); }
@container view-container (max-width: 750px) { .fin-grid-row { grid-template-columns: 1fr; } }
@container view-container (max-width: 520px) {
    .fin-card { padding: 0.85rem; }
    .kpi-group { grid-template-columns: 1fr; margin-bottom: 1rem; }
    .scroll-container { height: clamp(320px, 60vh, 560px); max-height: clamp(320px, 60vh, 560px); overflow-y: auto; overflow-x: hidden; padding-right: 0; }
    .timeline-item { gap: 8px; padding-left: 12px; }
    .tl-date { min-width: 44px; font-size: 0.78em; }
    .tl-content { flex-direction: column; align-items: stretch; gap: 4px; }
    .fin-title { font-size: 1em; }
    .fin-btn, .badge { min-height: 40px; display: inline-flex; align-items: center; }
    .wallet-transaction-section .fin-card-header { align-items: stretch; }
    .tx-section-title-wrap { flex: 1 1 100%; }
    .tx-amount-badge { min-height: 28px; }
}
`;
dv.container.innerHTML = `<style>${styles}</style>`;

const container = dv.container.createEl('div', { cls: 'fin-v9-container' });

// --- 读取 YAML Frontmatter ---
const fm = dv.current().file.frontmatter || {};
const BILL_BATCH_SIZE = Math.max(1, Number(fm.billBatchSize || fm["账单批量"] || 20) || 20);
const TRANSACTION_PAGE_SIZE = Math.max(1, Number(fm.transactionBatchSize || fm["交易批量"] || 50) || 50);

// --- 数据处理 ---
const w = new Wallet(dv.current().file.link);

const today = new Date(); today.setHours(0, 0, 0, 0);
const targetFile = dv.current().file;
const allBills = Utils.collectWalletBills([w]);
const allBillItems = Utils.collectWalletBillViewItems([w], { bills: allBills, targetFile, dv });
const summary = Utils.toWalletSummary(w, allBills, today);

const stats = {
    pos: summary.positiveBalance,
    debt: summary.debt,
    cred: summary.creditLimit,
    pendingInflow: summary.futureInflow,
    pendingOutflow: summary.futureOutflow
};

// 净值计算
const currentBalance = summary.currentBalance;
const availableCredit = summary.availableCredit;

// --- UI 1: KPI 仪表盘 ---
const kpiSection = container.createEl('div', { cls: 'kpi-group' });
const createKpi = (title, val, subHtml, icon, colorVar) => {
    const card = kpiSection.createEl('div', { cls: 'kpi-card' });
    if (colorVar) card.style.borderLeft = `4px solid ${colorVar}`;
    card.innerHTML = `
        <div class="kpi-label">${title}</div>
        <div class="kpi-value" style="color:${colorVar || 'inherit'}">${val}</div>
        <div class="kpi-sub">${subHtml}</div>
        <div class="kpi-icon">${icon}</div>
    `;
};

createKpi("当前余额", Utils.fmtMoney(currentBalance),
    `<span>现金流 ${Utils.fmtMoney(stats.pos)}</span> <span style="opacity:0.3">|</span> <span>贷款 ${Utils.fmtMoney(stats.debt)}</span>`,
    "💰", currentBalance >= 0 ? "var(--c-success)" : "var(--c-danger)");

createKpi("信用额度", Utils.fmtMoney(stats.cred),
    `<span>总额度</span>`,
    "💳", "var(--c-text-muted)");

createKpi("可用额度", Utils.fmtMoney(availableCredit),
    `<span>剩余可用</span>`,
    "✅", "var(--c-accent)");



// --- UI 3: 左右分栏 ---
const gridRow = container.createEl('div', { cls: 'fin-grid-row' });

// 左侧：近期账单 (Timeline)
const radarCol = gridRow.createEl('div', { cls: 'fin-col' });
const radarCard = radarCol.createEl('div', { cls: 'fin-card' });
radarCard.innerHTML = `<div class="fin-card-header"><div class="fin-title">📅 近期账单</div></div>`;

const upcoming = Utils.upcomingWalletBillViewItems(allBillItems, { today, targetFile, dv });

if (upcoming.length === 0) {
    radarCard.createEl('div', { cls: 'text-muted', text: '近期无待办账单 🎉' });
} else {
    const scrollBox = radarCard.createEl('div', { cls: 'scroll-container' });
    const tl = scrollBox.createEl('div', { cls: 'timeline' });
    const loadStatus = scrollBox.createEl('div', { cls: 'text-muted', style: 'font-size:0.8em; padding:6px 0; text-align:center;' });
    let renderedBillCount = 0;

    const renderBill = (u) => {
        const date = u.dateObj || Utils.resolveBillDate(u.date);
        const rel = Utils.getRelativeTimeDesc(u.date);
        const isIncome = u.value > 0;
        const item = tl.createEl('div', { cls: `timeline-item ${rel.type}` });
        const descriptionHtml = ViewKit.renderDisplayParts(u.displayParts, { fallback: u.displayText || u.description || u.text });
        const sourceButton = ViewKit.renderSourceButton(u, {
            className: "tl-source-link",
            label: "📄",
            style: "text-decoration:none; color:var(--c-text-muted); margin-right:4px;",
        });
        item.innerHTML = `
            <div class="tl-date">
                <div style="font-weight:bold">${rel.text}</div>
                <div style="font-size:0.8em">${date ? `${date.getMonth() + 1}/${date.getDate()}` : '-'}</div>
            </div>
            <div class="tl-content">
                <div style="display:flex; flex-direction:column;">
                    <span class="tl-desc">${sourceButton}${descriptionHtml}</span>
                </div>
                <div class="tl-amt ${isIncome ? 'text-success' : 'text-danger'}">
                    ${isIncome ? '+' : ''}${u.value.toFixed(0)}
                </div>
            </div>
        `;
    };

    const renderNextBills = () => {
        const nextItems = upcoming.slice(renderedBillCount, renderedBillCount + BILL_BATCH_SIZE);
        nextItems.forEach(renderBill);
        renderedBillCount += nextItems.length;
        loadStatus.setText(renderedBillCount < upcoming.length
            ? `已显示 ${renderedBillCount}/${upcoming.length}，继续下滑加载`
            : `已显示全部 ${upcoming.length} 条`);
    };

    renderNextBills();
    scrollBox.addEventListener('scroll', () => {
        if (renderedBillCount >= upcoming.length) return;
        const nearBottom = scrollBox.scrollTop + scrollBox.clientHeight >= scrollBox.scrollHeight - 80;
        if (nearBottom) renderNextBills();
    });
}

// 右侧：交易明细 (Transaction Table)
const walletCol = gridRow.createEl('div', { cls: 'fin-col' });
const walletCard = walletCol.createEl('div', { cls: 'fin-card wallet-transaction-section' });
const walletHeader = walletCard.createEl('div', { cls: 'fin-card-header' });
const walletTitleWrap = walletHeader.createEl('div', { cls: 'tx-section-title-wrap' });
const walletTitle = walletTitleWrap.createEl('div', { cls: 'fin-title', text: '📝 交易明细' });
const walletMeta = walletTitleWrap.createEl('div', { cls: 'tx-section-meta', text: '默认显示全部交易' });
const exportBtn = walletHeader.createEl('button', { text: '📤 导出 CSV', cls: 'fin-btn' });
exportBtn.style.cssText = "padding: 2px 8px; font-size: 0.8em; border-radius: 4px; border: 1px solid var(--c-border); background: var(--c-bg-hover); cursor: pointer;";

const txDirectionHost = walletCard.createEl('div', { cls: 'tx-direction-host' });
const txFilterHost = walletCard.createEl('div');
const txDebugHost = input?.debug ? walletCard.createEl('div') : null;
const scrollBox = walletCard.createEl('div', { cls: 'scroll-container' });
const table = scrollBox.createEl('table', { cls: 'wallet-table' });
table.innerHTML = `
    <thead>
        <tr>
            <th>描述</th>
            <th>日期</th>
            <th>金额</th>
            <th>标签</th>
        </tr>
    </thead>
    <tbody></tbody>`;
const tbody = table.querySelector('tbody');
const txCardList = scrollBox.createEl('div', { cls: 'wallet-card-list' });
const txPager = scrollBox.createEl('div', {
    cls: 'wallet-load-more-footer',
    style: 'display:flex; flex-direction:column; align-items:center; gap:6px; padding:10px 0 4px;',
});
const txPagerStatus = txPager.createEl('div', {
    cls: 'wallet-load-more-status',
    style: 'font-size:0.78em; color:var(--c-text-muted);',
});
const txLoadMoreBtn = txPager.createEl('button', {
    cls: 'wallet-load-more-button',
    text: '加载更多',
    style: 'min-height:40px; padding:4px 14px; border-radius:6px; cursor:pointer;',
});
const txSentinel = txPager.createEl('div', {
    cls: 'wallet-load-more-sentinel',
    attr: { 'aria-hidden': 'true' },
    style: 'width:1px; height:1px;',
});

function toWalletDateTime(value) {
    const ts = ViewKit.toTimestamp(value);
    const date = new Date(ts || Date.now());
    return {
        ts,
        toMillis() { return ts; },
        toFormat(format) {
            const yyyy = date.getFullYear();
            const MM = String(date.getMonth() + 1).padStart(2, '0');
            const dd = String(date.getDate()).padStart(2, '0');
            if (format === "MM-dd") return `${MM}-${dd}`;
            return `${yyyy}-${MM}-${dd}`;
        },
        valueOf() { return ts; },
    };
}

const startDate = fm[CONFIG.frontmatterKeys.startDate];
const endDate = fm[CONFIG.frontmatterKeys.endDate];
const maxRows = Math.max(0, Number(fm[CONFIG.frontmatterKeys.maxRows]) || 0);

const txItems = w.transactions.map(t => {
    const ctime = toWalletDateTime(t.time);
    return {
        ...t,
        ctime,
        vec: [Number(t.value) || 0, 0, 0],
        cleanText: t.text || t.displayText || "",
        displayText: t.displayText || t.text || "",
        tags: t.tags || [],
    };
});
const txDirectionLabels = { all: "全部", income: "收入", expense: "支出" };
let txDirection = "all";
let txBaseFiltered = txItems;
let visibleTx = txItems;
let isMobileView = false;
let txRenderLimit = 0;
let txInteractionState = {};

function getTxAmount(item) {
    return Number(item?.vec?.[0] ?? item?.value ?? 0) || 0;
}

function getDirectionCounts(data) {
    return {
        all: data.length,
        income: data.filter(item => getTxAmount(item) > 0).length,
        expense: data.filter(item => getTxAmount(item) < 0).length,
    };
}

function applyDirection(data, direction) {
    if (direction === "income") return data.filter(item => getTxAmount(item) > 0);
    if (direction === "expense") return data.filter(item => getTxAmount(item) < 0);
    return data;
}

function getDirectionClass(amount) {
    if (amount > 0) return "income";
    if (amount < 0) return "expense";
    return "neutral";
}

function getDirectionText(amount) {
    if (amount > 0) return "收入";
    if (amount < 0) return "支出";
    return "零额";
}

function renderDirectionTabs(counts) {
    ViewKit.renderSegmentedControl(txDirectionHost, {
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
            renderTransactionView();
        },
    });
}

function detectMobileView() {
    const width = walletCard.clientWidth || container.clientWidth || (typeof window !== "undefined" ? window.innerWidth : 9999);
    return Boolean(
        (typeof app !== "undefined" && app.isMobile) ||
        (typeof document !== "undefined" && document.body?.classList?.contains("is-mobile")) ||
        width < 520
    );
}

// 绑定导出事件
exportBtn.onclick = () => {
    Utils.exportToCSV(visibleTx, `${w.name}_交易明细_${new Date().toISOString().split('T')[0]}.csv`);
};

function getLimitedTransactions(data) {
    return maxRows > 0 ? data.slice(0, maxRows) : data;
}

function renderTransactions(data, options = {}) {
    visibleTx = data;
    isMobileView = detectMobileView();
    tbody.innerHTML = "";
    txCardList.innerHTML = "";
    table.style.display = isMobileView ? "none" : "table";
    txCardList.style.display = isMobileView ? "flex" : "none";
    const limitedTx = getLimitedTransactions(data);
    if (options.reset !== false || txRenderLimit <= 0) {
        txRenderLimit = Math.min(TRANSACTION_PAGE_SIZE, limitedTx.length);
    } else {
        txRenderLimit = Math.min(txRenderLimit, limitedTx.length);
    }
    const displayTx = limitedTx.slice(0, txRenderLimit);
    const directionName = txDirectionLabels[txDirection] || txDirectionLabels.all;
    walletTitle.setText(`📝 交易明细 · ${directionName} (${data.length}/${txBaseFiltered.length})`);
    walletMeta.setText(`已显示 ${displayTx.length} / ${limitedTx.length}`);
    txPagerStatus.setText(`已显示 ${displayTx.length} / ${limitedTx.length}`);
    txLoadMoreBtn.style.display = displayTx.length < limitedTx.length ? "" : "none";

    if (limitedTx.length === 0) {
        if (!isMobileView) {
            const row = tbody.insertRow();
            row.innerHTML = `<td colspan="4"><div class="tx-empty">无交易记录</div></td>`;
        } else {
            txCardList.createEl('div', { cls: 'tx-empty', text: '无交易记录' });
        }
        return;
    }

    for (const t of displayTx) {
        const amount = getTxAmount(t);
        const directionClass = getDirectionClass(amount);
        const directionBadge = txDirection === "all"
            ? `<span class="tx-amount-badge ${directionClass}">${getDirectionText(amount)}</span>`
            : "";
        const amountText = `${amount > 0 ? '+' : ''}${ViewKit.fmtMoney(amount)}`;
        const dateStr = t.ctime?.toFormat ? t.ctime.toFormat("yyyy-MM-dd") : "";
        const txText = ViewKit.renderDisplayParts(t.displayParts, { fallback: t.displayText || t.text });
        const srcBtn = ViewKit.renderSourceButton(t, {
            style: "text-decoration:none; color:var(--c-text-muted); font-size:0.85em; flex-shrink:0; margin-right:4px;",
        });
        const sourceLinksHtml = (t.sourceLinks || [])
            .map(link => `<a class="internal-link badge" href="${esc(link.target)}" data-href="${esc(link.target)}" target="_blank" rel="noopener" title="${esc(link.label || '来源')}" style="text-decoration:none;">来源</a>`)
            .join('');
        const tagsHtml = (t.tags || [])
            .filter(tag => tag !== '记账')
            .map(tag => `<span class="badge">${esc(String(tag).replace(/^#/, ''))}</span>`).join('');

        if (!isMobileView) {
            const row = tbody.insertRow();
            row.innerHTML = `
                <td>
                    <div style="display:flex; align-items:center; font-weight:500; color:var(--c-text-main);">${srcBtn}${txText}</div>
                </td>
                <td class="num-cell" style="color:var(--c-text-muted); font-size:0.9em">${dateStr}</td>
                <td class="num-cell" style="font-weight:bold; color:${amount > 0 ? 'var(--c-success)' : amount < 0 ? 'var(--c-danger)' : 'var(--c-text-muted)'}">${amountText}</td>
                <td class="num-cell"><div style="display:flex; gap:4px; justify-content:center; flex-wrap:wrap;">${directionBadge}${tagsHtml}${sourceLinksHtml}</div></td>
            `;
        } else {
            const card = txCardList.createEl('div', { cls: 'wallet-tx-card' });
            card.innerHTML = `
                <div class="wallet-tx-top">
                    <div class="wallet-tx-desc">${srcBtn}${txText}</div>
                    <div class="wallet-tx-amount ${amount > 0 ? 'text-success' : amount < 0 ? 'text-danger' : 'text-muted'}">${amountText}</div>
                </div>
                <div class="wallet-tx-meta">
                    <span>${dateStr || '-'}</span>
                    <div class="wallet-tx-tags">${directionBadge}${tagsHtml}${sourceLinksHtml}</div>
                </div>
            `;
        }
    }
}

function renderTransactionView(data = txBaseFiltered, options = {}) {
    if (Array.isArray(data)) txBaseFiltered = data;
    const counts = getDirectionCounts(txBaseFiltered);
    renderDirectionTabs(counts);
    renderTransactions(applyDirection(txBaseFiltered, txDirection), options);
    renderWalletTransactionDebug(txInteractionState);
}

function renderWalletTransactionDebug(state = {}) {
    if (!txDebugHost) return;
    txDebugHost.innerHTML = "";
    ViewKit.renderDebugPanel(txDebugHost, {
        title: "Debug",
        interaction: state,
        rows: [
            ["view", "WalletProfile:transaction"],
            ["target", dv.current().file.path],
            ["source", "Wallet legacy compatibility layer"],
            ["source transactions", txItems.length],
            ["filtered transactions", txBaseFiltered.length],
            ["visible transactions", visibleTx.length],
            ["direction", txDirection],
            ["available tags", ViewKit.collectTags(txItems).length],
            ["warnings", 0],
            ["interaction", state || {}],
        ],
    });
}

txLoadMoreBtn.onclick = () => {
    const limitedTx = getLimitedTransactions(visibleTx);
    txRenderLimit = Math.min(txRenderLimit + TRANSACTION_PAGE_SIZE, limitedTx.length);
    renderTransactions(visibleTx, { reset: false });
};

if (typeof IntersectionObserver !== 'undefined') {
    const txObserver = new IntersectionObserver(entries => {
        if (entries.some(entry => entry.isIntersecting) && txLoadMoreBtn.style.display !== "none") {
            txLoadMoreBtn.click();
        }
    }, { root: scrollBox, rootMargin: '80px 0px' });
    txObserver.observe(txSentinel);
}

if (typeof ResizeObserver !== 'undefined') {
    const ro = new ResizeObserver(() => {
        const next = detectMobileView();
        if (next !== isMobileView) {
            isMobileView = next;
            renderTransactions(visibleTx, { reset: false });
        }
    });
    ro.observe(walletCard);
}

new ViewKit.FilterBar(txFilterHost, {
    controls: ['search', 'tags', 'links', 'dateRange', 'sort'],
    availableTags: ViewKit.collectTags(txItems),
    sortFields: ViewKit.filterSortFields(['date', 'money']),
    initial: { startDate: startDate || '', endDate: endDate || '', sort: 'date', sortAsc: false },
    storageKey: `${dv.current().file.path}:WalletProfile:transaction`,
    onFilter: (filtered, state) => {
        txInteractionState = state || {};
        renderTransactionView(filtered);
    },
}).bind(txItems);

} // end else (transaction view)
