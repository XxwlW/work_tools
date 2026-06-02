/**
 * 日历热力图视图 (CalendarHeatmap.js) v2.0 - 多视图版
 *
 * 支持 6 种 范围×粒度 组合:
 *   年×日 (GitHub热力图) | 年×周 (52周柱状) | 年×月 (12月柱状)
 *   月×日 (日历网格,默认) | 月×周 (周柱状)  | 周×日 (7日详情)
 *
 * 用法:
 * ```dataviewjs
 * await dv.view("11 scripts/Views/CalendarHeatmap", {
 *     模式: "净值",         // "净值" | "专注" | 不传都显示
 *     标签: ["奖励"],       // 可选：脚本级基础约束，FilterBar 会在此基础上继续缩小
 *     范围: "月",           // "年" | "月" | "周"，默认 "月"
 *     粒度: "日",           // "月" | "周" | "日"，默认 "日"
 *     关联: true,           // true | "页面名" | 不传
 *     全局: true,           // true 时扫描全部 md；默认也是全库
 *     最大页面数: 800,      // 可选：全局/目录扫描预算，超出后在视图中显示警告
 *     数据源路径: "01 日记", // 可选：只有需要限制目录时才传
 * });
 * ```
 */

// --- 1. 导入核心库 ---
const core = {};
await dv.view("11 scripts/Core/FinanceCore", core);
const { Utils, Query } = core;
const viewKit = {};
await dv.view("11 scripts/Core/ViewKit", viewKit);
const { ViewKit } = viewKit;
const viewQuery = {};
await dv.view("11 scripts/Core/ViewQuery", viewQuery);
const { ViewQuery } = viewQuery;

// --- 2. 配置 ---
const fm = dv.current()?.file?.frontmatter || {};
const rawDataPath = input?.["数据源路径"] ?? input?.dataPath ?? input?.scope ?? fm["数据源路径"];
const rawGlobal = input?.["全局"] ?? input?.global ?? input?.allowGlobal ?? fm["全局"] ?? fm.global ?? fm.allowGlobal;

function isGlobalSourceValue(value) {
    if (value === true) return true;
    if (value === false || value == null) return false;
    const text = String(value).trim().toLowerCase();
    return ["全局", "全部", "全库", "global", "all", "*"].includes(text);
}

function toPositiveInt(value, fallback) {
    const num = Number(value);
    return Number.isFinite(num) && num > 0 ? Math.floor(num) : fallback;
}

const USE_GLOBAL_SOURCE = isGlobalSourceValue(rawGlobal) || isGlobalSourceValue(rawDataPath) || rawDataPath == null || rawDataPath === "";
const DATA_PATH = USE_GLOBAL_SOURCE ? null : rawDataPath;
const MAX_SOURCE_PAGES = toPositiveInt(
    input?.["最大页面数"] ?? input?.maxPages ?? input?.sourceMaxPages ?? fm["最大页面数"] ?? fm.maxPages ?? fm.sourceMaxPages,
    800
);

const filterRaw = input?.["关联"];
const filterName = filterRaw === true ? dv.current()?.file?.name : (typeof filterRaw === 'string' ? filterRaw : null);
const tagRaw = input?.["标签"];
const baseFilterTags = tagRaw ? (Array.isArray(tagRaw) ? tagRaw : [tagRaw]).map(t => t.replace(/^#/, '')).filter(Boolean) : [];
let activeFilterTags = [];
let activeFilterLinks = [];
let activeFilterMatchMode = "and";

const excludeTagRaw = input?.["排除标签"];
const excludeTags = excludeTagRaw ? (Array.isArray(excludeTagRaw) ? excludeTagRaw : [excludeTagRaw]).map(t => t.replace(/^#/, '')) : null;

const mode = input?.["模式"];
const SCOPE = input?.["范围"] || "月";  // 年 | 月 | 周
const GRAIN = input?.["粒度"] || "日";  // 月 | 周 | 日

function toDateKey(date) {
    if (!date) return null;
    const d = Utils.resolveDateValue ? Utils.resolveDateValue(date) : new Date(date);
    if (!d || isNaN(d.getTime())) return null;
    return `${d.getFullYear()}-${String(d.getMonth()+1).padStart(2,'0')}-${String(d.getDate()).padStart(2,'0')}`;
}

function getISOWeekStart(year, weekNum) {
    const jan4 = new Date(year, 0, 4);
    const start = new Date(jan4);
    start.setDate(jan4.getDate() - (jan4.getDay() + 6) % 7 + (weekNum - 1) * 7);
    start.setHours(0,0,0,0);
    return start;
}

function getInitialDateWindow() {
    const inputStart = input?.["开始时间"] || input?.["startDate"];
    const inputEnd = input?.["结束时间"] || input?.["endDate"];
    return {
        startDate: inputStart ? new Date(inputStart) : null,
        endDate: inputEnd ? new Date(inputEnd) : null,
    };
}

// --- 3. 数据采集 ---
const querySources = USE_GLOBAL_SOURCE
    ? { allowGlobal: true, maxPages: MAX_SOURCE_PAGES }
    : { scope: DATA_PATH, maxPages: MAX_SOURCE_PAGES };
if (filterName && !USE_GLOBAL_SOURCE) {
    const sourcePath = filterRaw === true ? dv.current()?.file?.path : filterName;
    querySources.linkedTo = sourcePath;
}

const inputDateWindow = getInitialDateWindow();
const dailyDataCache = new Map();

function withDayEnd(date) {
    if (!date) return null;
    const d = new Date(date);
    d.setHours(23, 59, 59, 999);
    return d;
}

function getDisplayDateWindow(year, month, week) {
    if (SCOPE === "年") {
        return {
            startDate: new Date(year, 0, 1),
            endDate: new Date(year, 11, 31, 23, 59, 59, 999),
        };
    }

    if (SCOPE === "周") {
        const start = getISOWeekStart(year, week);
        return {
            startDate: start,
            endDate: new Date(start.getFullYear(), start.getMonth(), start.getDate() + 6, 23, 59, 59, 999),
        };
    }

    return {
        startDate: new Date(year, month, 1),
        endDate: new Date(year, month + 1, 0, 23, 59, 59, 999),
    };
}

function getQueryDateWindow(displayWindow) {
    let startDate = displayWindow.startDate;
    let endDate = displayWindow.endDate;

    if (inputDateWindow.startDate && inputDateWindow.startDate > startDate) {
        startDate = inputDateWindow.startDate;
    }
    const inputEnd = withDayEnd(inputDateWindow.endDate);
    if (inputEnd && inputEnd < endDate) {
        endDate = inputEnd;
    }

    return { startDate, endDate };
}

function makeQueryRules(dateWindow) {
    const rules = {
        startDate: dateWindow.startDate,
        endDate: dateWindow.endDate,
    };
    if (filterName) rules.explicitTarget = filterRaw === true ? true : filterName;
    if (baseFilterTags.length > 0) rules.tags = baseFilterTags;
    if (excludeTags && excludeTags.length > 0) rules.excludeTags = excludeTags;
    return rules;
}

function makeTagAvailabilityRules() {
    const rules = {};
    if (filterName) rules.explicitTarget = filterRaw === true ? true : filterName;
    if (baseFilterTags.length > 0) rules.tags = baseFilterTags;
    if (excludeTags && excludeTags.length > 0) rules.excludeTags = excludeTags;
    return rules;
}

function entryConsumption(entry) {
    const tags = entry.meta?.tags || [];
    const isTransfer = tags.includes("转账");
    const money = entry.type === "journal" && !isTransfer ? (entry.vector?.money || 0) : 0;
    const time = entry.type === "event" && entry.vector?.time > 0 ? entry.vector.time : 0;
    return { money, time };
}

function isConsumedByMode(consumption) {
    if (mode === "净值") return consumption.money !== 0;
    if (mode === "专注") return consumption.time !== 0;
    return consumption.money !== 0 || consumption.time !== 0;
}

function isAnchorLink(link) {
    if (!filterName) return false;
    const key = ViewKit.normalizeFilterLink(link);
    const label = ViewKit.linkLabel(link);
    return key === filterName || label === filterName || key.split(/[\\/]/).pop() === filterName;
}

function collectBaseDataset(rules, interaction = {}, options = {}) {
    return ViewQuery.collect({
        Query,
        ViewKit,
        source: options.sourceEntries ? { sourceEntries: options.sourceEntries } : { querySources },
        rules,
        debug: options.debug,
        consume: {
            baseTags: baseFilterTags,
            entry: entryConsumption,
            include: consumption => isConsumedByMode(consumption),
        },
        interaction,
        excludeLink: isAnchorLink,
    });
}

function attachDailyMaps(dataset) {
    dataset.dailyNet = new Map();
    dataset.dailyTime = new Map();
    dataset.dailyNetPaths = new Map();
    dataset.dailyTimePaths = new Map();
    dataset.dailyNetPages = new Map();
    dataset.dailyTimePages = new Map();
    dataset.dailyItems = new Map();
    return dataset;
}

function addEntryToDailyDataset(dataset, entry, dateKey, consumption) {
    const { money, time } = consumption;
    if (money !== 0) {
        dataset.dailyNet.set(dateKey, (dataset.dailyNet.get(dateKey) || 0) + money);
        const chosenPath = chooseDailyPath(dataset.dailyNetPaths.get(dateKey), entry.sourcePath, dataset.dailyNetPages.get(dateKey), entry.sourcePage);
        dataset.dailyNetPaths.set(dateKey, chosenPath);
        dataset.dailyNetPages.set(dateKey, chosenPath === entry.sourcePath ? entry.sourcePage : dataset.dailyNetPages.get(dateKey));
    }
    if (time !== 0) {
        dataset.dailyTime.set(dateKey, (dataset.dailyTime.get(dateKey) || 0) + time);
        const chosenPath = chooseDailyPath(dataset.dailyTimePaths.get(dateKey), entry.sourcePath, dataset.dailyTimePages.get(dateKey), entry.sourcePage);
        dataset.dailyTimePaths.set(dateKey, chosenPath);
        dataset.dailyTimePages.set(dateKey, chosenPath === entry.sourcePath ? entry.sourcePage : dataset.dailyTimePages.get(dateKey));
    }

    const text = entry.cleanText || entry.rawText || entry.sourcePage?.file?.name || "";
    if (text) {
        dataset.dailyItems.set(dateKey, [
            ...(dataset.dailyItems.get(dateKey) || []),
            { text, money, time },
        ]);
    }
}

function finalizeHeatmapDataset(dataset) {
    delete dataset.dailyNetPages;
    delete dataset.dailyTimePages;
    return dataset;
}

function collectAvailableFilters() {
    try {
        const dataset = collectBaseDataset(makeTagAvailabilityRules());
        return {
            tags: dataset.availableTags,
            links: dataset.availableLinks,
            items: dataset.filterItems,
            dataset,
        };
    } catch (error) {
        console.warn("CalendarHeatmap filter collection failed", error);
        return { tags: baseFilterTags, links: [], items: [] };
    }
}

function collectHeatmapDataset(dateWindow) {
    const dataset = attachDailyMaps(collectBaseDataset(makeQueryRules(dateWindow), {
        tags: activeFilterTags,
        links: activeFilterLinks,
        matchMode: activeFilterMatchMode,
    }, {
        debug: true,
        sourceEntries: dateWindow.startDate > dateWindow.endDate ? [] : null,
    }));

    for (const entry of dataset.visibleEntries) {
        const consumption = dataset.consumptionByEntry.get(entry) || entryConsumption(entry);
        const dateKey = toDateKey(Utils.resolveEntryDate ? Utils.resolveEntryDate(entry) : entry.meta?.explicitDate);
        if (!dateKey) continue;
        addEntryToDailyDataset(dataset, entry, dateKey, consumption);
    }

    return finalizeHeatmapDataset(dataset);
}

function collectDailyData(dateWindow) {
    return collectHeatmapDataset(dateWindow);
}

function isDiaryPage(page) {
    return Utils.hasObjectSupertag(page, "日记") || Utils.resolveObjectType(page) === "日记";
}

function chooseDailyPath(currentPath, candidatePath, currentPage, candidatePage) {
    if (!currentPath) return candidatePath;
    if (isDiaryPage(currentPage)) return currentPath;
    return isDiaryPage(candidatePage) ? candidatePath : currentPath;
}

function getDailyDataForDisplay(year, month, week) {
    const dateWindow = getQueryDateWindow(getDisplayDateWindow(year, month, week));
    const tagKey = (activeFilterTags || []).join(",");
    const linkKey = (activeFilterLinks || []).join(",");
    const key = `${dateWindow.startDate?.getTime() || ""}:${dateWindow.endDate?.getTime() || ""}:${tagKey}:${linkKey}:${activeFilterMatchMode}`;
    if (!dailyDataCache.has(key)) {
        dailyDataCache.set(key, collectDailyData(dateWindow));
    }
    return dailyDataCache.get(key);
}

// --- 4. 数据聚合工具 ---
function dateToWeekKey(d) {
    const dt = new Date(d); dt.setHours(0,0,0,0);
    const thu = new Date(dt); thu.setDate(dt.getDate() + 3 - (dt.getDay() + 6) % 7);
    const y = thu.getFullYear();
    const w = 1 + Math.round(((thu - new Date(y,0,4)) / 86400000 - 3 + (new Date(y,0,4).getDay() + 6) % 7) / 7);
    return `${y}-W${String(w).padStart(2,'0')}`;
}

function aggregateBy(dataMap, keyFn) {
    const result = new Map();
    for (const [dateKey, val] of dataMap) {
        const k = keyFn(dateKey);
        result.set(k, (result.get(k) || 0) + val);
    }
    return result;
}

function aggByWeek(dataMap) { return aggregateBy(dataMap, k => dateToWeekKey(k)); }
function aggByMonth(dataMap) { return aggregateBy(dataMap, k => k.slice(0,7)); }

// --- 5. 样式 ---
const styles = `
.ch-wrap { font-family: var(--font-interface); display: flex; flex-direction: column; gap: 24px; }
.ch-section {
    background: var(--background-primary);
    border: 1px solid var(--background-modifier-border);
    border-radius: 12px; padding: 20px 24px;
    box-shadow: 0 4px 12px rgba(0, 0, 0, 0.03);
}
.ch-header {
    display: flex; justify-content: space-between; align-items: center;
    margin-bottom: 20px; padding-bottom: 12px;
    border-bottom: 1px solid var(--background-modifier-border-hover);
}
.ch-title { font-size: 1.15em; font-weight: 700; color: var(--text-normal); display: flex; align-items: center; gap: 8px; }
.ch-nav { display: flex; align-items: center; gap: 12px; }
.ch-nav-btn {
    background: transparent; border: none; box-shadow: none;
    border-radius: 6px; padding: 4px 8px; cursor: pointer; font-size: 1.1em;
    color: var(--text-muted); transition: all 0.2s ease;
}
.ch-nav-btn:hover { background: var(--background-modifier-hover); color: var(--text-normal); transform: scale(1.05); }
.ch-nav-label { font-size: 0.95em; font-weight: 600; color: var(--text-normal); min-width: 100px; text-align: center; font-variant-numeric: tabular-nums; }
.ch-body { display: flex; gap: 16px; align-items: stretch; }
.ch-body-grid { flex: 1; min-width: 0; }
.ch-legend {
    width: 50px; display: flex; flex-direction: column; align-items: center;
    justify-content: space-between; padding: 24px 0 4px 0; font-size: 0.65em; color: var(--text-muted);
}
.ch-legend-bar { width: 12px; flex: 1; border-radius: 6px; margin: 4px 0; }
/* 月×日 网格 */
.ch-grid { display: grid; grid-template-columns: 28px repeat(7, 1fr); gap: 6px; }
.ch-dow { font-size: 0.75em; color: var(--text-muted); text-align: center; padding-bottom: 8px; font-weight: 600; }
.ch-week-label { font-size: 0.65em; color: var(--text-faint); display: flex; align-items: center; justify-content: center; font-weight: 500; }
.ch-cell {
    aspect-ratio: 1; border-radius: 6px;
    display: flex; align-items: center; justify-content: center;
    font-size: 0.8em; color: var(--text-muted);
    transition: transform 0.2s cubic-bezier(0.34, 1.56, 0.64, 1), box-shadow 0.2s ease;
    cursor: default; position: relative; background: var(--background-secondary);
}
.ch-cell:hover { transform: scale(1.15); z-index: 10; box-shadow: 0 4px 8px rgba(0,0,0,0.15); }
.ch-cell.has-data { cursor: pointer; color: var(--text-normal); font-weight: 600; }
.ch-cell.empty { background: transparent; pointer-events: none; }
.ch-cell a { color: inherit; text-decoration: none; display: flex; width: 100%; height: 100%; align-items: center; justify-content: center; border-radius: 6px; }
/* 年×日 GitHub 网格 */
.ch-year-grid { display: flex; gap: 2px; overflow-x: auto; padding: 2px 0; }
.ch-year-col { display: flex; flex-direction: column; gap: 2px; }
.ch-year-cell { width: 14px; height: 14px; border-radius: 3px; background: var(--background-secondary); transition: transform 0.15s, box-shadow 0.15s; cursor: default; }
.ch-year-cell:hover { transform: scale(1.4); z-index: 10; box-shadow: 0 2px 6px rgba(0,0,0,0.2); }
.ch-year-cell.has-data { cursor: pointer; }
.ch-year-labels { display: flex; gap: 2px; margin-bottom: 4px; padding-left: 28px; }
.ch-year-mlabel { font-size: 0.65em; color: var(--text-muted); text-align: center; }
.ch-year-dlabel { width: 24px; font-size: 0.6em; color: var(--text-muted); display: flex; align-items: center; justify-content: end; padding-right: 4px; flex-shrink: 0; }
/* 柱状图 (双向) */
.ch-bars-wrap { position: relative; }
.ch-bars-top { display: flex; gap: 4px; align-items: flex-end; height: 80px; border-bottom: 1px solid var(--background-modifier-border); }
.ch-bars-bottom { display: flex; gap: 4px; align-items: flex-start; height: 80px; }
.ch-bars-top .ch-bar-col, .ch-bars-bottom .ch-bar-col { flex: 1; display: flex; flex-direction: column; align-items: center; height: 100%; }
.ch-bars-top .ch-bar-col { justify-content: flex-end; }
.ch-bars-bottom .ch-bar-col { justify-content: flex-start; }
.ch-bar { width: 100%; min-height: 0; transition: transform 0.2s, box-shadow 0.2s; cursor: default; max-width: 36px; }
.ch-bar.up { border-radius: 4px 4px 0 0; }
.ch-bar.down { border-radius: 0 0 4px 4px; }
.ch-bar:hover { box-shadow: 0 0 6px rgba(0,0,0,0.15); }
.ch-bar-labels { display: flex; gap: 4px; margin-top: 4px; }
.ch-bar-label { flex: 1; font-size: 0.6em; color: var(--text-muted); text-align: center; white-space: nowrap; }
/* 周×日 大格 */
.ch-week-detail { display: grid; grid-template-columns: repeat(7, 1fr); gap: 8px; }
.ch-week-cell {
    border-radius: 8px; padding: 12px 8px; text-align: center;
    background: var(--background-secondary); transition: transform 0.2s, box-shadow 0.2s;
    cursor: default; min-height: 60px; display: flex; flex-direction: column; align-items: center; justify-content: center; gap: 4px;
}
.ch-week-cell:hover { transform: scale(1.04); box-shadow: 0 4px 10px rgba(0,0,0,0.12); }
.ch-week-cell.has-data { cursor: pointer; }
.ch-week-cell .day-num { font-size: 1.2em; font-weight: 700; }
.ch-week-cell .day-val { font-size: 0.8em; font-weight: 600; }
.ch-week-cell .day-dow { font-size: 0.7em; color: var(--text-muted); }
.ch-legend-h {
    display: flex; gap: 12px; align-items: center; justify-content: center;
    margin-top: 12px; font-size: 0.65em; color: var(--text-muted);
}
.ch-legend-h-item { display: flex; align-items: center; gap: 6px; }
.ch-legend-h-bar { width: 80px; height: 10px; border-radius: 5px; }
/* 统计栏 */
.ch-stat {
    font-size: 0.85em; color: var(--text-muted); margin-top: 20px;
    display: flex; gap: 20px; justify-content: center; flex-wrap: wrap;
    background: var(--background-secondary-alt); padding: 12px; border-radius: 8px;
}
.ch-stat-item { display: flex; align-items: center; gap: 6px; font-weight: 500; }
.ch-stat-dot { width: 10px; height: 10px; border-radius: 50%; }
.ch-query-warning {
    display: none; margin-top: 10px; padding: 8px 10px; border-radius: 8px;
    background: var(--background-modifier-error); color: var(--text-on-accent);
    font-size: 0.78em; line-height: 1.45;
}
.ch-wrap, .ch-wrap * { box-sizing: border-box; }
.ch-wrap { max-width: 100%; overflow-x: hidden; container-type: inline-size; container-name: view-container; }
.ch-section, .ch-body-grid { min-width: 0; }
.ch-body-grid { max-width: 100%; overflow-x: auto; -webkit-overflow-scrolling: touch; }
.ch-nav-btn { min-width: 36px; min-height: 36px; }
.ch-grid { min-width: 0; }
.ch-year-grid { min-height: 112px; }
.ch-bars-wrap { min-height: 164px; min-width: 260px; }
.ch-week-detail { min-width: 0; }
@container view-container (max-width: 760px) {
    .ch-section { padding: 14px; }
    .ch-header { align-items: flex-start; gap: 10px; flex-wrap: wrap; margin-bottom: 14px; }
    .ch-nav { width: 100%; justify-content: space-between; gap: 8px; }
    .ch-nav-label { flex: 1; min-width: 0; }
    .ch-body { gap: 10px; }
    .ch-legend { width: 38px; }
    .ch-week-detail { grid-template-columns: repeat(2, minmax(0, 1fr)); }
    .ch-stat { justify-content: flex-start; gap: 10px; }
}
@container view-container (max-width: 520px) {
    .ch-wrap { gap: 14px; }
    .ch-section { padding: 10px; border-radius: 8px; }
    .ch-title { font-size: 1em; min-width: 0; overflow-wrap: anywhere; }
    .ch-body { flex-direction: column; }
    .ch-legend { width: 100%; min-height: 34px; flex-direction: row; justify-content: center; padding: 0; gap: 12px; }
    .ch-legend-bar { width: 72px; height: 10px; flex: 0 0 auto; margin: 0 4px; }
    .ch-grid { grid-template-columns: 22px repeat(7, minmax(28px, 1fr)); gap: 4px; }
    .ch-cell { border-radius: 5px; min-height: 28px; font-size: 0.72em; }
    .ch-dow { font-size: 0.68em; padding-bottom: 4px; }
    .ch-year-grid { min-width: 840px; }
    .ch-bars-wrap { min-width: 420px; }
    .ch-bar-label { font-size: 0.55em; }
    .ch-week-detail { grid-template-columns: 1fr; }
    .ch-week-cell { min-height: 52px; padding: 10px 8px; }
    .ch-legend-h { justify-content: flex-start; flex-wrap: wrap; gap: 8px; }
    .ch-stat { padding: 8px; }
}
`;
dv.container.innerHTML = '';
dv.container.innerHTML = `<style>${styles}</style>`;
const wrap = dv.container.createEl('div', { cls: 'ch-wrap' });
const sectionUpdaters = [];

const availableFilters = collectAvailableFilters();
if (availableFilters.tags.length > 0 || availableFilters.links.length > 0) {
    const filterHost = wrap.createEl('div');
    new ViewKit.FilterBar(filterHost, {
        controls: ['tags', 'links'],
        availableTags: availableFilters.tags,
        availableLinks: availableFilters.links,
        showCount: false,
        initial: { tags: [], links: [] },
        storageKey: `${dv.current()?.file?.path || 'CalendarHeatmap'}:CalendarHeatmap:${mode || '全部'}:${baseFilterTags.join(',')}`,
        onFilter: (_items, state) => {
            activeFilterTags = state.tags || [];
            activeFilterLinks = state.links || [];
            activeFilterMatchMode = state.matchMode || "and";
            dailyDataCache.clear();
            sectionUpdaters.forEach(update => update());
        },
    }).bind(availableFilters.items);
}

// --- 6. 颜色工具 ---
function lerpHSL(h1, s1, l1, h2, s2, l2, t) {
    return `hsl(${h1+(h2-h1)*t}, ${s1+(s2-s1)*t}%, ${l1+(l2-l1)*t}%)`;
}
const SCALES = {
    income:  { from: [160, 60, 85], to: [150, 75, 28] },
    expense: { from: [45, 90, 80],  to: [0, 70, 32] },
    time:    { from: [200, 80, 85], to: [155, 70, 32] },
};
function gradientCSS(scale, direction = 'to top') {
    const colors = [];
    for (let i = 0; i <= 5; i++) {
        const t = i / 5;
        colors.push(lerpHSL(...scale.from, ...scale.to, t));
    }
    return `linear-gradient(${direction}, ${colors.join(', ')})`;
}
function getRange(dataMap, filterFn) {
    let maxPos = 0, maxNeg = 0;
    for (const [k, v] of dataMap) {
        if (filterFn && !filterFn(k)) continue;
        if (v > 0) maxPos = Math.max(maxPos, v);
        else maxNeg = Math.max(maxNeg, Math.abs(v));
    }
    return { maxPos, maxNeg, maxAbs: Math.max(maxPos, maxNeg) };
}

// --- 7. Tooltip ---
function buildTooltip(label, val, dateKey, formatFn, itemsMap, tooltipFilterFn) {
    let items = itemsMap.get(dateKey) || [];
    if (tooltipFilterFn) items = items.filter(tooltipFilterFn);
    let tip = `${label}: ${formatFn(val)}`;
    if (items.length > 0) {
        const MAX = 5;
        tip += '\n' + items.slice(0, MAX).map(i => {
            const parts = [i.text];
            if (i.money) parts.push(`¥${i.money}`);
            if (i.time) parts.push(`${i.time}🍅`);
            return '· ' + parts.join(' ');
        }).join('\n');
        if (items.length > MAX) tip += `\n… 还有 ${items.length - MAX} 条`;
    }
    return tip;
}

// --- 8. 渲染器 ---

// === 月×日 (日历网格) ===
function renderMonthDay(parent, year, month, dataMap, pathMap, itemsMap, colorFnFactory, formatFn, legendScales, tooltipFilterFn) {
    parent.innerHTML = '';
    const filterFn = k => k.startsWith(`${year}-${String(month+1).padStart(2,'0')}-`);
    const range = getRange(dataMap, filterFn);
    const body = parent.createEl('div', { cls: 'ch-body' });
    const gridWrap = body.createEl('div', { cls: 'ch-body-grid' });
    const gridEl = gridWrap.createEl('div', { cls: 'ch-grid' });
    gridEl.createEl('div', { cls: 'ch-dow' });
    ['一','二','三','四','五','六','日'].forEach(d => gridEl.createEl('div', { cls: 'ch-dow', text: d }));
    const firstDay = new Date(year, month, 1);
    const daysInMonth = new Date(year, month+1, 0).getDate();
    const startDow = (firstDay.getDay() + 6) % 7;
    const today = new Date(); today.setHours(0,0,0,0);
    const rows = Math.ceil((startDow + daysInMonth) / 7);
    const colorFn = colorFnFactory(range);
    for (let row = 0; row < rows; row++) {
        gridEl.createEl('div', { cls: 'ch-week-label', text: `W${row+1}` });
        for (let col = 0; col < 7; col++) {
            const dayNum = row * 7 + col - startDow + 1;
            if (dayNum < 1 || dayNum > daysInMonth) { gridEl.createEl('div', { cls: 'ch-cell empty' }); continue; }
            const dateKey = `${year}-${String(month+1).padStart(2,'0')}-${String(dayNum).padStart(2,'0')}`;
            const isFuture = new Date(year, month, dayNum) > today;
            const val = dataMap.get(dateKey) || 0;
            const filePath = pathMap.get(dateKey);
            const cell = gridEl.createEl('div', { cls: `ch-cell${val !== 0 ? ' has-data' : ''}` });
            if (!isFuture && filePath && val !== 0) cell.innerHTML = `<a class="internal-link" href="${filePath}">${dayNum}</a>`;
            else cell.textContent = String(dayNum);
            cell.style.background = isFuture ? 'transparent' : val !== 0 ? colorFn(val) : 'var(--background-secondary)';
            if (isFuture) cell.style.opacity = '0.2';
            const dow = ['周一','周二','周三','周四','周五','周六','周日'][col];
            cell.title = val !== 0 ? buildTooltip(`${month+1}月${dayNum}日 ${dow}`, val, dateKey, formatFn, itemsMap, tooltipFilterFn) : `${month+1}月${dayNum}日 ${dow}: 无记录`;
        }
    }
    renderLegend(body, range, legendScales, formatFn);
}

// === 年×日 (GitHub 热力图) ===
function renderYearDay(parent, year, _, dataMap, pathMap, itemsMap, colorFnFactory, formatFn, legendScales, tooltipFilterFn) {
    parent.innerHTML = '';
    const filterFn = k => k.startsWith(`${year}-`);
    const range = getRange(dataMap, filterFn);
    const colorFn = colorFnFactory(range);
    const body = parent.createEl('div', { cls: 'ch-body' });
    const gridWrap = body.createEl('div', { cls: 'ch-body-grid' });
    const today = new Date(); today.setHours(0,0,0,0);
    // 主容器（左标签 + 网格）
    const container = gridWrap.createEl('div', { attr: { style: 'display:flex; gap:2px;' } });
    const dayLabels = container.createEl('div', { attr: { style: 'display:flex; flex-direction:column; gap:2px; margin-right:4px; padding-top:18px;' } });
    ['一','','三','','五','','日'].forEach(d => {
        const el = dayLabels.createEl('div', { cls: 'ch-year-dlabel' });
        el.innerText = d; el.style.height = '14px';
    });
    const rightPart = container.createEl('div', { attr: { style: 'flex:1; min-width:0; overflow-x:auto;' } });
    // 月标签行（先占位，后面定位）
    const monthRow = rightPart.createEl('div', { attr: { style: 'display:flex; height:18px; position:relative;' } });
    const gridEl = rightPart.createEl('div', { cls: 'ch-year-grid' });

    const startDate = new Date(year, 0, 1);
    const startDow = (startDate.getDay() + 6) % 7;
    const cursor = new Date(startDate);
    cursor.setDate(cursor.getDate() - startDow);
    const endDate = new Date(year, 11, 31);
    let lastMonth = -1;
    const monthStarts = []; // { month, colIndex }
    let colIndex = 0;
    while (cursor <= endDate || (cursor.getDay() + 6) % 7 !== 0) {
        const col = gridEl.createEl('div', { cls: 'ch-year-col' });
        const firstOfWeek = new Date(cursor);
        for (let d = 0; d < 7; d++) {
            const dateKey = `${cursor.getFullYear()}-${String(cursor.getMonth()+1).padStart(2,'0')}-${String(cursor.getDate()).padStart(2,'0')}`;
            const isInYear = cursor.getFullYear() === year;
            const isFuture = cursor > today;
            const val = isInYear ? (dataMap.get(dateKey) || 0) : 0;
            const cell = col.createEl('div', { cls: `ch-year-cell${val !== 0 ? ' has-data' : ''}` });
            if (!isInYear) { cell.style.visibility = 'hidden'; }
            else {
                cell.style.background = isFuture ? 'transparent' : val !== 0 ? colorFn(val) : 'var(--background-secondary)';
                if (isFuture) cell.style.opacity = '0.2';
                const mLabel = `${cursor.getMonth()+1}月${cursor.getDate()}日`;
                cell.title = val !== 0 ? buildTooltip(mLabel, val, dateKey, formatFn, itemsMap, tooltipFilterFn) : `${mLabel}: 无记录`;
                if (val !== 0) {
                    const fp = pathMap.get(dateKey);
                    if (fp) cell.addEventListener('click', () => app.workspace.openLinkText(fp.replace(/\.md$/, ''), ''));
                }
            }
            cursor.setDate(cursor.getDate() + 1);
        }
        const m = firstOfWeek.getMonth();
        if (firstOfWeek.getFullYear() === year && m !== lastMonth) {
            monthStarts.push({ month: m, col: colIndex });
            lastMonth = m;
        }
        colIndex++;
        if (cursor.getFullYear() > year && (cursor.getDay() + 6) % 7 === 0) break;
    }
    // 月标签定位：每格 16px (14+2gap)
    const cellW = 16;
    for (const { month, col } of monthStarts) {
        const span = monthRow.createEl('span', { cls: 'ch-year-mlabel', text: `${month+1}月` });
        span.style.position = 'absolute';
        span.style.left = `${col * cellW}px`;
    }
    renderLegend(body, range, legendScales, formatFn);
}

// === 柱状图通用 (年×周, 年×月, 月×周) - 双向 ===
function renderBars(parent, aggData, labels, colorFnFactory, formatFn, legendScales, rangeFn) {
    parent.innerHTML = '';
    const range = rangeFn ? rangeFn(aggData) : getRange(aggData);
    const colorFn = colorFnFactory(range);
    const body = parent.createEl('div', { cls: 'ch-body' });
    const gridWrap = body.createEl('div', { cls: 'ch-body-grid' });
    const vals = labels.map(l => aggData.get(l.key) || 0);
    const maxPos = Math.max(...vals.filter(v => v > 0), 0) || 1;
    const maxNeg = Math.max(...vals.filter(v => v < 0).map(Math.abs), 0) || 1;
    const hasPos = vals.some(v => v > 0);
    const hasNeg = vals.some(v => v < 0);
    const barsWrap = gridWrap.createEl('div', { cls: 'ch-bars-wrap' });
    // 上半部（收入）
    if (hasPos) {
        const topRow = barsWrap.createEl('div', { cls: 'ch-bars-top' });
        for (const { key, label } of labels) {
            const val = aggData.get(key) || 0;
            const col = topRow.createEl('div', { cls: 'ch-bar-col' });
            if (val > 0) {
                const h = Math.max(2, (val / maxPos) * 100);
                const bar = col.createEl('div', { cls: 'ch-bar up' });
                bar.style.height = `${h}%`;
                bar.style.background = colorFn(val);
                bar.title = `${label}: ${formatFn(val)}`;
            }
        }
    } else {
        // 无收入，只显示一条分隔线
        barsWrap.createEl('div', { attr: { style: 'border-bottom:1px solid var(--background-modifier-border);' } });
    }
    // 下半部（支出）
    if (hasNeg) {
        const bottomRow = barsWrap.createEl('div', { cls: 'ch-bars-bottom' });
        for (const { key, label } of labels) {
            const val = aggData.get(key) || 0;
            const col = bottomRow.createEl('div', { cls: 'ch-bar-col' });
            if (val < 0) {
                const h = Math.max(2, (Math.abs(val) / maxNeg) * 100);
                const bar = col.createEl('div', { cls: 'ch-bar down' });
                bar.style.height = `${h}%`;
                bar.style.background = colorFn(val);
                bar.title = `${label}: ${formatFn(val)}`;
            }
        }
    }
    // 标签行
    const labelRow = gridWrap.createEl('div', { cls: 'ch-bar-labels' });
    for (const { label } of labels) {
        labelRow.createEl('div', { cls: 'ch-bar-label', text: label });
    }
    renderLegendH(gridWrap, range, legendScales, formatFn);
}

// === 周×日 (大格详情) ===
function renderWeekDay(parent, year, weekNum, dataMap, pathMap, itemsMap, colorFnFactory, formatFn, legendScales, tooltipFilterFn) {
    parent.innerHTML = '';
    // 计算该周的周一日期
    const jan4 = new Date(year, 0, 4);
    const startOfWeek = new Date(jan4);
    startOfWeek.setDate(jan4.getDate() - (jan4.getDay() + 6) % 7 + (weekNum - 1) * 7);
    const filterFn = k => {
        const d = new Date(k); d.setHours(0,0,0,0);
        return d >= startOfWeek && d < new Date(startOfWeek.getTime() + 7*86400000);
    };
    const range = getRange(dataMap, filterFn);
    const colorFn = colorFnFactory(range);
    const body = parent.createEl('div', { cls: 'ch-body' });
    const gridWrap = body.createEl('div', { cls: 'ch-body-grid' });
    const grid = gridWrap.createEl('div', { cls: 'ch-week-detail' });
    const today = new Date(); today.setHours(0,0,0,0);
    const dows = ['周一','周二','周三','周四','周五','周六','周日'];
    for (let d = 0; d < 7; d++) {
        const date = new Date(startOfWeek); date.setDate(startOfWeek.getDate() + d);
        const dateKey = `${date.getFullYear()}-${String(date.getMonth()+1).padStart(2,'0')}-${String(date.getDate()).padStart(2,'0')}`;
        const isFuture = date > today;
        const val = dataMap.get(dateKey) || 0;
        const filePath = pathMap.get(dateKey);
        const cell = grid.createEl('div', { cls: `ch-week-cell${val !== 0 ? ' has-data' : ''}` });
        cell.style.background = isFuture ? 'transparent' : val !== 0 ? colorFn(val) : 'var(--background-secondary)';
        if (isFuture) cell.style.opacity = '0.3';
        cell.innerHTML = `
            <div class="day-dow">${dows[d]}</div>
            <div class="day-num">${date.getMonth()+1}/${date.getDate()}</div>
            ${val !== 0 ? `<div class="day-val">${formatFn(val)}</div>` : '<div class="day-val" style="opacity:0.3">—</div>'}
        `;
        cell.title = val !== 0 ? buildTooltip(`${date.getMonth()+1}月${date.getDate()}日 ${dows[d]}`, val, dateKey, formatFn, itemsMap, tooltipFilterFn) : `${dows[d]}: 无记录`;
        if (!isFuture && filePath && val !== 0) {
            cell.addEventListener('click', () => app.workspace.openLinkText(filePath.replace(/\.md$/, ''), ''));
        }
    }
    renderLegendH(gridWrap, range, legendScales, formatFn);
}

// 图例渲染 (垂直，用于月×日、年×日、柱状图)
function renderLegend(body, range, legendScales, formatFn) {
    const legendContainer = body.createEl('div', { cls: 'ch-legend' });
    for (const { scale, maxVal, label } of legendScales(range)) {
        if (maxVal <= 0) continue;
        const maxLabel = String(formatFn(maxVal)).replace(/^\+/, '');
        const lg = legendContainer.createEl('div', { attr: { style: 'display:flex;flex-direction:column;align-items:center;flex:1;width:100%;' } });
        lg.createEl('div', { text: label, attr: { style: 'font-weight:600;margin-bottom:2px;' } });
        lg.createEl('div', { text: maxLabel, attr: { style: 'font-size:0.9em;' } });
        const bar = lg.createEl('div', { cls: 'ch-legend-bar' });
        bar.style.background = gradientCSS(scale);
        lg.createEl('div', { text: '0', attr: { style: 'font-size:0.9em;' } });
    }
}

// 图例渲染 (水平，用于周×日)
function renderLegendH(container, range, legendScales, formatFn) {
    const el = container.createEl('div', { cls: 'ch-legend-h' });
    for (const { scale, maxVal, label } of legendScales(range)) {
        if (maxVal <= 0) continue;
        const maxLabel = String(formatFn(maxVal)).replace(/^\+/, '');
        const item = el.createEl('div', { cls: 'ch-legend-h-item' });
        item.createEl('span', { text: '0' });
        const bar = item.createEl('div', { cls: 'ch-legend-h-bar' });
        bar.style.background = gradientCSS(scale, 'to right');
        item.createEl('span', { text: `${label} ${maxLabel}` });
    }
}

// --- 9. 通用导航区块 ---
function getISOWeek(d) {
    const dt = new Date(d); dt.setHours(0,0,0,0);
    const thu = new Date(dt); thu.setDate(dt.getDate() + 3 - (dt.getDay() + 6) % 7);
    const y = thu.getFullYear();
    const w = 1 + Math.round(((thu - new Date(y,0,4)) / 86400000 - 3 + (new Date(y,0,4).getDay() + 6) % 7) / 7);
    return { y, w };
}

function renderQueryWarnings(container, entries) {
    const warnings = entries?.warnings || [];
    container.textContent = '';
    if (!warnings.length) {
        container.style.display = 'none';
        return;
    }
    container.style.display = 'block';
    for (const warning of warnings) {
        container.createEl('div', { text: warning });
    }
}

function createSection(parent, emoji, title, dataSelector, colorFnFactory, formatFn, statsFn, legendScales, tooltipFilterFn) {
    const section = parent.createEl('div', { cls: 'ch-section' });
    const now = new Date();
    let curYear = now.getFullYear(), curMonth = now.getMonth();
    const iw = getISOWeek(now); let curWeek = iw.w;

    const header = section.createEl('div', { cls: 'ch-header' });
    header.innerHTML = `<div class="ch-title">${emoji} ${title}</div>`;
    const nav = header.createEl('div', { cls: 'ch-nav' });
    const prevBtn = nav.createEl('button', { cls: 'ch-nav-btn', text: '◀' });
    const navLabel = nav.createEl('div', { cls: 'ch-nav-label' });
    const nextBtn = nav.createEl('button', { cls: 'ch-nav-btn', text: '▶' });
    const gridContainer = section.createEl('div');
    const statEl = section.createEl('div', { cls: 'ch-stat' });
    const warningEl = section.createEl('div', { cls: 'ch-query-warning' });

    function update() {
        const dailyData = getDailyDataForDisplay(curYear, curMonth, curWeek);
        const dataMap = dataSelector(dailyData);
        if (SCOPE === '年') {
            navLabel.innerText = `${curYear}年`;
            if (GRAIN === '日') {
                const pathMap = dataMap === dailyData.dailyNet ? dailyData.dailyNetPaths : dailyData.dailyTimePaths;
                renderYearDay(gridContainer, curYear, 0, dataMap, pathMap, dailyData.dailyItems, colorFnFactory, formatFn, legendScales, tooltipFilterFn);
            } else {
                const agg = GRAIN === '周' ? aggByWeek(dataMap) : aggByMonth(dataMap);
                let labels;
                if (GRAIN === '周') {
                    // 每4周标一个月份分界
                    labels = Array.from({length:52}, (_, i) => {
                        const d = new Date(curYear, 0, 1 + i * 7);
                        const showLabel = i === 0 || d.getMonth() !== new Date(curYear, 0, 1 + (i-1)*7).getMonth();
                        return { key: `${curYear}-W${String(i+1).padStart(2,'0')}`, label: showLabel ? `${d.getMonth()+1}月` : '' };
                    });
                } else {
                    labels = Array.from({length:12}, (_, i) => ({ key: `${curYear}-${String(i+1).padStart(2,'0')}`, label: `${i+1}月` }));
                }
                const rangeFn = (d) => getRange(d, k => k.startsWith(`${curYear}`));
                renderBars(gridContainer, agg, labels, colorFnFactory, formatFn, legendScales, rangeFn);
            }
        } else if (SCOPE === '周') {
            navLabel.innerText = `${curYear}年 第${curWeek}周`;
            const pathMap = dataMap === dailyData.dailyNet ? dailyData.dailyNetPaths : dailyData.dailyTimePaths;
            renderWeekDay(gridContainer, curYear, curWeek, dataMap, pathMap, dailyData.dailyItems, colorFnFactory, formatFn, legendScales, tooltipFilterFn);
        } else { // 月
            navLabel.innerText = `${curYear}年${curMonth + 1}月`;
            if (GRAIN === '周') {
                const agg = aggByWeek(dataMap);
                // 该月包含的周
                const first = new Date(curYear, curMonth, 1);
                const last = new Date(curYear, curMonth + 1, 0);
                const weeks = new Set();
                for (let d = new Date(first); d <= last; d.setDate(d.getDate() + 1)) {
                    weeks.add(dateToWeekKey(d.toISOString().slice(0,10)));
                }
                const labels = [...weeks].sort().map(w => ({ key: w, label: w.split('-')[1] }));
                const rangeFn = (d) => getRange(d, k => weeks.has(k));
                renderBars(gridContainer, agg, labels, colorFnFactory, formatFn, legendScales, rangeFn);
            } else {
                const pathMap = dataMap === dailyData.dailyNet ? dailyData.dailyNetPaths : dailyData.dailyTimePaths;
                renderMonthDay(gridContainer, curYear, curMonth, dataMap, pathMap, dailyData.dailyItems, colorFnFactory, formatFn, legendScales, tooltipFilterFn);
            }
        }
        statEl.innerHTML = statsFn(dataMap, curYear, curMonth, curWeek);
        renderQueryWarnings(warningEl, dailyData.entries);
    }

    sectionUpdaters.push(update);

    prevBtn.addEventListener('click', () => {
        if (SCOPE === '年') curYear--;
        else if (SCOPE === '周') { curWeek--; if (curWeek < 1) { curYear--; curWeek = 52; } }
        else { curMonth--; if (curMonth < 0) { curMonth = 11; curYear--; } }
        update();
    });
    nextBtn.addEventListener('click', () => {
        if (SCOPE === '年') curYear++;
        else if (SCOPE === '周') { curWeek++; if (curWeek > 52) { curYear++; curWeek = 1; } }
        else { curMonth++; if (curMonth > 11) { curMonth = 0; curYear++; } }
        update();
    });
    update();
}

// --- 10. 统计函数 ---
function makeStats(formatFn, posLabel, negLabel, unit) {
    return (dataMap, year, month, week) => {
        let filterFn;
        if (SCOPE === '年') filterFn = k => k.startsWith(`${year}-`);
        else if (SCOPE === '周') {
            const jan4 = new Date(year, 0, 4);
            const sw = new Date(jan4); sw.setDate(jan4.getDate() - (jan4.getDay()+6)%7 + (week-1)*7);
            const ew = new Date(sw.getTime() + 7*86400000);
            filterFn = k => { const d = new Date(k); return d >= sw && d < ew; };
        } else filterFn = k => k.startsWith(`${year}-${String(month+1).padStart(2,'0')}-`);

        let pos = 0, neg = 0, days = 0;
        for (const [k, v] of dataMap) {
            if (!filterFn(k)) continue;
            days++;
            if (v > 0) pos += v; else neg += v;
        }
        const parts = [];
        if (posLabel && pos > 0) parts.push(`<span class="ch-stat-item"><span class="ch-stat-dot" style="background:${lerpHSL(...SCALES.income.to,...SCALES.income.to,1)}"></span>${posLabel}: ${formatFn(pos)}</span>`);
        if (negLabel && neg < 0) parts.push(`<span class="ch-stat-item"><span class="ch-stat-dot" style="background:${lerpHSL(...SCALES.expense.to,...SCALES.expense.to,1)}"></span>${negLabel}: ${formatFn(Math.abs(neg))}</span>`);
        if (unit && (pos > 0 || neg < 0)) {
            const total = pos + Math.abs(neg);
            parts.push(`<span class="ch-stat-item">合计: ${total.toFixed(1)} ${unit}</span>`);
        }
        const net = pos + neg;
        if (days > 0) parts.push(`<span class="ch-stat-item">日均: ${formatFn(Math.abs(net / days))}</span>`);
        parts.push(`<span class="ch-stat-item">${days} 天有记录</span>`);
        return parts.join('');
    };
}

// --- 11. 渲染入口 ---
const netColorFactory = (range) => {
    const max = Math.max(range.maxPos, range.maxNeg, 1);
    return (value) => {
        if (!value) return 'var(--background-secondary)';
        const scale = value > 0 ? SCALES.income : SCALES.expense;
        const t = Math.min(1, Math.log1p(Math.abs(value)) / Math.log1p(max));
        return lerpHSL(...scale.from, ...scale.to, t);
    };
};
const netLegend = (range) => [
    { scale: SCALES.expense, maxVal: range.maxNeg, label: '支出' },
    { scale: SCALES.income,  maxVal: range.maxPos, label: '收入' },
];

const timeColorFactory = (range) => {
    const max = Math.max(range.maxAbs, 1);
    return (value) => {
        if (!value || value <= 0) return 'var(--background-secondary)';
        const t = Math.min(1, Math.log1p(value) / Math.log1p(max));
        return lerpHSL(...SCALES.time.from, ...SCALES.time.to, t);
    };
};
const timeLegend = (range) => [
    { scale: SCALES.time, maxVal: range.maxAbs, label: '专注' },
];

if (!mode || mode === "净值") {
    createSection(wrap, '💰', '净值热力图', data => data.dailyNet, netColorFactory,
        v => v > 0 ? `+¥${v.toFixed(1)}` : `-¥${Math.abs(v).toFixed(1)}`,
        makeStats(v => `¥${v.toFixed(0)}`, '收入', '支出'),
        netLegend,
        i => i.money !== 0
    );
}
if (!mode || mode === "专注") {
    createSection(wrap, '🍅', '专注热力图', data => data.dailyTime, timeColorFactory,
        v => `${v.toFixed(1)} 🍅`,
        makeStats(v => `${v.toFixed(1)} 🍅`, null, null, '🍅'),
        timeLegend,
        i => i.time !== 0
    );
}
