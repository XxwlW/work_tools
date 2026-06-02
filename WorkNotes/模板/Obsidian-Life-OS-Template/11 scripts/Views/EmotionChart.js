/**
 * 情感曲线视图脚本 (EmotionChart.js) v1.2
 *
 * 功能：
 * 1. 统计当前页面的反向链接中的情感记录（默认模式）
 * 2. 或统计日期范围内所有文件的情感记录（全局模式）
 * 3. 按日期聚合，绘制情感变化曲线
 * 4. 支持悬浮显示详细数据
 *
 * 调用方式：
 * dv.view("11 scripts/Views/EmotionChart", { global: true, startDate: "2025-11-01" })
 */

// --- 1. 导入核心库 ---
const core = {};
await dv.view("11 scripts/Core/FinanceCore", core);
const { Utils: CoreUtils, Query } = core;

// --- 2. 参数处理 ---
const targetName = dv.current().file.name;
const targetPath = dv.current().file.path;
const isGlobalMode = input?.global === true;

// 日期范围：默认最近30天
const now = new Date();
const defaultStart = new Date(now.getTime() - 30 * 24 * 60 * 60 * 1000);
defaultStart.setHours(0, 0, 0, 0); // 开始于当天 00:00:00

const startDateStr = input?.startDate;
const endDateStr = input?.endDate;

// 确保 startDate 是当天的开始
let startDate = startDateStr ? new Date(startDateStr) : defaultStart;
startDate.setHours(0, 0, 0, 0);

// 确保 endDate 是当天的结束
let endDate = endDateStr ? new Date(endDateStr) : now;
endDate.setHours(23, 59, 59, 999);

// 辅助函数：格式化本地日期为 YYYY-MM-DD（避免时区问题）
function formatLocalDate(date) {
    const y = date.getFullYear();
    const m = String(date.getMonth() + 1).padStart(2, '0');
    const d = String(date.getDate()).padStart(2, '0');
    return `${y}-${m}-${d}`;
}

// --- 3. 数据采集 ---
const emotionByDay = {}; // { "2025-12-01": [scores...], ... }
const allRecords = []; // 用于 E-I 计算

const sources = isGlobalMode
    ? { allowGlobal: true }
    : { linkedTo: true };

const rules = {
    type: 'event', // 情感通常伴随事件，排除记账项
    startDate: startDate,
    endDate: endDate
};

if (!isGlobalMode) {
    rules.explicitTarget = true; // 非全局时，要求条目链接向当前人物/页面
}

const entries = Query().from(sources).filter(rules).execute();

for (const entry of entries) {
    const score = entry.vector.emotion;
    if (score === 0) continue;

    // 基准时间：优先匹配显式的 @时间，再找文件级创建时间
    const fTime = entry.sourcePage.file.day || entry.sourcePage.file.frontmatter?.["创建时间"] || entry.sourcePage.file.ctime;
    // 抹平时区与对象差异
    const fTimeJs = fTime.ts ? new Date(fTime.ts) : new Date(fTime);
    const eDate = entry.meta.explicitDate || fTimeJs;

    const dateKey = formatLocalDate(eDate);

    if (!emotionByDay[dateKey]) emotionByDay[dateKey] = [];
    emotionByDay[dateKey].push(score);
    allRecords.push({ score: score, date: eDate });
}

// --- 4. 数据处理 ---

const dates = [];
let current = new Date(startDate);
while (current <= endDate) {
    dates.push(formatLocalDate(current));
    current.setDate(current.getDate() + 1);
}

const dailyAvg = [];
const dailyCount = [];
const cumulativeEI = [];

for (let i = 0; i < dates.length; i++) {
    const dateKey = dates[i];
    const scores = emotionByDay[dateKey] || [];
    const avg = scores.length > 0 ? scores.reduce((a, b) => a + b, 0) / scores.length : null;
    dailyAvg.push(avg);
    dailyCount.push(scores.length);

    const recordsUntilDate = allRecords.filter(r => {
        const rTs = r.date?.ts || r.date?.getTime?.() || 0;
        return rTs <= new Date(dateKey + "T23:59:59").getTime();
    });
    const eiScore = CoreUtils.calculateEmotionScore(recordsUntilDate);
    cumulativeEI.push(eiScore);
}

// --- 5. 渲染图表 ---
const container = dv.container;
const chartId = 'ec_' + Math.random().toString(36).substr(2, 9);

const styles = document.createElement('style');
styles.textContent = `
    .ec-container {
        padding: 20px;
        background: var(--background-primary);
        border-radius: 12px;
        font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, sans-serif;
    }
    .ec-header {
        display: flex;
        justify-content: space-between;
        align-items: center;
        margin-bottom: 20px;
        flex-wrap: wrap;
        gap: 12px;
    }
    .ec-title-wrap {
        display: flex;
        align-items: center;
        gap: 10px;
    }
    .ec-title {
        font-size: 1.3em;
        font-weight: 600;
        color: var(--text-normal);
        margin: 0;
    }
    .ec-mode-badge {
        font-size: 0.7em;
        padding: 3px 10px;
        border-radius: 12px;
        background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
        color: white;
        font-weight: 500;
    }
    .ec-date-range {
        font-size: 0.9em;
        color: var(--text-muted);
        background: var(--background-secondary);
        padding: 6px 12px;
        border-radius: 8px;
    }
    .ec-chart-area {
        position: relative;
        background: var(--background-secondary);
        border-radius: 12px;
        padding: 20px;
        margin-bottom: 16px;
    }
    .ec-svg-container {
        width: 100%;
        height: 220px;
    }
    .ec-legend {
        display: flex;
        justify-content: center;
        gap: 32px;
        padding: 12px 0;
        border-top: 1px solid var(--background-modifier-border);
        margin-top: 12px;
    }
    .ec-legend-item {
        display: flex;
        align-items: center;
        gap: 8px;
        font-size: 0.85em;
        color: var(--text-muted);
    }
    .ec-legend-line {
        width: 24px;
        height: 3px;
        border-radius: 2px;
    }
    .ec-legend-line.dashed {
        background: repeating-linear-gradient(90deg, #F59E0B, #F59E0B 4px, transparent 4px, transparent 8px);
    }
    .ec-summary {
        display: grid;
        grid-template-columns: repeat(4, 1fr);
        gap: 12px;
    }
    .ec-stat {
        text-align: center;
        padding: 16px 12px;
        background: var(--background-secondary);
        border-radius: 10px;
        transition: transform 0.2s, box-shadow 0.2s;
    }
    .ec-stat:hover {
        transform: translateY(-2px);
        box-shadow: 0 4px 12px rgba(0,0,0,0.1);
    }
    .ec-stat-val {
        font-size: 1.5em;
        font-weight: 700;
        margin-bottom: 4px;
    }
    .ec-stat-label {
        font-size: 0.8em;
        color: var(--text-muted);
    }
    .ec-no-data {
        text-align: center;
        padding: 60px 20px;
        color: var(--text-muted);
        font-size: 1.1em;
    }
    .ec-tooltip {
        position: absolute;
        background: var(--background-primary);
        border: 1px solid var(--background-modifier-border);
        border-radius: 8px;
        padding: 10px 14px;
        font-size: 0.85em;
        box-shadow: 0 4px 16px rgba(0,0,0,0.15);
        pointer-events: none;
        opacity: 0;
        transition: opacity 0.15s;
        z-index: 100;
        white-space: nowrap;
    }
    .ec-tooltip.visible {
        opacity: 1;
    }
    .ec-tooltip-date {
        font-weight: 600;
        color: var(--text-normal);
        margin-bottom: 6px;
    }
    .ec-tooltip-row {
        display: flex;
        align-items: center;
        gap: 8px;
        margin: 4px 0;
    }
    .ec-tooltip-dot {
        width: 8px;
        height: 8px;
        border-radius: 50%;
    }
    .ec-data-point {
        cursor: pointer;
        transition: r 0.15s;
    }
    .ec-data-point:hover {
        r: 7;
    }
    .ec-container, .ec-container * {
        box-sizing: border-box;
    }
    .ec-container {
        max-width: 100%;
        overflow-x: hidden;
        container-type: inline-size;
        container-name: view-container;
    }
    .ec-title-wrap, .ec-chart-area, .ec-summary {
        min-width: 0;
    }
    .ec-chart-area {
        min-height: 280px;
    }
    .ec-svg-container {
        min-height: 220px;
    }
    .ec-tooltip {
        max-width: min(280px, calc(100% - 20px));
        white-space: normal;
    }
    @container view-container (max-width: 760px) {
        .ec-container { padding: 14px; }
        .ec-header { align-items: flex-start; }
        .ec-title { font-size: 1.1em; overflow-wrap: anywhere; }
        .ec-chart-area { padding: 14px; min-height: 260px; }
        .ec-legend { gap: 14px; flex-wrap: wrap; }
        .ec-summary { grid-template-columns: repeat(2, minmax(0, 1fr)); }
    }
    @container view-container (max-width: 520px) {
        .ec-container { padding: 10px; border-radius: 8px; }
        .ec-header { gap: 8px; margin-bottom: 12px; }
        .ec-title-wrap { flex-wrap: wrap; }
        .ec-date-range { width: 100%; }
        .ec-chart-area { padding: 10px; min-height: 240px; }
        .ec-svg-container { height: 200px; min-height: 200px; }
        .ec-legend { justify-content: flex-start; padding: 8px 0; }
        .ec-summary { grid-template-columns: 1fr; gap: 8px; }
        .ec-stat { padding: 10px 8px; }
    }
`;
container.appendChild(styles);

const wrapper = container.createEl('div', { cls: 'ec-container' });

// 标题栏
const header = wrapper.createEl('div', { cls: 'ec-header' });
const titleWrap = header.createEl('div', { cls: 'ec-title-wrap' });
titleWrap.createEl('h3', { cls: 'ec-title', text: isGlobalMode ? '全局情感曲线' : `${targetName} 情感曲线` });
if (isGlobalMode) {
    titleWrap.createEl('span', { cls: 'ec-mode-badge', text: '全局' });
}
header.createEl('span', { cls: 'ec-date-range', text: `📅 ${formatLocalDate(startDate)} → ${formatLocalDate(endDate)}` });

if (allRecords.length === 0) {
    wrapper.createEl('div', { cls: 'ec-no-data', text: '📊 该时间范围内暂无情感记录' });
} else {
    const chartArea = wrapper.createEl('div', { cls: 'ec-chart-area' });

    // Tooltip
    const tooltip = chartArea.createEl('div', { cls: 'ec-tooltip' });

    // SVG 绘制
    const svgNS = "http://www.w3.org/2000/svg";
    const width = 700;
    const height = 200;
    const paddingLeft = 45;
    const paddingRight = 50; // 增加右侧空间给第二个 Y 轴
    const paddingTop = 20;
    const paddingBottom = 30;
    const chartWidth = width - paddingLeft - paddingRight;
    const chartHeight = height - paddingTop - paddingBottom;

    const svg = document.createElementNS(svgNS, "svg");
    svg.setAttribute("class", "ec-svg-container");
    svg.setAttribute("viewBox", `0 0 ${width} ${height}`);
    svg.setAttribute("preserveAspectRatio", "xMidYMid meet");

    // 左 Y 轴：当日情感 (-3 ~ +3)
    const yScaleDaily = (val) => paddingTop + ((3 - val) / 6) * chartHeight;

    // 右 Y 轴：E-I 累计余额（动态范围）
    const eiMin = Math.min(...cumulativeEI, 0);
    const eiMax = Math.max(...cumulativeEI, 0);
    const eiRange = Math.max(Math.abs(eiMin), Math.abs(eiMax), 1) * 1.2; // 加 20% 边距
    const yScaleEI = (val) => paddingTop + ((eiRange - val) / (eiRange * 2)) * chartHeight;

    const xScale = (i) => paddingLeft + (i / Math.max(dates.length - 1, 1)) * chartWidth;

    // 背景网格（左侧 -3 ~ +3）
    for (let v = -3; v <= 3; v++) {
        const y = yScaleDaily(v);
        const line = document.createElementNS(svgNS, "line");
        line.setAttribute("x1", paddingLeft);
        line.setAttribute("y1", y);
        line.setAttribute("x2", width - paddingRight);
        line.setAttribute("y2", y);
        line.setAttribute("stroke", v === 0 ? "#9CA3AF" : "rgba(156,163,175,0.2)");
        line.setAttribute("stroke-width", v === 0 ? "1.5" : "1");
        if (v === 0) line.setAttribute("stroke-dasharray", "6,4");
        svg.appendChild(line);

        // 左 Y 轴标签
        const text = document.createElementNS(svgNS, "text");
        text.setAttribute("x", paddingLeft - 10);
        text.setAttribute("y", y + 4);
        text.setAttribute("text-anchor", "end");
        text.setAttribute("fill", "#60A5FA");
        text.setAttribute("font-size", "11");
        text.textContent = v > 0 ? `+${v}` : v.toString();
        svg.appendChild(text);
    }

    // 右 Y 轴标签（E-I 累计余额）
    const eiTicks = [-eiRange, -eiRange / 2, 0, eiRange / 2, eiRange];
    eiTicks.forEach(v => {
        const y = yScaleEI(v);
        const text = document.createElementNS(svgNS, "text");
        text.setAttribute("x", width - paddingRight + 8);
        text.setAttribute("y", y + 4);
        text.setAttribute("text-anchor", "start");
        text.setAttribute("fill", "#F59E0B");
        text.setAttribute("font-size", "10");
        text.textContent = v >= 0 ? `+${v.toFixed(0)}` : v.toFixed(0);
        svg.appendChild(text);
    });

    // X 轴标签（每隔几天显示）
    const labelInterval = Math.max(1, Math.floor(dates.length / 8));
    dates.forEach((d, i) => {
        if (i % labelInterval === 0 || i === dates.length - 1) {
            const text = document.createElementNS(svgNS, "text");
            text.setAttribute("x", xScale(i));
            text.setAttribute("y", height - 8);
            text.setAttribute("text-anchor", "middle");
            text.setAttribute("fill", "#9CA3AF");
            text.setAttribute("font-size", "10");
            text.textContent = d.substring(5); // MM-DD
            svg.appendChild(text);
        }
    });

    // E-I 曲线（橙色虚线）- 使用右 Y 轴
    let eiPath = "";
    const eiPoints = [];
    cumulativeEI.forEach((val, i) => {
        const x = xScale(i);
        const y = yScaleEI(val);
        eiPath += (i === 0 ? "M" : "L") + `${x},${y} `;
        eiPoints.push({ x, y, val, date: dates[i] });
    });
    const eiLine = document.createElementNS(svgNS, "path");
    eiLine.setAttribute("d", eiPath);
    eiLine.setAttribute("fill", "none");
    eiLine.setAttribute("stroke", "#F59E0B");
    eiLine.setAttribute("stroke-width", "2.5");
    eiLine.setAttribute("stroke-dasharray", "6,4");
    eiLine.setAttribute("stroke-linecap", "round");
    svg.appendChild(eiLine);

    // 当日情感曲线（蓝色实线）- 使用左 Y 轴
    let avgPath = "";
    let firstPoint = true;
    const dataPoints = [];

    dailyAvg.forEach((val, i) => {
        if (val !== null) {
            const x = xScale(i);
            const y = yScaleDaily(val);
            avgPath += (firstPoint ? "M" : "L") + `${x},${y} `;
            firstPoint = false;
            dataPoints.push({ x, y, val, date: dates[i], count: dailyCount[i], ei: cumulativeEI[i] });
        }
    });

    // 填充区域
    if (avgPath) {
        const areaPath = document.createElementNS(svgNS, "path");
        const firstX = dataPoints[0]?.x || paddingLeft;
        const lastX = dataPoints[dataPoints.length - 1]?.x || width - paddingRight;
        areaPath.setAttribute("d", avgPath + `L${lastX},${yScaleDaily(0)} L${firstX},${yScaleDaily(0)} Z`);
        areaPath.setAttribute("fill", "url(#blueGradient)");
        areaPath.setAttribute("opacity", "0.3");
        svg.appendChild(areaPath);
    }

    // 渐变定义
    const defs = document.createElementNS(svgNS, "defs");
    const gradient = document.createElementNS(svgNS, "linearGradient");
    gradient.setAttribute("id", "blueGradient");
    gradient.setAttribute("x1", "0%");
    gradient.setAttribute("y1", "0%");
    gradient.setAttribute("x2", "0%");
    gradient.setAttribute("y2", "100%");
    const stop1 = document.createElementNS(svgNS, "stop");
    stop1.setAttribute("offset", "0%");
    stop1.setAttribute("stop-color", "#60A5FA");
    const stop2 = document.createElementNS(svgNS, "stop");
    stop2.setAttribute("offset", "100%");
    stop2.setAttribute("stop-color", "#60A5FA");
    stop2.setAttribute("stop-opacity", "0");
    gradient.appendChild(stop1);
    gradient.appendChild(stop2);
    defs.appendChild(gradient);
    svg.insertBefore(defs, svg.firstChild);

    // 曲线
    const avgLine = document.createElementNS(svgNS, "path");
    avgLine.setAttribute("d", avgPath);
    avgLine.setAttribute("fill", "none");
    avgLine.setAttribute("stroke", "#60A5FA");
    avgLine.setAttribute("stroke-width", "3");
    avgLine.setAttribute("stroke-linecap", "round");
    avgLine.setAttribute("stroke-linejoin", "round");
    svg.appendChild(avgLine);

    // 数据点（带悬浮交互）
    dataPoints.forEach((pt, idx) => {
        const circle = document.createElementNS(svgNS, "circle");
        circle.setAttribute("cx", pt.x);
        circle.setAttribute("cy", pt.y);
        circle.setAttribute("r", "5");
        circle.setAttribute("fill", "#60A5FA");
        circle.setAttribute("stroke", "white");
        circle.setAttribute("stroke-width", "2");
        circle.setAttribute("class", "ec-data-point");
        circle.setAttribute("data-idx", idx);

        circle.addEventListener('mouseenter', (e) => {
            const eiLabel = CoreUtils.getEmotionLabel(pt.ei);
            tooltip.innerHTML = `
                <div class="ec-tooltip-date">${pt.date}</div>
                <div class="ec-tooltip-row">
                    <span class="ec-tooltip-dot" style="background:#60A5FA"></span>
                    当日情感: <strong>${pt.val > 0 ? '+' : ''}${pt.val.toFixed(1)}</strong> (${pt.count}条)
                </div>
                <div class="ec-tooltip-row">
                    <span class="ec-tooltip-dot" style="background:#F59E0B"></span>
                    E-I 评分: <strong style="color:${eiLabel.color}">${eiLabel.emoji}${eiLabel.score}</strong>
                </div>
            `;
            tooltip.style.left = `${pt.x / width * 100}%`;
            tooltip.style.top = `${pt.y - 10}px`;
            tooltip.style.transform = 'translate(-50%, -100%)';
            tooltip.classList.add('visible');
        });

        circle.addEventListener('mouseleave', () => {
            tooltip.classList.remove('visible');
        });

        svg.appendChild(circle);
    });

    // E-I 数据点（橙色，带悬浮交互）
    eiPoints.forEach((pt, idx) => {
        const circle = document.createElementNS(svgNS, "circle");
        circle.setAttribute("cx", pt.x);
        circle.setAttribute("cy", pt.y);
        circle.setAttribute("r", "4");
        circle.setAttribute("fill", "#F59E0B");
        circle.setAttribute("stroke", "white");
        circle.setAttribute("stroke-width", "1.5");
        circle.setAttribute("class", "ec-data-point");
        circle.setAttribute("data-type", "ei");

        circle.addEventListener('mouseenter', (e) => {
            const eiLabel = CoreUtils.getEmotionLabel(pt.val);
            const dailyVal = dailyAvg[idx];
            const hasDaily = dailyVal !== null;
            tooltip.innerHTML = `
                <div class="ec-tooltip-date">${pt.date}</div>
                <div class="ec-tooltip-row">
                    <span class="ec-tooltip-dot" style="background:#F59E0B"></span>
                    E-I 评分: <strong style="color:${eiLabel.color}">${eiLabel.emoji}${eiLabel.score}</strong>
                </div>
                ${hasDaily ? `<div class="ec-tooltip-row">
                    <span class="ec-tooltip-dot" style="background:#60A5FA"></span>
                    当日情感: <strong>${dailyVal > 0 ? '+' : ''}${dailyVal.toFixed(1)}</strong>
                </div>` : '<div class="ec-tooltip-row" style="color:var(--text-muted)">当日无情感记录</div>'}
            `;
            tooltip.style.left = `${pt.x / width * 100}%`;
            tooltip.style.top = `${pt.y - 10}px`;
            tooltip.style.transform = 'translate(-50%, -100%)';
            tooltip.classList.add('visible');
        });

        circle.addEventListener('mouseleave', () => {
            tooltip.classList.remove('visible');
        });

        svg.appendChild(circle);
    });

    chartArea.appendChild(svg);

    // 图例
    const legend = chartArea.createEl('div', { cls: 'ec-legend' });
    legend.innerHTML = `
        <div class="ec-legend-item">
            <div class="ec-legend-line" style="background:#60A5FA"></div>
            <span>当日情感</span>
        </div>
        <div class="ec-legend-item">
            <div class="ec-legend-line dashed"></div>
            <span>E-I 累积评分</span>
        </div>
    `;

    // 统计摘要
    const totalRecords = allRecords.length;
    const currentEI = cumulativeEI[cumulativeEI.length - 1] || 0;
    const eiLabel = CoreUtils.getEmotionLabel(currentEI);
    const validAvgs = dailyAvg.filter(v => v !== null);
    const maxEmotion = validAvgs.length > 0 ? Math.max(...validAvgs) : 0;
    const minEmotion = validAvgs.length > 0 ? Math.min(...validAvgs) : 0;

    const summary = wrapper.createEl('div', { cls: 'ec-summary' });
    summary.innerHTML = `
        <div class="ec-stat">
            <div class="ec-stat-val" style="color:var(--text-normal)">${totalRecords}</div>
            <div class="ec-stat-label">情感记录数</div>
        </div>
        <div class="ec-stat">
            <div class="ec-stat-val" style="color:${eiLabel.color}">${eiLabel.emoji}${eiLabel.score}</div>
            <div class="ec-stat-label">当前 E-I 评分</div>
        </div>
        <div class="ec-stat">
            <div class="ec-stat-val" style="color:#34D399">+${maxEmotion.toFixed(1)}</div>
            <div class="ec-stat-label">最高情感</div>
        </div>
        <div class="ec-stat">
            <div class="ec-stat-val" style="color:#EF4444">${minEmotion.toFixed(1)}</div>
            <div class="ec-stat-label">最低情感</div>
        </div>
    `;
}
