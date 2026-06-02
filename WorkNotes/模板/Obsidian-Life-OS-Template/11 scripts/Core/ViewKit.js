/**
 * Shared view utilities for DataviewJS views.
 *
 * This file intentionally has no dependency on FinanceCore exports so views can
 * load it as a lightweight adapter with dv.view("11 scripts/Core/ViewKit").
 */

const ViewKit = {
    processText(text, tags, currentName, isJournal = true) {
        let str = String(text || "");
        if (tags && tags.length > 0) {
            const pattern = new RegExp(tags.map(t => t.replace("#", "\\#")).join("|"), "g");
            str = str.replace(pattern, "");
        }

        let wallet = null;
        let walletFound = false;
        const linkRegex = /\[\[([^\]|]+)(?:\|([^\]]+))?\]\]/g;

        if (isJournal) {
            str = str.replace(linkRegex, (match, path, display) => {
                if (walletFound) return match;
                if (path.includes(currentName)) return "";
                wallet = { path, display: display || path };
                walletFound = true;
                return "";
            });
            str = str.replace(linkRegex, (match, path, display) => display || path);
        } else {
            str = str.replace(linkRegex, (match, path, display) => {
                if (path.includes(currentName)) return "";
                return display || path;
            });
        }

        return { text: str.trim(), wallet };
    },

    fmtMoney(num) {
        const value = Number(num) || 0;
        return value.toLocaleString("en-US", { minimumFractionDigits: 2, maximumFractionDigits: 2 });
    },

    exportToCSV(data, filename, headers = null) {
        if (!data || !data.length) {
            new Notice("数据为空，无法导出");
            return;
        }

        const columns = headers || [
            { label: "描述", value: t => t.text },
            { label: "日期", value: t => t.ctime?.toFormat ? t.ctime.toFormat("yyyy-MM-dd HH:mm:ss") : "" },
            { label: "金额", value: t => t.vec?.[0] ?? "" },
            { label: "情感", value: t => t.vec?.[1] ?? "" },
            { label: "时间", value: t => t.vec?.[2] ?? "" },
            { label: "标签", value: t => (t.tags || []).join(";") },
            ...(data.some(t => t.wallet) ? [{ label: "支付方式", value: t => t.wallet ? t.wallet.display : "" }] : []),
            { label: "源文件", value: t => t.path },
        ];

        const escapeCsv = value => `"${String(value ?? "").replace(/"/g, '""')}"`;
        const BOM = "\uFEFF";
        const csvContent = BOM + [
            columns.map(c => c.label).join(","),
            ...data.map(row => columns.map(c => escapeCsv(typeof c.value === "function" ? c.value(row) : row[c.value])).join(",")),
        ].join("\n");

        const blob = new Blob([csvContent], { type: "text/csv;charset=utf-8;" });
        const link = document.createElement("a");
        link.href = URL.createObjectURL(blob);
        link.download = filename;
        document.body.appendChild(link);
        link.click();
        document.body.removeChild(link);
    },

    _styles: `
.vk-fb-host {
    container-type: inline-size;
    container-name: view-container;
    max-width: 100%;
}
.vk-fb-bar {
    display: block;
    min-height: 40px;
    margin: 0 0 12px;
    padding: 10px;
    border: 1px solid color-mix(in srgb, var(--background-modifier-border) 72%, transparent);
    border-radius: 10px;
    background: color-mix(in srgb, var(--background-primary) 78%, var(--background-secondary));
    color: var(--text-normal);
    box-sizing: border-box;
    box-shadow: 0 1px 3px rgba(0, 0, 0, 0.035);
}
.vk-fb-main {
    display: flex;
    align-items: flex-start;
    gap: 7px;
    flex-wrap: wrap;
    width: 100%;
    min-width: 0;
}
.vk-fb-controls {
    display: flex;
    align-items: center;
    gap: 7px;
    order: 40;
    flex: 1 1 100%;
    flex-wrap: wrap;
    min-width: 0;
    padding-top: 7px;
    border-top: 1px solid color-mix(in srgb, var(--background-modifier-border) 52%, transparent);
}
.vk-fb-status {
    display: flex;
    align-items: center;
    justify-content: flex-start;
    gap: 6px;
    order: 50;
    flex: 1 1 auto;
    min-height: 30px;
    margin-left: auto;
}
.vk-fb-date-group,
.vk-fb-sort-group {
    display: inline-flex;
    align-items: center;
    gap: 6px;
    flex: 0 0 auto;
    min-width: 0;
}
.vk-fb-date-group::before,
.vk-fb-sort-group::before {
    color: var(--text-muted);
    font-size: 0.78em;
    line-height: 1;
    white-space: nowrap;
}
.vk-fb-date-group::before { content: "日期"; }
.vk-fb-sort-group::before { content: "排序"; }
.vk-fb-search,
.vk-fb-date,
.vk-fb-sort {
    height: 30px;
    min-width: 0;
    border: 1px solid color-mix(in srgb, var(--background-modifier-border) 84%, transparent);
    border-radius: 6px;
    background: var(--background-primary);
    color: var(--text-normal);
    font-size: 0.85em;
    padding: 2px 8px;
    box-sizing: border-box;
}
.vk-fb-search {
    order: 10;
    flex: 1 1 220px;
    max-width: 320px;
}
.vk-fb-date { flex: 0 0 128px; width: 128px; }
.vk-fb-sort { flex: 0 0 112px; width: 112px; }
.vk-fb-date-sep { color: var(--text-muted); font-size: 0.85em; }
.vk-fb-tags {
    display: flex;
    align-items: flex-start;
    align-content: center;
    gap: 6px;
    order: 20;
    flex: 10 1 360px;
    flex-wrap: wrap;
    min-width: 0;
    max-height: 76px;
    overflow-x: hidden;
    overflow-y: auto;
    scrollbar-gutter: stable;
    padding: 1px 2px 1px 0;
}
.vk-fb-tags.is-expanded {
    max-height: 132px;
    overflow-y: auto;
    padding-right: 2px;
}
.vk-fb-tags::-webkit-scrollbar { width: 4px; }
.vk-fb-tags::-webkit-scrollbar-thumb {
    background: color-mix(in srgb, var(--background-modifier-border) 90%, transparent);
    border-radius: 999px;
}
.vk-fb-tags::before {
    content: "标签";
    align-self: flex-start;
    flex: 0 0 auto;
    color: var(--text-muted);
    font-size: 0.78em;
    line-height: 1;
    margin: 8px 1px 0 0;
}
.vk-fb-links {
    order: 30;
    flex-basis: 100%;
    max-height: 80px;
    padding-top: 7px;
    border-top: 1px solid color-mix(in srgb, var(--background-modifier-border) 52%, transparent);
}
.vk-fb-links.is-expanded {
    max-height: 136px;
}
.vk-fb-links::before { content: "链接"; }
button.vk-fb-tag,
button.vk-fb-clear,
button.vk-fb-match,
button.vk-fb-sort-dir,
button.vk-fb-more {
    display: inline-flex;
    align-items: center;
    justify-content: center;
    flex: 0 0 auto;
    width: auto !important;
    min-width: 0;
    max-width: 100%;
    height: 30px;
    border: 1px solid color-mix(in srgb, var(--background-modifier-border) 84%, transparent);
    border-radius: 6px;
    background: color-mix(in srgb, var(--background-primary) 86%, transparent);
    color: var(--text-muted);
    font-size: 0.82em;
    line-height: 1;
    padding: 0 9px;
    cursor: pointer;
    white-space: nowrap;
    box-sizing: border-box;
    overflow: hidden;
    text-overflow: ellipsis;
}
button.vk-fb-tag { max-width: 128px; }
button.vk-fb-tag:hover,
button.vk-fb-clear:hover,
button.vk-fb-match:hover,
button.vk-fb-sort-dir:hover,
button.vk-fb-more:hover {
    border-color: var(--interactive-accent);
    color: var(--interactive-accent);
}
button.vk-fb-tag.active {
    background: color-mix(in srgb, var(--interactive-accent) 13%, var(--background-primary));
    border-color: var(--interactive-accent);
    color: var(--interactive-accent);
}
button.vk-fb-more {
    order: 999;
    background: transparent;
    color: var(--text-muted);
}
button.vk-fb-clear {
    opacity: 0.68;
}
button.vk-fb-clear.is-active {
    opacity: 1;
    border-color: var(--interactive-accent);
    color: var(--interactive-accent);
    background: color-mix(in srgb, var(--interactive-accent) 10%, transparent);
}
button.vk-fb-match {
    font-weight: 650;
    min-width: 44px;
}
button.vk-fb-sort-dir { min-width: 30px; padding: 0; }
.vk-fb-count {
    color: var(--text-muted);
    font-size: 0.78em;
    white-space: nowrap;
    border: 1px solid color-mix(in srgb, var(--background-modifier-border) 70%, transparent);
    border-radius: 999px;
    background: color-mix(in srgb, var(--background-primary) 76%, transparent);
    padding: 5px 9px;
}
.vk-fb-empty {
    display: none;
    margin: -2px 0 10px;
    padding: 10px;
    border: 1px dashed var(--background-modifier-border);
    border-radius: 8px;
    color: var(--text-muted);
    text-align: center;
    font-size: 0.9em;
    cursor: pointer;
}
.vk-fb-empty.is-visible { display: block; }
@container view-container (max-width: 760px) {
    .vk-fb-bar { padding: 8px; }
    .vk-fb-search { flex: 1 1 100%; max-width: none; }
    .vk-fb-tags { flex-basis: 100%; max-height: 82px; }
    .vk-fb-links { max-height: 86px; }
    .vk-fb-controls { gap: 6px; }
    .vk-fb-date-group,
    .vk-fb-sort-group,
    button.vk-fb-clear,
    .vk-fb-status { flex: 1 1 auto; }
    .vk-fb-status { margin-left: 0; justify-content: flex-start; }
}
@container view-container (max-width: 520px) {
    .vk-fb-date-group,
    .vk-fb-sort-group { flex: 1 1 100%; }
    .vk-fb-date { flex: 1 1 0; width: auto; }
    .vk-fb-sort { flex: 1 1 0; width: auto; }
    button.vk-fb-clear { flex: 1 1 90px; }
    .vk-fb-status { flex-basis: 100%; }
}
.vk-segmented {
    display: flex;
    align-items: center;
    gap: 4px;
    max-width: 100%;
    min-height: 40px;
    margin: 0 0 10px;
    padding: 4px;
    overflow-x: auto;
    border: 1px solid color-mix(in srgb, var(--background-modifier-border) 72%, transparent);
    border-radius: 8px;
    background: color-mix(in srgb, var(--background-secondary) 70%, transparent);
    box-sizing: border-box;
    -webkit-overflow-scrolling: touch;
}
button.vk-segmented-button {
    display: inline-flex;
    align-items: center;
    justify-content: center;
    gap: 6px;
    flex: 0 0 auto;
    min-height: 40px;
    min-width: 74px;
    padding: 0 14px;
    border: 1px solid transparent;
    border-radius: 6px;
    background: transparent;
    color: var(--text-muted);
    font-size: 0.86em;
    line-height: 1;
    cursor: pointer;
    white-space: nowrap;
}
button.vk-segmented-button:hover {
    color: var(--interactive-accent);
    background: color-mix(in srgb, var(--interactive-accent) 8%, transparent);
}
button.vk-segmented-button.is-active {
    border-color: color-mix(in srgb, var(--interactive-accent) 42%, transparent);
    background: color-mix(in srgb, var(--interactive-accent) 16%, var(--background-primary));
    color: var(--text-normal);
    font-weight: 650;
}
.vk-segmented-count {
    color: var(--text-muted);
    font-size: 0.78em;
    font-weight: 500;
}
.vk-module {
    min-width: 0;
    border: 1px solid var(--background-modifier-border);
    border-radius: 8px;
    background: var(--background-primary);
    overflow: hidden;
}
.vk-module-header {
    display: flex;
    align-items: center;
    justify-content: space-between;
    gap: 8px;
    padding: 9px 11px;
    border-bottom: 1px solid var(--background-modifier-border);
    background: color-mix(in srgb, var(--background-secondary) 78%, transparent);
    font-weight: 700;
}
.vk-module-count {
    color: var(--text-muted);
    font-weight: 400;
    font-size: 0.82em;
    white-space: nowrap;
}
.vk-module-body {
    padding: 8px;
    display: flex;
    flex-direction: column;
    gap: 7px;
}
.vk-rank-list {
    display: flex;
    flex-direction: column;
    gap: 0;
    min-width: 0;
}
.vk-rank-row {
    display: flex;
    align-items: center;
    justify-content: space-between;
    gap: 8px;
    min-width: 0;
    padding: 7px 0;
    border-bottom: 1px solid color-mix(in srgb, var(--background-modifier-border) 70%, transparent);
}
.vk-rank-row:last-child { border-bottom: none; }
.vk-rank-name {
    min-width: 0;
    overflow: hidden;
    text-overflow: ellipsis;
    white-space: nowrap;
}
.vk-rank-meta {
    color: var(--text-muted);
    font-size: 0.8em;
    white-space: nowrap;
}
.vk-rank-evidence {
    display: inline-flex;
    align-items: center;
    gap: 4px;
    margin-left: 6px;
}
.vk-evidence-link {
    color: var(--text-muted);
    text-decoration: none;
    border: 1px solid var(--background-modifier-border);
    border-radius: 999px;
    padding: 1px 6px;
    font-size: 0.92em;
}
.vk-empty {
    color: var(--text-muted);
    text-align: center;
    padding: 18px 8px;
    font-size: 0.9em;
}
.vk-debug-body {
    display: flex;
    flex-direction: column;
    gap: 6px;
    font-family: var(--font-monospace);
    font-size: 0.78em;
}
.vk-debug-row {
    display: grid;
    grid-template-columns: minmax(120px, 0.35fr) minmax(0, 1fr);
    gap: 8px;
    min-width: 0;
}
.vk-debug-key { color: var(--text-muted); }
.vk-debug-value { min-width: 0; overflow-wrap: anywhere; color: var(--text-normal); }
@container view-container (max-width: 800px) {
    .vk-fb-main { flex-basis: 100%; }
    .vk-fb-search { flex: 0 1 240px; max-width: none; }
    .vk-fb-tags { flex: 1 1 280px; }
    .vk-fb-status { justify-content: flex-start; margin-left: 0; }
}
@container view-container (max-width: 500px) {
    .vk-fb-bar { align-items: stretch; }
    .vk-fb-main,
    .vk-fb-status,
    .vk-fb-search,
    .vk-fb-tags,
    .vk-fb-date-group,
    .vk-fb-sort-group {
        flex-basis: 100%;
        width: 100%;
    }
    .vk-segmented { min-height: 44px; }
    button.vk-segmented-button { min-height: 44px; }
    .vk-fb-date { flex: 1 1 0; width: auto; }
    .vk-fb-sort { flex: 1 1 0; width: auto; }
    .vk-fb-clear { margin-left: 0; }
    .vk-fb-status { justify-content: flex-start; }
    .vk-fb-search,
    .vk-fb-date,
    .vk-fb-sort,
    button.vk-fb-tag,
    button.vk-fb-clear,
    button.vk-fb-sort-dir,
    button.vk-fb-more {
        min-height: 40px;
        height: 40px;
    }
    button.vk-fb-tag,
    button.vk-fb-clear,
    button.vk-fb-more { padding: 0 10px; }
    .vk-fb-tags { max-height: 90px; overflow-y: auto; }
    .vk-fb-tags.is-expanded { max-height: 160px; }
}
`,

    _injectStyles(containerEl) {
        if (typeof document !== "undefined" && document.getElementById && document.head) {
            const id = "viewkit-filterbar-styles";
            const existing = document.getElementById(id);
            if (!existing) {
                const style = document.createElement("style");
                style.id = id;
                style.textContent = this._styles;
                document.head.appendChild(style);
            } else if (existing.textContent !== this._styles) {
                existing.textContent = this._styles;
            }
            return;
        }
        if (containerEl?.createEl && !containerEl._vkFbStyleInjected) {
            containerEl.createEl("style", { text: this._styles });
            containerEl._vkFbStyleInjected = true;
        }
    },

    renderSegmentedControl(host, options = {}) {
        if (!host) return null;

        host.innerHTML = "";
        host._vkFbStyleInjected = false;
        this._injectStyles(host);

        const optionList = Array.isArray(options.options) ? options.options : [];
        const className = ["vk-segmented", options.className || ""].filter(Boolean).join(" ");
        const bar = host.createEl("div", {
            cls: className,
            attr: { role: "tablist", "aria-label": options.ariaLabel || "视图筛选" },
        });

        for (const item of optionList) {
            const key = String(item.key);
            const active = key === String(options.value);
            const button = bar.createEl("button", {
                cls: `vk-segmented-button${active ? " is-active" : ""}${item.className ? ` ${item.className}` : ""}`,
                attr: {
                    type: "button",
                    role: "tab",
                    "aria-selected": active ? "true" : "false",
                    "aria-pressed": active ? "true" : "false",
                    "data-value": key,
                },
            });
            button.createEl("span", { cls: "vk-segmented-label", text: item.label ?? key });
            if (item.count !== undefined && item.count !== null) {
                button.createEl("span", { cls: "vk-segmented-count", text: String(item.count) });
            }
            button.onclick = () => {
                if (!active && typeof options.onChange === "function") options.onChange(key);
            };
        }

        return bar;
    },

    toTimestamp(value) {
        if (value == null || value === "") return null;
        if (typeof value === "number" && Number.isFinite(value)) return value;
        if (value instanceof Date) {
            const time = value.getTime();
            return Number.isFinite(time) ? time : null;
        }
        if (typeof value === "object") {
            if (typeof value.toMillis === "function") {
                const time = value.toMillis();
                if (Number.isFinite(time)) return time;
            }
            if (typeof value.ts === "number" && Number.isFinite(value.ts)) return value.ts;
            if (typeof value.valueOf === "function") {
                const time = value.valueOf();
                if (typeof time === "number" && Number.isFinite(time)) return time;
            }
        }
        const parsed = new Date(value).getTime();
        return Number.isFinite(parsed) ? parsed : null;
    },

    toDateInputValue(value) {
        if (!value) return "";
        if (typeof value === "string") {
            const match = value.match(/\d{4}-\d{2}-\d{2}/);
            if (match) return match[0];
        }
        if (value?.toFormat) {
            try {
                const formatted = value.toFormat("yyyy-MM-dd");
                if (formatted && formatted !== "Invalid DateTime") return formatted;
            } catch (_) {}
        }
        const ts = this.toTimestamp(value);
        if (ts == null) return "";
        const date = new Date(ts);
        return [
            date.getFullYear(),
            String(date.getMonth() + 1).padStart(2, "0"),
            String(date.getDate()).padStart(2, "0"),
        ].join("-");
    },

    normalizeFilterTag(tag) {
        return String(tag || "")
            .replace(/^#/, "")
            .replace(/\[\[[\s\S]*$/, "")
            .replace(/[\[\]]/g, "")
            .trim();
    },

    normalizeFilterLink(link) {
        const raw = typeof link === "object"
            ? (link.target || link.path || link.href || link.key || link.label || "")
            : link;
        return String(raw || "")
            .replace(/^\[\[/, "")
            .replace(/\]\]$/, "")
            .split("|")[0]
            .replace(/\.md$/i, "")
            .trim();
    },

    linkLabel(link) {
        const rawLabel = typeof link === "object" ? (link.label || link.display || link.name || "") : "";
        const target = this.normalizeFilterLink(link);
        const label = rawLabel || String(target).split(/[\\/]/).pop() || target;
        return String(label || target).trim();
    },

    collectTags(items = []) {
        return [...new Set((items || [])
            .flatMap(item => item?.tags || item?.meta?.tags || [])
            .map(tag => this.normalizeFilterTag(tag))
            .filter(Boolean))]
            .sort((a, b) => a.localeCompare(b, "zh-CN"));
    },

    collectLinks(items = []) {
        const byKey = new Map();
        const add = link => {
            const key = this.normalizeFilterLink(link);
            if (!key || byKey.has(key)) return;
            byKey.set(key, { key, label: this.linkLabel(link) });
        };
        for (const item of items || []) {
            const meta = item?.meta || {};
            for (const link of item?.links || item?.outlinks || meta.outlinks || []) add(link);
            for (const link of item?.linksDetailed || meta.linksDetailed || []) add(link);
            for (const part of item?.displayParts || []) {
                if (part?.type === "link") add(part);
            }
        }
        return [...byKey.values()].sort((a, b) => a.label.localeCompare(b.label, "zh-CN"));
    },

    filterSortFields(keys = []) {
        const defs = {
            date: { key: "date", label: "日期", fn: (a, b) => this.toTimestamp(b.ctime ?? b.date ?? b.lastInteraction) - this.toTimestamp(a.ctime ?? a.date ?? a.lastInteraction) },
            time: { key: "time", label: "时间", fn: (a, b) => this._filterNumber(b, "time") - this._filterNumber(a, "time") },
            emotion: { key: "emotion", label: "情感", fn: (a, b) => this._filterNumber(b, "emotion") - this._filterNumber(a, "emotion") },
            money: { key: "money", label: "金额", fn: (a, b) => Math.abs(this._filterNumber(b, "money")) - Math.abs(this._filterNumber(a, "money")) },
            count: { key: "count", label: "次数", fn: (a, b) => this._filterNumber(b, "count") - this._filterNumber(a, "count") },
            name: { key: "name", label: "名称", fn: (a, b) => String(a.name || a.displayText || "").localeCompare(String(b.name || b.displayText || ""), "zh-CN") },
        };
        return keys.map(item => typeof item === "string" ? defs[item] : item).filter(Boolean);
    },

    _filterNumber(item, key) {
        if (!item) return 0;
        if (key === "money") return Number(item.money ?? item.value ?? item.vec?.[0] ?? 0) || 0;
        if (key === "emotion") return Number(item.emotion ?? item.vec?.[1] ?? 0) || 0;
        if (key === "time") return Number(item.time ?? item.totalTime ?? item.vec?.[2] ?? 0) || 0;
        if (key === "count") return Number(item.count ?? item.interactionCount ?? item.length ?? 0) || 0;
        return Number(item[key] ?? 0) || 0;
    },

    _filterText(item) {
        if (!item) return "";
        const partText = Array.isArray(item.displayParts)
            ? item.displayParts.map(part => part?.label || part?.text || part?.target || "").join(" ")
            : "";
        return [
            item.displayText,
            item.cleanText,
            item.text,
            item.name,
            item.path,
            partText,
        ].filter(Boolean).join(" ").toLowerCase();
    },

    FilterBar: class FilterBar {
        constructor(containerEl, options = {}) {
            this.containerEl = containerEl;
            this.options = options;
            this.controls = new Set(options.controls || ["search", "tags", "dateRange", "sort"]);
            if (options.matchModeToggle !== false && (this.controls.has("tags") || this.controls.has("links"))) {
                this.controls.add("matchMode");
            }
            this.sortFields = options.sortFields || ViewKit.filterSortFields(["date"]);
            this.availableTags = (options.availableTags || []).map(tag => ViewKit.normalizeFilterTag(tag)).filter(Boolean);
            this.availableLinks = ViewKit.collectLinks((options.availableLinks || []).map(link => ({ links: [link] })));
            this.optionsProvidedTags = Array.isArray(options.availableTags);
            this.optionsProvidedLinks = Array.isArray(options.availableLinks);
            this.onFilter = typeof options.onFilter === "function" ? options.onFilter : () => {};
            this.showCount = options.showCount !== false;
            this.maxVisibleTags = Math.max(1, Number(options.maxVisibleTags || 8));
            this.tagsExpanded = false;
            this.linksExpanded = false;
            this.items = [];
            this.filteredItems = [];
            this.initialState = this._normalizeState({
                search: "",
                tags: [],
                links: [],
                matchMode: "and",
                startDate: "",
                endDate: "",
                sort: this.sortFields[0]?.key || "",
                sortAsc: false,
                ...(options.initial || {}),
            });
            this.storageKey = this._resolveStorageKey(options);
            this.state = this._normalizeState({ ...this.initialState, ...this._loadState() });
            if (this.containerEl?.addClass) this.containerEl.addClass("vk-fb-host");
            else this.containerEl?.classList?.add("vk-fb-host");
            ViewKit._injectStyles(containerEl);
            this.render();
        }

        _resolveStorageKey(options) {
            if (options.persist === false || options.storageKey === false) return null;
            if (options.storageKey) return `ViewKit.FilterBar:${options.storageKey}`;
            try {
                if (typeof dv !== "undefined" && dv.current) {
                    const path = dv.current()?.file?.path;
                    if (path) return `ViewKit.FilterBar:${path}`;
                }
            } catch (_) {}
            return null;
        }

        _loadState() {
            if (!this.storageKey || typeof sessionStorage === "undefined") return {};
            try {
                const raw = sessionStorage.getItem(this.storageKey);
                return raw ? JSON.parse(raw) : {};
            } catch (_) {
                return {};
            }
        }

        _saveState() {
            if (!this.storageKey || typeof sessionStorage === "undefined") return;
            try {
                sessionStorage.setItem(this.storageKey, JSON.stringify(this.state));
            } catch (_) {}
        }

        _normalizeState(state = {}) {
            const tags = Array.isArray(state.tags)
                ? state.tags.map(tag => ViewKit.normalizeFilterTag(tag)).filter(Boolean)
                : [];
            const links = Array.isArray(state.links)
                ? state.links.map(link => ViewKit.normalizeFilterLink(link)).filter(Boolean)
                : [];
            const matchMode = String(state.matchMode || "and").toLowerCase() === "or" ? "or" : "and";
            const sort = state.sort || this?.sortFields?.[0]?.key || "";
            return {
                search: String(state.search || ""),
                tags: [...new Set(tags)],
                links: [...new Set(links)],
                matchMode,
                startDate: ViewKit.toDateInputValue(state.startDate),
                endDate: ViewKit.toDateInputValue(state.endDate),
                sort,
                sortAsc: Boolean(state.sortAsc),
            };
        }

        _createEl(parent, tag, options = {}) {
            if (!parent?.createEl) return null;
            const el = parent.createEl(tag, options);
            if (options.attr) {
                for (const [key, value] of Object.entries(options.attr)) {
                    if (key in el) el[key] = value;
                    else if (el.setAttribute) el.setAttribute(key, value);
                }
            }
            return el;
        }

        render() {
            if (!this.containerEl?.createEl) return;
            if (typeof this.containerEl.empty === "function") this.containerEl.empty();
            else this.containerEl.innerHTML = "";
            this.searchEl = null;
            this.tagsEl = null;
            this.tagButtons = null;
            this.moreBtn = null;
            this.linksEl = null;
            this.linkButtons = null;
            this.linksMoreBtn = null;
            this.matchModeEl = null;
            this.dateGroupEl = null;
            this.startEl = null;
            this.endEl = null;
            this.sortGroupEl = null;
            this.sortEl = null;
            this.sortDirEl = null;
            this.clearEl = null;
            this.countEl = null;

            this.barEl = this._createEl(this.containerEl, "div", { cls: "vk-fb-bar" });
            this.mainEl = this._createEl(this.barEl, "div", { cls: "vk-fb-main" });

            if (this.controls.has("search")) {
                this.searchEl = this._createEl(this.mainEl, "input", {
                    cls: "vk-fb-search",
                    attr: { type: "text", placeholder: "搜索..." },
                });
                this.searchEl.value = this.state.search;
                this.searchEl.addEventListener?.("input", () => this.setState({ search: this.searchEl.value }));
            }

            if (this.controls.has("tags") && this.availableTags.length > 0) {
                this.tagsEl = this._createEl(this.mainEl, "div", { cls: "vk-fb-tags" });
                this.tagButtons = [];
                this.availableTags.forEach((tag, index) => {
                    const btn = this._createEl(this.tagsEl, "button", {
                        cls: `vk-fb-tag${this.state.tags.includes(tag) ? " active" : ""}`,
                        text: tag,
                        attr: { type: "button", "data-tag": tag },
                    });
                    btn.addEventListener?.("click", () => this._toggleTag(tag));
                    this.tagButtons.push({ btn, tag, index });
                });
                if (this.availableTags.length > this.maxVisibleTags) {
                    this.moreBtn = this._createEl(this.tagsEl, "button", {
                        cls: "vk-fb-more",
                        text: "更多",
                        attr: { type: "button" },
                    });
                    this.moreBtn.addEventListener?.("click", () => {
                        this.tagsExpanded = !this.tagsExpanded;
                        this._syncTagVisibility();
                    });
                    this._syncTagVisibility();
                }
            }

            if (this.controls.has("links") && this.availableLinks.length > 0) {
                this.linksEl = this._createEl(this.mainEl, "div", { cls: "vk-fb-tags vk-fb-links" });
                this.linkButtons = [];
                this.availableLinks.forEach((link, index) => {
                    const btn = this._createEl(this.linksEl, "button", {
                        cls: `vk-fb-tag vk-fb-link${this.state.links.includes(link.key) ? " active" : ""}`,
                        text: link.label,
                        attr: { type: "button", "data-link": link.key, title: link.key },
                    });
                    btn.addEventListener?.("click", () => this._toggleLink(link.key));
                    this.linkButtons.push({ btn, link, index });
                });
                if (this.availableLinks.length > this.maxVisibleTags) {
                    this.linksMoreBtn = this._createEl(this.linksEl, "button", {
                        cls: "vk-fb-more",
                        text: "更多",
                        attr: { type: "button" },
                    });
                    this.linksMoreBtn.addEventListener?.("click", () => {
                        this.linksExpanded = !this.linksExpanded;
                        this._syncLinkVisibility();
                    });
                    this._syncLinkVisibility();
                }
            }

            this.controlsEl = this._createEl(this.mainEl, "div", { cls: "vk-fb-controls" });
            this.statusEl = this._createEl(this.controlsEl, "div", { cls: "vk-fb-status" });

            if (this.controls.has("dateRange")) {
                this.dateGroupEl = this._createEl(this.controlsEl, "div", { cls: "vk-fb-date-group" });
                this.startEl = this._createEl(this.dateGroupEl, "input", {
                    cls: "vk-fb-date",
                    attr: { type: "date", title: "开始日期" },
                });
                this.startEl.value = this.state.startDate;
                this.startEl.addEventListener?.("change", () => this.setState({ startDate: this.startEl.value }));
                this._createEl(this.dateGroupEl, "span", { cls: "vk-fb-date-sep", text: "-" });
                this.endEl = this._createEl(this.dateGroupEl, "input", {
                    cls: "vk-fb-date",
                    attr: { type: "date", title: "结束日期" },
                });
                this.endEl.value = this.state.endDate;
                this.endEl.addEventListener?.("change", () => this.setState({ endDate: this.endEl.value }));
            }

            if (this.controls.has("sort") && this.sortFields.length > 0) {
                this.sortGroupEl = this._createEl(this.controlsEl, "div", { cls: "vk-fb-sort-group" });
                this.sortEl = this._createEl(this.sortGroupEl, "select", { cls: "vk-fb-sort" });
                this.sortFields.forEach(field => {
                    const option = this._createEl(this.sortEl, "option", {
                        text: field.label || field.key,
                        attr: { value: field.key },
                    });
                    option.value = field.key;
                });
                this.sortEl.value = this.state.sort;
                this.sortEl.addEventListener?.("change", () => this.setState({ sort: this.sortEl.value }));
                this.sortDirEl = this._createEl(this.sortGroupEl, "button", {
                    cls: "vk-fb-sort-dir",
                    text: this.state.sortAsc ? "↑" : "↓",
                    attr: { type: "button", title: "切换升序/降序" },
                });
                this.sortDirEl.addEventListener?.("click", () => this.setState({ sortAsc: !this.state.sortAsc }));
            }

            this.clearEl = this._createEl(this.controlsEl, "button", {
                cls: "vk-fb-clear",
                text: "清除",
                attr: { type: "button" },
            });
            this.clearEl.addEventListener?.("click", () => this.clear());

            if (this.controls.has("matchMode")) {
                this.matchModeEl = this._createEl(this.statusEl, "button", {
                    cls: "vk-fb-match",
                    attr: { type: "button", title: "切换筛选关系：AND=全部命中，OR=任一命中" },
                });
                this.matchModeEl.addEventListener?.("click", () => this.setState({
                    matchMode: this.state.matchMode === "and" ? "or" : "and",
                }));
            }

            if (this.showCount) {
                this.countEl = this._createEl(this.statusEl, "span", { cls: "vk-fb-count", text: "0 / 0 项" });
            }
            this.emptyEl = this._createEl(this.containerEl, "div", {
                cls: "vk-fb-empty",
                text: this.options.emptyText || "无匹配项，点击清除过滤器",
            });
            this.emptyEl.addEventListener?.("click", () => this.clear());
            this._syncControls();
        }

        _syncTagVisibility() {
            if (!this.tagButtons) return;
            this.tagButtons.forEach(({ btn, index, tag }) => {
                btn.style.display = "";
            });
            if (this.moreBtn) {
                this.moreBtn.textContent = this.tagsExpanded ? "收起" : "展开";
            }
            this.tagsEl?.classList?.toggle("is-expanded", Boolean(this.tagsExpanded));
        }

        _syncLinkVisibility() {
            if (!this.linkButtons) return;
            this.linkButtons.forEach(({ btn, index, link }) => {
                btn.style.display = "";
            });
            if (this.linksMoreBtn) {
                this.linksMoreBtn.textContent = this.linksExpanded ? "收起" : "展开";
            }
            this.linksEl?.classList?.toggle("is-expanded", Boolean(this.linksExpanded));
        }

        _syncControls() {
            if (this.searchEl) this.searchEl.value = this.state.search;
            if (this.startEl) this.startEl.value = this.state.startDate;
            if (this.endEl) this.endEl.value = this.state.endDate;
            if (this.sortEl) this.sortEl.value = this.state.sort;
            if (this.sortDirEl) this.sortDirEl.textContent = this.state.sortAsc ? "↑" : "↓";
            if (this.matchModeEl) {
                this.matchModeEl.textContent = this.state.matchMode === "or" ? "OR" : "AND";
                this.matchModeEl.setAttribute?.("aria-pressed", this.state.matchMode === "or" ? "true" : "false");
            }
            if (this.tagButtons) {
                this.tagButtons.forEach(({ btn, tag }) => {
                    if (this.state.tags.includes(tag)) btn.classList?.add("active");
                    else btn.classList?.remove("active");
                });
                this._syncTagVisibility();
            }
            if (this.linkButtons) {
                this.linkButtons.forEach(({ btn, link }) => {
                    if (this.state.links.includes(link.key)) btn.classList?.add("active");
                    else btn.classList?.remove("active");
                });
                this._syncLinkVisibility();
            }
            const hasActiveFilter = Boolean(
                this.state.search ||
                this.state.tags.length ||
                this.state.links.length ||
                this.state.startDate ||
                this.state.endDate
            );
            if (this.clearEl) {
                this.clearEl.classList?.toggle("is-active", hasActiveFilter);
                if (this.clearEl.setAttribute) this.clearEl.setAttribute("aria-pressed", hasActiveFilter ? "true" : "false");
            }
        }

        _toggleTag(tag) {
            const tags = new Set(this.state.tags);
            if (tags.has(tag)) tags.delete(tag);
            else tags.add(tag);
            this.setState({ tags: [...tags] });
        }

        _toggleLink(link) {
            const links = new Set(this.state.links);
            if (links.has(link)) links.delete(link);
            else links.add(link);
            this.setState({ links: [...links] });
        }

        setState(partial = {}, options = {}) {
            this.state = this._normalizeState({ ...this.state, ...partial });
            this._syncControls();
            if (options.persist !== false) this._saveState();
            if (options.apply !== false) return this.apply();
            return this.filteredItems;
        }

        clear() {
            this.state = this._normalizeState({
                ...this.initialState,
                search: "",
                tags: [],
                links: [],
                matchMode: this.initialState.matchMode || "and",
                startDate: "",
                endDate: "",
            });
            this._syncControls();
            this._saveState();
            return this.apply();
        }

        bind(items = []) {
            this.items = Array.isArray(items) ? [...items] : [];
            if (!this.optionsProvidedTags) {
                const nextTags = ViewKit.collectTags(this.items);
                if (nextTags.join("\u0000") !== this.availableTags.join("\u0000")) {
                    this.availableTags = nextTags;
                    this.render();
                }
            }
            if (!this.optionsProvidedLinks) {
                const nextLinks = ViewKit.collectLinks(this.items);
                const currentKey = this.availableLinks.map(link => link.key).join("\u0000");
                const nextKey = nextLinks.map(link => link.key).join("\u0000");
                if (nextKey !== currentKey) {
                    this.availableLinks = nextLinks;
                    this.render();
                }
            }
            return this.apply();
        }

        apply(items = this.items) {
            const allItems = Array.isArray(items) ? [...items] : [];
            const search = this.state.search.trim().toLowerCase();
            const tagFilters = this.state.tags;
            const linkFilters = this.state.links;
            const matchMode = this.state.matchMode;
            const start = this.state.startDate ? new Date(`${this.state.startDate}T00:00:00`).getTime() : null;
            const end = this.state.endDate ? new Date(`${this.state.endDate}T23:59:59.999`).getTime() : null;

            let result = allItems.filter(item => {
                if (search && !ViewKit._filterText(item).includes(search)) return false;
                const hasChipFilters = tagFilters.length > 0 || linkFilters.length > 0;
                if (hasChipFilters) {
                    const tags = new Set((item?.tags || []).map(tag => ViewKit.normalizeFilterTag(tag)));
                    const links = new Set(ViewKit.collectLinks([item]).map(link => link.key));
                    const tagHits = tagFilters.map(tag => tags.has(tag));
                    const linkHits = linkFilters.map(link => links.has(link));
                    const hits = [...tagHits, ...linkHits];
                    if (matchMode === "or") {
                        if (!hits.some(Boolean)) return false;
                    } else if (!hits.every(Boolean)) {
                        return false;
                    }
                }
                if (start != null || end != null) {
                    const ts = ViewKit.toTimestamp(item?.ctime ?? item?.date ?? item?.lastInteraction ?? item?.time);
                    if (ts == null) return false;
                    if (start != null && ts < start) return false;
                    if (end != null && ts > end) return false;
                }
                return true;
            });

            const sortDef = this.sortFields.find(field => field.key === this.state.sort);
            if (sortDef?.fn) {
                result = [...result].sort(sortDef.fn);
                if (this.state.sortAsc) result.reverse();
            }

            this.filteredItems = result;
            if (this.countEl) this.countEl.textContent = `${result.length} / ${allItems.length} 项`;
            if (this.emptyEl) {
                const visible = result.length === 0 && allItems.length > 0;
                this.emptyEl.classList?.toggle("is-visible", visible);
                if (!this.emptyEl.classList && this.emptyEl.style) this.emptyEl.style.display = visible ? "block" : "none";
            }
            this.onFilter(result, { ...this.state, tags: [...this.state.tags], links: [...this.state.links] }, this);
            return result;
        }
    },

    LunarUtils: {
        lunarInfo: [0x04bd8, 0x04ae0, 0x0a570, 0x054d5, 0x0d260, 0x0d950, 0x16554, 0x056a0, 0x09ad0, 0x055d2,
            0x04ae0, 0x0a5b6, 0x0a4d0, 0x0d250, 0x1d255, 0x0b540, 0x0d6a0, 0x0ada2, 0x095b0, 0x14977,
            0x04970, 0x0a4b0, 0x0b4b5, 0x06a50, 0x06d40, 0x1ab54, 0x02b60, 0x09570, 0x052f2, 0x04970,
            0x06566, 0x0d4a0, 0x0ea50, 0x06e95, 0x05ad0, 0x02b60, 0x186e3, 0x092e0, 0x1c8d7, 0x0c950,
            0x0d4a0, 0x1d8a6, 0x0b550, 0x056a0, 0x1a5b4, 0x025d0, 0x092d0, 0x0d2b2, 0x0a950, 0x0b557,
            0x06ca0, 0x0b550, 0x15355, 0x04da0, 0x0a5d0, 0x14573, 0x052d0, 0x0a9a8, 0x0e950, 0x06aa0,
            0x0aea6, 0x0ab50, 0x04b60, 0x0aae4, 0x0a570, 0x05260, 0x0f263, 0x0d950, 0x05b57, 0x056a0,
            0x096d0, 0x04dd5, 0x04ad0, 0x0a4d0, 0x0d4d4, 0x0d250, 0x0d558, 0x0b540, 0x0b5a0, 0x195a6,
            0x095b0, 0x049b0, 0x0a974, 0x0a4b0, 0x0b27a, 0x06a50, 0x06d40, 0x0af46, 0x0ab60, 0x09570,
            0x04af5, 0x04970, 0x064b0, 0x074a3, 0x0ea50, 0x06b58, 0x055c0, 0x0ab60, 0x096d5, 0x092e0,
            0x0c960, 0x0d954, 0x0d4a0, 0x0da50, 0x07552, 0x056a0, 0x0abb7, 0x025d0, 0x092d0, 0x0cab5,
            0x0a950, 0x0b4a0, 0x0baa4, 0x0ad50, 0x055d9, 0x04ba0, 0x0a5b0, 0x15176, 0x052b0, 0x0a930,
            0x07954, 0x06aa0, 0x0ad50, 0x05b52, 0x04b60, 0x0a6e6, 0x0a4e0, 0x0d260, 0x0ea65, 0x0d530,
            0x05aa0, 0x076a3, 0x096d0, 0x04bd7, 0x04ad0, 0x0a4d0, 0x1d0b6, 0x0d250, 0x0d520, 0x0dd45,
            0x0b5a0, 0x056d0, 0x055b2, 0x049b0, 0x0a577, 0x0a4b0, 0x0aa50, 0x1b255, 0x06d20, 0x0ada0],

        leapMonth(y) { return (this.lunarInfo[y - 1900] & 0xf); },
        monthDays(y, m) { return ((this.lunarInfo[y - 1900] & (0x10000 >> m)) ? 30 : 29); },
        leapDays(y) { return this.leapMonth(y) ? ((this.lunarInfo[y - 1900] & 0x10000) ? 30 : 29) : 0; },
        lunarYearDays(y) {
            let sum = 348;
            for (let i = 0x8000; i > 0x8; i >>= 1) sum += (this.lunarInfo[y - 1900] & i) ? 1 : 0;
            return sum + this.leapDays(y);
        },
        lunar2Solar(y, m, d) {
            let offset = 0;
            for (let i = 1900; i < y; i++) offset += this.lunarYearDays(i);
            const leap = this.leapMonth(y);
            for (let i = 1; i < m; i++) {
                offset += this.monthDays(y, i);
                if (i === leap) offset += this.leapDays(y);
            }
            offset += d - 1;
            const solarDate = new Date(new Date(1900, 0, 31).getTime() + offset * 86400000 + 43200000);
            solarDate.setHours(0, 0, 0, 0);
            return solarDate;
        },
        solar2Lunar(objDate) {
            let i;
            let temp = 0;
            const baseDate = new Date(1900, 0, 31);
            let offset = Math.floor((objDate - baseDate) / 86400000);

            for (i = 1900; i < 2050 && offset > 0; i++) {
                temp = this.lunarYearDays(i);
                offset -= temp;
            }
            if (offset < 0) {
                offset += temp;
                i--;
            }

            const year = i;
            const leap = this.leapMonth(i);
            let isLeap = false;

            for (i = 1; i < 13 && offset > 0; i++) {
                if (leap > 0 && i === (leap + 1) && isLeap === false) {
                    --i;
                    isLeap = true;
                    temp = this.leapDays(year);
                } else {
                    temp = this.monthDays(year, i);
                }
                if (isLeap === true && i === (leap + 1)) isLeap = false;
                offset -= temp;
            }

            if (offset === 0 && leap > 0 && i === leap + 1) {
                if (isLeap) isLeap = false;
                else {
                    isLeap = true;
                    --i;
                }
            }
            if (offset < 0) {
                offset += temp;
                --i;
            }

            return { year, month: i, day: offset + 1, isLeap };
        },
    },

    getBirthdayInfo(page) {
        const manualTarget = page?.["今年生日"] || page?.["TargetBirthday"];
        if (manualTarget) {
            const targetDate = dv.date(manualTarget);
            if (targetDate) {
                const today = new Date();
                today.setHours(0, 0, 0, 0);
                const diffDays = Math.ceil((targetDate - today) / 86400000);
                if (diffDays >= 0) return { date: targetDate, days: diffDays, original: targetDate, isManual: true };
            }
        }

        const dateStr = page?.["生日"] || page?.["birthday"];
        if (!dateStr) return null;

        let type = page["生日类型"];
        if (!type && page.file?.path && typeof app !== "undefined") {
            const tFile = app.vault.getAbstractFileByPath(page.file.path);
            const cache = tFile ? app.metadataCache.getFileCache(tFile) : null;
            type = cache?.frontmatter?.["生日类型"] || cache?.frontmatter?.["Type"];
        }

        const typeStr = String(type || "").trim();
        const isSolar = typeStr === "阳历" || typeStr === "公历" || typeStr === "Solar";
        const today = new Date();
        today.setHours(0, 0, 0, 0);
        const currentYear = today.getFullYear();

        if (!isSolar) {
            const parts = dateStr.toString().split(/[-/]/);
            if (parts.length < 2) return { error: "农历格式错误" };
            const lMonth = parseInt(parts[parts.length - 2]);
            const lDay = parseInt(parts[parts.length - 1]);
            const todayLunar = this.LunarUtils.solar2Lunar(today);
            let targetYear = todayLunar.year;
            const hasPassed = todayLunar.month > lMonth || (todayLunar.month === lMonth && todayLunar.day > lDay);
            if (hasPassed) targetYear += 1;

            let nextBirthday = this.LunarUtils.lunar2Solar(targetYear, lMonth, lDay);
            if (nextBirthday < today) nextBirthday = this.LunarUtils.lunar2Solar(targetYear + 1, lMonth, lDay);
            const diffDays = Math.ceil((nextBirthday - today) / 86400000);
            return { date: nextBirthday, days: diffDays, original: dv.date(dateStr), isLunar: true, targetDate: nextBirthday };
        }

        const dateObj = dv.date(dateStr);
        if (!dateObj) return null;
        const nextBirthday = new Date(currentYear, dateObj.month - 1, dateObj.day);
        if (nextBirthday < today) nextBirthday.setFullYear(currentYear + 1);
        const diffDays = Math.ceil((nextBirthday - today) / 86400000);
        return { date: nextBirthday, days: diffDays, original: dateObj, isManual: false, targetDate: nextBirthday };
    },

    sourceHref(item) {
        const link = item?.link || {};
        let href = link.path || item?.path || "";
        if (!href) return "";
        if (link.blockId) href += `#^${link.blockId}`;
        else if (typeof link.subpath === "string" && link.subpath.trim()) {
            const subpath = link.subpath.trim();
            href += subpath.startsWith("#") ? subpath : `#${subpath}`;
        }
        return href;
    },

    escapeHtml(value) {
        return String(value ?? "")
            .replace(/&/g, "&amp;")
            .replace(/</g, "&lt;")
            .replace(/>/g, "&gt;")
            .replace(/"/g, "&quot;")
            .replace(/'/g, "&#39;");
    },

    safeLinkText(value) {
        return this.escapeHtml(String(value ?? "").replace(/\[\[([^\]|]+)(?:\|([^\]]+))?\]\]/g, (_, path, display) => display || path));
    },

    renderSourceButton(item, options = {}) {
        const href = this.sourceHref(item);
        if (!href) return "";
        const className = this.escapeHtml(options.className || "pp-source-link");
        const label = this.escapeHtml(options.label || "📄");
        const title = this.escapeHtml(options.title || "打开来源");
        const style = options.style === undefined
            ? "text-decoration:none; color:var(--c-text-muted); margin-right:4px;"
            : options.style;
        const styleAttr = style ? ` style="${this.escapeHtml(style)}"` : "";
        const safeHref = this.escapeHtml(href);
        return `<a class="internal-link ${className}" href="${safeHref}" data-href="${safeHref}" target="_blank" rel="noopener" title="${title}"${styleAttr}>${label}</a>`;
    },

    renderDisplayParts(parts = [], options = {}) {
        if (!Array.isArray(parts) || parts.length === 0) {
            return this.escapeHtml(options.fallback || "");
        }
        return parts.map(part => {
            if (!part) return "";
            if (part.type === "link") {
                const href = this.escapeHtml(part.target || part.label || "");
                const label = this.escapeHtml(part.label || part.target || "");
                const role = this.escapeHtml(part.role || "object");
                return `<a class="internal-link vk-display-link vk-display-link-${role}" href="${href}" data-href="${href}" target="_blank" rel="noopener" style="text-decoration:none; color:var(--c-accent, var(--text-accent));">${label}</a>`;
            }
            return this.escapeHtml(part.text || "");
        }).filter(Boolean).join(" ");
    },

    renderModuleShell(parent, options = {}) {
        if (!parent?.createEl) return null;
        this._injectStyles(parent);
        const shell = parent.createEl(options.tag || "section", {
            cls: options.shellClass || "vk-module",
        });
        const header = shell.createEl("div", { cls: options.headerClass || "vk-module-header" });
        const titleEl = header.createEl(options.titleTag || "span", {
            cls: options.titleClass || "vk-module-title",
            text: options.title || "",
        });
        const countValue = options.count;
        const countText = options.countText !== undefined
            ? options.countText
            : countValue !== undefined
                ? `${countValue}${options.countSuffix || ""}`
                : "";
        const countEl = header.createEl("span", {
            cls: options.countClass || "vk-module-count",
            text: countText,
        });
        if (!countText) countEl.style.display = "none";
        const body = shell.createEl("div", { cls: options.bodyClass || "vk-module-body" });
        const api = {
            shell,
            header,
            titleEl,
            countEl,
            body,
            setCount(nextCount, suffix = options.countSuffix || "") {
                const nextText = nextCount === undefined || nextCount === null ? "" : `${nextCount}${suffix}`;
                countEl.textContent = nextText;
                countEl.style.display = nextText ? "" : "none";
            },
        };
        if (typeof options.renderBody === "function") options.renderBody(body, api);
        return api;
    },

    renderRankList(parent, options = {}) {
        if (!parent?.createEl) return null;
        const rows = Array.isArray(options.rows) ? options.rows : [];
        if (!rows.length) {
            return parent.createEl("div", {
                cls: options.emptyClass || "vk-empty",
                text: options.emptyText || "暂无数据",
            });
        }
        const list = parent.createEl("div", { cls: options.listClass || "vk-rank-list" });
        const formatMeta = typeof options.formatMeta === "function"
            ? options.formatMeta
            : row => {
                const parts = [];
                if (row.count !== undefined) parts.push(`${row.count} 次`);
                if (row.money) parts.push(this.fmtMoney(row.money));
                if (row.time) parts.push(`${row.time}h`);
                return parts.join(" · ");
            };
        for (const row of rows) {
            const target = row.target || row.path || "";
            const label = row.label || row.name || target || "";
            const meta = formatMeta(row);
            const el = list.createEl("div", { cls: options.rowClass || "vk-rank-row" });
            const safeLabel = this.escapeHtml(label);
            const safeTarget = this.escapeHtml(target);
            const safeMeta = this.escapeHtml(meta);
            const nameClass = this.escapeHtml(options.nameClass || "vk-rank-name");
            const metaClass = this.escapeHtml(options.metaClass || "vk-rank-meta");
            const evidenceHtml = this.renderEvidenceLinks(row.evidence || row.evidenceItems || [], {
                maxItems: options.maxEvidence || 3,
                className: options.evidenceLinkClass || "vk-evidence-link",
                wrapperClass: options.evidenceClass || "vk-rank-evidence",
                label: options.evidenceLabel || "证据",
            });
            el.innerHTML = target
                ? `<a class="internal-link ${nameClass}" href="${safeTarget}" data-href="${safeTarget}" target="_blank" rel="noopener">${safeLabel}</a><span class="${metaClass}">${safeMeta}${evidenceHtml}</span>`
                : `<span class="${nameClass}">${safeLabel}</span><span class="${metaClass}">${safeMeta}${evidenceHtml}</span>`;
        }
        return list;
    },

    renderEvidenceLinks(items = [], options = {}) {
        const rows = Array.isArray(items) ? items : [items];
        const links = [];
        const seen = new Set();
        const maxItems = Math.max(1, Number(options.maxItems || 3) || 3);
        for (const item of rows) {
            const href = typeof item === "string" ? item : this.sourceHref(item);
            if (!href || seen.has(href)) continue;
            seen.add(href);
            const label = `${options.label || "证据"}${links.length + 1}`;
            links.push(`<a class="internal-link ${this.escapeHtml(options.className || "vk-evidence-link")}" href="${this.escapeHtml(href)}" data-href="${this.escapeHtml(href)}" target="_blank" rel="noopener">${this.escapeHtml(label)}</a>`);
            if (links.length >= maxItems) break;
        }
        if (!links.length) return "";
        return `<span class="${this.escapeHtml(options.wrapperClass || "vk-rank-evidence")}">${links.join("")}</span>`;
    },

    renderTransactionList(container, options = {}) {
        if (!container?.createEl) return null;
        const items = Array.isArray(options.items) ? options.items : [];
        const totalCount = Number(options.totalCount ?? items.length) || 0;
        const title = options.title || "全部交易";
        const direction = options.direction || "all";
        const getAmount = typeof options.getAmount === "function"
            ? options.getAmount
            : item => Number(item?.vec?.[0] ?? item?.value ?? 0) || 0;
        const formatMoney = typeof options.formatMoney === "function"
            ? options.formatMoney
            : value => this.fmtMoney(value);
        const directionClass = typeof options.directionClass === "function"
            ? options.directionClass
            : amount => amount > 0 ? "income" : amount < 0 ? "expense" : "neutral";
        const directionText = typeof options.directionText === "function"
            ? options.directionText
            : amount => amount > 0 ? "收入" : amount < 0 ? "支出" : "零额";

        container.innerHTML = "";
        if (typeof options.setTitle === "function") options.setTitle(title);
        if (typeof options.onVisibleItems === "function") options.onVisibleItems(items);

        if (!items.length) {
            if (typeof options.setMeta === "function") options.setMeta(`结果 0 / ${totalCount}`);
            return container.createEl("div", {
                cls: options.emptyClass || "tx-empty",
                text: options.emptyText || "无记录",
            });
        }

        const table = container.createEl("table", { cls: options.tableClass || "pp-table" });
        const tbody = table.createEl("tbody");
        const sourceButtonOptions = options.sourceButtonOptions || {
            className: "pp-source-link",
            style: "text-decoration:none; color:var(--c-text-muted); font-size:0.85em; flex-shrink:0; margin-right:4px;",
        };
        const sourceLinkClass = this.escapeHtml(options.sourceLinkClass || "pp-source-link");
        const tagClass = this.escapeHtml(options.tagClass || "pp-tag-pill");

        const renderRow = (_tbody, item) => {
            const amount = getAmount(item);
            const dirClass = directionClass(amount);
            const directionBadge = direction === "all" && options.showDirectionBadge !== false
                ? `<span class="tx-amount-badge ${this.escapeHtml(dirClass)}">${this.escapeHtml(directionText(amount))}</span>`
                : "";
            const amountText = `${amount > 0 ? "+" : ""}${this.escapeHtml(formatMoney(amount))}`;
            const sourceButton = this.renderSourceButton(item, sourceButtonOptions);
            let walletHtml = "";
            if (item.wallet) {
                const walletPath = this.escapeHtml(item.wallet.path);
                walletHtml = `<a class="internal-link" href="${walletPath}" data-href="${walletPath}" target="_blank" rel="noopener" style="font-size:0.85em; color:var(--c-text-muted);">${this.escapeHtml(item.wallet.display)}</a>`;
            }
            const tagsHtml = (item.tags || [])
                .map(tag => `<span class="${tagClass}">${this.escapeHtml(String(tag).replace("#", ""))}</span>`)
                .join("");
            const text = this.renderDisplayParts(item.displayParts, { fallback: item.displayText || item.text });
            const sourceHtml = (item.sourceLinks || [])
                .map(link => {
                    const target = this.escapeHtml(link.target);
                    const label = this.escapeHtml(link.label || "来源");
                    return `<a class="internal-link ${sourceLinkClass}" href="${target}" data-href="${target}" target="_blank" rel="noopener" title="${label}" style="font-size:0.8em; color:var(--c-text-muted); text-decoration:none;">来源</a>`;
                })
                .join(" ");
            const dateText = item.ctime?.toFormat ? item.ctime.toFormat(options.dateFormat || "MM-dd") : "";
            const tr = tbody.createEl("tr");
            tr.innerHTML = `
                <td>
                    <div class="tx-row-main">
                        <div class="tx-row-text">${sourceButton}<span>${text}</span></div>
                        <span class="tx-amount ${this.escapeHtml(dirClass)}">${amountText}</span>
                    </div>
                    <div class="tx-row-meta">
                        <div class="tx-row-tags">${directionBadge}${tagsHtml}${walletHtml}${sourceHtml}</div>
                        <span>${this.escapeHtml(dateText)}</span>
                    </div>
                </td>
            `;
        };

        return this.renderProgressiveList(container, {
            items,
            pageSize: options.pageSize || options.batchSize || 30,
            listEl: tbody,
            footerClass: options.footerClass || "pp-load-more-footer",
            statusClass: options.statusClass || "pp-load-more-status",
            buttonClass: options.buttonClass || "pp-load-more-button",
            sentinelClass: options.sentinelClass || "pp-load-more-sentinel",
            root: options.root || container,
            loadMoreText: options.loadMoreText || "加载更多",
            renderItem: renderRow,
            onUpdate: ({ shown }) => {
                if (typeof options.setMeta === "function") {
                    options.setMeta(`结果 ${items.length} / ${totalCount} · 已显示 ${shown} / ${items.length}`);
                }
                if (typeof options.onUpdate === "function") options.onUpdate({ shown, total: items.length });
            },
        });
    },

    sanitizeDebugValue(value) {
        return String(value ?? "")
            .replace(/[A-Za-z]:[\\/][^\n\r\t]+/g, "本地路径")
            .replace(/\\\\[^\\/\s]+\\[^\n\r\t]+/g, "本地路径");
    },

    renderDebugPanel(parent, options = {}) {
        if (!parent?.createEl) return null;
        const dataset = options.dataset || {};
        const rows = Array.isArray(options.rows) ? [...options.rows] : [];
        const len = value => Array.isArray(value) ? value.length : 0;
        const warnings = dataset.warnings || [];

        if (!rows.length) {
            rows.push(["source entries", len(dataset.sourceEntries)]);
            rows.push(["consumed entries", len(dataset.consumedEntries)]);
            rows.push(["visible entries", len(dataset.visibleEntries)]);
            rows.push(["available tags", len(dataset.availableTags)]);
            rows.push(["available links", len(dataset.availableLinks)]);
            rows.push(["warnings", warnings.length]);
            if (options.interaction) rows.push(["interaction", options.interaction]);
            if (dataset.queryMetrics) rows.push(["query metrics", dataset.queryMetrics]);
        }

        const module = this.renderModuleShell(parent, {
            title: options.title || "Debug",
            count: rows.length,
            countSuffix: " 项",
            shellClass: options.shellClass || "vk-module vk-debug-panel",
            headerClass: options.headerClass || "vk-module-header",
            countClass: options.countClass || "vk-module-count",
            bodyClass: options.bodyClass || "vk-module-body vk-debug-body",
        });

        for (const [key, value] of rows) {
            const row = module.body.createEl("div", { cls: options.rowClass || "vk-debug-row" });
            row.createEl("span", { cls: options.keyClass || "vk-debug-key", text: String(key) });
            const displayValue = typeof value === "string"
                ? value
                : JSON.stringify(value);
            row.createEl("span", {
                cls: options.valueClass || "vk-debug-value",
                text: this.sanitizeDebugValue(displayValue),
            });
        }

        if (warnings.length) {
            const row = module.body.createEl("div", { cls: options.rowClass || "vk-debug-row" });
            row.createEl("span", { cls: options.keyClass || "vk-debug-key", text: "warning details" });
            row.createEl("span", {
                cls: options.valueClass || "vk-debug-value",
                text: this.sanitizeDebugValue(warnings.join(" | ")),
            });
        }

        return module;
    },

    renderProgressiveList(target, options = {}) {
        if (!target) return null;
        const rows = Array.isArray(options.items) ? options.items : [];
        const pageSize = Math.max(1, Number(options.pageSize || options.batchSize || 30) || 30);
        const renderItem = typeof options.renderItem === "function"
            ? options.renderItem
            : (parent, item) => parent.createEl(options.itemTag || "div", { text: String(item ?? "") });

        if (Array.isArray(target.__vkProgressiveObservers)) {
            for (const observer of target.__vkProgressiveObservers) observer.disconnect();
        }
        target.__vkProgressiveObservers = [];

        if (!rows.length) {
            const emptyEl = target.createEl("div", {
                cls: options.emptyClass || "vk-empty",
                text: options.emptyText || "暂无记录",
                style: options.emptyStyle || "color:var(--c-text-muted, var(--text-muted)); text-align:center; padding:20px;",
            });
            return { list: null, footer: null, emptyEl, renderNext() {}, disconnect() {} };
        }

        const list = options.listEl || target.createEl(options.listTag || "div", { cls: options.listClass || "vk-progressive-list" });
        const footer = target.createEl("div", {
            cls: options.footerClass || "vk-load-more-footer",
            style: options.footerStyle || "display:flex; flex-direction:column; align-items:center; gap:6px; padding:10px 0 4px;",
        });
        const status = footer.createEl("div", {
            cls: options.statusClass || "vk-load-more-status",
            style: options.statusStyle || "font-size:0.78em; color:var(--c-text-muted, var(--text-muted));",
        });
        const button = footer.createEl("button", {
            cls: options.buttonClass || "vk-load-more-button",
            text: options.loadMoreText || "加载更多",
            style: options.buttonStyle || "min-height:40px; padding:4px 14px; border-radius:6px; cursor:pointer;",
        });
        const sentinel = footer.createEl("div", {
            cls: options.sentinelClass || "vk-load-more-sentinel",
            attr: { "aria-hidden": "true" },
            style: options.sentinelStyle || "width:1px; height:1px;",
        });

        let rendered = 0;
        let observer = null;

        const updateFooter = () => {
            const shown = Math.min(rendered, rows.length);
            status.textContent = `${options.progressText || "已显示"} ${shown} / ${rows.length}`;
            const done = rendered >= rows.length;
            button.style.display = done ? "none" : "";
            if (done && observer) observer.disconnect();
            if (typeof options.onUpdate === "function") options.onUpdate({ shown, total: rows.length, done });
        };

        const renderNext = () => {
            if (rendered >= rows.length) return;
            const nextRows = rows.slice(rendered, rendered + pageSize);
            for (const item of nextRows) renderItem(list, item, rendered++);
            updateFooter();
        };

        const disconnect = () => {
            if (observer) observer.disconnect();
        };

        button.onclick = renderNext;
        renderNext();

        if (typeof IntersectionObserver !== "undefined") {
            observer = new IntersectionObserver(entries => {
                if (entries.some(entry => entry.isIntersecting) && rendered < rows.length) renderNext();
            }, { root: options.root || target, rootMargin: options.rootMargin || "80px 0px" });
            target.__vkProgressiveObservers.push(observer);
            observer.observe(sentinel);
        }

        return {
            list,
            footer,
            status,
            button,
            sentinel,
            renderNext,
            disconnect,
            get rendered() { return rendered; },
            total: rows.length,
        };
    },

    renderTimeline(container, options = {}) {
        const items = options.items || [];
        if (Array.isArray(container.__vkTimelineObservers)) {
            for (const observer of container.__vkTimelineObservers) observer.disconnect();
        }
        container.__vkTimelineObservers = [];

        if (!items.length) {
            container.createEl("div", { text: options.emptyText || "暂无记录", style: "color:var(--c-text-muted); text-align:center; padding:20px;" });
            return;
        }

        const renderTimelineItem = (timeline, item) => {
            const itemClass = options.itemClass || "pp-timeline-item";
            const dotClass = options.dotClass || "pp-timeline-dot";
            const contentClass = options.contentClass || "pp-timeline-content";
            const mainClass = options.mainClass || "pp-timeline-main";
            const dateClass = options.dateClass || "pp-timeline-date";
            const textClass = options.textClass || "pp-timeline-text";
            const metaClass = options.metaClass || "pp-timeline-meta";
            const badgeClass = options.badgeClass || "pp-badge";
            const timeBadgeClass = options.timeBadgeClass || `${badgeClass} pp-badge-time`;
            const emotionBadgeClass = options.emotionBadgeClass || `${badgeClass} pp-badge-emotion`;
            const incomeBadgeClass = options.incomeBadgeClass || `${badgeClass} pp-badge-income`;
            const expenseBadgeClass = options.expenseBadgeClass || `${badgeClass} pp-badge-expense`;
            const displayTextClass = options.displayTextClass || "pp-display-text";
            const el = timeline.createEl("div", { cls: itemClass });
            let badgesHtml = "";
            if (item.vec?.[2] > 0) badgesHtml += `<span class="${this.escapeHtml(timeBadgeClass)}">⏳${this.escapeHtml(item.vec[2])}</span>`;
            if ((item.vec?.[1] || 0) !== 0) badgesHtml += `<span class="${this.escapeHtml(emotionBadgeClass)}">❤${this.escapeHtml(item.vec[1])}</span>`;
            if ((item.vec?.[0] || 0) !== 0) {
                const cls = item.vec[0] > 0 ? incomeBadgeClass : expenseBadgeClass;
                badgesHtml += `<span class="${this.escapeHtml(cls)}">${this.escapeHtml(this.fmtMoney(item.vec[0]))}</span>`;
            }

            const href = this.sourceHref(item);
            const rawLabel = item.displayText || item.text || "无描述";
            const label = this.escapeHtml(rawLabel);
            const displayHtml = this.renderDisplayParts(item.displayParts, { fallback: rawLabel });
            const sourceLink = this.renderSourceButton(item, options.sourceButtonOptions || {});
            const textLink = href
                ? `<span class="${this.escapeHtml(displayTextClass)}">${displayHtml}</span>`
                : displayHtml;
            const dateText = item.ctime?.toFormat ? item.ctime.toFormat(options.dateFormat || "MM-dd") : "";

            el.innerHTML = `
                <div class="${this.escapeHtml(dotClass)}"></div>
                <div class="${this.escapeHtml(contentClass)}">
                    <div class="${this.escapeHtml(mainClass)}">
                        <span class="${this.escapeHtml(dateClass)}">${this.escapeHtml(dateText)}</span>
                        <div class="${this.escapeHtml(textClass)}" title="${label}">${sourceLink}${textLink}</div>
                    </div>
                    <div class="${this.escapeHtml(metaClass)}">${badgesHtml}</div>
                </div>
            `;
        };

        const renderItems = (target, rows) => {
            const timeline = target.createEl("div", { cls: options.timelineClass || "pp-timeline" });
            for (const item of rows) renderTimelineItem(timeline, item);
            return timeline;
        };

        const renderProgressiveItems = (target, rows) => {
            return this.renderProgressiveList(target, {
                items: rows,
                pageSize: options.pageSize || options.batchSize || 30,
                listClass: options.timelineClass || "pp-timeline",
                loadMoreText: options.loadMoreText || "加载更多",
                progressText: options.progressText || "已显示",
                renderItem: (timeline, item) => renderTimelineItem(timeline, item),
            });
        };

        if (options.columns === "split") {
            const grid = container.createEl("div", { cls: options.gridClass || "pp-split-grid" });
            const left = grid.createEl("div", { cls: options.columnClass || "pp-column" });
            const right = grid.createEl("div", { cls: options.columnClass || "pp-column" });
            left.createEl("div", { cls: "pp-column-header", text: options.leftLabel || "事件" });
            right.createEl("div", { cls: "pp-column-header", text: options.rightLabel || "账单" });
            const splitBy = options.splitBy || "type";
            const leftItems = [];
            const rightItems = [];
            for (const item of items) {
                const key = typeof splitBy === "function" ? splitBy(item) : item[splitBy];
                (key === options.rightKey || key === "journal" || key === "transaction" || item.vec?.[0] !== 0 ? rightItems : leftItems).push(item);
            }
            const listRenderer = options.progressive ? renderProgressiveItems : renderItems;
            listRenderer(left, leftItems);
            listRenderer(right, rightItems);
            return;
        }

        if (options.progressive) return renderProgressiveItems(container, items);
        return renderItems(container, items);
    },
};

if (typeof input !== "undefined" && input && typeof input === "object") {
    input.ViewKit = ViewKit;
}
