/**
 * 人物集合视图脚本 (PersonCollection.js) v3.0
 *
 * 功能：
 * 1. 按人物 supertag 收集人物文件（默认 tags: 人物/人，可由 input/frontmatter 覆盖）。
 * 2. 聚合计算每个人物的：陪伴时长(Time)、情感价值(Emotion)、资金往来(Money)、最近互动时间。
 * 3. 展示：全景仪表盘、排行榜(条形图)、生日提醒(紧迫度)、人脉列表。
 * 4. 交互：标签筛选、表头排序、搜索过滤、响应式布局。
 */

// --- 1. 导入核心库 ---
const core = {};
await dv.view("11 scripts/Core/FinanceCore", core);
const { Utils: CoreUtils, Query, ObjectSummary } = core;
const viewKit = {};
await dv.view("11 scripts/Core/ViewKit", viewKit);
const { ViewKit } = viewKit;

const Utils = { ...CoreUtils, getBirthdayInfo: ViewKit.getBirthdayInfo, LunarUtils: ViewKit.LunarUtils, escapeHtml: ViewKit.escapeHtml };
const esc = value => ViewKit.escapeHtml(value);

// --- 样式注入 ---
const styles = `
.pc-container {
    --c-bg-card: var(--background-secondary); --c-bg-hover: var(--background-secondary-alt);
    --c-border: var(--background-modifier-border); --c-accent: var(--interactive-accent);
    --c-success: var(--color-green); --c-danger: var(--color-red);
    --c-text-muted: var(--text-muted); --c-text-normal: var(--text-normal);
    font-family: var(--font-interface); display: flex; flex-direction: column; gap: 20px;
    container-type: inline-size; container-name: view-container; max-width: 100%; overflow-x: hidden;
}
.pc-dashboard { display: grid; grid-template-columns: repeat(auto-fit, minmax(120px, 1fr)); gap: 12px; flex-shrink: 0; }
.pc-kpi-card {
    background: var(--c-bg-card); border: 1px solid var(--c-border); border-radius: 12px; padding: 15px; text-align: center;
    display: flex; flex-direction: column; justify-content: center; align-items: center;
    box-shadow: 0 2px 4px rgba(0,0,0,0.05); transition: transform 0.2s, box-shadow 0.2s;
}
.pc-kpi-card:hover { transform: translateY(-2px); box-shadow: 0 4px 12px rgba(0,0,0,0.1); }
.pc-kpi-icon { font-size: 1.8em; margin-bottom: 6px; opacity: 0.8; }
.pc-kpi-val { font-size: 1.5em; font-weight: 700; color: var(--c-text-normal); line-height: 1.2; }
.pc-kpi-label { font-size: 0.85em; color: var(--c-text-muted); margin-top: 4px; }
.pc-section-header {
    display: flex; align-items: center; gap: 8px; margin-bottom: 10px;
    font-size: 1.1em; font-weight: 600; color: var(--c-text-normal);
    border-bottom: 2px solid var(--c-border); padding-bottom: 6px;
}
.pc-rank-grid { display: grid; grid-template-columns: 1fr 1fr 1fr; gap: 15px; flex-shrink: 0; }
.pc-rank-list {
    background: var(--c-bg-card); border: 1px solid var(--c-border); border-radius: 8px;
    max-height: 320px; overflow-y: auto; display: flex; flex-direction: column; position: relative;
}
.pc-rank-list .pc-section-header { position: sticky; top: 0; background: var(--c-bg-card); z-index: 10; margin: 0; padding: 10px; border-bottom: 2px solid var(--c-border); }
.pc-rank-item, .pc-birthday-card { margin: 0 10px; }
.pc-rank-item:first-of-type { margin-top: 10px; }
.pc-rank-item:last-of-type, .pc-birthday-card:last-of-type { margin-bottom: 10px; }
.pc-rank-item { display: flex; justify-content: space-between; align-items: center; padding: 6px 8px; border-bottom: 1px dashed var(--c-border); }
.pc-rank-name { font-weight: 500; color: var(--c-text-normal); text-decoration: none; white-space: nowrap; overflow: hidden; text-overflow: ellipsis; }
.pc-rank-val { font-weight: bold; font-size: 0.9em; flex-shrink: 0; }
.pc-rank-bar-wrap { display: flex; align-items: center; gap: 6px; min-width: 100px; }
.pc-rank-bar { height: 6px; border-radius: 3px; transition: width 0.4s ease; min-width: 2px; }
.pc-birthday-card {
    background: linear-gradient(135deg, rgba(255,152,0,0.08) 0%, rgba(255,87,34,0.08) 100%);
    border: 1px solid rgba(255,152,0,0.3); border-radius: 8px; padding: 10px 15px;
    display: flex; align-items: center; gap: 10px; margin-bottom: 8px;
    border-left: 4px solid var(--c-border); transition: border-color 0.2s;
}
.pc-birthday-card.critical { border-left-color: #ef4444; animation: pc-pulse 1.5s infinite; }
.pc-birthday-card.urgent { border-left-color: #f59e0b; }
.pc-birthday-card.soon { border-left-color: #eab308; }
.pc-birthday-card.normal { border-left-color: #60a5fa; }
.pc-birthday-card.distant { border-left-color: var(--c-border); }
@keyframes pc-pulse { 0%,100% { opacity:1; } 50% { opacity:0.7; } }
.pc-bday-progress { width: 100%; height: 4px; background: var(--c-border); border-radius: 2px; margin-top: 4px; }
.pc-bday-progress-bar { height: 100%; border-radius: 2px; transition: width 0.3s; }
.pc-search {
    width: 100%; padding: 8px 12px; border-radius: 8px; border: 1px solid var(--c-border);
    background: var(--background-primary); color: var(--c-text-normal); font-size: 0.9em;
    margin-bottom: 8px; outline: none; transition: border-color 0.2s; box-sizing: border-box;
}
.pc-search:focus { border-color: var(--c-accent); }
.pc-table-wrap { background: var(--c-bg-card); border: 1px solid var(--c-border); border-radius: 8px; max-height: 600px; overflow: auto; position: relative; -webkit-overflow-scrolling: touch; overscroll-behavior-x: contain; }
.pc-table { width: 100%; min-width: 760px; border-collapse: separate; border-spacing: 0; font-size: 0.9em; }
.pc-table th {
    text-align: left; color: var(--c-text-muted); padding: 10px; border-bottom: 2px solid var(--c-border);
    font-weight: 600; cursor: pointer; position: sticky; top: 0; background: var(--c-bg-card); z-index: 10;
    user-select: none; white-space: nowrap; transition: color 0.2s;
}
.pc-table th:hover { color: var(--c-accent); }
.pc-table th .sort-arrow { font-size: 0.75em; margin-left: 4px; opacity: 0.4; }
.pc-table th.sorted .sort-arrow { opacity: 1; color: var(--c-accent); }
.pc-table td { padding: 8px 10px; border-bottom: 1px solid var(--c-border); vertical-align: middle; }
.pc-table tr:hover td { background: var(--c-bg-hover); }
.pc-avatar { width: 24px; height: 24px; border-radius: 50%; background: var(--c-accent); color: white; display: inline-flex; align-items: center; justify-content: center; font-size: 0.8em; margin-right: 8px; flex-shrink: 0; }
.pc-tag { font-size: 0.75em; padding: 1px 6px; border-radius: 4px; background: var(--background-primary); border: 1px solid var(--c-border); color: var(--c-text-muted); margin-right: 4px; }
.pc-source-badge { align-self: flex-start; font-size: 0.75em; color: var(--c-text-muted); border: 1px solid var(--c-border); border-radius: 999px; padding: 2px 8px; background: var(--background-primary); margin-bottom: -8px; }
.pc-card-list { display: none; flex-direction: column; gap: 8px; }
.pc-mobile-card {
    border: 1px solid var(--c-border); border-radius: 8px; background: var(--background-primary);
    padding: 10px; display: flex; flex-direction: column; gap: 8px; min-width: 0;
}
.pc-mobile-card-head { display: flex; align-items: center; justify-content: space-between; gap: 10px; min-width: 0; }
.pc-mobile-card-title { display: flex; align-items: center; min-width: 0; flex: 1; }
.pc-mobile-card-title a { min-width: 0; overflow: hidden; text-overflow: ellipsis; white-space: nowrap; }
.pc-mobile-card-date { flex: 0 0 auto; color: var(--c-text-muted); font-size: 0.82em; }
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
    .pc-table-wrap { max-height: clamp(360px, 58vh, 640px); overflow-y: auto; overflow-x: hidden; }
    .pc-kpi-card { padding: 10px 8px; }
    .pc-rank-item, .pc-birthday-card { min-height: 40px; }
    .pc-tag { min-height: 40px; display: inline-flex; align-items: center; margin-right: 0; }
}
`;
dv.container.innerHTML = `<style>${styles}</style>`;
const container = dv.container.createEl('div', { cls: 'pc-container' });

// --- 1. 数据采集 (统一管线: 文件去重解析) ---
// 获取所有人物文件
const currentFm = dv.current()?.file?.frontmatter || {};
const RANK_LIMIT = Math.max(1, Number(input?.rankLimit || input?.["排行数量"] || currentFm.rankLimit || currentFm["排行数量"] || 30) || 30);
const configuredPersonTags = input?.personTags || input?.["人物标签组"] || currentFm.personTags || currentFm["人物标签组"];
const personTag = configuredPersonTags || input?.["标签"] || input?.tag || currentFm["人物标签"] || currentFm.personTag || ["人物", "人"];
const personScope = input?.personScope || input?.["人物目录"] || currentFm.personScope || currentFm["人物目录"] || null;
const personPages = Utils.collectSupertagPages({ tag: personTag, scope: personScope });
const personSourceLabel = `supertag tags:${Utils.normalizeSupertagInput(personTag).join("/") || "人物"}`;
container.createEl('div', { cls: 'pc-source-badge', text: personSourceLabel });

function createPersonSummary(p) {
    const file = p.file;
    const rawTypes = p.类型 || p.type;
    const extractedTypes = Utils.normalizeArrayField(rawTypes).map(t => {
        if (t.includes('/')) return t.split('/').pop().replace('.md', '');
        return t;
    }).filter(t => t);

    return {
        name: file.name, link: file.link, path: file.path, tags: extractedTypes,
        totalTime: 0, netMoney: 0, lastInteraction: null, interactionCount: 0,
        emotionRecords: [], page: p  // 保留 page 引用用于后续生日计算
    };
}

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

// 1.2 一次性解析所有去重后的 inlink 文件；同一个源文件只进入 Query 一次
const summaryResult = ObjectSummary.collect({
    objectPages: personPages,
    Query,
    createSummary: createPersonSummary,
    accumulate(data, entry) {
        const actualItemTime = toDateTime(Utils.resolveEntryDate(entry));
        if (!actualItemTime) return;

        if (!data.lastInteraction || actualItemTime > data.lastInteraction) {
            data.lastInteraction = actualItemTime;
        }

        data.totalTime += entry.vector.time || 0;
        if (entry.type === "event" && entry.vector.emotion !== 0) {
            data.emotionRecords.push({ score: entry.vector.emotion, date: actualItemTime });
        }
        const isTransfer = entry.meta?.tags?.includes("转账");
        data.netMoney += entry.type === "journal" && !isTransfer ? (entry.vector.money || 0) : 0;
        data.interactionCount++;
    },
});
const entries = summaryResult.entries;
const peopleMap = summaryResult.summaryMap;

// 1.3 生成最终 peopleData 数组
let peopleData = [];
for (const [name, data] of peopleMap) {
    const birthdayInfo = Utils.getBirthdayInfo(data.page);
    const weightedEmotion = Utils.calculateEmotionScore(data.emotionRecords);

    peopleData.push({
        name: data.name,
        link: data.link,
        path: data.path,
        tags: data.tags,
        displayText: `${data.name} ${(data.tags || []).join(' ')}`,
        ctime: data.lastInteraction,
        time: data.totalTime,
        emotion: weightedEmotion,
        emotionRecords: data.emotionRecords,
        money: data.netMoney,
        lastInteraction: data.lastInteraction,
        count: data.interactionCount,
        birthday: birthdayInfo
    });
}


// --- 2. 收集标签 & 渲染函数 ---
const allTags = ViewKit.collectTags(peopleData);
const dashHost = container.createEl('div');
const filterHost = container.createEl('div');
const debugHost = input?.debug ? container.createEl('div') : null;
const contentHost = container.createEl('div');

function renderDashboard() {
    dashHost.innerHTML = "";
    const totalPeople = peopleData.length;
    const grandTotalTime = peopleData.reduce((s, p) => s + p.time, 0);
    const allEmotionRecords = peopleData.flatMap(p => p.emotionRecords || []);
    const globalEmotion = Utils.calculateEmotionScore(allEmotionRecords);
    const globalEmotionLabel = Utils.getEmotionLabel(globalEmotion);
    const activePeople = peopleData.filter(p => {
        const ts = ViewKit.toTimestamp(p.lastInteraction);
        return ts && (Date.now() - ts) < 30 * 24 * 60 * 60 * 1000;
    }).length;

    const dash = dashHost.createEl('div', { cls: 'pc-dashboard' });
    dash.innerHTML = `
        <div class="pc-kpi-card"><div class="pc-kpi-icon">👥</div><div class="pc-kpi-val">${totalPeople}</div><div class="pc-kpi-label">人脉总数</div></div>
        <div class="pc-kpi-card"><div class="pc-kpi-icon">🔥</div><div class="pc-kpi-val">${activePeople}</div><div class="pc-kpi-label">月活核心圈</div></div>
        <div class="pc-kpi-card"><div class="pc-kpi-icon">⏳</div><div class="pc-kpi-val">${grandTotalTime.toFixed(1)}🍅</div><div class="pc-kpi-label">总陪伴投入</div></div>
        <div class="pc-kpi-card"><div class="pc-kpi-icon">${globalEmotionLabel.emoji}</div><div class="pc-kpi-val" style="color:${globalEmotionLabel.color}">${globalEmotionLabel.score}</div><div class="pc-kpi-label">${globalEmotionLabel.label}</div></div>
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

function renderAll(data, state = {}) {
    contentHost.innerHTML = "";
    const activeSort = state.sort || 'time';

    // 2.3 排行榜 (3 列)
    const grid = contentHost.createEl('div', { cls: 'pc-rank-grid' });

    const metricDefs = {
        time: { title: '⏳ 筛选排行：陪伴', value: p => p.time, format: p => `${p.time.toFixed(1)}🍅`, color: 'var(--c-accent)' },
        emotion: { title: '❤ 筛选排行：情感', value: p => p.emotion, format: p => Utils.getEmotionLabel(p.emotion).score, color: 'var(--c-success)' },
        money: { title: '💰 筛选排行：资金', value: p => p.money, format: p => ViewKit.fmtMoney(p.money), color: 'var(--c-success)' },
        count: { title: '📅 筛选排行：互动', value: p => p.count, format: p => `${p.count} 次`, color: 'var(--c-accent)' },
    };
    const metric = metricDefs[activeSort] || metricDefs.time;
    const primaryRank = [...data];
    const primaryShown = primaryRank.slice(0, RANK_LIMIT);
    const maxMetric = Math.max(...primaryRank.map(p => Math.abs(metric.value(p))), 0.1);
    const timeCol = grid.createEl('div', { cls: 'pc-rank-list' });
    timeCol.createEl('div', { cls: 'pc-section-header', text: `${metric.title} Top ${primaryShown.length}/${primaryRank.length}` });
    timeCol.innerHTML += primaryShown.map((p, i) => {
        const rawValue = metric.value(p);
        const pct = Math.min(100, (Math.abs(rawValue) / maxMetric) * 100);
        const name = esc(p.name);
        const path = esc(p.path);
        const color = rawValue < 0 ? 'var(--c-danger)' : metric.color;
        return `<div class="pc-rank-item">
            <div style="display:flex;align-items:center;min-width:0;flex:1"><span class="pc-rank-idx">${i+1}</span><a class="pc-rank-name internal-link" href="${path}">${name}</a></div>
            <div class="pc-rank-bar-wrap"><div class="pc-rank-bar" style="width:${pct}%;background:${color}"></div><span class="pc-rank-val" style="color:${color}">${metric.format(p)}</span></div>
        </div>`;
    }).join('');

    // 情感榜 (带条形图)
    const emoRank = [...data].sort((a, b) => b.emotion - a.emotion);
    const emoShown = emoRank.slice(0, RANK_LIMIT);
    const emoCol = grid.createEl('div', { cls: 'pc-rank-list' });
    emoCol.createEl('div', { cls: 'pc-section-header', text: `❤ 情感榜 Top ${emoShown.length}/${emoRank.length}` });
    emoCol.innerHTML += emoShown.map((p, i) => {
        const emoLabel = Utils.getEmotionLabel(p.emotion);
        const absMax = emoRank.length > 0 ? Math.max(Math.abs(emoRank[0].emotion), Math.abs(emoRank[emoRank.length-1].emotion), 0.1) : 1;
        const pct = Math.min(100, (Math.abs(p.emotion) / absMax) * 100);
        const barColor = p.emotion >= 0 ? 'var(--c-success)' : 'var(--c-danger)';
        const name = esc(p.name);
        const path = esc(p.path);
        return `<div class="pc-rank-item">
            <div style="display:flex;align-items:center;min-width:0;flex:1"><span class="pc-rank-idx">${i+1}</span><a class="pc-rank-name internal-link" href="${path}">${name}</a></div>
            <div class="pc-rank-bar-wrap"><div class="pc-rank-bar" style="width:${pct}%;background:${barColor}"></div><span class="pc-rank-val" style="color:${emoLabel.color}">${emoLabel.emoji}${emoLabel.score}</span></div>
        </div>`;
    }).join('');

    // 生日雷达 (带紧迫度)
    const bdayList = data.filter(p => p.birthday && !p.birthday.error).sort((a, b) => a.birthday.days - b.birthday.days);
    const bdayCol = grid.createEl('div', { cls: 'pc-rank-list' });
    bdayCol.createEl('div', { cls: 'pc-section-header', text: '🎂 生日雷达' });
    if (bdayList.length === 0) {
        bdayCol.createEl('div', { text: "近期没有人生日哦~", style: "color:var(--c-text-muted); font-size:0.9em; padding:10px;" });
    } else {
        bdayCol.querySelector('.pc-section-header').textContent = `🎂 生日雷达 Top ${Math.min(RANK_LIMIT, bdayList.length)}/${bdayList.length}`;
        bdayCol.innerHTML += bdayList.slice(0, RANK_LIMIT).map(p => {
            const d = p.birthday.days;
            const urgency = d <= 3 ? 'critical' : d <= 7 ? 'urgent' : d <= 30 ? 'soon' : d <= 90 ? 'normal' : 'distant';
            const urgColor = d <= 3 ? 'var(--color-red)' : d <= 7 ? 'var(--color-orange)' : d <= 30 ? 'var(--color-yellow)' : d <= 90 ? 'var(--color-blue)' : 'var(--c-border)';
            const progressPct = Math.max(0, Math.min(100, ((90 - d) / 90) * 100));
            const isLunar = p.birthday.isLunar;
            const targetStr = p.birthday.targetDate ? moment(p.birthday.targetDate).format("MM-DD") : "?";
            const originalStr = p.birthday.original ? p.birthday.original.toFormat("MM-dd") : "??";
            const name = esc(p.name);
            const path = esc(p.path);
            return `<div class="pc-birthday-card ${urgency}">
                <div style="font-size:1.5em">🎂</div>
                <div style="flex:1;min-width:0">
                    <div style="font-weight:bold">
                        <a class="internal-link" href="${path}" style="color:inherit;text-decoration:none">${name}</a>
                        ${isLunar ? '<span style="font-size:0.7em;background:var(--background-modifier-border);color:var(--text-normal);padding:1px 4px;border-radius:4px;margin-left:4px;">农</span>' : ''}
                    </div>
                    <div style="font-size:0.8em;color:var(--c-text-muted)">
                        ${isLunar ? '农历' + originalStr + ' (转公历' + targetStr + ')' : originalStr}
                        · 还有 <span style="color:${urgColor};font-weight:bold;">${d}</span> 天
                    </div>
                    <div class="pc-bday-progress"><div class="pc-bday-progress-bar" style="width:${progressPct}%;background:${urgColor}"></div></div>
                </div>
            </div>`;
        }).join('');
    }

    // 2.4 人脉名录 (全宽独立区域)
    const rosterSection = contentHost.createEl('div');
    rosterSection.createEl('div', { cls: 'pc-section-header', text: '📇 人脉名录' });

    // 表格
    const tableWrap = rosterSection.createEl('div', { cls: 'pc-table-wrap' });
    const table = tableWrap.createEl('table', { cls: 'pc-table' });

    const columns = [
        { label: '姓名', key: 'name', sort: (a, b) => a.name.localeCompare(b.name, 'zh-CN') },
        { label: '类型', key: 'tags', sort: (a, b) => (b.tags?.length || 0) - (a.tags?.length || 0) },
        { label: '最近互动', key: 'lastInteraction', sort: (a, b) => (b.lastInteraction || 0) - (a.lastInteraction || 0) },
        { label: '互动', key: 'count', sort: (a, b) => b.count - a.count, align: 'right' },
        { label: '陪伴(🍅)', key: 'time', sort: (a, b) => b.time - a.time, align: 'right' },
        { label: '情感指数', key: 'emotion', sort: (a, b) => b.emotion - a.emotion, align: 'right' },
        { label: '资金往来', key: 'money', sort: (a, b) => b.money - a.money, align: 'right' },
    ];

    const sortIndex = { count: 3, time: 4, emotion: 5, money: 6 };
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
            if (sortCol === ci) { sortAsc = !sortAsc; } else { sortCol = ci; sortAsc = false; }
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

    function renderRow(p) {
        const avatar = esc(String(p.name || "?").charAt(0).toUpperCase());
        const tags = p.tags || [];
        const lastDate = p.lastInteraction ? p.lastInteraction.toFormat("MM-dd") : "-";
        const emoLabel = Utils.getEmotionLabel(p.emotion);
        const name = esc(p.name);
        const path = esc(p.path);

        if (!isMobileView) {
            const tr = tbody.insertRow();
            const tagsHtml = tags.map(t => `<span class="pc-tag">${esc(String(t).replace("#", ""))}</span>`).join("");
            tr.innerHTML = `
                <td><div style="display:flex;align-items:center"><span class="pc-avatar">${avatar}</span><a class="internal-link" href="${path}" style="font-weight:500;color:var(--c-text-normal);text-decoration:none">${name}</a></div></td>
                <td>${tagsHtml || '<span style="color:var(--c-text-muted)">-</span>'}</td>
                <td style="color:var(--c-text-muted);font-size:0.9em">${lastDate}</td>
                <td style="text-align:right;font-family:monospace">${p.count}</td>
                <td style="text-align:right;font-family:monospace;color:var(--c-accent);font-weight:bold">${p.time.toFixed(1)}</td>
                <td style="text-align:right;font-family:monospace;color:${emoLabel.color}">${emoLabel.emoji}${emoLabel.score}</td>
                <td style="text-align:right;font-family:monospace;color:${p.money >= 0 ? 'var(--c-success)' : 'var(--c-danger)'}">${ViewKit.fmtMoney(p.money)}</td>
            `;
        } else {
            const card = cardList.createEl('div', { cls: 'pc-mobile-card' });
            const mobileTagsHtml = [
                ...tags.slice(0, 3).map(t => `<span class="pc-tag">${esc(String(t).replace("#", ""))}</span>`),
                tags.length > 3 ? `<span class="pc-tag">+${tags.length - 3}</span>` : "",
            ].filter(Boolean).join("");
            card.innerHTML = `
                <div class="pc-mobile-card-head">
                    <div class="pc-mobile-card-title"><span class="pc-avatar">${avatar}</span><a class="internal-link" href="${path}" style="font-weight:600;color:var(--c-text-normal);text-decoration:none">${name}</a></div>
                    <span class="pc-mobile-card-date">${lastDate}</span>
                </div>
                <div class="pc-mobile-card-stats">
                    <div class="pc-mobile-stat"><div class="pc-mobile-stat-label">互动</div><div class="pc-mobile-stat-val">${p.count}</div></div>
                    <div class="pc-mobile-stat"><div class="pc-mobile-stat-label">陪伴</div><div class="pc-mobile-stat-val" style="color:var(--c-accent)">${p.time.toFixed(1)}</div></div>
                    <div class="pc-mobile-stat"><div class="pc-mobile-stat-label">情感</div><div class="pc-mobile-stat-val" style="color:${emoLabel.color}">${emoLabel.emoji}${emoLabel.score}</div></div>
                    <div class="pc-mobile-stat"><div class="pc-mobile-stat-label">资金</div><div class="pc-mobile-stat-val" style="color:${p.money >= 0 ? 'var(--c-success)' : 'var(--c-danger)'}">${ViewKit.fmtMoney(p.money)}</div></div>
                </div>
                <div class="pc-mobile-card-tags">${mobileTagsHtml || '<span class="pc-tag">-</span>'}</div>
            `;
        }
    }

    function renderPage() {
        const end = currentPage * PAGE_SIZE;
        const slice = filteredSortedData.slice(0, end);

        tbody.innerHTML = '';
        cardList.innerHTML = '';
        table.style.display = isMobileView ? 'none' : 'table';
        cardList.style.display = isMobileView ? 'flex' : 'none';

        slice.forEach(p => renderRow(p));

        if (end >= filteredSortedData.length) {
            loadMoreTrigger.style.display = 'none';
        } else {
            loadMoreTrigger.style.display = 'block';
        }
    }

    function rebuildTable() {
        // 更新表头
        headerRow.querySelectorAll('th').forEach((th, ci) => {
            th.className = ci === sortCol ? 'sorted' : '';
            th.querySelector('.sort-arrow').textContent = ci === sortCol ? (sortAsc ? '▲' : '▼') : '⇅';
        });

        // 过滤和排序
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
        const ro = new ResizeObserver(() => {
            const next = detectMobileView();
            if (next !== isMobileView) {
                isMobileView = next;
                rebuildTable();
            }
        });
        ro.observe(container);
    }

    // 交叉观察器：滚动加载更多
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


function renderCollectionDebug(data = peopleData, state = {}) {
    if (!debugHost) return;
    debugHost.innerHTML = "";
    ViewKit.renderDebugPanel(debugHost, {
        title: "Debug",
        interaction: state,
        rows: [
            ["view", "PersonCollection"],
            ["object pages", summaryResult.objectPages?.length || 0],
            ["source paths", summaryResult.sourcePaths?.length || 0],
            ["source entries", entries?.length || 0],
            ["summaries", peopleData.length],
            ["visible summaries", data?.length || 0],
            ["matches", summaryResult.matches?.size || 0],
            ["available tags", allTags.length],
            ["available links", ViewKit.collectLinks(peopleData).length],
            ["warnings", 0],
            ["interaction", state || {}],
        ],
    });
}

renderDashboard();
new ViewKit.FilterBar(filterHost, {
    controls: ['search', 'tags', 'links', 'dateRange', 'sort'],
    availableTags: allTags,
    sortFields: ViewKit.filterSortFields(['time', 'emotion', 'money', 'count']),
    initial: { sort: 'time', sortAsc: false },
    storageKey: 'PersonCollection',
    onFilter: (data, state, isEmpty) => {
        renderAll(data, state, isEmpty);
        renderCollectionDebug(data, state);
    },
}).bind(peopleData);
