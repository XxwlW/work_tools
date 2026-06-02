/**
 * Shared query contract helpers for DataviewJS views.
 *
 * ViewQuery keeps the public Query() semantics intact and adds the view-layer
 * contract on top: base constraints, consume rules, interaction filters, and
 * available filter options.
 */

const ViewQuery = {
    normalizeTag(tag, ViewKit) {
        return ViewKit?.normalizeFilterTag
            ? ViewKit.normalizeFilterTag(tag)
            : String(tag || "").replace(/^#/, "").trim();
    },

    normalizeLink(link, ViewKit) {
        return ViewKit?.normalizeFilterLink
            ? ViewKit.normalizeFilterLink(link)
            : String(typeof link === "object" ? (link.target || link.path || link.key || link.label || "") : link || "")
                .replace(/\.md$/i, "")
                .trim();
    },

    entryFilterItem(entry) {
        return {
            tags: entry?.meta?.tags || [],
            links: entry?.meta?.outlinks || [],
            linksDetailed: entry?.linksDetailed || entry?.meta?.linksDetailed || [],
            displayParts: entry?.displayParts || [],
            displayText: entry?.displayText,
            cleanText: entry?.cleanText,
            text: entry?.text,
            path: entry?.sourcePath,
            ctime: this.resolveEntryDate(entry),
            vec: [
                entry?.vector?.money || 0,
                entry?.vector?.emotion || 0,
                entry?.vector?.time || 0,
            ],
            money: entry?.vector?.money || 0,
            emotion: entry?.vector?.emotion || 0,
            time: entry?.vector?.time || 0,
        };
    },

    resolveEntryDate(entry) {
        return entry?.ctime
            || entry?.date
            || entry?.meta?.explicitDate
            || entry?.sourcePage?.file?.day
            || entry?.sourcePage?.file?.ctime
            || null;
    },

    toTimestamp(value, ViewKit) {
        if (value == null) return null;
        if (ViewKit?.toTimestamp) return ViewKit.toTimestamp(value);
        if (typeof value === "number") return Number.isFinite(value) ? value : null;
        if (value instanceof Date) return Number.isNaN(value.getTime()) ? null : value.getTime();
        if (typeof value.toMillis === "function") return value.toMillis();
        if (Number.isFinite(value.ts)) return value.ts;
        const parsed = new Date(value).getTime();
        return Number.isNaN(parsed) ? null : parsed;
    },

    entrySearchText(entry) {
        const partText = Array.isArray(entry?.displayParts)
            ? entry.displayParts.map(part => part?.label || part?.text || part?.target || "").join(" ")
            : "";
        return [
            entry?.displayText,
            entry?.cleanText,
            entry?.text,
            entry?.rawText,
            entry?.sourcePath,
            partText,
        ].filter(Boolean).join(" ").toLowerCase();
    },

    entryHasAllTags(entry, tags, ViewKit) {
        const required = (tags || []).map(tag => this.normalizeTag(tag, ViewKit)).filter(Boolean);
        if (!required.length) return true;
        const entryTags = new Set((entry?.meta?.tags || []).map(tag => this.normalizeTag(tag, ViewKit)));
        return required.every(tag => entryTags.has(tag));
    },

    entryLinks(entry, ViewKit) {
        if (ViewKit?.collectLinks) return ViewKit.collectLinks([this.entryFilterItem(entry)]);
        return (entry?.meta?.outlinks || []).map(link => ({ key: this.normalizeLink(link, ViewKit), label: this.normalizeLink(link, ViewKit) }));
    },

    entryMatchesInteraction(entry, interaction = {}, ViewKit) {
        const search = String(interaction.search || "").trim().toLowerCase();
        if (search && !this.entrySearchText(entry).includes(search)) return false;

        const tagFilters = (interaction.tags || []).map(tag => this.normalizeTag(tag, ViewKit)).filter(Boolean);
        const linkFilters = (interaction.links || []).map(link => this.normalizeLink(link, ViewKit)).filter(Boolean);
        if (tagFilters.length || linkFilters.length) {
            const entryTags = new Set((entry?.meta?.tags || []).map(tag => this.normalizeTag(tag, ViewKit)));
            const entryLinks = new Set(this.entryLinks(entry, ViewKit).map(link => link.key));
            const hits = [
                ...tagFilters.map(tag => entryTags.has(tag)),
                ...linkFilters.map(link => entryLinks.has(link)),
            ];
            const matched = String(interaction.matchMode || "and").toLowerCase() === "or"
                ? hits.some(Boolean)
                : hits.every(Boolean);
            if (!matched) return false;
        }

        const start = interaction.startDate ? new Date(`${interaction.startDate}T00:00:00`).getTime() : null;
        const end = interaction.endDate ? new Date(`${interaction.endDate}T23:59:59.999`).getTime() : null;
        if (start != null || end != null) {
            const ts = this.toTimestamp(this.resolveEntryDate(entry), ViewKit);
            if (ts == null) return false;
            if (start != null && ts < start) return false;
            if (end != null && ts > end) return false;
        }

        return true;
    },

    sortEntries(entries = [], interaction = {}, ViewKit) {
        const sortKey = interaction.sort || "";
        if (!sortKey) return entries;
        const sortFields = ViewKit?.filterSortFields ? ViewKit.filterSortFields([sortKey]) : [];
        const sortDef = sortFields.find(field => field?.key === sortKey);
        if (!sortDef?.fn) return entries;
        const sortable = entries.map((entry, index) => ({ entry, index, item: this.entryFilterItem(entry) }));
        sortable.sort((a, b) => {
            const result = sortDef.fn(a.item, b.item);
            return result || a.index - b.index;
        });
        if (interaction.sortAsc) sortable.reverse();
        return sortable.map(row => row.entry);
    },

    normalizeSource(source = {}) {
        if (source.querySources) return { ...source.querySources };
        if (source.sources) return { ...source.sources };
        const maxPages = source.maxPages || source.sourceMaxPages;
        if (source.allowGlobal || !source.scope) {
            return { allowGlobal: true, ...(maxPages ? { maxPages } : {}) };
        }
        return { scope: source.scope, ...(maxPages ? { maxPages } : {}) };
    },

    queryEntries({ Query, source = {}, rules = {}, debug = false }) {
        if (Array.isArray(source.sourceEntries)) return source.sourceEntries;
        const querySources = this.normalizeSource(source);
        let query = Query().from(querySources).filter(rules || {});
        if (debug) query = query.debug(true);
        return query.execute();
    },

    consumeEntry(entry, consume = {}) {
        const value = typeof consume.entry === "function" ? consume.entry(entry) : {};
        if (typeof consume.include === "function") {
            return consume.include(value, entry) ? value : null;
        }

        const types = consume.types || [];
        if (types.length && !types.includes(entry?.type)) return null;
        const metrics = consume.metrics || [];
        if (!metrics.length) return value;

        const hasMetric = metrics.some(metric => {
            const direct = Number(value?.[metric]);
            const vectorValue = Number(entry?.vector?.[metric]);
            return (Number.isFinite(direct) && direct !== 0) || (Number.isFinite(vectorValue) && vectorValue !== 0);
        });
        return hasMetric ? value : null;
    },

    collect(options = {}) {
        const { Query, ViewKit } = options;
        if (typeof Query !== "function") throw new Error("ViewQuery.collect requires Query");

        const sourceEntries = this.queryEntries(options);
        const dataset = {
            entries: sourceEntries,
            sourceEntries,
            consumedEntries: [],
            visibleEntries: [],
            filterItems: [],
            availableTags: [],
            availableLinks: [],
            warnings: sourceEntries?.warnings || [],
            queryMetrics: sourceEntries?.metrics || {},
            metrics: {
                sourceEntries: sourceEntries.length || 0,
                consumedEntries: 0,
                visibleEntries: 0,
            },
            consumptionByEntry: new Map(),
        };

        const consume = options.consume || {};
        const baseTags = consume.baseTags || [];
        for (const entry of sourceEntries || []) {
            if (!this.entryHasAllTags(entry, baseTags, ViewKit)) continue;
            const consumption = this.consumeEntry(entry, consume);
            if (!consumption) continue;
            dataset.consumedEntries.push(entry);
            dataset.consumptionByEntry.set(entry, consumption);
            if (this.entryMatchesInteraction(entry, options.interaction || {}, ViewKit)) {
                dataset.visibleEntries.push(entry);
            }
        }
        dataset.visibleEntries = this.sortEntries(dataset.visibleEntries, options.interaction || {}, ViewKit);

        const consumedItems = dataset.consumedEntries.map(entry => this.entryFilterItem(entry));
        dataset.availableTags = [...new Set([
            ...(ViewKit?.collectTags ? ViewKit.collectTags(consumedItems) : consumedItems.flatMap(item => item.tags || [])),
            ...baseTags.map(tag => this.normalizeTag(tag, ViewKit)),
        ].filter(Boolean))]
            .sort((a, b) => a.localeCompare(b, "zh-CN"));

        const allLinks = ViewKit?.collectLinks ? ViewKit.collectLinks(consumedItems) : [];
        dataset.availableLinks = typeof options.excludeLink === "function"
            ? allLinks.filter(link => !options.excludeLink(link))
            : allLinks;
        dataset.filterItems = consumedItems;
        dataset.metrics.consumedEntries = dataset.consumedEntries.length;
        dataset.metrics.visibleEntries = dataset.visibleEntries.length;
        return dataset;
    },
};

if (typeof input !== "undefined" && input && typeof input === "object") {
    input.ViewQuery = ViewQuery;
}
