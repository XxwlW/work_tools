/**
 * 通用对象画像视图 (ObjectProfile.js) v0.1
 *
 * 用法:
 * await dv.view("11 scripts/Views/ObjectProfile", { preset: "topic" });
 *
 * 核心模型:
 * - 当前页是对象锚点
 * - Query linkedTo 收集相关原子
 * - preset 决定模块组合和展示侧重点
 */

const core = {};
await dv.view("11 scripts/Core/FinanceCore", core);
const { Utils: CoreUtils, Query } = core;
const viewKit = {};
await dv.view("11 scripts/Core/ViewKit", viewKit);
const { ViewKit } = viewKit;
const viewQuery = {};
await dv.view("11 scripts/Core/ViewQuery", viewQuery);
const { ViewQuery } = viewQuery;

const esc = value => ViewKit.escapeHtml(value);
const current = dv.current();
const fm = current?.file?.frontmatter || {};

const presetDefs = {
    topic: {
        label: "主题画像",
        modules: ["timeline", "relations", "finance", "sources"],
        accent: "var(--interactive-accent)",
        empty: "暂无关联原子",
    },
    person: {
        label: "人物画像",
        modules: ["timeline", "emotion", "finance", "relations", "sources"],
        accent: "var(--color-purple)",
        empty: "暂无人物相关原子",
    },
    project: {
        label: "项目画像",
        modules: ["progress", "timeline", "finance", "relations", "sources"],
        accent: "var(--color-blue)",
        empty: "暂无项目相关原子",
    },
    wallet: {
        label: "钱包事件画像",
        modules: ["timeline", "finance", "relations", "sources"],
        accent: "var(--color-cyan)",
        empty: "暂无钱包相关事件",
    },
    asset: {
        label: "资产画像",
        modules: ["lifecycle", "finance", "timeline", "sources"],
        accent: "var(--color-orange)",
        empty: "暂无资产生命周期原子",
    },
    source: {
        label: "来源证据",
        modules: ["sources", "timeline", "finance", "relations"],
        accent: "var(--color-green)",
        empty: "暂无引用当前页的原子",
    },
};

function autoPreset(page) {
    const tags = CoreUtils.normalizeSupertagInput(page?.file?.frontmatter?.tags);
    const types = CoreUtils.normalizeArrayField(page?.file?.frontmatter?.["类型"] ?? page?.file?.frontmatter?.type);
    const tokens = new Set([...tags, ...types].map(t => String(t).replace(/^#/, "")));
    if ([...tokens].some(t => ["钱包", "信用卡", "储蓄卡", "现金", "平台账户"].includes(t))) return "wallet";
    if ([...tokens].some(t => ["人物", "人"].includes(t))) return "person";
    if (tokens.has("项目")) return "project";
    if ([...tokens].some(t => ["资产", "设备", "订阅", "服务"].includes(t))) return "asset";
    if ([...tokens].some(t => ["来源", "证据", "订单", "资料"].includes(t))) return "source";
    return "topic";
}

const presetKey = String(input?.preset || input?.["类型"] || fm.objectPreset || fm["对象画像"] || autoPreset(current)).trim();
const preset = presetDefs[presetKey] || presetDefs.topic;

const maxSourcePages = input?.maxPages || input?.["最大页面数"] || fm.maxPages || fm["最大页面数"];
const objectData = {
    targetPage: current,
    targetFile: current?.file || null,
    targetPath: current?.file?.path || "",
    targetName: current?.file?.name || "",
    warnings: [],
    metrics: {},
};

const querySources = {
    linkedTo: objectData.targetPath || true,
    ...(maxSourcePages ? { maxPages: maxSourcePages } : {}),
};
const rules = {
    explicitTarget: true,
    ...(input?.filters || {}),
};

function isTransferEntry(entry) {
    return (entry?.meta?.tags || []).includes("转账");
}

function makePresetConsume(key) {
    return {
        entry(entry) {
            return {
                money: entry?.type === "journal" && !isTransferEntry(entry) ? (entry.vector?.money || 0) : 0,
                emotion: entry?.vector?.emotion || 0,
                time: entry?.type === "event" ? (entry.vector?.time || 0) : 0,
                lifeDays: entry?.meta?.lifeDays || 0,
                entryType: entry?.type,
            };
        },
        include(consumption) {
            if (key === "asset") {
                return consumption.lifeDays > 0 || consumption.entryType === "event";
            }
            return true;
        },
    };
}

function isCurrentObjectLink(link) {
    if (!objectData.targetFile) return false;
    const key = ViewKit.normalizeFilterLink(link);
    const label = ViewKit.linkLabel(link);
    return CoreUtils.linkMatchesTarget(key, objectData.targetFile)
        || CoreUtils.linkMatchesTarget(label, objectData.targetFile);
}

function collectObjectProfileDataset(interaction = {}) {
    objectData.interaction = interaction;
    const dataset = ViewQuery.collect({
        Query,
        ViewKit,
        source: { querySources },
        rules,
        consume: makePresetConsume(presetKey),
        interaction,
        excludeLink: isCurrentObjectLink,
    });
    objectData.entries = dataset.sourceEntries;
    objectData.warnings = dataset.warnings || [];
    objectData.metrics = dataset.metrics || {};
    objectData.queryMetrics = dataset.queryMetrics || {};
    if (input && typeof input === "object") input.objectProfileDataset = dataset;
    return dataset;
}

function entryToObjectItem(entry) {
    const item = CoreUtils.entryToViewItem(entry, {
        targetFile: objectData.targetFile,
        target: objectData.targetFile,
        dv,
    });
    if (!item) return null;
    return {
        ...item,
        entry,
        entryType: entry.type,
        sourcePath: entry.sourcePath,
        sourcePage: entry.sourcePage,
        lineIndex: entry.lineIndex,
        lifeDays: entry.meta?.lifeDays || 0,
        actualDays: entry.meta?.actualDays ?? null,
        retiredDate: entry.meta?.retiredDate ?? null,
        billingDate: entry.meta?.billingDate ?? null,
    };
}

function datasetToItems(dataset) {
    return (dataset?.visibleEntries || [])
        .map(entryToObjectItem)
        .filter(Boolean);
}

let currentDataset = collectObjectProfileDataset();
let items = datasetToItems(currentDataset);

const styles = `
.op-container {
    --op-accent: ${preset.accent};
    --op-bg-card: var(--background-secondary);
    --op-bg-hover: var(--background-secondary-alt);
    --op-border: var(--background-modifier-border);
    --op-muted: var(--text-muted);
    --op-normal: var(--text-normal);
    --op-success: var(--color-green);
    --op-danger: var(--color-red);
    max-width: 100%;
    overflow-x: hidden;
    container-type: inline-size;
    container-name: object-profile;
    font-family: var(--font-interface);
}
.op-head {
    display: flex;
    align-items: flex-start;
    justify-content: space-between;
    gap: 12px;
    margin-bottom: 12px;
}
.op-title { min-width: 0; }
.op-title-main { font-size: 1.15em; font-weight: 700; color: var(--op-normal); line-height: 1.25; }
.op-title-sub { color: var(--op-muted); font-size: 0.82em; margin-top: 3px; }
.op-pill {
    flex: 0 0 auto;
    border: 1px solid color-mix(in srgb, var(--op-accent) 48%, transparent);
    background: color-mix(in srgb, var(--op-accent) 12%, transparent);
    color: var(--op-accent);
    border-radius: 999px;
    padding: 4px 10px;
    font-size: 0.8em;
    white-space: nowrap;
}
.op-dashboard {
    display: grid;
    grid-template-columns: repeat(5, minmax(0, 1fr));
    gap: 8px;
    margin-bottom: 12px;
}
.op-kpi {
    min-width: 0;
    border: 1px solid var(--op-border);
    border-radius: 8px;
    background: var(--op-bg-card);
    padding: 9px 10px;
}
.op-kpi-label { color: var(--op-muted); font-size: 0.75em; margin-bottom: 3px; }
.op-kpi-value { color: var(--op-normal); font-size: 1.05em; font-weight: 700; overflow: hidden; text-overflow: ellipsis; white-space: nowrap; }
.op-grid {
    display: grid;
    grid-template-columns: minmax(0, 1.35fr) minmax(260px, 0.65fr);
    gap: 12px;
}
.op-section {
    min-width: 0;
    border: 1px solid var(--op-border);
    border-radius: 8px;
    background: var(--background-primary);
    overflow: hidden;
    margin-bottom: 12px;
}
.op-section-title {
    display: flex;
    justify-content: space-between;
    gap: 8px;
    align-items: center;
    padding: 9px 11px;
    border-left: 4px solid var(--op-accent);
    border-bottom: 1px solid var(--op-border);
    background: color-mix(in srgb, var(--op-bg-card) 78%, transparent);
    font-weight: 700;
}
.op-section-count { color: var(--op-muted); font-weight: 400; font-size: 0.82em; }
.op-list { padding: 8px; display: flex; flex-direction: column; gap: 7px; }
.op-item {
    border: 1px solid color-mix(in srgb, var(--op-border) 76%, transparent);
    border-radius: 7px;
    background: var(--op-bg-card);
    padding: 8px 9px;
    min-width: 0;
}
.op-item-main { display: flex; align-items: flex-start; gap: 7px; min-width: 0; }
.op-source-btn {
    flex: 0 0 auto;
    min-width: 24px;
    min-height: 24px;
    display: inline-flex;
    align-items: center;
    justify-content: center;
    border: 1px solid var(--op-border);
    border-radius: 6px;
    color: var(--op-muted);
    text-decoration: none;
    font-size: 0.78em;
}
.op-text { min-width: 0; flex: 1; line-height: 1.35; overflow-wrap: anywhere; }
.op-meta { display: flex; flex-wrap: wrap; gap: 5px; margin-top: 6px; color: var(--op-muted); font-size: 0.78em; }
.op-badge {
    display: inline-flex;
    align-items: center;
    min-height: 22px;
    padding: 0 7px;
    border-radius: 999px;
    border: 1px solid var(--op-border);
    background: color-mix(in srgb, var(--background-primary) 72%, transparent);
    white-space: nowrap;
}
.op-money-pos { color: var(--op-success); }
.op-money-neg { color: var(--op-danger); }
.op-timeline { position: relative; padding-left: 16px; }
.op-timeline::before {
    content: '';
    position: absolute;
    left: 5px;
    top: 0;
    bottom: 0;
    width: 2px;
    background: var(--op-border);
    border-radius: 1px;
}
.op-timeline-item { position: relative; margin-bottom: 10px; padding-left: 12px; }
.op-timeline-dot {
    position: absolute;
    left: -16px;
    top: 9px;
    width: 8px;
    height: 8px;
    border-radius: 50%;
    background: var(--op-bg-card);
    border: 2px solid var(--op-accent);
    z-index: 1;
}
.op-timeline-content {
    border: 1px solid color-mix(in srgb, var(--op-border) 76%, transparent);
    border-radius: 7px;
    background: var(--op-bg-card);
    padding: 8px 9px;
    display: flex;
    justify-content: space-between;
    gap: 8px;
    min-width: 0;
}
.op-timeline-main { min-width: 0; flex: 1; }
.op-timeline-date { color: var(--op-muted); font-size: 0.76em; display: block; margin-bottom: 3px; }
.op-timeline-text { line-height: 1.35; overflow-wrap: anywhere; }
.op-timeline-meta { display: flex; flex-wrap: wrap; justify-content: flex-end; gap: 5px; align-items: flex-start; }
.op-rank-row {
    display: grid;
    grid-template-columns: minmax(0, 1fr) auto;
    gap: 8px;
    align-items: center;
    padding: 7px 0;
    border-bottom: 1px solid color-mix(in srgb, var(--op-border) 64%, transparent);
}
.op-rank-row:last-child { border-bottom: none; }
.op-rank-name { min-width: 0; overflow: hidden; text-overflow: ellipsis; white-space: nowrap; }
.op-rank-meta { color: var(--op-muted); font-size: 0.8em; white-space: nowrap; }
.op-empty {
    padding: 18px;
    color: var(--op-muted);
    text-align: center;
    border: 1px dashed var(--op-border);
    border-radius: 8px;
    background: color-mix(in srgb, var(--op-bg-card) 55%, transparent);
}
.op-container .vk-display-link { overflow-wrap: anywhere; }
@container object-profile (max-width: 900px) {
    .op-dashboard { grid-template-columns: repeat(3, minmax(0, 1fr)); }
    .op-grid { grid-template-columns: 1fr; }
}
@container object-profile (max-width: 560px) {
    .op-head { flex-direction: column; }
    .op-dashboard { grid-template-columns: repeat(2, minmax(0, 1fr)); }
    .op-pill { white-space: normal; }
}
`;

dv.container.innerHTML = `<style>${styles}</style>`;
const container = dv.container.createEl("div", { cls: "op-container" });

const header = container.createEl("div", { cls: "op-head" });
const title = header.createEl("div", { cls: "op-title" });
title.createEl("div", { cls: "op-title-main", text: current?.file?.name || objectData.targetName || "对象画像" });
title.createEl("div", { cls: "op-title-sub", text: "以当前页为锚点，聚合所有链接到它的记录原子" });
header.createEl("div", { cls: "op-pill", text: `${preset.label} · ${presetKey}` });

const filterHost = container.createEl("div");
const dashboard = container.createEl("div", { cls: "op-dashboard" });
const mainGrid = container.createEl("div", { cls: "op-grid" });

function moneyText(value) {
    const num = Number(value || 0);
    return `${num < 0 ? "-" : ""}¥${Math.abs(num).toLocaleString("en-US", { maximumFractionDigits: 1 })}`;
}

function dateText(item) {
    const ts = item?.ctime?.ts || item?.ctime?.toMillis?.();
    if (!ts) return "";
    const d = new Date(ts);
    return `${d.getFullYear()}-${String(d.getMonth() + 1).padStart(2, "0")}-${String(d.getDate()).padStart(2, "0")}`;
}

function summarize(list) {
    const finance = list.filter(item => item.entryType === "journal" && !(item.tags || []).includes("转账"));
    const emotion = list.filter(item => item.vec?.[1] !== 0);
    return {
        count: list.length,
        events: list.filter(item => item.entryType === "event").length,
        journals: list.filter(item => item.entryType === "journal").length,
        time: list.reduce((sum, item) => sum + (item.vec?.[2] || 0), 0),
        money: finance.reduce((sum, item) => sum + (item.vec?.[0] || 0), 0),
        sources: new Set(list.flatMap(item => (item.sourceLinks || []).map(link => link.target))).size,
        avgEmotion: emotion.length ? emotion.reduce((sum, item) => sum + (item.vec?.[1] || 0), 0) / emotion.length : 0,
    };
}

function makeSection(parent, titleText, count, renderBody) {
    const module = ViewKit.renderModuleShell(parent, {
        title: titleText,
        count,
        countSuffix: " 项",
        shellClass: "op-section",
        headerClass: "op-section-title",
        countClass: "op-section-count",
        bodyClass: "op-list",
    });
    const body = module.body;
    renderBody(body);
    return module.shell;
}

function renderItem(parent, item) {
    const card = parent.createEl("div", { cls: "op-item" });
    const main = card.createEl("div", { cls: "op-item-main" });
    main.innerHTML = ViewKit.renderSourceButton(item, { className: "op-source-btn", label: "源", style: "" });
    const text = main.createEl("div", { cls: "op-text" });
    text.innerHTML = ViewKit.renderDisplayParts(item.displayParts, { fallback: item.displayText || item.text || item.path });
    const meta = card.createEl("div", { cls: "op-meta" });
    meta.createEl("span", { cls: "op-badge", text: dateText(item) || "无日期" });
    meta.createEl("span", { cls: "op-badge", text: item.entryType === "journal" ? "账务" : "事件" });
    if (item.vec?.[2]) meta.createEl("span", { cls: "op-badge", text: `时间 ${item.vec[2]}` });
    if (item.vec?.[1]) meta.createEl("span", { cls: "op-badge", text: `情绪 ${item.vec[1] > 0 ? "+" : ""}${item.vec[1]}` });
    if (item.vec?.[0]) {
        const money = meta.createEl("span", {
            cls: `op-badge ${item.vec[0] > 0 ? "op-money-pos" : "op-money-neg"}`,
            text: moneyText(item.vec[0]),
        });
        money.setAttr?.("title", "金额");
    }
    if (item.lifeDays) meta.createEl("span", { cls: "op-badge", text: `LIFE ${item.lifeDays}天` });
    (item.tags || []).slice(0, 4).forEach(tag => meta.createEl("span", { cls: "op-badge", text: `#${tag}` }));
}

function relationRows(list) {
    const map = new Map();
    for (const item of list) {
        for (const link of item.linksDetailed || []) {
            if (!link.target || link.role === "source" || link.role === "self") continue;
            if (objectData.targetFile && CoreUtils.linkMatchesTarget(link.target, objectData.targetFile)) continue;
            const key = String(link.target).replace(/\.md$/, "");
            if (!map.has(key)) map.set(key, { target: link.target, label: link.label || key.split(/[\\/]/).pop(), count: 0, money: 0, time: 0, evidence: [] });
            const row = map.get(key);
            row.count += 1;
            row.money += item.vec?.[0] || 0;
            row.time += item.vec?.[2] || 0;
            row.evidence.push(item);
        }
    }
    return Array.from(map.values()).sort((a, b) => b.count - a.count || Math.abs(b.money) - Math.abs(a.money)).slice(0, 12);
}

function sourceRows(list) {
    const map = new Map();
    for (const item of list) {
        for (const link of item.sourceLinks || []) {
            const key = String(link.target || "").replace(/\.md$/, "");
            if (!key) continue;
            if (!map.has(key)) map.set(key, { target: link.target, label: link.label || key.split(/[\\/]/).pop(), count: 0, money: 0, evidence: [] });
            const row = map.get(key);
            row.count += 1;
            row.money += item.vec?.[0] || 0;
            row.evidence.push(item);
        }
    }
    return Array.from(map.values()).sort((a, b) => b.count - a.count || Math.abs(b.money) - Math.abs(a.money)).slice(0, 12);
}

function renderRank(parent, rows, kind) {
    return ViewKit.renderRankList(parent, {
        rows,
        emptyClass: "op-empty",
        emptyText: kind === "source" ? "暂无来源引用" : "暂无关系对象",
        listClass: "op-rank-list",
        rowClass: "op-rank-row",
        nameClass: "op-rank-name",
        metaClass: "op-rank-meta",
        evidenceClass: "op-rank-evidence",
        evidenceLinkClass: "op-source-btn",
        evidenceLabel: "证据",
        formatMeta: row => {
            const parts = [`${row.count} 次`];
            if (row.money) parts.push(moneyText(row.money));
            if (row.time) parts.push(`${row.time}h`);
            return parts.join(" · ");
        },
    });
}

function updateDashboard(list) {
    const s = summarize(list);
    dashboard.innerHTML = "";
    [
        ["原子", s.count],
        ["事件", s.events],
        ["账务", s.journals],
        ["时间", `${s.time.toFixed(1)}`],
        ["净额", moneyText(s.money)],
    ].forEach(([label, value]) => {
        const card = dashboard.createEl("div", { cls: "op-kpi" });
        card.createEl("div", { cls: "op-kpi-label", text: label });
        const val = card.createEl("div", { cls: "op-kpi-value", text: String(value) });
        if (label === "净额") val.style.color = s.money >= 0 ? "var(--op-success)" : "var(--op-danger)";
    });
}

function renderLifecycle(parent, list) {
    const rows = list.filter(item => item.lifeDays > 0);
    makeSection(parent, "生命周期", rows.length, body => {
        if (!rows.length) {
            body.createEl("div", { cls: "op-empty", text: "暂无 LIFE 原子" });
            return;
        }
        rows.forEach(item => renderItem(body, item));
    });
}

function renderAll(list) {
    updateDashboard(list);
    mainGrid.innerHTML = "";
    const primary = mainGrid.createEl("div");
    const side = mainGrid.createEl("aside");

    if (!list.length) {
        primary.createEl("div", { cls: "op-empty", text: preset.empty });
        return;
    }

    if (preset.modules.includes("lifecycle")) renderLifecycle(primary, list);
    makeSection(primary, "相关原子", list.length, body => {
        ViewKit.renderTimeline(body, {
            items: list,
            progressive: true,
            pageSize: Number(input?.pageSize || input?.["批量"] || 30) || 30,
            timelineClass: "op-timeline",
            itemClass: "op-timeline-item",
            dotClass: "op-timeline-dot",
            contentClass: "op-timeline-content",
            mainClass: "op-timeline-main",
            dateClass: "op-timeline-date",
            textClass: "op-timeline-text",
            metaClass: "op-timeline-meta",
            badgeClass: "op-badge",
            timeBadgeClass: "op-badge",
            emotionBadgeClass: "op-badge",
            incomeBadgeClass: "op-badge op-money-pos",
            expenseBadgeClass: "op-badge op-money-neg",
            sourceButtonOptions: { className: "op-source-btn", label: "源", style: "" },
        });
    });

    if (preset.modules.includes("finance")) {
        const finance = list.filter(item => item.entryType === "journal");
        makeSection(side, "账务侧重点", finance.length, body => {
            if (!finance.length) body.createEl("div", { cls: "op-empty", text: "暂无账务原子" });
            else finance.slice(0, 12).forEach(item => renderItem(body, item));
        });
    }
    if (preset.modules.includes("relations")) {
        const rows = relationRows(list);
        makeSection(side, "关系对象", rows.length, body => renderRank(body, rows, "relation"));
    }
    if (preset.modules.includes("sources")) {
        const rows = sourceRows(list);
        makeSection(side, "来源引用", rows.length, body => renderRank(body, rows, "source"));
    }
    if (input?.debug) {
        ViewKit.renderDebugPanel(side, {
            title: "调试信息",
            dataset: currentDataset,
            interaction: objectData.interaction || {},
            rows: [
                ["preset", presetKey],
                ["source entries", currentDataset?.sourceEntries?.length || 0],
                ["consumed entries", currentDataset?.consumedEntries?.length || 0],
                ["visible entries", currentDataset?.visibleEntries?.length || 0],
                ["available tags", currentDataset?.availableTags?.length || 0],
                ["available links", currentDataset?.availableLinks?.length || 0],
                ["warnings", currentDataset?.warnings?.length || 0],
                ["interaction", objectData.interaction || {}],
                ["query metrics", currentDataset?.queryMetrics || {}],
            ],
            shellClass: "op-section",
            headerClass: "op-section-title",
            countClass: "op-section-count",
            bodyClass: "op-list vk-debug-body",
        });
    }
}

const filterBar = new ViewKit.FilterBar(filterHost, {
    controls: ["search", "tags", "links", "dateRange", "sort"],
    availableTags: currentDataset.availableTags,
    availableLinks: currentDataset.availableLinks,
    sortFields: ViewKit.filterSortFields(["date", "time", "emotion", "money"]),
    initial: { sort: "date", sortAsc: false },
    storageKey: `ObjectProfile:v2:${presetKey}:${current?.file?.path || "ObjectProfile"}`,
    emptyText: "无匹配原子，点击清除过滤器",
    onFilter: (_filtered, state) => {
        currentDataset = collectObjectProfileDataset(state);
        items = datasetToItems(currentDataset);
        renderAll(items);
    },
});
filterBar.bind(currentDataset.filterItems);
