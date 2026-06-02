/**
 * Diary living-cost KPI widget.
 *
 * Renders the daily living-cost KPI card using Utils.collectLivingCostItems().
 * Called unconditionally from DiaryProfile to show the aggregated daily cost.
 */

const core = {};
await dv.view("11 scripts/Core/FinanceCore", core);
const { Utils } = core;

const target = input?.container || dv.container;
const asOfDate = input?.asOfDate || new Date();
const lcDate = new Date(asOfDate.ts || asOfDate);
lcDate.setHours(0, 0, 0, 0);

const lcItems = Utils.collectLivingCostItems({ dv, asOfDate: lcDate });
const lcActiveItems = lcItems
    .filter(i => i.isActive)
    .map(i => ({
        name: i.name,
        dailyCost: i.actualDailyCost ?? i.dailyCost,
        elapsed: i.elapsed,
        days: i.progressBaseDays || i.days,
    }));
const livingCostDaily = lcActiveItems.reduce((sum, item) => sum + item.dailyCost, 0);

const lcTooltip = lcActiveItems.length > 0
    ? `🟢 在役 ${lcActiveItems.length} 项\n` + lcActiveItems
        .sort((a, b) => b.dailyCost - a.dailyCost)
        .map(i => `${i.name}  ¥${i.dailyCost.toFixed(1)}/天  ${i.elapsed}/${i.days}天`)
        .join("\n")
    : "暂无在役生活成本项";

if (target.setAttr) target.setAttr("title", lcTooltip);
else target.setAttribute?.("title", lcTooltip);
target.innerHTML = `
    <div class="dp-kpi-icon">🏠</div>
    <div class="dp-kpi-val" style="color:var(--interactive-accent)">¥${livingCostDaily.toFixed(1)}</div>
    <div class="dp-kpi-label">日均成本</div>
`;
