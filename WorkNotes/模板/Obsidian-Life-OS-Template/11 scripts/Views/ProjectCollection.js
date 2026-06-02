/**
 * 项目集合视图脚本 (ProjectCollection.js) v1.0
 *
 * 功能：
 * 1. 按项目 supertag 收集项目文件（默认 #项目，可由 input/frontmatter 覆盖）。
 * 2. 聚合计算每个项目的：投入时长、净成本、最近活动、互动次数、目标进度。
 * 3. 展示：状态型仪表盘、排序榜、进度预警、费用榜、项目名录。
 * 4. 交互：FilterBar 搜索、标签、日期、排序；名录表头排序与滚动加载。
 */

// --- 1. 导入核心库 ---
const core = {};
await dv.view("11 scripts/Core/FinanceCore", core);
const { Utils: CoreUtils, Query, ObjectSummary } = core;
const viewKit = {};
await dv.view("11 scripts/Core/ViewKit", viewKit);
const { ViewKit } = viewKit;

const Utils = { ...CoreUtils, escapeHtml: ViewKit.escapeHtml };
const esc = value => ViewKit.escapeHtml(value);

// --- 样式注入 ---
const styles = `
.pc-container {
    --c-bg-card: var(--background-secondary); --c-bg-hover: var(--background-secondary-alt);
    --c-border: var(--background-modifier-border); --c-accent: var(--interactive-accent);
    --c-success: var(--color-green); --c-danger: var(--color-red); --c-warning: var(--color-orange);
    --c-text-muted: var(--text-muted); --c-text-normal: var(--text-normal);
    font-family: var(--font-interface); display: flex; flex-direction: column; gap: 20px;
    container-type: inline-size; container-name: view-container; max-width: 100%; overflow-x: hidden;
}
.pc-container, .pc-container * { box-sizing: border-box; }
.pc-dashboard { display: grid; grid-template-columns: repeat(auto-fit, minmax(120px, 1fr)); gap: 12px; flex-shrink: 0; }
.pc-kpi-card {
    background: var(--c-bg-card); border: 1px solid var(--c-border); border-radius: 12px; padding: 15px; text-align: center;
    display: flex; flex-direction: column; justify-content: center; align-items: center;
    box-shadow: 0 2px 4px rgba(0,0,0,0.05); transition: transform 0.2s, box-shadow 0.2s; min-width: 0;
}
.pc-kpi-card:hover { transform: translateY(-2px); box-shadow: 0 4px 12px rgba(0,0,0,0.1); }
.pc-kpi-icon { font-size: 1.8em; margin-bottom: 6px; opacity: 0.8; }
.pc-kpi-val { font-size: 1.5em; font-weight: 700; color: var(--c-text-normal); line-height: 1.2; max-width: 100%; overflow: hidden; text-overflow: ellipsis; }
.pc-kpi-label { font-size: 0.85em; color: var(--c-text-muted); margin-top: 4px; }
.pc-section-header {
    display: flex; align-items: center; gap: 8px; margin-bottom: 10px;
    font-size: 1.1em; font-weight: 600; color: var(--c-text-normal);
    border-bottom: 2px solid var(--c-border); padding-bottom: 6px;
}
.pc-rank-grid { display: grid; grid-template-columns: 1fr 1fr 1fr; gap: 15px; flex-shrink: 0; }
.pc-rank-list {
    background: var(--c-bg-card); border: 1px solid var(--c-border); border-radius: 8px;
    max-height: 320px; overflow-y: auto; display: flex; flex-direction: column; position: relative; min-width: 0;
}
.pc-rank-list .pc-section-header { position: sticky; top: 0; background: var(--c-bg-card); z-index: 10; margin: 0; padding: 10px; border-bottom: 2px solid var(--c-border); }
.pc-rank-item { margin: 0 10px; display: flex; justify-content: space-between; align-items: center; gap: 8px; padding: 6px 8px; border-bottom: 1px dashed var(--c-border); min-width: 0; }
.pc-rank-item:first-of-type { margin-top: 10px; }
.pc-rank-item:last-of-type { margin-bottom: 10px; }
.pc-rank-name { font-weight: 500; color: var(--c-text-normal); text-decoration: none; white-space: nowrap; overflow: hidden; text-overflow: ellipsis; max-width: 100%; }
.pc-rank-val { font-weight: bold; font-size: 0.9em; flex-shrink: 0; }
.pc-rank-bar-wrap { display: flex; align-items: center; gap: 6px; min-width: 100px; }
.pc-rank-bar { height: 6px; border-radius: 3px; transition: width 0.4s ease; min-width: 2px; }
.pc-empty { color: var(--c-text-muted); font-size: 0.9em; padding: 14px; text-align: center; }
.pc-table-wrap {
    background: var(--c-bg-card); border: 1px solid var(--c-border); border-radius: 8px;
    max-height: 600px; overflow: auto; position: relative; min-width: 0;
    -webkit-overflow-scrolling: touch; overscroll-behavior-x: contain;
}
.pc-table { width: 100%; min-width: 760px; border-collapse: separate; border-spacing: 0; font-size: 0.9em; }
.pc-table th {
    text-align: left; color: var(--c-text-muted); padding: 10px; border-bottom: 2px solid var(--c-border);
    font-weight: 600; cursor: pointer; position: sticky; top: 0; background: var(--c-bg-card); z-index: 10;
    user-select: none; white-space: nowrap; transition: color 0.2s;
}
.pc-table th:hover { color: var(--c-accent); }
.pc-table th .sort-arrow { font-size: 0.75em; margin-left: 4px; opacity: 0.4; }
.pc-table th.sorted .sort-arrow { opacity: 1; color: var(--c-accent); }
.pc-table td { padding: 8px 10px; border-bottom: 1px solid var(--c-border); vertical-align: middle; overflow: hidden; text-overflow: ellipsis; }
.pc-table tr:hover td { background: var(--c-bg-hover); }
.pc-avatar { width: 24px; height: 24px; border-radius: 50%; background: var(--c-accent); color: white; display: inline-flex; align-items: center; justify-content: center; font-size: 0.8em; margin-right: 8px; flex-shrink: 0; }
.pc-tag { font-size: 0.75em; padding: 1px 6px; border-radius: 4px; background: var(--background-primary); border: 1px solid var(--c-border); color: var(--c-text-muted); margin-right: 4px; white-space: nowrap; display: inline-flex; align-items: center; max-width: 120px; overflow: hidden; text-overflow: ellipsis; }
.pc-source-badge { align-self: flex-start; font-size: 0.75em; color: var(--c-text-muted); border: 1px solid var(--c-border); border-radius: 999px; padding: 2px 8px; background: var(--background-primary); margin-bottom: -8px; }
.pc-status { font-size: 0.75em; padding: 2px 7px; border-radius: 999px; border: 1px solid var(--c-border); white-space: nowrap; display: inline-flex; align-items: center; }
.pc-status.active { color: var(--c-accent); border-color: var(--c-accent); background: color-mix(in srgb, var(--c-accent) 12%, transparent); }
.pc-status.done { color: var(--c-success); border-color: var(--c-success); background: color-mix(in srgb, var(--c-success) 12%, transparent); }
.pc-status.paused { color: var(--c-warning); border-color: var(--c-warning); background: color-mix(in srgb, var(--c-warning) 12%, transparent); }
.pc-status.planned { color: var(--c-text-muted); }
.pc-progress { display: flex; align-items: center; gap: 6px; min-width: 120px; }
.pc-progress-track { height: 7px; flex: 1; min-width: 54px; border-radius: 999px; background: var(--background-modifier-border); overflow: hidden; }
.pc-progress-bar { height: 100%; border-radius: 999px; transition: width 0.3s ease; }
.pc-progress.none { color: var(--c-text-muted); min-width: auto; }
.pc-progress-value { font-size: 0.8em; font-variant-numeric: tabular-nums; white-space: nowrap; }
.pc-over-badge { font-size: 0.7em; color: var(--c-danger); border: 1px solid var(--c-danger); border-radius: 999px; padding: 1px 5px; margin-left: 4px; }
.pc-card-list { display: none; flex-direction: column; gap: 8px; }
.pc-mobile-card {
    border: 1px solid var(--c-border); border-radius: 8px; background: var(--background-primary);
    padding: 10px; display: flex; flex-direction: column; gap: 8px; min-width: 0;
}
.pc-mobile-card-head { display: flex; align-items: center; justify-content: space-between; gap: 10px; min-width: 0; }
.pc-mobile-card-title { display: flex; align-items: center; min-width: 0; flex: 1; }
.pc-mobile-card-title a { min-width: 0; overflow: hidden; text-overflow: ellipsis; white-space: nowrap; }
.pc-mobile-card-meta { display: flex; flex-wrap: wrap; gap: 6px; align-items: center; color: var(--c-text-muted); font-size: 0.82em; }
.pc-mobile-card-stats { display: grid; grid-template-columns: repeat(2, minmax(0, 1fr)); gap: 6px; }
.pc-mobile-stat { border: 1px solid var(--c-border); border-radius: 6px; padding: 6px; min-width: 0; }
.pc-mobile-stat-label { font-size: 0.72em; color: var(--c-text-muted); margin-bottom: 2px; }
.pc-mobile-stat-val { font-family: monospace; font-weight: 700; overflow: hidden; text-overflow: ellipsis; white-space: nowrap; }
.pc-mobile-card-tags { display: flex; flex-wrap: wrap; gap: 4px; }
@container view-container (max-width: 900px) { .pc-rank-grid { grid-template-columns: 1fr 1fr; } }
@container view-container (max-width: 600px) {
    .pc-rank-grid { grid-template-columns: 1fr; }
    .pc-dashboard { grid-template-columns: repeat(2, 1fr); }
}
@container view-container (max-width: 520px) {
    .pc-dashboard { grid-template-columns: repeat(2, minmax(0, 1fr)); gap: 8px; }
    .pc-kpi-card { padding: 10px 8px; }
    .pc-kpi-icon { font-size: 1.35em; margin-bottom: 3px; }
    .pc-kpi-val { font-size: 1.2em; overflow-wrap: anywhere; }
    .pc-section-header { font-size: 1em; flex-wrap: wrap; }
    .pc-rank-list { max-height: clamp(240px, 42vh, 360px); overflow-y: auto; }
    .pc-rank-item { min-height: 40px; }
    .pc-rank-bar-wrap { min-width: 86px; }
    .pc-table-wrap { max-height: clamp(360px, 58vh, 640px); overflow-y: auto; overflow-x: hidden; }
    .pc-progress { min-width: 0; }
    .pc-tag { min-height: 40px; max-width: 100%; margin-right: 0; }
    .pc-status { max-width: 100%; }
    .pc-card-list { padding: 8px; }
}
`;
dv.container.innerHTML = `<style>${styles}</style>`;
const container = dv.container.createEl('div', { cls: 'pc-container' });

const currentFm = dv.current()?.file?.frontmatter || {};
const RANK_LIMIT = Math.max(1, Number(input?.rankLimit || input?.["排行数量"] || currentFm.rankLimit || currentFm["排行数量"] || 30) || 30);
const projectTag = input?.["标签"] || input?.tag || currentFm["项目标签"] || currentFm.projectTag || "项目";
const projectScope = input?.projectScope || input?.["项目目录"] || currentFm.projectScope || currentFm["项目目录"] || null;
const projectPages = Utils.collectSupertagPages({ tag: projectTag || "项目", scope: projectScope, dv });
const projectSourceLabel = `supertag tags:${Utils.normalizeSupertagInput(projectTag || "项目").join("/") || "项目"}`;
container.createEl('div', { cls: 'pc-source-badge', text: projectSourceLabel });

function toDateTime(value) {
    const date = Utils.resolveDateValue(value);
    if (!date) return null;
    if (dv.luxon?.DateTime?.fromJSDate) return dv.luxon.DateTime.fromJSDate(date);
    return {
        ts: date.getTime(),
        valueOf() { return this.ts; },
        toFormat(format) {
            const yyyy = date.getFullYear();
            const MM = String(date.getMonth() + 1).padStart(2, "0");
            const dd = String(date.getDate()).padStart(2, "0");
            if (format === "MM-dd") return `${MM}-${dd}`;
            return `${yyyy}-${MM}-${dd}`;
        },
    };
}

function readField(page, keys, fallback = "") {
    const fm = page?.file?.frontmatter || {};
    for (const key of keys) {
        const value = page?.[key] ?? fm[key];
        if (value !== undefined && value !== null && value !== "") return value;
    }
    return fallback;
}

function readNumber(page, keys) {
    const value = readField(page, keys, "");
    const list = Utils.normalizeArrayField(value);
    const raw = list.length ? list[0] : value;
    const num = Number(String(raw ?? "").replace(/[^\d.-]/g, ""));
    return Number.isFinite(num) && num > 0 ? num : 0;
}

function readCategoryTags(page) {
    const raw = readField(page, ["标签", "类型", "type", "category"], []);
    return Utils.normalizeArrayField(raw)
        .map(tag => String(tag).replace(/^#/, "").trim())
        .filter(tag => tag && !["项目"].includes(tag));
}

function createProjectSummary(p) {
    const file = p.file;
    const status = String(readField(p, ["状态", "status"], "进行中")).trim() || "进行中";
    const targetEffort = readNumber(p, ["期望努力值", "目标努力值", "targetEffort"]);
    const categoryTags = readCategoryTags(p);

    return {
        name: file.name,
        link: file.link,
        path: file.path,
        tags: categoryTags,
        status,
        targetEffort,
        startDate: toDateTime(readField(p, ["开始时间", "startDate"], null)),
        endDate: toDateTime(readField(p, ["结束时间", "endDate"], null)),
        totalTime: 0,
        netCost: 0,
        lastInteraction: null,
        interactionCount: 0,
        page: p,
    };
}

const summaryResult = ObjectSummary.collect({
    objectPages: projectPages,
    Query,
    createSummary: createProjectSummary,
    accumulate(data, entry) {
        const actualItemTime = toDateTime(Utils.resolveEntryDate(entry));
        if (!actualItemTime) return;

        if (!data.lastInteraction || actualItemTime > data.lastInteraction) data.lastInteraction = actualItemTime;
        data.totalTime += entry.vector.time || 0;
        const isTransfer = entry.meta?.tags?.includes("转账");
        data.netCost += entry.type === "journal" && !isTransfer ? (entry.vector.money || 0) : 0;
        data.interactionCount++;
    },
});
const entries = summaryResult.entries;
const projectsMap = summaryResult.summaryMap;

let projectData = [];
for (const [name, data] of projectsMap) {
    const progress = data.targetEffort > 0 ? (data.totalTime / data.targetEffort) * 100 : null;
    projectData.push({
        name,
        link: data.link,
        path: data.path,
        tags: data.tags,
        displayText: `${data.name} ${(data.tags || []).join(' ')} ${data.status}`,
        ctime: data.lastInteraction,
        time: data.totalTime,
        money: data.netCost,
        count: data.interactionCount,
        status: data.status,
        targetEffort: data.targetEffort,
        progress,
        lastInteraction: data.lastInteraction,
        startDate: data.startDate,
        endDate: data.endDate,
    });
}

const dashHost = container.createEl('div');
const filterHost = container.createEl('div');
const debugHost = input?.debug ? container.createEl('div') : null;
const contentHost = container.createEl('div');
const allTags = ViewKit.collectTags(projectData);

function fmtMoney(value) {
    const prefix = value > 0 ? "+" : value < 0 ? "-" : "";
    return `${prefix}${ViewKit.fmtMoney(Math.abs(value))}`;
}

function statusClass(status) {
    const text = String(status || "");
    if (/完成|done/i.test(text)) return "done";
    if (/搁置|暂停|归档|paused/i.test(text)) return "paused";
    if (/规划|计划|planned/i.test(text)) return "planned";
    return "active";
}

function progressColor(progress) {
    if (progress == null) return "var(--c-text-muted)";
    if (progress > 100) return "var(--c-danger)";
    if (progress >= 80) return "var(--c-success)";
    if (progress >= 50) return "var(--c-warning)";
    return "var(--c-accent)";
}

function renderStatus(status) {
    const safe = esc(status || "进行中");
    return `<span class="pc-status ${statusClass(status)}">${safe}</span>`;
}

function renderProgress(project) {
    if (!project.targetEffort || project.progress == null) {
        return `<div class="pc-progress none">${project.time.toFixed(1)}🍅</div>`;
    }
    const pct = Math.max(0, Math.min(100, project.progress));
    const color = progressColor(project.progress);
    const overBadge = project.progress > 100 ? `<span class="pc-over-badge">超额</span>` : "";
    return `<div class="pc-progress">
        <div class="pc-progress-track"><div class="pc-progress-bar" style="width:${pct}%;background:${color}"></div></div>
        <span class="pc-progress-value" style="color:${color}">${project.progress.toFixed(0)}%</span>${overBadge}
    </div>`;
}

function projectLink(project) {
    const path = esc(project.path);
    const name = esc(project.name);
    return `<a class="pc-rank-name internal-link" href="${path}">${name}</a>`;
}

function renderDashboard() {
    dashHost.innerHTML = "";
    const totalProjects = projectData.length;
    const activeProjects = projectData.filter(p => {
        const ts = ViewKit.toTimestamp(p.lastInteraction);
        return ts && (Date.now() - ts) < 30 * 24 * 60 * 60 * 1000;
    }).length;
    const totalTime = projectData.reduce((sum, p) => sum + p.time, 0);
    const totalNetCost = projectData.reduce((sum, p) => sum + p.money, 0);

    const dash = dashHost.createEl('div', { cls: 'pc-dashboard' });
    dash.innerHTML = `
        <div class="pc-kpi-card"><div class="pc-kpi-icon">📁</div><div class="pc-kpi-val">${totalProjects}</div><div class="pc-kpi-label">项目总数</div></div>
        <div class="pc-kpi-card"><div class="pc-kpi-icon">🔥</div><div class="pc-kpi-val">${activeProjects}</div><div class="pc-kpi-label">近月活跃</div></div>
        <div class="pc-kpi-card"><div class="pc-kpi-icon">⏳</div><div class="pc-kpi-val">${totalTime.toFixed(1)}🍅</div><div class="pc-kpi-label">全库总投入</div></div>
        <div class="pc-kpi-card"><div class="pc-kpi-icon">💸</div><div class="pc-kpi-val" style="color:${totalNetCost < 0 ? 'var(--c-danger)' : 'var(--c-success)'}">${fmtMoney(totalNetCost)}</div><div class="pc-kpi-label">总净成本</div></div>
    `;
}

const sortFields = [
    { key: "date", label: "最近", fn: (a, b) => ViewKit.toTimestamp(b.lastInteraction) - ViewKit.toTimestamp(a.lastInteraction) },
    { key: "time", label: "投入", fn: (a, b) => b.time - a.time },
    { key: "money", label: "净成本", fn: (a, b) => a.money - b.money },
    { key: "count", label: "互动", fn: (a, b) => b.count - a.count },
    { key: "progress", label: "进度", fn: (a, b) => (b.progress || 0) - (a.progress || 0) },
];

function getSortField(key) {
    return sortFields.find(field => field.key === key) || sortFields[1];
}

function renderRankItem(parent, project, valueHtml, pct, color) {
    const row = parent.createEl('div', { cls: 'pc-rank-item' });
    row.innerHTML = `
        <div style="display:flex;align-items:center;min-width:0;flex:1">${projectLink(project)}</div>
        <div class="pc-rank-bar-wrap"><div class="pc-rank-bar" style="width:${pct}%;background:${color}"></div><span class="pc-rank-val" style="color:${color}">${valueHtml}</span></div>
    `;
}

function detectMobileView() {
    const width = container.clientWidth || (typeof window !== "undefined" ? window.innerWidth : 9999);
    return Boolean(
        (typeof app !== "undefined" && app.isMobile) ||
        (typeof document !== "undefined" && document.body?.classList?.contains("is-mobile")) ||
        width < 520
    );
}

function renderAll(data, state = {}, isEmpty = false) {
    contentHost.innerHTML = "";
    const activeSort = state.sort || 'time';
    if (data.length === 0) {
        contentHost.createEl('div', {
            text: isEmpty ? '无匹配项目' : '暂无项目记录',
            style: 'color:var(--c-text-muted); text-align:center; padding:20px;',
        });
        return;
    }

    const grid = contentHost.createEl('div', { cls: 'pc-rank-grid' });
    const sortField = getSortField(activeSort);
    const primaryRank = [...data].sort(sortField.fn);
    const primaryShown = primaryRank.slice(0, RANK_LIMIT);
    const primaryMax = Math.max(...primaryRank.map(p => Math.abs(activeSort === "progress" ? (p.progress || 0) : activeSort === "money" ? p.money : p[activeSort] || 0)), 0.1);
    const primaryCol = grid.createEl('div', { cls: 'pc-rank-list' });
    primaryCol.createEl('div', { cls: 'pc-section-header', text: `📊 排序榜：${sortField.label} Top ${primaryShown.length}/${primaryRank.length}` });
    primaryShown.forEach(project => {
        const raw = activeSort === "progress" ? (project.progress || 0) : activeSort === "money" ? project.money : project[activeSort] || 0;
        const pct = Math.min(100, (Math.abs(raw) / Math.abs(primaryMax)) * 100);
        const color = activeSort === "money" && raw < 0
            ? 'var(--c-danger)'
            : activeSort === "progress"
                ? progressColor(raw)
                : 'var(--c-accent)';
        const value = activeSort === "money"
            ? fmtMoney(raw)
            : activeSort === "progress"
                ? `${raw.toFixed(0)}%`
                : activeSort === "time"
                    ? `${raw.toFixed(1)}🍅`
                    : `${raw}`;
        renderRankItem(primaryCol, project, value, pct, color);
    });

    const progressCol = grid.createEl('div', { cls: 'pc-rank-list' });
    progressCol.createEl('div', { cls: 'pc-section-header', text: '⚠️ 进度预警' });
    const progressList = data
        .filter(project => project.targetEffort > 0 && (project.progress || 0) >= 80)
        .sort((a, b) => (b.progress || 0) - (a.progress || 0));
    if (!progressList.length) {
        progressCol.createEl('div', { cls: 'pc-empty', text: '暂无接近目标的项目' });
    } else {
        progressCol.querySelector('.pc-section-header').textContent = `⚠️ 进度预警 Top ${Math.min(RANK_LIMIT, progressList.length)}/${progressList.length}`;
        progressList.slice(0, RANK_LIMIT).forEach(project => renderRankItem(progressCol, project, `${project.progress.toFixed(0)}%`, Math.min(100, project.progress), progressColor(project.progress)));
    }

    const costCol = grid.createEl('div', { cls: 'pc-rank-list' });
    costCol.createEl('div', { cls: 'pc-section-header', text: '💸 费用超支' });
    const costList = data.filter(project => project.money < 0).sort((a, b) => a.money - b.money);
    if (!costList.length) {
        costCol.createEl('div', { cls: 'pc-empty', text: '暂无项目成本记录' });
    } else {
        const maxCost = Math.max(...costList.map(project => Math.abs(project.money)), 0.1);
        costCol.querySelector('.pc-section-header').textContent = `💸 费用超支 Top ${Math.min(RANK_LIMIT, costList.length)}/${costList.length}`;
        costList.slice(0, RANK_LIMIT).forEach(project => renderRankItem(costCol, project, fmtMoney(project.money), Math.min(100, Math.abs(project.money) / maxCost * 100), 'var(--c-danger)'));
    }

    const rosterSection = contentHost.createEl('div');
    rosterSection.createEl('div', { cls: 'pc-section-header', text: '📇 项目名录' });
    const tableWrap = rosterSection.createEl('div', { cls: 'pc-table-wrap' });
    const table = tableWrap.createEl('table', { cls: 'pc-table' });
    const columns = [
        { label: '名称', key: 'name', sort: (a, b) => a.name.localeCompare(b.name, 'zh-CN') },
        { label: '类型', key: 'tags', sort: (a, b) => (b.tags?.length || 0) - (a.tags?.length || 0) },
        { label: '状态', key: 'status', sort: (a, b) => String(a.status || "").localeCompare(String(b.status || ""), 'zh-CN') },
        { label: '最近活动', key: 'lastInteraction', sort: (a, b) => (ViewKit.toTimestamp(b.lastInteraction) || 0) - (ViewKit.toTimestamp(a.lastInteraction) || 0) },
        { label: '投入', key: 'time', sort: (a, b) => b.time - a.time, align: 'right' },
        { label: '成本', key: 'money', sort: (a, b) => a.money - b.money, align: 'right' },
        { label: '进度', key: 'progress', sort: (a, b) => (b.progress || 0) - (a.progress || 0) },
    ];
    const sortIndex = { date: 3, time: 4, money: 5, progress: 6, count: 4 };
    let sortCol = sortIndex[activeSort] ?? 4;
    let sortAsc = Boolean(state.sortAsc);

    const thead = table.createEl('thead');
    const headerRow = thead.createEl('tr');
    columns.forEach((col, ci) => {
        const th = headerRow.createEl('th');
        if (col.align) th.style.textAlign = col.align;
        th.innerHTML = `${col.label}<span class="sort-arrow">${ci === sortCol ? (sortAsc ? '▲' : '▼') : '⇅'}</span>`;
        if (ci === sortCol) th.classList.add('sorted');
        th.addEventListener('click', () => {
            if (sortCol === ci) sortAsc = !sortAsc;
            else { sortCol = ci; sortAsc = false; }
            rebuildTable();
        });
    });

    const tbody = table.createEl('tbody');
    const cardList = tableWrap.createEl('div', { cls: 'pc-card-list' });
    const loadMoreTrigger = tableWrap.createEl('div', { style: 'height: 40px; line-height: 40px; text-align: center; color: var(--c-text-muted); font-size: 0.9em;', text: '正在加载...' });
    let currentPage = 1;
    const PAGE_SIZE = 20;
    let filteredSortedData = [];
    let isMobileView = false;

    function renderRow(project) {
        const avatar = esc(String(project.name || "?").charAt(0).toUpperCase());
        const mobileTags = project.tags || [];
        const lastDate = project.lastInteraction ? project.lastInteraction.toFormat("MM-dd") : "-";
        const path = esc(project.path);
        const name = esc(project.name);

        if (!isMobileView) {
            const tr = tbody.insertRow();
            const tagsHtml = mobileTags.map(tag => `<span class="pc-tag">${esc(String(tag).replace("#", ""))}</span>`).join("");
            tr.innerHTML = `
                <td><div style="display:flex;align-items:center"><span class="pc-avatar">${avatar}</span><a class="internal-link" href="${path}" style="font-weight:500;color:var(--c-text-normal);text-decoration:none">${name}</a></div></td>
                <td>${tagsHtml || '<span style="color:var(--c-text-muted)">-</span>'}</td>
                <td>${renderStatus(project.status)}</td>
                <td style="color:var(--c-text-muted);font-size:0.9em">${lastDate}</td>
                <td style="text-align:right;font-family:monospace;color:var(--c-accent);font-weight:bold">${project.time.toFixed(1)}</td>
                <td style="text-align:right;font-family:monospace;color:${project.money < 0 ? 'var(--c-danger)' : 'var(--c-success)'}">${fmtMoney(project.money)}</td>
                <td>${renderProgress(project)}</td>
            `;
        } else {
            const card = cardList.createEl('div', { cls: 'pc-mobile-card' });
            const mobileTagsHtml = [
                ...mobileTags.slice(0, 3).map(tag => `<span class="pc-tag">${esc(String(tag).replace("#", ""))}</span>`),
                mobileTags.length > 3 ? `<span class="pc-tag">+${mobileTags.length - 3}</span>` : "",
            ].filter(Boolean).join("");
            card.innerHTML = `
                <div class="pc-mobile-card-head">
                    <div class="pc-mobile-card-title"><span class="pc-avatar">${avatar}</span><a class="internal-link" href="${path}" style="font-weight:600;color:var(--c-text-normal);text-decoration:none">${name}</a></div>
                    ${renderStatus(project.status)}
                </div>
                <div class="pc-mobile-card-meta">
                    <span>最近 ${lastDate}</span>
                    <span>互动 ${project.count}</span>
                </div>
                <div class="pc-mobile-card-stats">
                    <div class="pc-mobile-stat"><div class="pc-mobile-stat-label">投入</div><div class="pc-mobile-stat-val" style="color:var(--c-accent)">${project.time.toFixed(1)}</div></div>
                    <div class="pc-mobile-stat"><div class="pc-mobile-stat-label">成本</div><div class="pc-mobile-stat-val" style="color:${project.money < 0 ? 'var(--c-danger)' : 'var(--c-success)'}">${fmtMoney(project.money)}</div></div>
                </div>
                ${renderProgress(project)}
                <div class="pc-mobile-card-tags">${mobileTagsHtml || '<span class="pc-tag">-</span>'}</div>
            `;
        }
    }

    function renderPage() {
        const end = currentPage * PAGE_SIZE;

        tbody.innerHTML = '';
        cardList.innerHTML = '';

        table.style.display = isMobileView ? 'none' : 'table';
        cardList.style.display = isMobileView ? 'flex' : 'none';

        filteredSortedData.slice(0, end).forEach(project => renderRow(project));
        loadMoreTrigger.style.display = end >= filteredSortedData.length ? 'none' : 'block';
    }

    function rebuildTable() {
        headerRow.querySelectorAll('th').forEach((th, ci) => {
            th.className = ci === sortCol ? 'sorted' : '';
            th.querySelector('.sort-arrow').textContent = ci === sortCol ? (sortAsc ? '▲' : '▼') : '⇅';
        });
        filteredSortedData = [...data].sort((a, b) => {
            const result = columns[sortCol].sort(a, b);
            return sortAsc ? -result : result;
        });
        currentPage = 1;
        isMobileView = detectMobileView();
        renderPage();
    }

    rebuildTable();

    if (typeof ResizeObserver !== 'undefined') {
        const ro = new ResizeObserver(entries => {
            for (let entry of entries) {
                const newMobileView = detectMobileView();
                if (newMobileView !== isMobileView) {
                    isMobileView = newMobileView;
                    rebuildTable();
                }
            }
        });
        ro.observe(container);
    }

    if (typeof IntersectionObserver !== 'undefined') {
        const observer = new IntersectionObserver((entries) => {
            if (entries[0].isIntersecting && currentPage * PAGE_SIZE < filteredSortedData.length) {
                currentPage++;
                renderPage();
            }
        }, { root: tableWrap, rootMargin: '100px' });
        observer.observe(loadMoreTrigger);
    }
}

function renderCollectionDebug(data = projectData, state = {}) {
    if (!debugHost) return;
    debugHost.innerHTML = "";
    ViewKit.renderDebugPanel(debugHost, {
        title: "Debug",
        interaction: state,
        rows: [
            ["view", "ProjectCollection"],
            ["object pages", summaryResult.objectPages?.length || 0],
            ["source paths", summaryResult.sourcePaths?.length || 0],
            ["source entries", entries?.length || 0],
            ["summaries", projectData.length],
            ["visible summaries", data?.length || 0],
            ["matches", summaryResult.matches?.size || 0],
            ["available tags", allTags.length],
            ["available links", ViewKit.collectLinks(projectData).length],
            ["warnings", 0],
            ["interaction", state || {}],
        ],
    });
}

renderDashboard();
new ViewKit.FilterBar(filterHost, {
    controls: ['search', 'tags', 'links', 'dateRange', 'sort'],
    availableTags: allTags,
    sortFields,
    initial: { sort: 'time', sortAsc: false },
    storageKey: 'ProjectCollection',
    onFilter: (data, state, isEmpty) => {
        renderAll(data, state, isEmpty);
        renderCollectionDebug(data, state);
    },
}).bind(projectData);
