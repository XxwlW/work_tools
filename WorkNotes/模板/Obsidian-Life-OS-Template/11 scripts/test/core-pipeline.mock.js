const assert = require("assert");
const fs = require("fs");
const path = require("path");
const vm = require("vm");

const { createMockDataview, makeWalletFieldFixtures } = require("./fixtures/core-pipeline.fixtures");

function loadCore(options = {}) {
    const corePath = path.resolve(__dirname, "../Core/FinanceCore.js");
    const source = fs.readFileSync(corePath, "utf8");
    const input = {};
    const context = {
        console,
        input,
        dv: createMockDataview(options),
        Date,
        Map,
        Set,
        RegExp,
        Number,
        Array,
        Object,
        String,
        JSON,
        Math,
        Promise,
        Blob: function Blob() {},
        URL: { createObjectURL: () => "" },
        document: {
            createElement: () => ({ click() {} }),
            body: { appendChild() {}, removeChild() {} },
        },
        Notice: function Notice() {},
    };
    vm.createContext(context);
    vm.runInContext(source, context, { filename: corePath });
    return { core: input, dv: context.dv };
}

function loadViewKit(options = {}) {
    const viewKitPath = path.resolve(__dirname, "../Core/ViewKit.js");
    const source = fs.readFileSync(viewKitPath, "utf8");
    const input = {};
    const dv = createMockDataview(options);
    dv.date = value => {
        const date = new Date(value);
        return {
            ts: date.getTime(),
            month: date.getMonth() + 1,
            day: date.getDate(),
            toFormat(format) {
                if (format === "yyyy-MM-dd") return dateOnly(date);
                return date.toISOString();
            },
        };
    };
    const context = {
        console,
        input,
        dv,
        Date,
        Number,
        Array,
        Object,
        String,
        RegExp,
        Blob: function Blob() {},
        URL: { createObjectURL: () => "" },
        document: {
            createElement: () => ({ click() {} }),
            body: { appendChild() {}, removeChild() {} },
        },
        sessionStorage: options.sessionStorage,
        Notice: function Notice() {},
    };
    vm.createContext(context);
    vm.runInContext(source, context, { filename: viewKitPath });
    return input.ViewKit;
}

function loadViewQuery() {
    const viewQueryPath = path.resolve(__dirname, "../Core/ViewQuery.js");
    const source = fs.readFileSync(viewQueryPath, "utf8");
    const input = {};
    const context = {
        console,
        input,
        Date,
        Map,
        Set,
        Number,
        Array,
        Object,
        String,
        Error,
    };
    vm.createContext(context);
    vm.runInContext(source, context, { filename: viewQueryPath });
    return input.ViewQuery;
}

function test(name, fn) {
    return { name, fn, status: "normal" };
}

function dateOnly(date) {
    const d = new Date(date);
    return [
        d.getFullYear(),
        String(d.getMonth() + 1).padStart(2, "0"),
        String(d.getDate()).padStart(2, "0"),
    ].join("-");
}

function squish(text) {
    return String(text || "").replace(/\s+/g, " ").trim();
}

function mockDateTime(value) {
    const date = new Date(value);
    return {
        ts: date.getTime(),
        toMillis() { return this.ts; },
        toFormat(format) {
            if (format === "yyyy-MM-dd") return value.slice(0, 10);
            return value;
        },
    };
}

function makeDomElement(tag = "div") {
    const el = {
        tagName: String(tag).toUpperCase(),
        children: [],
        style: {},
        attributes: {},
        className: "",
        textContent: "",
        value: "",
        _listeners: {},
        createEl(childTag, options = {}) {
            const child = makeDomElement(childTag);
            child.parentElement = this;
            if (options.cls) child.className = options.cls;
            if (options.text !== undefined) child.textContent = options.text;
            if (options.attr) {
                for (const [key, value] of Object.entries(options.attr)) child.setAttribute(key, value);
            }
            this.children.push(child);
            return child;
        },
        empty() {
            this.children = [];
        },
        setAttribute(key, value) {
            this.attributes[key] = String(value);
            this[key] = value;
        },
        addEventListener(event, handler) {
            if (!this._listeners[event]) this._listeners[event] = [];
            this._listeners[event].push(handler);
        },
    };
    el.classList = {
        add(cls) {
            const classes = new Set(el.className.split(/\s+/).filter(Boolean));
            classes.add(cls);
            el.className = [...classes].join(" ");
        },
        remove(cls) {
            const classes = new Set(el.className.split(/\s+/).filter(Boolean));
            classes.delete(cls);
            el.className = [...classes].join(" ");
        },
        toggle(cls, force) {
            const shouldAdd = force === undefined ? !this.contains(cls) : Boolean(force);
            if (shouldAdd) this.add(cls);
            else this.remove(cls);
            return shouldAdd;
        },
        contains(cls) {
            return el.className.split(/\s+/).filter(Boolean).includes(cls);
        },
    };
    return el;
}

function makePage(spec) {
    const frontmatter = spec.frontmatter || {};
    return {
        ...spec.fields,
        file: {
            path: spec.path,
            name: spec.name || path.basename(spec.path, ".md"),
            tags: spec.tags || [],
            frontmatter,
            lists: spec.lists || [],
            inlinks: spec.inlinks || [],
            outlinks: spec.outlinks || [],
            ctime: mockDateTime(spec.ctime || "2026-05-01"),
            mtime: mockDateTime(spec.mtime || spec.ctime || "2026-05-01"),
            day: spec.day ? mockDateTime(spec.day) : null,
            link: { path: spec.path },
        },
        tags: (spec.tags || []).map(t => t.replace(/^#/, "")),
    };
}

function listItem(line, text, tags = [], children = [], parent = null) {
    const item = { line, text, tags, children, parent };
    for (const child of children) {
        if (child.parent == null) child.parent = line;
    }
    return item;
}

function child(line, text, tags = [], children = []) {
    return listItem(line, text, tags, children, null);
}

function buildHeatmapData(core, options = {}) {
    const baseTags = options.tags || null;
    const activeTags = options.activeTags || null;
    const activeLinks = options.activeLinks || null;
    const matchMode = String(options.matchMode || "and").toLowerCase() === "or" ? "or" : "and";
    const excludeTags = options.excludeTags || null;
    const filterName = options.linkedToName || null;
    const mode = options.mode || null;
    const startDate = options.startDate ? new Date(options.startDate) : null;
    const endDate = options.endDate ? new Date(options.endDate) : null;

    const rawDataPath = options.dataPath;
    const rawGlobal = options.global ?? options.allowGlobal;
    const rawMaxPages = options.maxPages ?? 800;
    const maxPages = Number.isFinite(Number(rawMaxPages)) && Number(rawMaxPages) > 0
        ? Math.floor(Number(rawMaxPages))
        : 800;
    const isGlobalSource = value => {
        if (value === true) return true;
        if (value === false || value == null) return false;
        return ["全局", "全部", "全库", "global", "all", "*"].includes(String(value).trim().toLowerCase());
    };
    const useGlobal = isGlobalSource(rawGlobal) || isGlobalSource(rawDataPath) || rawDataPath == null || rawDataPath === "";
    const sources = useGlobal ? { allowGlobal: true, maxPages } : { scope: rawDataPath, maxPages };
    const rules = { startDate, endDate };
    if (filterName) {
        if (!useGlobal) sources.linkedTo = filterName;
        rules.explicitTarget = filterName;
    }
    const normalizedBaseTags = [...new Set([...(baseTags || [])].map(tag => String(tag).replace(/^#/, "")))];
    const normalizedActiveTags = [...new Set([...(activeTags || [])].map(tag => String(tag).replace(/^#/, "")))];
    const normalizedActiveLinks = [...new Set([...(activeLinks || [])].map(link => String(link).replace(/\.md$/i, "")))];
    if (normalizedBaseTags.length) rules.tags = normalizedBaseTags;
    if (excludeTags) rules.excludeTags = excludeTags;

    const entries = core.Query().from(sources).filter(rules).debug(true).execute();
    const consumedEntries = [];
    const visibleEntries = [];
    const dailyNet = new Map();
    const dailyTime = new Map();
    const dailyItems = new Map();

    for (const entry of entries) {
        const entryTags = new Set((entry.meta?.tags || []).map(tag => String(tag).replace(/^#/, "")));
        if (normalizedBaseTags.length && !normalizedBaseTags.every(tag => entryTags.has(tag))) continue;
        const entryLinks = new Set((entry.meta?.outlinks || []).map(link => String(link).replace(/\.md$/i, "")));
        const activeHits = [
            ...normalizedActiveTags.map(tag => entryTags.has(tag)),
            ...normalizedActiveLinks.map(link => entryLinks.has(link)),
        ];
        const dateKey = dateOnly(core.Utils.resolveEntryDate(entry));
        const isTransfer = (entry.meta?.tags || []).includes("转账");
        const money = entry.type === "journal" && !isTransfer ? (entry.vector.money || 0) : 0;
        const time = entry.type === "event" && entry.vector.time > 0 ? entry.vector.time : 0;
        if (mode === "净值" && money === 0) continue;
        if (mode === "专注" && time === 0) continue;
        if (money === 0 && time === 0) continue;
        consumedEntries.push(entry);
        if (activeHits.length) {
            const keep = matchMode === "or" ? activeHits.some(Boolean) : activeHits.every(Boolean);
            if (!keep) continue;
        }
        visibleEntries.push(entry);
        if (money !== 0) dailyNet.set(dateKey, (dailyNet.get(dateKey) || 0) + money);
        if (time !== 0) dailyTime.set(dateKey, (dailyTime.get(dateKey) || 0) + time);
        dailyItems.set(dateKey, [...(dailyItems.get(dateKey) || []), { text: entry.cleanText, money, time }]);
    }

    return {
        entries,
        sourceEntries: entries,
        consumedEntries,
        visibleEntries,
        warnings: entries.warnings || [],
        metrics: {
            sourceEntries: entries.length,
            consumedEntries: consumedEntries.length,
            visibleEntries: visibleEntries.length,
        },
        dailyNet,
        dailyTime,
        dailyItems,
    };
}

function collectHeatmapAvailableTags(core, options = {}) {
    const data = buildHeatmapData(core, options);
    return [...new Set([
        ...data.consumedEntries.flatMap(entry => entry.meta?.tags || []),
        ...(options.tags || []),
    ].map(tag => String(tag).replace(/^#/, "")).filter(Boolean))]
        .sort((a, b) => a.localeCompare(b, "zh-CN"));
}

function collectHeatmapAvailableLinks(core, options = {}) {
    const data = buildHeatmapData(core, options);
    const anchor = options.linkedToName || "";
    const labels = new Set();
    for (const entry of data.consumedEntries) {
        for (const link of entry.meta?.outlinks || []) {
            const label = String(link).replace(/\.md$/i, "").split(/[\\/]/).pop();
            if (label && label !== anchor) labels.add(label);
        }
    }
    return [...labels].sort((a, b) => a.localeCompare(b, "zh-CN"));
}

function buildObjectProfileDataset(core, dv, options = {}) {
    const ViewKit = options.ViewKit || loadViewKit();
    const ViewQuery = options.ViewQuery || loadViewQuery();
    const targetFile = dv.current()?.file || null;
    const presetKey = options.preset || "topic";
    const isTransferEntry = entry => (entry?.meta?.tags || []).includes("转账");
    const consume = {
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
            if (presetKey === "asset") {
                return consumption.lifeDays > 0 || consumption.entryType === "event";
            }
            return true;
        },
    };

    return ViewQuery.collect({
        Query: core.Query,
        ViewKit,
        source: { querySources: { linkedTo: targetFile?.path || true, maxPages: 800 } },
        rules: { explicitTarget: true, ...(options.rules || {}) },
        consume,
        interaction: options.interaction || {},
        excludeLink: link => {
            const key = ViewKit.normalizeFilterLink(link);
            const label = ViewKit.linkLabel(link);
            return core.Utils.linkMatchesTarget(key, targetFile)
                || core.Utils.linkMatchesTarget(label, targetFile);
        },
    });
}

function buildPersonProfileDataset(core, dv, options = {}) {
    const ViewKit = options.ViewKit || loadViewKit();
    const ViewQuery = options.ViewQuery || loadViewQuery();
    const targetFile = dv.current()?.file || null;
    const viewType = options.type || "event";
    const entryType = viewType === "transaction" ? "journal" : "event";

    return ViewQuery.collect({
        Query: core.Query,
        ViewKit,
        source: { querySources: { linkedTo: true } },
        rules: { type: entryType, explicitTarget: true, ...(options.rules || {}) },
        consume: {
            types: [entryType],
            entry(entry) {
                return {
                    money: entry?.type === "journal" ? (entry.vector?.money || 0) : 0,
                    emotion: entry?.type === "event" ? (entry.vector?.emotion || 0) : 0,
                    time: entry?.type === "event" ? (entry.vector?.time || 0) : 0,
                    entryType: entry?.type,
                };
            },
        },
        interaction: options.interaction || {},
        excludeLink: link => {
            const key = ViewKit.normalizeFilterLink(link);
            const label = ViewKit.linkLabel(link);
            return core.Utils.linkMatchesTarget(key, targetFile)
                || core.Utils.linkMatchesTarget(label, targetFile);
        },
    });
}

function buildProjectProfileDataset(core, dv, options = {}) {
    const ViewKit = options.ViewKit || loadViewKit();
    const ViewQuery = options.ViewQuery || loadViewQuery();
    const targetFile = dv.current()?.file || null;
    const viewType = options.type || "event";
    const entryType = viewType === "transaction" ? "journal" : "event";

    return ViewQuery.collect({
        Query: core.Query,
        ViewKit,
        source: { querySources: { linkedTo: true } },
        rules: { type: entryType, explicitTarget: true, ...(options.rules || {}) },
        consume: {
            types: [entryType],
            entry(entry) {
                return {
                    money: entry?.type === "journal" ? (entry.vector?.money || 0) : 0,
                    emotion: entry?.type === "event" ? (entry.vector?.emotion || 0) : 0,
                    time: entry?.type === "event" ? (entry.vector?.time || 0) : 0,
                    entryType: entry?.type,
                };
            },
        },
        interaction: options.interaction || {},
        excludeLink: link => {
            const key = ViewKit.normalizeFilterLink(link);
            const label = ViewKit.linkLabel(link);
            return core.Utils.linkMatchesTarget(key, targetFile)
                || core.Utils.linkMatchesTarget(label, targetFile);
        },
    });
}

function buildWalletEventProfileDataset(core, dv, options = {}) {
    const ViewKit = options.ViewKit || loadViewKit();
    const ViewQuery = options.ViewQuery || loadViewQuery();
    const targetFile = dv.current()?.file || null;

    return ViewQuery.collect({
        Query: core.Query,
        ViewKit,
        source: { querySources: { linkedTo: true } },
        rules: { type: "event", explicitTarget: true, ...(options.rules || {}) },
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
        interaction: options.interaction || {},
        excludeLink: link => {
            const key = ViewKit.normalizeFilterLink(link);
            const label = ViewKit.linkLabel(link);
            return core.Utils.linkMatchesTarget(key, targetFile)
                || core.Utils.linkMatchesTarget(label, targetFile);
        },
    });
}

function buildDiaryProfileDataset(core, options = {}) {
    const ViewKit = options.ViewKit || loadViewKit();
    const ViewQuery = options.ViewQuery || loadViewQuery();

    return ViewQuery.collect({
        Query: core.Query,
        ViewKit,
        source: { querySources: { currentAndLinkedDiary: true } },
        consume: {
            entry(entry) {
                return {
                    money: entry?.vector?.money || 0,
                    emotion: entry?.vector?.emotion || 0,
                    time: entry?.vector?.time || 0,
                    entryType: entry?.type,
                };
            },
            include(consumption) {
                if (consumption.entryType === "journal") return true;
                return Boolean(consumption.money || consumption.emotion || consumption.time);
            },
        },
        interaction: options.interaction || {},
    });
}

function entryTargetsPerson(entry, person) {
    const candidates = [person.name, person.path, person.path.replace(/\.md$/, "")];
    const normalize = value => String(value || "").replace(/\.md$/, "");
    return (entry.meta.outlinks || []).some(link => {
        const normalizedLink = normalize(link);
        return candidates.some(candidate => {
            const normalizedCandidate = normalize(candidate);
            return normalizedLink === normalizedCandidate || normalizedLink.endsWith(`/${normalizedCandidate}`);
        });
    }) || candidates.some(candidate => entry.rawText?.includes(`[[${candidate}`));
}

function buildPersonCollectionData(core, dv, options = {}) {
    const personPages = core.Utils.collectSupertagPages({
        tag: options.personTags || options.personTag || options.personQuery || ["人物", "人"],
        scope: options.personScope,
        dv,
    });
    const peopleMap = new Map();
    const inlinkToPersons = new Map();

    for (const p of personPages) {
        const person = { name: p.file.name, path: p.file.path };
        peopleMap.set(person.name, { ...person, totalTime: 0, netMoney: 0, count: 0, emotionRecords: [] });
        for (const link of p.file.inlinks) {
            if (!inlinkToPersons.has(link.path)) inlinkToPersons.set(link.path, new Set());
            inlinkToPersons.get(link.path).add(person.name);
        }
    }

    const entries = core.Query().from({ paths: [...inlinkToPersons.keys()] }).debug(true).execute();
    for (const entry of entries) {
        const personNames = inlinkToPersons.get(entry.sourcePath);
        if (!personNames) continue;
        for (const name of personNames) {
            const person = peopleMap.get(name);
            if (!person || !entryTargetsPerson(entry, person)) continue;
            person.totalTime += entry.vector.time || 0;
            const isTransfer = entry.meta?.tags?.includes("转账");
            person.netMoney += entry.type === "journal" && !isTransfer ? (entry.vector.money || 0) : 0;
            if (entry.type === "event" && entry.vector.emotion !== 0) {
                person.emotionRecords.push({ score: entry.vector.emotion, date: core.Utils.resolveEntryDate(entry) });
            }
            person.count += 1;
        }
    }

    return { entries, peopleMap };
}

function buildPersonCollectionDataIndexed(core, dv, options = {}) {
    const personPages = core.Utils.collectSupertagPages({
        tag: options.personTags || options.personTag || options.personQuery || ["人物", "人"],
        scope: options.personScope,
        dv,
    });
    const peopleMap = new Map();
    const inlinkToPersons = new Map();

    for (const p of personPages) {
        const person = { name: p.file.name, path: p.file.path };
        peopleMap.set(person.name, { ...person, totalTime: 0, netMoney: 0, count: 0, emotionRecords: [] });
        for (const link of p.file.inlinks) {
            if (!inlinkToPersons.has(link.path)) inlinkToPersons.set(link.path, new Set());
            inlinkToPersons.get(link.path).add(person.name);
        }
    }

    const outlinkIndex = new Map();
    const addIndex = (candidate, name) => {
        const normalized = String(candidate || "").replace(/\.md$/, "");
        if (!normalized) return;
        if (!outlinkIndex.has(normalized)) outlinkIndex.set(normalized, new Set());
        outlinkIndex.get(normalized).add(name);
    };
    for (const [name, person] of peopleMap) {
        addIndex(name, name);
        addIndex(person.path, name);
        addIndex(person.path.replace(/\.md$/, ""), name);
    }

    const findTargets = (entry, allowedNames) => {
        const allowed = allowedNames instanceof Set ? allowedNames : new Set(allowedNames || []);
        const matched = new Set();
        const addMatches = link => {
            const normalized = String(link || "").replace(/\.md$/, "");
            for (const key of [normalized, normalized.split("/").pop()].filter(Boolean)) {
                const names = outlinkIndex.get(key);
                if (!names) continue;
                for (const name of names) if (allowed.has(name)) matched.add(name);
            }
        };
        for (const link of entry.meta?.outlinks || []) addMatches(link);
        return matched;
    };

    const entries = core.Query().from({ paths: [...inlinkToPersons.keys()] }).debug(true).execute();
    for (const entry of entries) {
        const personNames = inlinkToPersons.get(entry.sourcePath);
        if (!personNames) continue;
        for (const name of findTargets(entry, personNames)) {
            const person = peopleMap.get(name);
            if (!person) continue;
            person.totalTime += entry.vector.time || 0;
            const isTransfer = entry.meta?.tags?.includes("转账");
            person.netMoney += entry.type === "journal" && !isTransfer ? (entry.vector.money || 0) : 0;
            if (entry.type === "event" && entry.vector.emotion !== 0) {
                person.emotionRecords.push({ score: entry.vector.emotion, date: core.Utils.resolveEntryDate(entry) });
            }
            person.count += 1;
        }
    }

    return { entries, peopleMap };
}

function buildProjectCollectionData(core, dv, options = {}) {
    const projectPages = core.Utils.collectSupertagPages({
        tag: options.projectTag || options.projectQuery || "项目",
        scope: options.projectScope,
        dv,
    });
    const projectsMap = new Map();
    const inlinkToProjects = new Map();
    const readArray = value => core.Utils.normalizeArrayField(value).map(item => String(item).replace(/^#/, "")).filter(Boolean);
    const readNumber = value => {
        const raw = Array.isArray(value) ? value[0] : value;
        const num = Number(String(raw ?? "").replace(/[^\d.-]/g, ""));
        return Number.isFinite(num) && num > 0 ? num : 0;
    };

    for (const p of projectPages) {
        const fm = p.file.frontmatter || {};
        const project = {
            name: p.file.name,
            path: p.file.path,
            tags: readArray(p["标签"] ?? fm["标签"]).filter(tag => tag !== "项目"),
            status: p["状态"] ?? fm["状态"] ?? "进行中",
            targetEffort: readNumber(p["期望努力值"] ?? fm["期望努力值"]),
            totalTime: 0,
            netCost: 0,
            count: 0,
        };
        projectsMap.set(project.name, project);
        for (const link of p.file.inlinks) {
            if (!inlinkToProjects.has(link.path)) inlinkToProjects.set(link.path, new Set());
            inlinkToProjects.get(link.path).add(project.name);
        }
    }

    const outlinkIndex = new Map();
    const addIndex = (candidate, name) => {
        const normalized = String(candidate || "").replace(/\.md$/, "");
        if (!normalized) return;
        if (!outlinkIndex.has(normalized)) outlinkIndex.set(normalized, new Set());
        outlinkIndex.get(normalized).add(name);
    };
    for (const [name, project] of projectsMap) {
        addIndex(name, name);
        addIndex(project.path, name);
        addIndex(project.path.replace(/\.md$/, ""), name);
    }

    const entries = core.Query().from({ paths: [...inlinkToProjects.keys()] }).debug(true).execute();
    for (const entry of entries) {
        const projectNames = inlinkToProjects.get(entry.sourcePath);
        if (!projectNames) continue;
        const outlinks = entry.meta?.outlinks || [];
        const matched = new Set();
        for (const link of outlinks) {
            const normalized = String(link || "").replace(/\.md$/, "");
            for (const key of [normalized, normalized.split("/").pop()].filter(Boolean)) {
                const names = outlinkIndex.get(key);
                if (!names) continue;
                for (const name of names) {
                    if (projectNames.has(name)) matched.add(name);
                }
            }
        }
        for (const name of matched) {
            const project = projectsMap.get(name);
            if (!project) continue;
            project.totalTime += entry.vector.time || 0;
            const isTransfer = entry.meta?.tags?.includes("转账");
            project.netCost += entry.type === "journal" && !isTransfer ? (entry.vector.money || 0) : 0;
            project.count += 1;
        }
    }

    const projectData = Array.from(projectsMap.values()).map(project => ({
        name: project.name,
        path: project.path,
        tags: project.tags,
        status: project.status,
        targetEffort: project.targetEffort,
        time: project.totalTime,
        money: project.netCost,
        count: project.count,
        progress: project.targetEffort > 0 ? (project.totalTime / project.targetEffort) * 100 : null,
    }));

    return { entries, projectData };
}

const cases = [
    test("Query exports the current pipeline entrypoint", () => {
        const { core } = loadCore();
        assert.equal(typeof core.Query, "function");
        assert.equal(core.Utils["parseListItems"], undefined);
        assert.equal(typeof core.Utils.filterPartsForView, "function");
        assert.equal(typeof core.Utils.entryToViewItem, "function");
        assert.equal(typeof core.Utils.walletFromEntry, "function");
        assert.equal(typeof core.Utils.toDateTime, "function");
        assert.equal(typeof core.Utils.normalizeArrayField, "function");
        assert.equal(typeof core.Utils.normalizeNumber, "function");
        assert.equal(typeof core.Utils.resolveBillDate, "function");
        assert.equal(typeof core.Utils.normalizeSupertagInput, "function");
        assert.equal(typeof core.Utils.hasObjectSupertag, "function");
        assert.equal(typeof core.Utils.hasFrontmatterTag, "function");
        assert.equal(typeof core.Utils.isWalletPage, "function");
        assert.equal(typeof core.Utils.collectSupertagPages, "function");
        assert.equal(typeof core.Utils.collectWalletPages, "function");
        assert.equal(typeof core.Utils.collectWallets, "function");
        assert.equal(typeof core.Utils.collectWalletBills, "function");
        assert.equal(typeof core.Utils.toWalletSummary, "function");
        assert.equal(typeof core.Utils.collectWalletSummaries, "function");
        assert.equal(typeof core.Utils.detectEntryAnomalies, "function");
        assert.equal(typeof core.Utils.inspectObjectQuality, "function");
        assert.equal(typeof core.Utils.collectLivingCostItems, "function");
        assert.equal(typeof core.Utils.escapeHtml, "function");
        assert.equal(typeof core.Utils.safeLinkText, "function");
        assert.equal(typeof core.ObjectSummary, "object");
        assert.equal(typeof core.ObjectSummary.collect, "function");
        assert.equal(typeof core.ObjectSummary.findEntryTargets, "function");
        assert.equal(core.Utils.escapeHtml(`小明 <tag> & "账单"`), "小明 &lt;tag&gt; &amp; &quot;账单&quot;");
        assert.equal(core.Utils.safeLinkText("[[03 人物/人/小明|小明 <&>]]"), "小明 &lt;&amp;&gt;");
        assert.equal(typeof core.Wallet, "function");
    }),
    test("B1 ViewKit exposes shared view adapters", () => {
        const ViewKit = loadViewKit();
        assert.equal(typeof ViewKit.processText, "function");
        assert.equal(typeof ViewKit.fmtMoney, "function");
        assert.equal(typeof ViewKit.exportToCSV, "function");
        assert.equal(typeof ViewKit.getBirthdayInfo, "function");
        assert.equal(typeof ViewKit.renderTimeline, "function");
        assert.equal(typeof ViewKit.renderModuleShell, "function");
        assert.equal(typeof ViewKit.renderRankList, "function");
        assert.equal(typeof ViewKit.renderTransactionList, "function");
        assert.equal(typeof ViewKit.renderDebugPanel, "function");
        assert.equal(typeof ViewKit.renderEvidenceLinks, "function");
        assert.equal(typeof ViewKit.sanitizeDebugValue, "function");
        assert.equal(typeof ViewKit.renderSegmentedControl, "function");
        assert.equal(typeof ViewKit.renderProgressiveList, "function");
        assert.equal(typeof ViewKit.escapeHtml, "function");
        assert.equal(typeof ViewKit.safeLinkText, "function");
        assert.equal(typeof ViewKit.sourceHref, "function");
        assert.equal(typeof ViewKit.renderSourceButton, "function");
        assert.equal(typeof ViewKit.FilterBar, "function");
        assert.equal(typeof ViewKit.collectTags, "function");
        assert.equal(typeof ViewKit.collectLinks, "function");
        assert.equal(typeof ViewKit.filterSortFields, "function");
        assert.equal(ViewKit.escapeHtml(`A&B <"'>`), "A&amp;B &lt;&quot;&#39;&gt;");
        assert.equal(ViewKit.safeLinkText("[[钱包/信用卡|信用 <卡>]]"), "信用 &lt;卡&gt;");
        assert.equal(ViewKit.fmtMoney(1234.5), "1,234.50");
        assert.equal(
            ViewKit.sourceHref({ link: { path: "01 日记/2026-05-06.md", blockId: "bill1", subpath: 7 } }),
            "01 日记/2026-05-06.md#^bill1"
        );
        assert.equal(
            ViewKit.sourceHref({ link: { path: "01 日记/2026-05-06.md", subpath: 7 } }),
            "01 日记/2026-05-06.md"
        );
        assert.equal(
            ViewKit.sourceHref({ link: { path: "01 日记/2026-05-06.md", subpath: "小结" } }),
            "01 日记/2026-05-06.md#小结"
        );
        assert.equal(
            ViewKit.renderSourceButton({ path: "01 日记/2026-05-06.md" }, { className: "pp-source-link", label: "src", style: "" }),
            `<a class="internal-link pp-source-link" href="01 日记/2026-05-06.md" data-href="01 日记/2026-05-06.md" target="_blank" rel="noopener" title="打开来源">src</a>`
        );

        assert.equal(typeof ViewKit.renderDisplayParts, "function");
        const processed = ViewKit.processText("午餐 [[样例信用卡|信用卡]] [[小明]] #餐饮", ["#餐饮"], "小明", true);
        assert.equal(processed.wallet.display, "信用卡");
        assert.equal(squish(processed.text), "午餐 小明");
        assert.equal(
            ViewKit.renderDisplayParts([{ type: "text", text: "和" }, { type: "link", target: "项目A", label: "项目A", role: "object" }]),
            `和 <a class="internal-link vk-display-link vk-display-link-object" href="项目A" data-href="项目A" target="_blank" rel="noopener" style="text-decoration:none; color:var(--c-accent, var(--text-accent));">项目A</a>`
        );

        const birthday = ViewKit.getBirthdayInfo({ "生日": "2026-05-20", "生日类型": "公历" });
        assert.ok(birthday);
        assert.equal(typeof birthday.days, "number");
        assert.deepEqual(ViewKit.collectTags([{ tags: ["#零食[[微信0991]]", "记账"] }]), ["记账", "零食"]);
        assert.deepEqual(
            ViewKit.collectLinks([{ links: ["04 项目/华鼎装饰", { target: "03 人物/钱包/微信0991", label: "微信0991" }] }]).map(link => link.label),
            ["华鼎装饰", "微信0991"]
        );
    }),
    test("ViewKit segmented control renders counts and emits changes", () => {
        const ViewKit = loadViewKit();
        const host = makeDomElement("div");
        let nextValue = null;

        const bar = ViewKit.renderSegmentedControl(host, {
            value: "all",
            options: [
                { key: "all", label: "全部", count: 3 },
                { key: "income", label: "收入", count: 1 },
            ],
            onChange: value => { nextValue = value; },
        });

        assert.equal(bar.className, "vk-segmented");
        assert.equal(bar.attributes.role, "tablist");
        assert.equal(bar.children.length, 2);
        assert.ok(bar.children[0].className.includes("is-active"));
        assert.equal(bar.children[0].children[0].textContent, "全部");
        assert.equal(bar.children[0].children[1].textContent, "3");
        bar.children[1].onclick();
        assert.equal(nextValue, "income");
    }),
    test("ViewKit module shell and rank list render reusable module structure", () => {
        const ViewKit = loadViewKit();
        const host = makeDomElement("div");
        const module = ViewKit.renderModuleShell(host, {
            title: "关系对象",
            count: 2,
            countSuffix: " 项",
            renderBody(body) {
                ViewKit.renderRankList(body, {
                    rows: [
                        { target: "03 人物/人/小明", label: "小明", count: 3, money: -12 },
                        { target: "04 项目/项目A", label: "项目A", count: 1, time: 2 },
                    ],
                });
            },
        });

        assert.equal(host.children[1].className, "vk-module");
        assert.equal(module.titleEl.textContent, "关系对象");
        assert.equal(module.countEl.textContent, "2 项");
        assert.equal(module.body.children[0].className, "vk-rank-list");
        assert.equal(module.body.children[0].children.length, 2);
        assert.ok(module.body.children[0].children[0].innerHTML.includes("小明"));
        module.setCount(5, " 条");
        assert.equal(module.countEl.textContent, "5 条");
    }),
    test("ViewKit rank list can render source evidence links", () => {
        const ViewKit = loadViewKit();
        const host = makeDomElement("div");
        ViewKit.renderRankList(host, {
            rows: [
                {
                    target: "03 人物/人/小明",
                    label: "小明",
                    count: 2,
                    evidence: [
                        { link: { path: "01 日记/a.md", blockId: "a1" } },
                        { link: { path: "01 日记/b.md", subpath: "段落" } },
                    ],
                },
            ],
        });

        const html = host.children[0].children[0].innerHTML;
        assert.ok(html.includes("03 人物/人/小明"));
        assert.ok(html.includes("01 日记/a.md#^a1"));
        assert.ok(html.includes("01 日记/b.md#段落"));
        assert.ok(html.includes("证据1"));
        assert.ok(html.includes("证据2"));
    }),
    test("ViewKit progressive timeline renders an initial page and loads more", () => {
        const ViewKit = loadViewKit();
        const host = makeDomElement("div");
        const items = [
            { id: "a", text: "A", vec: [0, 0, 0], ctime: mockDateTime("2026-01-01") },
            { id: "b", text: "B", vec: [0, 0, 0], ctime: mockDateTime("2026-01-02") },
            { id: "c", text: "C", vec: [0, 0, 0], ctime: mockDateTime("2026-01-03") },
        ];

        ViewKit.renderTimeline(host, { items, progressive: true, pageSize: 2 });

        const timeline = host.children[0];
        const footer = host.children[1];
        const button = footer.children[1];

        assert.equal(timeline.className, "pp-timeline");
        assert.equal(timeline.children.length, 2);
        assert.equal(footer.children[0].textContent, "已显示 2 / 3");
        button.onclick();
        assert.equal(timeline.children.length, 3);
        assert.equal(footer.children[0].textContent, "已显示 3 / 3");
        assert.equal(button.style.display, "none");
    }),
    test("ViewKit timeline accepts module-specific classes", () => {
        const ViewKit = loadViewKit();
        const host = makeDomElement("div");
        const items = [
            { id: "a", text: "A", vec: [10, 1, 2], ctime: mockDateTime("2026-01-01"), path: "01 日记/a.md" },
        ];

        const timeline = ViewKit.renderTimeline(host, {
            items,
            timelineClass: "op-timeline",
            itemClass: "op-timeline-item",
            dotClass: "op-timeline-dot",
            contentClass: "op-timeline-content",
            mainClass: "op-timeline-main",
            dateClass: "op-timeline-date",
            textClass: "op-timeline-text",
            metaClass: "op-timeline-meta",
            badgeClass: "op-badge",
            sourceButtonOptions: { className: "op-source-btn", label: "源", style: "" },
        });

        assert.equal(timeline.className, "op-timeline");
        assert.equal(timeline.children[0].className, "op-timeline-item");
        assert.ok(timeline.children[0].innerHTML.includes("op-timeline-content"));
        assert.ok(timeline.children[0].innerHTML.includes("op-source-btn"));
    }),
    test("ViewKit transaction list renders rows and progressive meta", () => {
        const ViewKit = loadViewKit();
        const host = makeDomElement("div");
        let titleText = "";
        let metaText = "";
        const items = [
            {
                text: "午餐",
                displayText: "午餐",
                vec: [-12, 0, 0],
                ctime: mockDateTime("2026-01-01"),
                path: "01 日记/a.md",
                tags: ["餐饮"],
                wallet: { path: "03 人物/钱包/样例信用卡", display: "样例信用卡" },
                sourceLinks: [{ target: "02 事件/订单A", label: "订单A" }],
            },
            {
                text: "报销",
                displayText: "报销",
                vec: [20, 0, 0],
                ctime: mockDateTime("2026-01-02"),
                path: "01 日记/b.md",
            },
        ];

        const list = ViewKit.renderTransactionList(host, {
            title: "全部往来",
            items,
            totalCount: 2,
            pageSize: 1,
            setTitle: value => { titleText = value; },
            setMeta: value => { metaText = value; },
            directionText: amount => amount > 0 ? "收入/借入" : amount < 0 ? "支出/借出" : "零额",
        });

        assert.equal(titleText, "全部往来");
        assert.equal(host.children[0].className, "pp-table");
        assert.equal(host.children[0].children[0].children.length, 1);
        assert.ok(host.children[0].children[0].children[0].innerHTML.includes("支出/借出"));
        assert.ok(host.children[0].children[0].children[0].innerHTML.includes("样例信用卡"));
        assert.equal(metaText, "结果 2 / 2 · 已显示 1 / 2");
        list.button.onclick();
        assert.equal(host.children[0].children[0].children.length, 2);
        assert.equal(metaText, "结果 2 / 2 · 已显示 2 / 2");
    }),
    test("ViewKit debug panel reports dataset counts without local paths", () => {
        const ViewKit = loadViewKit();
        const host = makeDomElement("div");
        const dataset = {
            sourceEntries: [{}, {}],
            consumedEntries: [{}],
            visibleEntries: [{}],
            availableTags: ["项目"],
            availableLinks: [{ target: "项目A" }],
            warnings: ["C:\\Users\\private\\vault\\note.md exceeded maxPages"],
            queryMetrics: { sourcePages: 20 },
        };

        const panel = ViewKit.renderDebugPanel(host, { dataset, interaction: { tags: ["项目"] } });

        assert.equal(panel.titleEl.textContent, "Debug");
        assert.ok(panel.body.children.some(row => row.children[0]?.textContent === "source entries" && row.children[1]?.textContent === "2"));
        const warningRow = panel.body.children.find(row => row.children[0]?.textContent === "warning details");
        assert.ok(warningRow.children[1].textContent.includes("本地路径"));
        assert.ok(!warningRow.children[1].textContent.includes("Users"));
    }),
    test("Person and Project profiles expose debug panels only behind debug input", () => {
        const personSource = fs.readFileSync(path.resolve(__dirname, "../Views/PersonProfile.js"), "utf8");
        const projectSource = fs.readFileSync(path.resolve(__dirname, "../Views/ProjectProfile.js"), "utf8");

        for (const source of [personSource, projectSource]) {
            assert.ok(source.includes("const debugHost = input?.debug ? container.createEl('div') : null;"));
            assert.ok(source.includes("function renderProfileDebug"));
            assert.ok(source.includes("ViewKit.renderDebugPanel(debugHost"));
            assert.ok(source.includes("renderProfileDebug(state);"));
        }
    }),
    test("Wallet and Diary profiles expose debug panels only behind debug input", () => {
        const walletSource = fs.readFileSync(path.resolve(__dirname, "../Views/WalletProfile.js"), "utf8");
        const diarySource = fs.readFileSync(path.resolve(__dirname, "../Views/DiaryProfile.js"), "utf8");

        assert.ok(walletSource.includes("const debugHost = input?.debug ? container.createEl('div') : null;"));
        assert.ok(walletSource.includes("const txDebugHost = input?.debug ? walletCard.createEl('div') : null;"));
        assert.ok(walletSource.includes("function renderWalletEventDebug"));
        assert.ok(walletSource.includes("function renderWalletTransactionDebug"));
        assert.ok(walletSource.includes("renderWalletEventDebug(state);"));
        assert.ok(walletSource.includes("renderWalletTransactionDebug(txInteractionState);"));
        assert.ok(walletSource.includes("Wallet legacy compatibility layer"));

        assert.ok(diarySource.includes("const debugHost = input?.debug ? container.createEl('div') : null;"));
        assert.ok(diarySource.includes("function renderDiaryDebug"));
        assert.ok(diarySource.includes("ViewKit.renderDebugPanel(debugHost"));
        assert.ok(diarySource.includes("renderDiaryDebug(state);"));
        assert.ok(diarySource.includes("currentAndLinkedDiary"));
    }),
    test("Person and Project collections expose ObjectSummary debug panels only behind debug input", () => {
        const personSource = fs.readFileSync(path.resolve(__dirname, "../Views/PersonCollection.js"), "utf8");
        const projectSource = fs.readFileSync(path.resolve(__dirname, "../Views/ProjectCollection.js"), "utf8");

        for (const source of [personSource, projectSource]) {
            assert.ok(source.includes("const debugHost = input?.debug ? container.createEl('div') : null;"));
            assert.ok(source.includes("function renderCollectionDebug"));
            assert.ok(source.includes("ViewKit.renderDebugPanel(debugHost"));
            assert.ok(source.includes("[\"object pages\", summaryResult.objectPages?.length || 0]"));
            assert.ok(source.includes("[\"source paths\", summaryResult.sourcePaths?.length || 0]"));
            assert.ok(source.includes("[\"matches\", summaryResult.matches?.size || 0]"));
            assert.ok(source.includes("renderCollectionDebug(data, state);"));
        }
    }),
    test("FilterBar filters by search text across display and clean text", () => {
        const ViewKit = loadViewKit();
        let received = [];
        const fb = new ViewKit.FilterBar(null, {
            controls: [],
            persist: false,
            onFilter: items => { received = items; },
        });
        const items = [
            { id: "a", displayText: "整理旅游计划", cleanText: "出行", ctime: mockDateTime("2026-01-01"), tags: [] },
            { id: "b", displayText: "买菜", cleanText: "餐饮", ctime: mockDateTime("2026-01-02"), tags: [] },
            { id: "c", displayText: "复盘", cleanText: "旅游预算", ctime: mockDateTime("2026-01-03"), tags: [] },
        ];
        fb.bind(items);
        fb.setState({ search: "旅游" });
        assert.deepEqual(received.map(i => i.id), ["c", "a"]);
    }),
    test("FilterBar tag filtering uses AND semantics", () => {
        const ViewKit = loadViewKit();
        const fb = new ViewKit.FilterBar(null, { controls: [], persist: false });
        const items = [
            { id: "a", tags: ["健康", "记账"], ctime: mockDateTime("2026-01-01") },
            { id: "b", tags: ["健康"], ctime: mockDateTime("2026-01-02") },
            { id: "c", tags: ["记账", "餐饮"], ctime: mockDateTime("2026-01-03") },
        ];
        fb.bind(items);
        const result = fb.setState({ tags: ["健康", "记账"] });
        assert.deepEqual(result.map(i => i.id), ["a"]);
    }),
    test("FilterBar link filtering uses AND semantics", () => {
        const ViewKit = loadViewKit();
        const fb = new ViewKit.FilterBar(null, { controls: [], persist: false });
        const items = [
            { id: "a", links: ["项目A", "微信0991"], tags: [] },
            { id: "b", links: ["项目A"], tags: [] },
            { id: "c", links: ["微信0991"], tags: [] },
        ];
        fb.bind(items);
        const result = fb.setState({ links: ["项目A", "微信0991"] });
        assert.deepEqual(result.map(i => i.id), ["a"]);
    }),
    test("FilterBar can switch tag and link chips to OR semantics", () => {
        const ViewKit = loadViewKit();
        const fb = new ViewKit.FilterBar(null, { controls: [], persist: false });
        const items = [
            { id: "tag", tags: ["零食"], links: [] },
            { id: "link", tags: [], links: ["微信0991"] },
            { id: "both", tags: ["零食"], links: ["微信0991"] },
            { id: "none", tags: ["工作"], links: ["项目A"] },
        ];
        fb.bind(items);

        assert.deepEqual(fb.setState({ tags: ["零食"], links: ["微信0991"], matchMode: "and" }).map(i => i.id), ["both"]);
        assert.deepEqual(fb.setState({ matchMode: "or" }).map(i => i.id), ["tag", "link", "both"]);
    }),
    test("FilterBar date range includes the full end date", () => {
        const ViewKit = loadViewKit();
        const fb = new ViewKit.FilterBar(null, { controls: [], persist: false });
        const items = [
            { id: "before", ctime: mockDateTime("2025-12-31T23:59:00"), tags: [] },
            { id: "start", ctime: mockDateTime("2026-01-01T00:00:00"), tags: [] },
            { id: "end", ctime: mockDateTime("2026-03-31T21:30:00"), tags: [] },
            { id: "after", ctime: mockDateTime("2026-04-01T00:00:00"), tags: [] },
        ];
        fb.bind(items);
        const result = fb.setState({ startDate: "2026-01-01", endDate: "2026-03-31" });
        assert.deepEqual(result.map(i => i.id), ["end", "start"]);
    }),
    test("FilterBar sorts money by absolute value and reverses for ascending", () => {
        const ViewKit = loadViewKit();
        const fb = new ViewKit.FilterBar(null, {
            controls: [],
            persist: false,
            sortFields: ViewKit.filterSortFields(["money"]),
            initial: { sort: "money", sortAsc: false },
        });
        const items = [
            { id: "small", vec: [-10, 0, 0], ctime: mockDateTime("2026-01-01"), tags: [] },
            { id: "mid", vec: [100, 0, 0], ctime: mockDateTime("2026-01-02"), tags: [] },
            { id: "large", vec: [-250, 0, 0], ctime: mockDateTime("2026-01-03"), tags: [] },
        ];
        fb.bind(items);
        assert.deepEqual(fb.filteredItems.map(i => i.id), ["large", "mid", "small"]);
        const asc = fb.setState({ sortAsc: true });
        assert.deepEqual(asc.map(i => i.id), ["small", "mid", "large"]);
    }),
    test("FilterBar bind, apply, and clear restore the full list", () => {
        const ViewKit = loadViewKit();
        const fb = new ViewKit.FilterBar(null, {
            controls: [],
            persist: false,
            sortFields: ViewKit.filterSortFields(["date"]),
        });
        const items = [
            { id: "a", displayText: "健康 记账", tags: ["健康", "记账"], ctime: mockDateTime("2026-01-01") },
            { id: "b", displayText: "健康", tags: ["健康"], ctime: mockDateTime("2026-01-02") },
        ];
        assert.equal(fb.bind(items).length, 2);
        assert.equal(fb.setState({ search: "记账", tags: ["健康"] }).length, 1);
        assert.equal(fb.apply().length, 1);
        assert.equal(fb.clear().length, 2);
    }),
    test("FilterBar persists state through sessionStorage by storage key", () => {
        const store = new Map();
        const sessionStorage = {
            getItem(key) { return store.has(key) ? store.get(key) : null; },
            setItem(key, value) { store.set(key, value); },
        };
        const ViewKit = loadViewKit({ sessionStorage });
        const first = new ViewKit.FilterBar(null, {
            controls: [],
            storageKey: "persisted-filter",
            sortFields: ViewKit.filterSortFields(["date"]),
        });
        first.setState({ search: "旅游", tags: ["健康"], startDate: "2026-01-01", sortAsc: true }, { apply: false });

        const second = new ViewKit.FilterBar(null, {
            controls: [],
            storageKey: "persisted-filter",
            sortFields: ViewKit.filterSortFields(["date"]),
        });
        assert.equal(second.state.search, "旅游");
        assert.deepEqual(second.state.tags, ["健康"]);
        assert.equal(second.state.startDate, "2026-01-01");
        assert.equal(second.state.sortAsc, true);
    }),
    test("FilterBar renders grouped toolbar structure and bounded tags", () => {
        const ViewKit = loadViewKit();
        const container = makeDomElement("div");
        const fb = new ViewKit.FilterBar(container, {
            persist: false,
            maxVisibleTags: 2,
            availableTags: ["健康", "记账", "项目", "人物"],
            sortFields: ViewKit.filterSortFields(["date", "money"]),
        });
        fb.bind([
            { id: "a", displayText: "健康", tags: ["健康"], ctime: mockDateTime("2026-01-01") },
            { id: "b", displayText: "项目", tags: ["项目"], ctime: mockDateTime("2026-01-02") },
        ]);

        assert.equal(fb.barEl.className, "vk-fb-bar");
        assert.equal(fb.mainEl.className, "vk-fb-main");
        assert.equal(fb.statusEl.className, "vk-fb-status");
        assert.equal(fb.countEl.parentElement, fb.statusEl);
        assert.equal(fb.searchEl.parentElement, fb.mainEl);
        assert.equal(fb.tagsEl.parentElement, fb.mainEl);
        assert.equal(fb.dateGroupEl.parentElement, fb.mainEl);
        assert.equal(fb.sortGroupEl.parentElement, fb.mainEl);
        assert.equal(fb.tagButtons[2].btn.style.display, "none");
        assert.ok(!fb.tagsEl.classList.contains("is-expanded"));

        fb.moreBtn._listeners.click[0]();
        assert.ok(fb.tagsEl.classList.contains("is-expanded"));
        assert.equal(fb.tagButtons[2].btn.style.display, "");

        fb.setState({ search: "健康" });
        assert.ok(fb.clearEl.classList.contains("is-active"));
        fb.clear();
        assert.ok(!fb.clearEl.classList.contains("is-active"));
    }),
    test("filterPartsForView removes trailing self links but keeps inline self links", () => {
        const { core } = loadCore();
        const parts = [
            { type: "text", text: "Talk" },
            { type: "link", target: "Alex", label: "Alex", role: "object" },
            { type: "link", target: "02 Projects/Alex", label: "Alex Project", role: "object" },
            { type: "link", target: "Receipts/Alex", label: "Receipt", role: "source" },
            { type: "link", target: "03 People/Alex", label: "Alex", role: "object" },
        ];
        const filtered = core.Utils.filterPartsForView(parts, { path: "03 People/Alex.md", name: "Alex" });
        assert.deepEqual(filtered.map(part => part.label || part.text), ["Talk", "Alex", "Alex Project", "Receipt"]);
        assert.equal(parts.length, 5);
    }),
    test("filterPartsForView removes wallet self links regardless of position", () => {
        const { core } = loadCore();
        const walletContext = {
            path: "03 People/Wallets/TestWallet.md",
            name: "TestWallet",
            frontmatter: { tags: ["钱包"] },
        };
        const leading = [
            { type: "link", target: "03 People/Wallets/TestWallet", label: "Wallet", role: "self" },
            { type: "text", text: "Pay" },
            { type: "link", target: "ProjectX", label: "ProjectX", role: "object" },
        ];
        const inline = [
            { type: "text", text: "Pay with" },
            { type: "link", target: "03 People/Wallets/TestWallet", label: "Wallet", role: "self" },
            { type: "text", text: "for ProjectX" },
        ];

        assert.deepEqual(core.Utils.filterPartsForView(leading, walletContext).map(part => part.label || part.text), ["Pay", "ProjectX"]);
        assert.deepEqual(core.Utils.filterPartsForView(inline, walletContext).map(part => part.label || part.text), ["Pay with", "for ProjectX"]);
    }),
    test("parses ordinary event list items", () => {
        const { core } = loadCore();
        const entries = core.Query()
            .from({ scope: "01 日记" })
            .filter({ type: "event", tags: ["#健康"] })
            .execute();
        assert.equal(entries.length, 1);
        assert.equal(squish(entries[0].cleanText), "和 跑步");
        assert.equal(squish(entries[0].displayText), "和 小明 跑步");
        assert.deepEqual(entries[0].linksDetailed.map(link => ({ target: link.target, label: link.label, role: link.role })), [
            { target: "小明", label: "小明", role: "self" },
        ]);
        assert.equal(entries[0].vector.time, 2);
        assert.equal(entries[0].vector.emotion, 3);
        assert.equal(dateOnly(entries[0].meta.explicitDate), "2026-05-03");
    }),
    test("does not emit event value child rows as entries", () => {
        const { core } = loadCore();
        const valueChild = { line: 2, text: "1", tags: [], children: [], parent: 1 };
        const parent = {
            line: 1,
            text: "education devaluation civil service exam fever sleep reading @20251209",
            tags: [],
            children: [valueChild],
            parent: null,
        };
        const page = { file: { path: "01 diary/2025-12-09.md" } };
        const entries = core.UnifiedParser.parseListItems([parent, valueChild], page);
        assert.equal(entries.length, 1);
        assert.equal(entries[0].lineIndex, 1);
        assert.equal(entries[0].vector.time, 1);
        assert.equal(dateOnly(entries[0].meta.explicitDate), "2025-12-09");
        assert.ok(!entries.some(entry => entry.rawText === "1" || entry.cleanText === "1"));
    }),
    test("event value grandchildren isolate complete child atoms from parent descriptions", () => {
        const { core } = loadCore();
        const zhangValue = child(3, "10, 0");
        const zhang = child(2, "[[样例伙伴]]", [], [zhangValue]);
        const dragonValue = child(5, "5, -1");
        const dragon = child(4, "样例任务", [], [dragonValue]);
        const chatValue = child(7, "2, 1");
        const chat = child(6, "和[[小明]]聊天", [], [chatValue]);
        const parent = listItem(1, "取旅游", [], [zhang, dragon]);
        parent.children.push(chat);
        chat.parent = 1;
        const page = { file: { path: "01 日记/2026-07-10.md" } };

        const entries = core.UnifiedParser.parseListItems([parent, zhang, zhangValue, dragon, dragonValue, chat, chatValue], page);
        assert.equal(entries.length, 3);
        assert.deepEqual(entries.map(entry => squish(entry.cleanText)), ["取旅游 样例伙伴", "样例任务", "和 聊天"]);
        assert.equal(entries[2].displayText, "和 小明 聊天");
        assert.equal(entries[0].vector.time, 10);
        assert.equal(entries[0].vector.emotion, 0);
        assert.deepEqual(entries[0].meta.outlinks, ["样例伙伴"]);
        assert.equal(entries[1].vector.time, 5);
        assert.equal(entries[1].vector.emotion, -1);
        assert.deepEqual(entries[1].meta.outlinks, []);
        assert.equal(entries[2].vector.time, 2);
        assert.deepEqual(entries[2].meta.outlinks, ["小明"]);
    }),
    test("event context supplies nearest ancestor link without inheriting ancestor description", () => {
        const { core } = loadCore();
        const pureValue = child(3, "1, 0");
        const pureChild = child(2, "问意愿", [], [pureValue]);
        const pureParent = listItem(1, "[[样例项目]]", [], [pureChild]);
        const semanticValue = child(12, "2, 1");
        const semanticChild = child(11, "问意愿地方，随便。可以留总部，没留", [], [semanticValue]);
        const semanticParent = listItem(10, "分模块 [[样例项目]]", [], [semanticChild]);
        const page = { file: { path: "01 日记/2026-07-10.md" } };

        const entries = core.UnifiedParser.parseListItems([
            pureParent, pureChild, pureValue,
            semanticParent, semanticChild, semanticValue,
        ], page);
        const byLine = new Map(entries.map(entry => [entry.lineIndex, entry]));
        assert.ok(!byLine.has(1));
        assert.equal(byLine.get(2).cleanText, "问意愿");
        assert.deepEqual(byLine.get(2).meta.outlinks, ["样例项目"]);
        assert.ok(!byLine.has(10));
        assert.equal(byLine.get(11).cleanText, "问意愿地方，随便。可以留总部，没留");
        assert.equal(byLine.get(11).displayText, "问意愿地方，随便。可以留总部，没留 样例项目");
        assert.deepEqual(byLine.get(11).meta.outlinks, ["样例项目"]);
    }),
    test("parser assembles event records from minimum core elements", () => {
        const { core } = loadCore();
        const zhangValue = child(3, "10, 0");
        const zhang = child(2, "[[张远]]", [], [zhangValue]);
        const dragonValue = child(5, "5, -1");
        const dragon = child(4, "[[姚海龙|龙]]", [], [dragonValue]);
        const travelParent = listItem(1, "取旅游", [], [zhang, dragon]);

        const reviewValue = child(12, "2, 1");
        const review = child(11, "评审方案", [], [reviewValue]);
        const projectParent = listItem(10, "分项目 [[项目A]]", [], [review]);

        const page = { file: { path: "01 日记/2026-07-11.md" } };
        const entries = core.UnifiedParser.parseListItems([
            travelParent, zhang, zhangValue, dragon, dragonValue,
            projectParent, review, reviewValue,
        ], page);
        const byLine = new Map(entries.map(entry => [entry.lineIndex, entry]));

        assert.equal(byLine.get(2).displayText, "取旅游 张远");
        assert.equal(byLine.get(2).cleanText, "取旅游 张远");
        assert.deepEqual(byLine.get(2).meta.outlinks, ["张远"]);
        assert.equal(byLine.get(2).vector.time, 10);
        assert.equal(byLine.get(2).vector.emotion, 0);

        assert.equal(byLine.get(4).displayText, "取旅游 龙");
        assert.deepEqual(byLine.get(4).meta.outlinks, ["姚海龙"]);
        assert.equal(byLine.get(4).vector.time, 5);
        assert.equal(byLine.get(4).vector.emotion, -1);

        assert.ok(!byLine.has(10));
        assert.equal(byLine.get(11).cleanText, "评审方案");
        assert.equal(byLine.get(11).displayText, "评审方案 项目A");
        assert.deepEqual(byLine.get(11).meta.outlinks, ["项目A"]);
        assert.equal(byLine.get(11).vector.time, 2);
        assert.equal(byLine.get(11).vector.emotion, 1);
    }),
    test("parser stops link inheritance once a child event is complete", () => {
        const { core } = loadCore();
        const c04Value = child(3, "1, 1");
        const c04Child = child(2, "找 [[小明]] 评审", [], [c04Value]);
        const c04Parent = listItem(1, "[[项目A]]", [], [c04Child]);

        const c05Value = child(13, "1, 1");
        const c05Task = child(12, "讨论方案", [], [c05Value]);
        const c05Person = child(11, "[[小明]]", [], [c05Task]);
        const c05Project = listItem(10, "[[项目A]]", [], [c05Person]);

        const page = { file: { path: "01 日记/2026-07-12.md" } };
        const entries = core.UnifiedParser.parseListItems([
            c04Parent, c04Child, c04Value,
            c05Project, c05Person, c05Task, c05Value,
        ], page);
        const byLine = new Map(entries.map(entry => [entry.lineIndex, entry]));

        assert.equal(byLine.get(2).displayText, "找 小明 评审");
        assert.deepEqual(byLine.get(2).meta.outlinks, ["小明"]);
        assert.equal(byLine.get(12).displayText, "讨论方案 小明");
        assert.deepEqual(byLine.get(12).meta.outlinks, ["小明"]);
    }),
    test("parser uses only the first direct value child for a candidate", () => {
        const { core } = loadCore();
        const firstValue = child(2, "2, 1");
        const secondValue = child(3, "3, 0");
        const item = listItem(1, "和 [[小明]] 打球", [], [firstValue, secondValue]);
        const page = { file: { path: "01 日记/2026-07-13.md" } };

        const entries = core.UnifiedParser.parseListItems([item, firstValue, secondValue], page);
        assert.equal(entries.length, 1);
        assert.equal(entries[0].lineIndex, 1);
        assert.equal(entries[0].displayText, "和 小明 打球");
        assert.equal(entries[0].vector.time, 2);
        assert.equal(entries[0].vector.emotion, 1);
    }),
    test("parser preserves event tag union info and defaults no-value linked child events", () => {
        const { core } = loadCore();
        const reviewValue = child(3, "2, 1");
        const reviewInfo = child(4, "说明：确认周末时间");
        const review = child(2, "评审方案 #会议", ["#会议"], [reviewValue, reviewInfo]);
        const projectContainer = listItem(1, "[[项目A]] #工作 #长期", ["#工作", "#长期"], [review]);

        const ownValue = child(12, "2, 1");
        const ownInfo = child(13, "联系的是 [[小明]]");
        const ownRecord = listItem(11, "分项目 [[项目A]]", [], [ownValue, ownInfo]);

        const emptyNote = child(22, "仅备注一下");
        const emptyContainer = listItem(21, "[[项目B]]", [], [emptyNote]);
        const page = { file: { path: "01 日记/2026-07-13-extra.md" } };

        const entries = core.UnifiedParser.parseListItems([
            projectContainer, review, reviewValue, reviewInfo,
            ownRecord, ownValue, ownInfo,
            emptyContainer, emptyNote,
        ], page);
        const byLine = new Map(entries.map(entry => [entry.lineIndex, entry]));

        assert.equal(byLine.get(2).type, "event");
        assert.equal(byLine.get(2).cleanText, "评审方案");
        assert.deepEqual(byLine.get(2).meta.outlinks, ["项目A"]);
        assert.ok(byLine.get(2).meta.tags.includes("工作"));
        assert.ok(byLine.get(2).meta.tags.includes("长期"));
        assert.ok(byLine.get(2).meta.tags.includes("会议"));
        assert.equal(byLine.get(2).meta.info, "说明：确认周末时间");

        assert.equal(byLine.get(11).type, "event");
        assert.equal(byLine.get(11).cleanText, "分项目");
        assert.equal(byLine.get(11).displayText, "分项目 项目A");
        assert.deepEqual(byLine.get(11).meta.outlinks, ["项目A"]);
        assert.equal(byLine.get(11).meta.info, "联系的是 小明");

        assert.ok(!byLine.has(21));
        assert.equal(byLine.get(22).type, "event");
        assert.equal(byLine.get(22).cleanText, "仅备注一下");
        assert.equal(byLine.get(22).displayText, "仅备注一下 项目B");
        assert.deepEqual(byLine.get(22).meta.outlinks, ["项目B"]);
        assert.equal(byLine.get(22).meta.valueDefaulted, true);
    }),
    test("parser normalizes tags that touch wiki links without whitespace", () => {
        const { core } = loadCore();
        const value = child(2, "-8");
        const item = listItem(1, "买零食 #零食[[03 人物/钱包/微信0991|微信0991]] #记账", ["#零食[[微信0991]]", "#记账"], [value]);
        const page = { file: { path: "01 日记/2026-07-13-tags.md" } };

        const entries = core.UnifiedParser.parseListItems([item, value], page);

        assert.equal(entries.length, 1);
        assert.ok(entries[0].meta.tags.includes("零食"));
        assert.ok(entries[0].meta.tags.includes("记账"));
        assert.ok(!entries[0].meta.tags.some(tag => tag.includes("[[")));
        assert.ok(!entries[0].meta.tags.some(tag => tag.includes("]")));
    }),
    test("parser assembles journal records from wallet and tag containers", () => {
        const { core } = loadCore();
        const waterValue = child(3, "-5");
        const water = child(2, "买水 #零食", ["#零食"], [waterValue]);
        const mealValue = child(5, "-12");
        const meal = child(4, "买饭 #吃饭", ["#吃饭"], [mealValue]);
        const walletContainer = listItem(1, "[[03 人物/钱包/钱包|钱包]] #记账", ["#记账"], [water, meal]);

        const clothesValue = child(13, "-200");
        const clothesBill = child(14, "BILL:2026-05-05");
        const clothes = child(12, "[[03 人物/钱包/信用卡|信用卡]] 衣服 #衣物", ["#衣物"], [clothesValue, clothesBill]);
        const shopping = listItem(11, "购物 #记账 #消费", ["#记账", "#消费"], [clothes]);

        const phoneValue = child(23, "-30");
        const phoneLife = child(24, "LIFE:30");
        const phoneSource = child(25, "SOURCE:[[账单截图]]");
        const phone = listItem(22, "[[03 人物/钱包/中国银行|中国银行]] 电话费 #记账 #订阅", ["#记账", "#订阅"], [phoneValue, phoneLife, phoneSource]);

        const page = { file: { path: "01 日记/2026-07-14.md" } };
        const entries = core.UnifiedParser.parseListItems([
            walletContainer, water, waterValue, meal, mealValue,
            shopping, clothes, clothesValue, clothesBill,
            phone, phoneValue, phoneLife, phoneSource,
        ], page);
        const byLine = new Map(entries.map(entry => [entry.lineIndex, entry]));

        assert.ok(!byLine.has(1));
        assert.equal(byLine.get(2).type, "journal");
        assert.equal(byLine.get(2).cleanText, "买水");
        assert.deepEqual(byLine.get(2).meta.outlinks, ["03 人物/钱包/钱包"]);
        assert.ok(byLine.get(2).meta.tags.includes("记账"));
        assert.ok(byLine.get(2).meta.tags.includes("零食"));
        assert.equal(byLine.get(2).vector.money, -5);
        assert.equal(byLine.get(4).type, "journal");
        assert.equal(byLine.get(4).cleanText, "买饭");
        assert.deepEqual(byLine.get(4).meta.outlinks, ["03 人物/钱包/钱包"]);
        assert.ok(byLine.get(4).meta.tags.includes("吃饭"));
        assert.equal(byLine.get(4).vector.money, -12);

        assert.equal(byLine.get(12).type, "journal");
        assert.equal(byLine.get(12).cleanText, "衣服");
        assert.ok(!byLine.get(12).cleanText.includes("购物"));
        assert.deepEqual(byLine.get(12).meta.outlinks, ["03 人物/钱包/信用卡"]);
        assert.ok(byLine.get(12).meta.tags.includes("消费"));
        assert.ok(byLine.get(12).meta.tags.includes("衣物"));
        assert.equal(dateOnly(byLine.get(12).meta.explicitDate), "2026-05-05");
        assert.equal(byLine.get(12).vector.money, -200);

        assert.equal(byLine.get(22).type, "journal");
        assert.equal(byLine.get(22).cleanText, "电话费");
        assert.equal(byLine.get(22).meta.lifeDays, 30);
        assert.deepEqual(byLine.get(22).meta.sourceLinks.map(link => link.target), ["账单截图"]);
        assert.ok(!byLine.has(24));
        assert.ok(!byLine.has(25));
    }),
    test("W01 wallet-only repayment rows inherit parent journal description", () => {
        const walletPages = ["支付宝0991", "中国银行"].map(name => makePage({
            path: `03 人物/钱包/${name}.md`,
            name,
            tags: ["#钱包"],
            frontmatter: { tags: ["钱包"] },
        }));
        const { core } = loadCore({ extraPages: walletPages });
        const aliValue = child(3, "897.09");
        const ali = child(2, "[[支付宝0991]] #贷款", ["#贷款"], [aliValue]);
        const bankValue = child(5, "-897.09");
        const bank = child(4, "[[中国银行]]", [], [bankValue]);
        const repayment = listItem(1, "还款 #记账 #转账", ["#记账", "#转账"], [ali, bank]);
        const page = { file: { path: "01 日记/2026-07-17.md" } };

        const entries = core.UnifiedParser.parseListItems([repayment, ali, aliValue, bank, bankValue], page);
        assert.equal(entries.length, 2);
        assert.equal(entries[0].type, "journal");
        assert.equal(entries[0].cleanText, "还款");
        assert.equal(entries[0].displayText, "还款");
        assert.ok(entries[0].meta.outlinks.includes("支付宝0991"));
        assert.ok(entries[0].meta.tags.includes("记账"));
        assert.ok(entries[0].meta.tags.includes("转账"));
        assert.ok(entries[0].meta.tags.includes("贷款"));
        assert.equal(entries[0].vector.money, 897.09);
        assert.equal(entries[1].type, "journal");
        assert.equal(entries[1].cleanText, "还款");
        assert.equal(entries[1].displayText, "还款");
        assert.ok(entries[1].meta.outlinks.includes("中国银行"));
        assert.ok(entries[1].meta.tags.includes("记账"));
        assert.ok(entries[1].meta.tags.includes("转账"));
        assert.equal(entries[1].vector.money, -897.09);
        assert.notEqual(entries[0].displayText, "支付宝0991");
        assert.notEqual(entries[1].displayText, "中国银行");
    }),
    test("W02 wallet-only rows under one journal parent share the parent description", () => {
        const walletPages = ["中国银行", "微信0991"].map(name => makePage({
            path: `03 人物/钱包/${name}.md`,
            name,
            tags: ["#钱包"],
            frontmatter: { tags: ["钱包"] },
        }));
        const { core } = loadCore({ extraPages: walletPages });
        const bankOutValue = child(3, "-10");
        const bankOut = child(2, "[[中国银行]] #转账", ["#转账"], [bankOutValue]);
        const wechatValue = child(5, "10");
        const wechat = child(4, "[[微信0991]] #转账", ["#转账"], [wechatValue]);
        const bankFeeValue = child(7, "-3");
        const bankFee = child(6, "[[中国银行]] #杂费", ["#杂费"], [bankFeeValue]);
        const parent = listItem(1, "买胶布 #记账", ["#记账"], [bankOut, wechat, bankFee]);
        const page = { file: { path: "01 日记/2026-07-18.md" } };

        const entries = core.UnifiedParser.parseListItems([parent, bankOut, bankOutValue, wechat, wechatValue, bankFee, bankFeeValue], page);
        assert.equal(entries.length, 3);
        assert.deepEqual(entries.map(entry => entry.type), ["journal", "journal", "journal"]);
        assert.deepEqual(entries.map(entry => entry.cleanText), ["买胶布", "买胶布", "买胶布"]);
        assert.deepEqual(entries.map(entry => entry.displayText), ["买胶布", "买胶布", "买胶布"]);
        assert.deepEqual(entries.map(entry => entry.vector.money), [-10, 10, -3]);
        assert.ok(entries[0].meta.tags.includes("转账"));
        assert.ok(entries[1].meta.tags.includes("转账"));
        assert.ok(entries[2].meta.tags.includes("杂费"));
    }),
    test("W03 current journal description overrides parent description", () => {
        const { core } = loadCore();
        const clothesValue = child(3, "-200");
        const clothes = child(2, "[[样例信用卡|信用卡]] 衣服 #衣物", ["#衣物"], [clothesValue]);
        const shopping = listItem(1, "购物 #记账 #消费", ["#记账", "#消费"], [clothes]);
        const page = { file: { path: "01 日记/2026-07-19.md" } };

        const entries = core.UnifiedParser.parseListItems([shopping, clothes, clothesValue], page);
        assert.equal(entries.length, 1);
        assert.equal(entries[0].type, "journal");
        assert.equal(entries[0].cleanText, "衣服");
        assert.equal(entries[0].displayText, "衣服");
        assert.notEqual(entries[0].cleanText, "购物 衣服");
    }),
    test("W04 direct wallet journal rows do not display the wallet name", () => {
        const walletPage = makePage({
            path: "03 人物/钱包/中国银行.md",
            name: "中国银行",
            tags: ["#钱包"],
            frontmatter: { tags: ["钱包"] },
        });
        const { core } = loadCore({ extraPages: [walletPage] });
        const value = child(2, "-25");
        const lunch = listItem(1, "[[中国银行]] 午饭 #记账 #吃饭", ["#记账", "#吃饭"], [value]);
        const page = { file: { path: "01 日记/2026-07-20.md" } };

        const entries = core.UnifiedParser.parseListItems([lunch, value], page);
        assert.equal(entries.length, 1);
        assert.equal(entries[0].type, "journal");
        assert.equal(entries[0].cleanText, "午饭");
        assert.equal(entries[0].displayText, "午饭");
        assert.ok(entries[0].meta.outlinks.includes("中国银行"));
        assert.notEqual(entries[0].displayText, "中国银行 午饭");
    }),
    test("W05 event link-only child keeps object label in the event text", () => {
        const { core } = loadCore();
        const value = child(3, "10, 0");
        const person = child(2, "[[张远]]", [], [value]);
        const travel = listItem(1, "取旅游", [], [person]);
        const page = { file: { path: "01 日记/2026-07-21.md" } };

        const entries = core.UnifiedParser.parseListItems([travel, person, value], page);
        assert.equal(entries.length, 1);
        assert.equal(entries[0].type, "event");
        assert.equal(entries[0].cleanText, "取旅游 张远");
        assert.equal(entries[0].displayText, "取旅游 张远");
        assert.deepEqual(entries[0].meta.outlinks, ["张远"]);
        assert.equal(entries[0].vector.money, 0);
        assert.equal(entries[0].vector.emotion, 0);
        assert.equal(entries[0].vector.time, 10);
    }),
    test("D01 direct no-value events default to a zero event vector", () => {
        const { core } = loadCore();
        const item = listItem(1, "和 [[小明]] 复盘");
        const page = { file: { path: "01 日记/2026-07-22.md" } };

        const entries = core.UnifiedParser.parseListItems([item], page);
        assert.equal(entries.length, 1);
        assert.equal(entries[0].type, "event");
        assert.equal(entries[0].cleanText, "和 小明 复盘");
        assert.equal(entries[0].displayText, "和 小明 复盘");
        assert.deepEqual(entries[0].meta.outlinks, ["小明"]);
        assert.deepEqual([entries[0].vector.money, entries[0].vector.emotion, entries[0].vector.time], [0, 0, 0]);
        assert.equal(entries[0].meta.valueDefaulted, true);
    }),
    test("D02 pure link containers can supply links to no-value child events", () => {
        const { core } = loadCore();
        const review = child(2, "评审方案");
        const project = listItem(1, "[[项目A]]", [], [review]);
        const page = { file: { path: "01 日记/2026-07-23.md" } };

        const entries = core.UnifiedParser.parseListItems([project, review], page);
        assert.equal(entries.length, 1);
        assert.equal(entries[0].type, "event");
        assert.equal(entries[0].cleanText, "评审方案");
        assert.equal(entries[0].displayText, "评审方案 项目A");
        assert.deepEqual(entries[0].meta.outlinks, ["项目A"]);
        assert.deepEqual([entries[0].vector.money, entries[0].vector.emotion, entries[0].vector.time], [0, 0, 0]);
        assert.equal(entries[0].meta.valueDefaulted, true);
    }),
    test("D03 no-amount journals are not defaulted into journal or event entries", () => {
        const walletPage = makePage({
            path: "03 人物/钱包/中国银行.md",
            name: "中国银行",
            tags: ["#钱包"],
            frontmatter: { tags: ["钱包"] },
        });
        const { core } = loadCore({ extraPages: [walletPage] });
        const bank = child(2, "[[中国银行]] #转账", ["#转账"]);
        const parent = listItem(1, "买胶布 #记账", ["#记账"], [bank]);
        const page = { file: { path: "01 日记/2026-07-24.md" } };

        const entries = core.UnifiedParser.parseListItems([parent, bank], page);
        assert.equal(entries.length, 0);
    }),
    test("D04 explicit value events do not also emit default no-value events", () => {
        const { core } = loadCore();
        const value = child(3, "2, 1");
        const review = child(2, "评审方案", [], [value]);
        const project = listItem(1, "[[项目A]]", [], [review]);
        const page = { file: { path: "01 日记/2026-07-25.md" } };

        const entries = core.UnifiedParser.parseListItems([project, review, value], page);
        assert.equal(entries.length, 1);
        assert.equal(entries[0].type, "event");
        assert.equal(entries[0].cleanText, "评审方案");
        assert.equal(entries[0].displayText, "评审方案 项目A");
        assert.deepEqual(entries[0].meta.outlinks, ["项目A"]);
        assert.deepEqual([entries[0].vector.money, entries[0].vector.emotion, entries[0].vector.time], [0, 1, 2]);
        assert.notEqual(entries[0].meta.valueDefaulted, true);
    }),
    test("parser preserves LIFE retirement suffix metadata", () => {
        const { core } = loadCore();
        const lifetimeValue = child(2, "-365");
        const lifetimeInfo = child(3, "LIFE:365@@");
        const lifetime = listItem(1, "[[03 人物/钱包/信用卡|信用卡]] 会员 #记账 #订阅", ["#记账", "#订阅"], [lifetimeValue, lifetimeInfo]);
        const actualValue = child(12, "-30");
        const actualInfo = child(13, "LIFE:30@12");
        const actual = listItem(11, "[[03 人物/钱包/信用卡|信用卡]] 试用会员 #记账 #订阅", ["#记账", "#订阅"], [actualValue, actualInfo]);
        const retiredValue = child(22, "-100");
        const retiredInfo = child(23, "LIFE:100@2026-06-01");
        const retired = listItem(21, "[[03 人物/钱包/信用卡|信用卡]] 临时服务 #记账 #订阅", ["#记账", "#订阅"], [retiredValue, retiredInfo]);
        const page = { file: { path: "01 日记/2026-07-14-life.md" } };

        const entries = core.UnifiedParser.parseListItems([
            lifetime, lifetimeValue, lifetimeInfo,
            actual, actualValue, actualInfo,
            retired, retiredValue, retiredInfo,
        ], page);
        const byLine = new Map(entries.map(entry => [entry.lineIndex, entry]));

        assert.equal(byLine.get(1).meta.lifeDays, 365);
        assert.equal(byLine.get(1).meta.isLifetime, true);
        assert.equal(byLine.get(1).meta.actualDays, 365);
        assert.equal(byLine.get(11).meta.lifeDays, 30);
        assert.equal(byLine.get(11).meta.actualDays, 12);
        assert.equal(byLine.get(21).meta.lifeDays, 100);
        assert.equal(dateOnly(byLine.get(21).meta.retiredDate), "2026-06-01");
    }),
    test("parser does not leak a completed journal into nested ordinary events", () => {
        const { core } = loadCore();
        const transferValue = child(2, "-550");
        const siblingEventValue = child(4, "1");
        const siblingEvent = child(3, "给[[弟弟]]转，让他换成现金给姥姥", [], [siblingEventValue]);
        const transfer = listItem(1, "[[03 人物/钱包/中国银行|中国银行]] 给姥姥转 #记账 #社交", ["#记账", "#社交"], [
            transferValue,
            siblingEvent,
        ]);
        const page = { file: { path: "01 日记/2026-07-15.md" } };

        const entries = core.UnifiedParser.parseListItems([transfer, transferValue, siblingEvent, siblingEventValue], page);
        const byLine = new Map(entries.map(entry => [entry.lineIndex, entry]));

        assert.equal(entries.length, 2);
        assert.equal(byLine.get(1).type, "journal");
        assert.equal(byLine.get(1).cleanText, "给姥姥转");
        assert.equal(byLine.get(1).vector.money, -550);
        assert.equal(byLine.get(3).type, "event");
        assert.equal(byLine.get(3).displayText, "给 弟弟 转，让他换成现金给姥姥");
        assert.deepEqual(byLine.get(3).meta.outlinks, ["弟弟"]);
        assert.ok(!byLine.get(3).meta.tags.includes("记账"));
        assert.equal(byLine.get(3).vector.money, 0);
        assert.equal(byLine.get(3).vector.time, 1);
    }),
    test("parser records missing-link anomaly for value events without link context", () => {
        const { core } = loadCore();
        const value = child(3, "2, 0");
        const item = child(2, "读书", [], [value]);
        const parent = listItem(1, "周末安排", [], [item]);
        const page = { file: { path: "01 日记/2026-07-16.md" } };

        const entries = core.UnifiedParser.parseListItems([parent, item, value], page);
        assert.equal(entries.length, 1);
        assert.equal(entries[0].type, "event");
        assert.equal(entries[0].cleanText, "读书");
        assert.deepEqual(entries[0].meta.outlinks, []);
        assert.ok((entries[0].meta.anomalies || []).includes("missing-link"));
        assert.ok(core.Utils.detectEntryAnomalies(entries[0]).some(anomaly => anomaly.type === "event-missing-link"));
    }),
    test("narrative parent suppresses linked children when parent has no own links", () => {
        // 叙事型父节点（无链接）+ 含链接叶子子项
        // 规则：父先缺省（父无链接，0条），子被父抑制（不产生），总计 0 条
        const { core } = loadCore();
        const childA = child(2, "[[样例伙伴]] 买房，某地");
        const childB = child(3, "[[妈妈]] 和大娘的事");
        const parent = listItem(1, "弟弟跟我说了很多不知道的事情", [], [childA, childB]);
        const page = { file: { path: "01 日记/2026-05-07.md" } };

        const entries = core.UnifiedParser.parseListItems([parent, childA, childB], page);
        // 父无链接 → 自身缺省被过滤；子被父抑制 → 总计 0 条
        assert.equal(entries.length, 0, "叙事型父无链接时整组不产生条目");
    }),
    test("narrative parent with own link produces one default entry, children suppressed", () => {
        // 叙事型父节点（有链接）+ 叶子子项
        // 规则：父在自身产生 1 个缺省事件；子被父抑制，不独立产生
        const { core } = loadCore();
        const childA = child(2, "[[样例伙伴]] 买房，某地");
        const childB = child(3, "[[妈妈]] 和大娘的事");
        const parent = listItem(1, "[[妈妈]] 跟我说了很多不知道的事情", [], [childA, childB]);
        const page = { file: { path: "01 日记/2026-05-07.md" } };

        const entries = core.UnifiedParser.parseListItems([parent, childA, childB], page);
        // 父有链接 → 在父节点上缺省产生 1 条；子被抑制
        assert.equal(entries.length, 1, "叙事型父有链接时只产生父节点条目");
        assert.equal(entries[0].lineIndex, 1, "条目来自父节点");
        assert.ok(entries[0].meta.valueDefaulted, "父节点是缺省事件");
        assert.deepEqual(entries[0].meta.outlinks, ["妈妈"]);
        // 子项不独立产生
        const lines = entries.map(e => e.lineIndex);
        assert.ok(!lines.includes(2) && !lines.includes(3), "子项不独立产生条目");
    }),

    test("narrative parent with journal children defaults parent and suppresses detail child", () => {
        const { core } = loadCore();
        const detail = child(2, "只好兜底。。。");
        const ticketValue = child(4, "-130.48");
        const ticketInfo = child(5, "20260402@-130.48");
        const ticket = child(3, "[[京东]] 给[[弟弟]] 买车票 #记账 #社交 #贷款", ["#记账", "#社交", "#贷款"], [ticketValue, ticketInfo]);
        const loanValue = child(7, "-200");
        const loan = child(6, "[[中国银行]] 借[[弟弟]] #记账 #借还", ["#记账", "#借还"], [loanValue]);
        const parent = listItem(1, "[[弟弟]] 被辞，没钱回家。", [], [detail, ticket, loan]);
        const page = { file: { path: "01 日记/2026-03-04.md" } };

        const entries = core.UnifiedParser.parseListItems([parent, detail, ticket, ticketValue, ticketInfo, loan, loanValue], page);
        assert.deepEqual(entries.map(e => e.lineIndex), [1, 3, 6]);

        const event = entries[0];
        assert.equal(event.type, "event");
        assert.equal(event.cleanText, "弟弟 被辞，没钱回家。");
        assert.equal(event.meta.info, "只好兜底。。。");
        assert.deepEqual(event.meta.outlinks, ["弟弟"]);
        assert.deepEqual([event.vector.money, event.vector.emotion, event.vector.time], [0, 0, 0]);
        assert.equal(event.meta.valueDefaulted, true);

        assert.ok(entries.slice(1).every(entry => entry.type === "journal"));
        assert.ok(!entries.some(entry => entry.lineIndex === 2), "纯描述子项不独立产生条目");
    }),

    test("grouping parent supplies prefix only to link-only child atoms", () => {
        const { core } = loadCore();
        const valueA = child(3, "10, 0");
        const personA = child(2, "[[样例伙伴]]", [], [valueA]);
        const valueB = child(5, "5, -1");
        const taskB = child(4, "和[[小明]]聊天", [], [valueB]);
        const parent = listItem(1, "取旅游", [], [personA, taskB]);
        const page = { file: { path: "01 日记/2026-05-07.md" } };

        const entries = core.UnifiedParser.parseListItems([parent, personA, valueA, taskB, valueB], page);
        assert.equal(entries.length, 2);
        const byLine = new Map(entries.map(e => [e.lineIndex, e]));
        assert.equal(byLine.get(2).cleanText, "取旅游 样例伙伴");
        assert.equal(byLine.get(4).cleanText, "和 聊天");
        assert.equal(byLine.get(4).displayText, "和 小明 聊天");
    }),
    test("minimum atom matrix separates assembly, defaults, and isolation", () => {
        const { core } = loadCore();
        const completeValue = child(3, "2, 1");
        const completeAtom = child(2, "和[[小明]]聊天", [], [completeValue]);
        const linkOnlyValue = child(5, "3, -1");
        const linkOnlyAtom = child(4, "[[小刘]]", [], [linkOnlyValue]);
        const group = listItem(1, "去旅游", [], [completeAtom, linkOnlyAtom]);
        const fileLinkAtom = listItem(10, "[[小张]]");
        const page = { file: { path: "02 事件/022 事/旅行测试.md", name: "旅行测试" } };

        const entries = core.UnifiedParser.parseListItems([
            group, completeAtom, completeValue,
            linkOnlyAtom, linkOnlyValue,
            fileLinkAtom,
        ], page);
        const byLine = new Map(entries.map(e => [e.lineIndex, e]));

        assert.ok(!byLine.has(1), "分组行不应自己生成原子");

        assert.equal(byLine.get(2).cleanText, "和 聊天");
        assert.equal(byLine.get(2).displayText, "和 小明 聊天");
        assert.deepEqual(byLine.get(2).meta.outlinks, ["小明"]);
        assert.deepEqual([byLine.get(2).vector.time, byLine.get(2).vector.emotion], [2, 1]);

        assert.equal(byLine.get(4).cleanText, "去旅游 小刘");
        assert.equal(byLine.get(4).displayText, "去旅游 小刘");
        assert.deepEqual(byLine.get(4).meta.outlinks, ["小刘"]);
        assert.deepEqual([byLine.get(4).vector.time, byLine.get(4).vector.emotion], [3, -1]);

        assert.equal(byLine.get(10).cleanText, "旅行测试");
        assert.equal(byLine.get(10).displayText, "旅行测试");
        assert.deepEqual(byLine.get(10).meta.outlinks, ["小张"]);
        assert.deepEqual([byLine.get(10).vector.money, byLine.get(10).vector.emotion, byLine.get(10).vector.time], [0, 0, 0]);
        assert.equal(byLine.get(10).meta.valueDefaulted, true);
        assert.deepEqual(byLine.get(10).displayParts, [{
            type: "link",
            target: "02 事件/022 事/旅行测试",
            label: "旅行测试",
            role: "object",
        }]);
    }),
    test("journal atom matrix defaults descriptions but never amounts", () => {
        const { core } = loadCore();
        const noAmount = listItem(1, "[[样例信用卡]] #记账", ["#记账"]);
        const amount = child(3, "-20");
        const withAmount = listItem(2, "[[样例信用卡]] #记账", ["#记账"], [amount]);
        const page = { file: { path: "02 事件/022 事/旅行测试.md", name: "旅行测试" } };

        const entries = core.UnifiedParser.parseListItems([noAmount, withAmount, amount], page);
        assert.equal(entries.length, 1);
        assert.equal(entries[0].lineIndex, 2);
        assert.equal(entries[0].type, "journal");
        assert.equal(entries[0].cleanText, "旅行测试");
        assert.equal(entries[0].displayText, "旅行测试");
        assert.equal(entries[0].vector.money, -20);
        assert.notEqual(entries[0].meta.valueDefaulted, true);
        assert.ok(entries[0].linksDetailed.some(link => link.role === "wallet" && link.target === "样例信用卡"));
        assert.deepEqual(entries[0].displayParts, [{
            type: "link",
            target: "02 事件/022 事/旅行测试",
            label: "旅行测试",
            role: "object",
        }]);
    }),
    test("parses ordinary journal items with BILL/LIFE/MULTI metadata", () => {
        const { core } = loadCore();
        const entries = core.Query()
            .from({ scope: "01 日记" })
            .filter({ type: "journal", tags: ["#餐饮"] })
            .execute();
        assert.equal(entries.length, 1);
        assert.equal(entries[0].cleanText, "午餐");
        assert.equal(entries[0].displayText, "午餐");
        assert.deepEqual(entries[0].linksDetailed.map(link => ({ target: link.target, label: link.label, role: link.role })), [
            { target: "样例信用卡", label: "信用卡", role: "wallet" },
        ]);
        assert.equal(entries[0].vector.money, -120);
        assert.equal(entries[0].vector.emotion, -1);
        assert.equal(entries[0].meta.lifeDays, 30);
        assert.equal(entries[0].meta.multiRule, "MULTI:3@120");
        assert.equal(dateOnly(entries[0].meta.explicitDate), "2026-05-02");
    }),
    test("journal display keeps business links while wallet and SOURCE stay out of body", () => {
        const sourcePage = makePage({
            path: "01 日记/2026-05-10.md",
            day: "2026-05-10",
            lists: [
                listItem(1, "[[样例信用卡|信用卡]] 原型工具 [[项目A]] #记账 #项目 #工具", ["#记账", "#项目", "#工具"], [
                    child(2, "-120, 0"),
                    child(3, "BILL:2026-05-20; LIFE:365; SOURCE:[[样例订单001]]"),
                ]),
            ],
        });
        const { core } = loadCore({ extraPages: [sourcePage] });
        const entries = core.Query()
            .from({ paths: ["01 日记/2026-05-10.md"] })
            .filter({ type: "journal" })
            .execute();

        assert.equal(entries.length, 1);
        assert.equal(entries[0].cleanText, "原型工具");
        assert.equal(entries[0].displayText, "原型工具 项目A");
        assert.equal(entries[0].meta.info, "");
        assert.deepEqual(entries[0].linksDetailed.map(link => ({ target: link.target, label: link.label, role: link.role })), [
            { target: "样例信用卡", label: "信用卡", role: "wallet" },
            { target: "项目A", label: "项目A", role: "object" },
            { target: "样例订单001", label: "样例订单001", role: "source" },
        ]);
        assert.deepEqual(entries[0].displayParts, [
            { type: "text", text: "原型工具" },
            { type: "link", target: "项目A", label: "项目A", role: "object" },
        ]);
    }),
    test("journal link-only atoms default description to source file link", () => {
        const wallet = makePage({
            path: "03 人物/钱包/样例信用卡.md",
            name: "样例信用卡",
            tags: ["#钱包"],
            frontmatter: { tags: ["钱包"] },
            inlinks: [{ path: "02 事件/022 事/51旅行记-4852.md" }],
        });
        const event = makePage({
            path: "02 事件/022 事/51旅行记-4852.md",
            name: "51旅行记-4852",
            day: "2026-05-08",
            lists: [
                listItem(1, "[[样例信用卡|信用卡]] #记账 #消费", ["#记账", "#消费"], [
                    child(2, "-20"),
                ]),
            ],
        });
        const { core, dv } = loadCore({ extraPages: [wallet, event] });

        const entries = core.Query()
            .from({ paths: [event.file.path] })
            .filter({ type: "journal" })
            .execute();
        assert.equal(entries.length, 1);
        assert.equal(entries[0].cleanText, "51旅行记-4852");
        assert.equal(entries[0].vector.money, -20);
        assert.ok(entries[0].linksDetailed.some(link => link.role === "wallet" && link.target === "样例信用卡"));

        const item = core.Utils.entryToViewItem(entries[0], { dv });
        assert.equal(item.displayText, "51旅行记-4852");
        assert.deepEqual(item.displayParts, [{
            type: "link",
            target: "02 事件/022 事/51旅行记-4852",
            label: "51旅行记-4852",
            role: "object",
        }]);
        assert.equal(item.wallet.display, "样例信用卡");
    }),
    test("journal first non-wallet link surfaces an anomaly without dropping the wallet", () => {
        const badPage = makePage({
            path: "01 日记/2026-05-11.md",
            day: "2026-05-11",
            lists: [
                listItem(1, "原型工具 [[项目A]] [[样例信用卡|信用卡]] #记账 #工具", ["#记账", "#工具"], [
                    child(2, "-120, 0"),
                ]),
            ],
        });
        const { core } = loadCore({ extraPages: [badPage] });
        const entries = core.Query()
            .from({ paths: ["01 日记/2026-05-11.md"] })
            .filter({ type: "journal" })
            .debug(true)
            .execute();

        assert.equal(entries.length, 1);
        assert.equal(entries[0].displayText, "原型工具 项目A");
        assert.ok(entries[0].linksDetailed.some(link => link.role === "wallet" && link.target === "样例信用卡"));
        assert.ok(entries.metrics.anomalies.some(a => a.type === "journal-first-link-not-wallet"));
    }),
    test("parses nested journal items and inherits parent journal tags", () => {
        const { core } = loadCore();
        const entries = core.Query()
            .from({ scope: "01 日记" })
            .filter({ type: "journal", tags: ["#衣物"] })
            .execute();
        assert.equal(entries.length, 1);
        assert.equal(entries[0].cleanText, "衣服");
        assert.equal(entries[0].vector.money, -200);
        assert.equal(entries[0].meta.parentLine, 20);
        assert.ok(entries[0].meta.tags.includes("消费"));
    }),
    test("parses Frontmatter entries", () => {
        const { core } = loadCore();
        const entries = core.Query()
            .from({ scope: "01 日记" })
            .filter({ tags: ["#贷款"] })
            .execute();
        assert.equal(entries.length, 1);
        assert.equal(entries[0].lineIndex, -1);
        assert.equal(entries[0].type, "journal");
        assert.equal(entries[0].vector.money, -88);
        assert.equal(entries[0].meta.lifeDays, 8);
        assert.equal(entries[0].meta.multiRule, "MULTI:2@88");
    }),
    test("uses source file link as display description for body link-only event atoms", () => {
        const person = makePage({
            path: "03 人物/人/小刘.md",
            name: "小刘",
            tags: ["#人"],
            frontmatter: { tags: ["人"] },
            inlinks: [{ path: "02 事件/022 事/51旅行记-4852.md" }],
        });
        const event = makePage({
            path: "02 事件/022 事/51旅行记-4852.md",
            name: "51旅行记-4852",
            day: "2026-05-08",
            lists: [
                listItem(1, "[[小刘]]"),
            ],
        });
        const { core, dv } = loadCore({
            currentPath: "03 人物/人/小刘.md",
            extraPages: [person, event],
        });

        const entries = core.Query()
            .from({ linkedTo: true })
            .filter({ type: "event", targetTag: "#人" })
            .execute();
        const entry = entries.find(item => item.sourcePath === event.file.path);
        assert.ok(entry);
        assert.equal(entry.lineIndex, 1);
        assert.equal(entry.cleanText, "51旅行记-4852");
        assert.ok(entry.meta.outlinks.includes("小刘"));
        assert.deepEqual([entry.vector.money, entry.vector.emotion, entry.vector.time], [0, 0, 0]);
        assert.equal(entry.meta.valueDefaulted, true);

        const item = core.Utils.entryToViewItem(entry, { targetFile: dv.current().file, dv });
        assert.equal(item.displayText, "51旅行记-4852");
        assert.deepEqual(item.displayParts, [{
            type: "link",
            target: "02 事件/022 事/51旅行记-4852",
            label: "51旅行记-4852",
            role: "object",
        }]);
    }),
    test("preserves #转账 money in Query results", () => {
        const { core } = loadCore();
        const entries = core.Query()
            .from({ scope: "01 日记" })
            .filter({ type: "journal", tags: ["#转账"] })
            .execute();
        assert.equal(entries.length, 1);
        assert.equal(entries[0].vector.money, 500);
    }),
    test("filters by targetTag through linked pages", () => {
        const { core } = loadCore();
        const entries = core.Query()
            .from({ scope: "01 日记" })
            .filter({ targetTag: "#人" })
            .execute();
        const names = entries.map(entry => squish(entry.cleanText));
        assert.ok(names.includes("和 跑步"));
        assert.ok(names.includes("给 送书"));
    }),
    test("resolves linkedTo:true from current page inlinks", () => {
        const { core } = loadCore({ currentPath: "03 人物/人/小明.md" });
        const entries = core.Query()
            .from({ linkedTo: true })
            .filter({ targetTag: "#人" })
            .execute();
        assert.equal(entries.length, 4);
        assert.ok(entries.every(entry => [
            "01 日记/2026-05-01.md",
            "01 日记/2026-05-03.md",
            "02 事件/小明借款.md",
        ].includes(entry.sourcePath)));
    }),
    test("emits metrics for debug queries", () => {
        const { core } = loadCore();
        const entries = core.Query()
            .from({ scope: "01 日记" })
            .debug(true)
            .execute();
        assert.equal(entries.metrics.sourcePages, 4);
        assert.equal(entries.metrics.parsedEntries, 9);
        assert.equal(entries.metrics.filteredEntries, 9);
        assert.equal(entries.metrics.frontmatterEntries, 2);
        assert.equal(entries.metrics.listEntries, 7);
        assert.equal(entries.metrics.skippedEntries, 1);
        assert.equal(entries.metrics.cacheHits, 0);
        assert.equal(entries.metrics.cacheMisses, 4);
        assert.equal(entries.metrics.cacheHitRate, 0);
        assert.ok(Array.isArray(entries.metrics.anomalies));
        assert.ok(typeof entries.metrics.elapsedMs === "number");
    }),
    test("reuses parsed pages across repeated Query executions without changing results", () => {
        const { core } = loadCore();
        const first = core.Query()
            .from({ scope: "01 日记" })
            .filter({ type: "journal" })
            .debug(true)
            .execute();
        const second = core.Query()
            .from({ scope: "01 日记" })
            .filter({ type: "journal" })
            .debug(true)
            .execute();

        assert.equal(first.length, second.length);
        assert.deepEqual(second.map(entry => entry.cleanText), first.map(entry => entry.cleanText));
        assert.equal(first.metrics.cacheHits, 0);
        assert.equal(first.metrics.cacheMisses, 4);
        assert.equal(second.metrics.cacheHits, 4);
        assert.equal(second.metrics.cacheMisses, 0);
        assert.equal(second.metrics.cacheHitRate, 1);
    }),
    test("surfaces journal and date anomalies without filtering out entries", () => {
        const { core } = loadCore();
        const missingWalletPage = makePage({
            path: "02 事件/缺钱包账本.md",
            lists: [
                listItem(1, "未归属消费 #记账 #餐饮", ["#记账", "#餐饮"], [
                    child(2, "-12"),
                ]),
            ],
        });
        const entries = core.Query()
            .from({ pages: [missingWalletPage] })
            .filter({ type: "journal" })
            .debug(true)
            .execute();

        assert.equal(entries.length, 1);
        assert.equal(entries[0].vector.money, -12);
        assert.ok(entries.metrics.anomalies.some(a => a.type === "journal-missing-wallet"));
        assert.ok(entries.metrics.anomalies.some(a => a.type === "ctime-fallback"));
        assert.ok(entries.metrics.anomalies.every(a => typeof a.sourcePath === "string" && typeof a.lineIndex === "number"));
    }),
    test("inspectObjectQuality reports missing object properties and block ids read-only", () => {
        const { core } = loadCore();
        const objectPage = makePage({
            path: "02 事件/对象质量样例.md",
            frontmatter: {
                "值": 1,
            },
            lists: [
                listItem(1, "关联 [[小明]] 的条目", [], [
                    child(2, "1, 1"),
                ]),
            ],
        });
        const quality = core.Utils.inspectObjectQuality({ pages: [objectPage], sampleSize: 3 });
        const bucketTypes = quality.buckets.map(bucket => bucket.type);

        assert.equal(quality.totalPages, 1);
        assert.equal(quality.checkedPages, 1);
        assert.ok(bucketTypes.includes("object-missing-type"));
        assert.ok(bucketTypes.includes("object-missing-tags"));
        assert.ok(bucketTypes.includes("object-missing-created-time"));
        assert.ok(bucketTypes.includes("referenced-list-missing-block-id"));
    }),
    test("source guard warns when Query has no source and no allowGlobal:true", () => {
        const { core } = loadCore();
        const result = core.Query().filter({ type: "journal" }).execute();
        assert.ok(result.metrics);
        assert.ok(result.warnings.length > 0);
        assert.ok(result.warnings[0].includes("Source guard"));
        assert.equal(result.metrics.sourcePages, 11);
        assert.equal(result.metrics.filteredEntries, 7);
    }),
    test("allowGlobal:true makes a global query explicit", () => {
        const { core } = loadCore();
        const result = core.Query()
            .from({ allowGlobal: true })
            .filter({ type: "journal" })
            .execute();
        assert.equal(result.warnings.length, 0);
        assert.equal(result.metrics.sourcePages, 11);
        assert.equal(result.metrics.filteredEntries, 7);
    }),
    test("strict source guard returns an empty result without an explicit source", () => {
        const { core } = loadCore();
        const result = core.Query()
            .from({ strictSourceGuard: true })
            .filter({ type: "journal" })
            .execute();
        assert.equal(result.metrics.sourcePages, 0);
        assert.equal(result.metrics.parsedEntries, 0);
        assert.equal(result.metrics.filteredEntries, 0);
        assert.equal(result.length, 0);
        assert.ok(result.warnings.some(w => w.includes("Strict mode")));
    }),
    test("strict source guard still permits allowGlobal:true", () => {
        const { core } = loadCore();
        const result = core.Query()
            .from({ strictSourceGuard: true, allowGlobal: true })
            .filter({ type: "journal" })
            .execute();
        assert.equal(result.warnings.length, 0);
        assert.equal(result.metrics.sourcePages, 11);
        assert.equal(result.metrics.filteredEntries, 7);
    }),
    test("CalendarHeatmap-style query defaults to a global scan and aggregates StandardEntry values", () => {
        const { core } = loadCore();
        const data = buildHeatmapData(core);
        assert.equal(data.entries.warnings.length, 0);
        assert.equal(data.entries.metrics.sourcePages, 11);
        assert.equal(data.dailyNet.get("2026-05-02"), -120);
        assert.equal(data.dailyNet.get("2026-05-05"), -200);
        assert.equal(data.dailyNet.get("2026-05-04"), -123);
        assert.equal(data.dailyNet.get("2026-05-06"), 300);
        assert.equal(data.dailyNet.get("2026-05-07"), -80);
        assert.equal(data.dailyTime.get("2026-05-03"), 4);
        assert.equal(data.dailyTime.get("2026-05-08"), 4);
        assert.equal(data.dailyItems.get("2026-05-02").length, 1);
    }),
    test("CalendarHeatmap-style query can still be scoped to a folder", () => {
        const { core } = loadCore();
        const data = buildHeatmapData(core, { dataPath: "01 日记" });
        assert.equal(data.entries.warnings.length, 0);
        assert.equal(data.entries.metrics.sourcePages, 4);
        assert.equal(data.dailyNet.get("2026-05-02"), -120);
        assert.equal(data.dailyNet.get("2026-05-05"), -200);
        assert.equal(data.dailyNet.get("2026-05-04"), -123);
        assert.equal(data.dailyTime.get("2026-05-03"), 4);
        assert.equal(data.dailyNet.has("2026-05-06"), false);
        assert.equal(data.dailyTime.has("2026-05-08"), false);
    }),
    test("CalendarHeatmap-style global linked query scans all md files before target filtering", () => {
        const { core } = loadCore();
        const data = buildHeatmapData(core, { linkedToName: "小明" });
        assert.equal(data.entries.warnings.length, 0);
        assert.equal(data.entries.metrics.sourcePages, 11);
        assert.equal(data.dailyTime.get("2026-05-03"), 4);
        assert.equal(data.dailyNet.get("2026-05-06"), 300);
        assert.ok([...data.dailyItems.values()].flat().some(item => squish(item.text) === "和 跑步"));
    }),
    test("CalendarHeatmap-style global scan surfaces a source budget warning", () => {
        const { core } = loadCore();
        const data = buildHeatmapData(core, { maxPages: 5 });
        assert.equal(data.entries.metrics.sourcePages, 11);
        assert.ok(data.entries.warnings.some(w => w.includes("sourcePages 11 exceeds maxPages 5")));
        assert.equal(data.dailyNet.get("2026-05-02"), -120);
    }),
    test("CalendarHeatmap-style global linked query includes focus events outside the journal folder", () => {
        const { core } = loadCore();
        const data = buildHeatmapData(core, { linkedToName: "项目A" });
        assert.equal(data.entries.warnings.length, 0);
        assert.equal(data.entries.metrics.sourcePages, 11);
        assert.equal(data.dailyTime.get("2026-05-08"), 4);
        assert.equal(data.dailyTime.get("2026-04-12"), 2);
        assert.equal(data.dailyNet.get("2026-05-07"), -80);
        assert.ok([...data.dailyItems.values()].flat().some(item => squish(item.text) === "推进 原型"));
        assert.ok([...data.dailyItems.values()].flat().some(item => squish(item.text) === "项目A方案"));
    }),
    test("CalendarHeatmap-style month navigation can query only the displayed month", () => {
        const { core } = loadCore();
        const data = buildHeatmapData(core, {
            linkedToName: "项目A",
            startDate: "2026-04-01",
            endDate: "2026-04-30",
        });
        assert.equal(data.entries.warnings.length, 0);
        assert.equal(data.entries.metrics.sourcePages, 11);
        assert.equal(data.dailyTime.get("2026-04-12"), 2);
        assert.equal(data.dailyTime.has("2026-05-08"), false);
        assert.equal(data.dailyNet.has("2026-05-07"), false);
    }),
    test("CalendarHeatmap focused tag bar is scoped to consumed event atoms", () => {
        const projectPage = makePage({
            path: "04 项目/华鼎装饰.md",
            name: "华鼎装饰",
            tags: ["#项目"],
            frontmatter: { tags: ["项目"] },
        });
        const sourcePage = makePage({
            path: "01 日记/2026-02-01.md",
            name: "2026-02-01",
            ctime: "2026-02-01",
            day: "2026-02-01",
            lists: [
                listItem(1, "[[华鼎装饰]] 深度设计 #工作 #奖励", ["#工作", "#奖励"], [
                    child(2, "3"),
                ]),
                listItem(6, "[[华鼎装饰]] [[微信0991]] 奖励采购 #奖励 #券", ["#奖励", "#券"], [
                    child(7, "5"),
                ]),
                listItem(11, "[[华鼎装饰]] 材料付款 #记账 #购物", ["#记账", "#购物"], [
                    child(12, "-100"),
                ]),
                listItem(21, "[[华鼎装饰]] 纯文本记录 #杂项", ["#杂项"]),
                listItem(31, "[[其他项目]] 无关专注 #通勤", ["#通勤"], [
                    child(32, "2"),
                ]),
            ],
        });
        const { core } = loadCore({ extraPages: [projectPage, sourcePage] });

        const tags = collectHeatmapAvailableTags(core, {
            linkedToName: "华鼎装饰",
            mode: "专注",
            tags: ["奖励"],
        });
        const links = collectHeatmapAvailableLinks(core, {
            linkedToName: "华鼎装饰",
            mode: "专注",
            tags: ["奖励"],
        });
        const focused = buildHeatmapData(core, {
            linkedToName: "华鼎装饰",
            mode: "专注",
            tags: ["奖励"],
        });
        const narrowed = buildHeatmapData(core, {
            linkedToName: "华鼎装饰",
            mode: "专注",
            tags: ["奖励"],
            activeTags: ["工作"],
        });
        const linkNarrowed = buildHeatmapData(core, {
            linkedToName: "华鼎装饰",
            mode: "专注",
            tags: ["奖励"],
            activeLinks: ["微信0991"],
        });
        const orNarrowed = buildHeatmapData(core, {
            linkedToName: "华鼎装饰",
            mode: "专注",
            tags: ["奖励"],
            activeTags: ["工作"],
            activeLinks: ["微信0991"],
            matchMode: "or",
        });

        assert.deepEqual(tags, ["工作", "奖励", "券"]);
        assert.deepEqual(links, ["微信0991"]);
        assert.equal(focused.dailyTime.get("2026-02-01"), 8);
        assert.equal(focused.dailyNet.size, 0);
        assert.equal(narrowed.dailyTime.get("2026-02-01"), 3);
        assert.equal(linkNarrowed.dailyTime.get("2026-02-01"), 5);
        assert.equal(orNarrowed.dailyTime.get("2026-02-01"), 8);
        assert.equal(focused.metrics.consumedEntries, 2);
        assert.equal(focused.metrics.visibleEntries, 2);
        assert.equal(narrowed.metrics.consumedEntries, 2);
        assert.equal(narrowed.metrics.visibleEntries, 1);
        assert.equal(linkNarrowed.metrics.consumedEntries, 2);
        assert.equal(linkNarrowed.metrics.visibleEntries, 1);
        assert.ok(!tags.includes("记账"));
        assert.ok(!tags.includes("购物"));
        assert.ok(!tags.includes("通勤"));
        assert.ok(!tags.includes("杂项"));
    }),
    test("ViewQuery contract separates consumed and visible heatmap atoms", () => {
        const projectPage = makePage({
            path: "04 项目/华鼎装饰.md",
            name: "华鼎装饰",
            tags: ["#项目"],
            frontmatter: { tags: ["项目"] },
        });
        const sourcePage = makePage({
            path: "01 日记/2026-02-01.md",
            name: "2026-02-01",
            ctime: "2026-02-01",
            day: "2026-02-01",
            lists: [
                listItem(1, "[[华鼎装饰]] 深度设计 #工作 #奖励", ["#工作", "#奖励"], [
                    child(2, "3"),
                ]),
                listItem(6, "[[华鼎装饰]] [[微信0991]] 奖励采购 #奖励 #券", ["#奖励", "#券"], [
                    child(7, "5"),
                ]),
                listItem(11, "[[华鼎装饰]] 材料付款 #记账 #购物", ["#记账", "#购物"], [
                    child(12, "-100"),
                ]),
                listItem(21, "[[华鼎装饰]] 纯文本记录 #杂项", ["#杂项"]),
            ],
        });
        const { core } = loadCore({ extraPages: [projectPage, sourcePage] });
        const ViewKit = loadViewKit();
        const ViewQuery = loadViewQuery();
        const dataset = ViewQuery.collect({
            Query: core.Query,
            ViewKit,
            source: { querySources: { allowGlobal: true, maxPages: 800 } },
            rules: { explicitTarget: "华鼎装饰", tags: ["奖励"] },
            consume: {
                baseTags: ["奖励"],
                entry(entry) {
                    const isTransfer = (entry.meta?.tags || []).includes("转账");
                    return {
                        money: entry.type === "journal" && !isTransfer ? (entry.vector.money || 0) : 0,
                        time: entry.type === "event" && entry.vector.time > 0 ? entry.vector.time : 0,
                    };
                },
                include: consumption => consumption.time !== 0,
            },
            interaction: { links: ["微信0991"], matchMode: "and" },
            excludeLink: link => {
                const key = ViewKit.normalizeFilterLink(link);
                const label = ViewKit.linkLabel(link);
                return key === "华鼎装饰" || label === "华鼎装饰";
            },
        });

        assert.equal(dataset.metrics.consumedEntries, 2);
        assert.equal(dataset.metrics.visibleEntries, 1);
        assert.deepEqual(dataset.availableTags, ["工作", "奖励", "券"]);
        assert.deepEqual(dataset.availableLinks.map(link => link.label), ["微信0991"]);
        assert.equal(dataset.consumptionByEntry.get(dataset.visibleEntries[0]).time, 5);
        assert.ok(dataset.filterItems.every(item => item.tags.includes("奖励")));
    }),
    test("ObjectProfile-style dataset excludes the current object from link chips", () => {
        const targetPage = makePage({
            path: "Objects/ProjectX.md",
            name: "ProjectX",
            frontmatter: { tags: ["项目"] },
            inlinks: [{ path: "Logs/project-x.md" }],
        });
        const sourcePage = makePage({
            path: "Logs/project-x.md",
            day: "2026-05-09",
            lists: [
                listItem(1, "[[Objects/ProjectX|ProjectX]] kickoff #alpha", ["#alpha"], [
                    child(2, "2"),
                ]),
                listItem(6, "[[Objects/ProjectX|ProjectX]] with [[People/StakeholderY|StakeholderY]] #beta", ["#beta"], [
                    child(7, "3"),
                ]),
                listItem(11, "[[OtherObject]] unrelated #outside", ["#outside"], [
                    child(12, "4"),
                ]),
            ],
        });
        const { core, dv } = loadCore({
            currentPath: "Objects/ProjectX.md",
            extraPages: [targetPage, sourcePage],
        });

        const dataset = buildObjectProfileDataset(core, dv);

        assert.equal(dataset.metrics.sourceEntries, 2);
        assert.equal(dataset.metrics.consumedEntries, 2);
        assert.equal(dataset.metrics.visibleEntries, 2);
        assert.deepEqual(dataset.availableTags, ["alpha", "beta"]);
        assert.deepEqual(dataset.availableLinks.map(link => link.label), ["StakeholderY"]);
        assert.ok(!dataset.availableLinks.some(link => link.key === "Objects/ProjectX" || link.label === "ProjectX"));
    }),
    test("ObjectProfile-style link interaction only narrows visible entries", () => {
        const targetPage = makePage({
            path: "Objects/ProjectX.md",
            name: "ProjectX",
            frontmatter: { tags: ["项目"] },
            inlinks: [{ path: "Logs/project-x-interaction.md" }],
        });
        const sourcePage = makePage({
            path: "Logs/project-x-interaction.md",
            day: "2026-05-09",
            lists: [
                listItem(1, "[[Objects/ProjectX|ProjectX]] kickoff #alpha", ["#alpha"], [
                    child(2, "2"),
                ]),
                listItem(6, "[[Objects/ProjectX|ProjectX]] with [[People/StakeholderY|StakeholderY]] #beta", ["#beta"], [
                    child(7, "3"),
                ]),
            ],
        });
        const { core, dv } = loadCore({
            currentPath: "Objects/ProjectX.md",
            extraPages: [targetPage, sourcePage],
        });

        const all = buildObjectProfileDataset(core, dv);
        const narrowed = buildObjectProfileDataset(core, dv, {
            interaction: { links: ["People/StakeholderY"], matchMode: "and" },
        });

        assert.equal(all.metrics.consumedEntries, 2);
        assert.equal(narrowed.metrics.consumedEntries, 2);
        assert.equal(narrowed.metrics.visibleEntries, 1);
        assert.equal(narrowed.visibleEntries[0].lineIndex, 6);
    }),
    test("ObjectProfile-style asset preset keeps lifecycle tags from consumed entries only", () => {
        const targetPage = makePage({
            path: "Objects/Camera.md",
            name: "Camera",
            frontmatter: { tags: ["资产"] },
            inlinks: [{ path: "Logs/camera.md" }],
        });
        const sourcePage = makePage({
            path: "Logs/camera.md",
            day: "2026-05-09",
            lists: [
                listItem(1, "[[Objects/Camera|Camera]] ordinary note #noise #记账", ["#noise", "#记账"], [
                    child(2, "-10"),
                ]),
                listItem(6, "[[Objects/Camera|Camera]] warranty #lifeTag #记账 LIFE:30", ["#lifeTag", "#记账"], [
                    child(7, "-30"),
                ]),
                listItem(11, "[[Objects/Camera|Camera]] serviced #maintenance", ["#maintenance"], [
                    child(12, "1"),
                ]),
            ],
        });
        const { core, dv } = loadCore({
            currentPath: "Objects/Camera.md",
            extraPages: [targetPage, sourcePage],
        });

        const dataset = buildObjectProfileDataset(core, dv, { preset: "asset" });

        assert.equal(dataset.metrics.sourceEntries, 3);
        assert.equal(dataset.metrics.consumedEntries, 2);
        assert.equal(dataset.metrics.visibleEntries, 2);
        assert.ok(dataset.availableTags.includes("lifeTag"));
        assert.ok(dataset.availableTags.includes("maintenance"));
        assert.ok(!dataset.availableTags.includes("noise"));
    }),
    test("ViewQuery interaction applies search, date range, and sort before rendering", () => {
        const targetPage = makePage({
            path: "Objects/SearchSort.md",
            name: "SearchSort",
            inlinks: [
                { path: "Logs/search-sort-a.md" },
                { path: "Logs/search-sort-b.md" },
            ],
        });
        const firstPage = makePage({
            path: "Logs/search-sort-a.md",
            day: "2026-05-01",
            lists: [
                listItem(1, "[[Objects/SearchSort|SearchSort]] alpha design #work", ["#work"], [
                    child(2, "2"),
                ]),
            ],
        });
        const secondPage = makePage({
            path: "Logs/search-sort-b.md",
            day: "2026-05-03",
            lists: [
                listItem(1, "[[Objects/SearchSort|SearchSort]] alpha build #work", ["#work"], [
                    child(2, "5"),
                ]),
                listItem(6, "[[Objects/SearchSort|SearchSort]] beta archive #work", ["#work"], [
                    child(7, "8"),
                ]),
            ],
        });
        const { core, dv } = loadCore({
            currentPath: "Objects/SearchSort.md",
            extraPages: [targetPage, firstPage, secondPage],
        });

        const dataset = buildObjectProfileDataset(core, dv, {
            interaction: {
                search: "alpha",
                startDate: "2026-05-02",
                endDate: "2026-05-03",
                sort: "time",
                sortAsc: false,
            },
        });

        assert.equal(dataset.metrics.consumedEntries, 3);
        assert.equal(dataset.metrics.visibleEntries, 1);
        assert.equal(dataset.visibleEntries[0].lineIndex, 1);
        assert.equal(dataset.visibleEntries[0].sourcePath, "Logs/search-sort-b.md");
    }),
    test("ViewQuery propagates query warnings and metrics through the dataset", () => {
        const { core } = loadCore();
        const ViewKit = loadViewKit();
        const ViewQuery = loadViewQuery();

        const dataset = ViewQuery.collect({
            Query: core.Query,
            ViewKit,
            source: { querySources: { linkedTo: "MissingTarget" } },
            consume: {
                entry: () => ({}),
                include: () => true,
            },
        });

        assert.ok(dataset.warnings.some(item => String(item).includes("linkedTo target not found")));
        assert.equal(dataset.queryMetrics.sourcePages, 0);
        assert.equal(dataset.metrics.sourceEntries, 0);
    }),
    test("CalendarHeatmap-style scoped linked query can opt into the old folder-limited behavior", () => {
        const { core } = loadCore();
        const data = buildHeatmapData(core, { linkedToName: "小明", dataPath: "01 日记" });
        assert.equal(data.entries.warnings.length, 0);
        assert.equal(data.entries.metrics.sourcePages, 2);
        assert.equal(data.dailyTime.get("2026-05-03"), 4);
        assert.equal(data.dailyNet.size, 0);
        assert.ok([...data.dailyItems.values()].flat().some(item => squish(item.text) === "和 跑步"));
    }),
    test("PersonProfile-style event query uses linkedTo and excludes journal entries", () => {
        const { core } = loadCore({ currentPath: "03 人物/人/小明.md" });
        const entries = core.Query()
            .from({ linkedTo: true })
            .filter({ type: "event", explicitTarget: true })
            .execute();
        assert.equal(entries.metrics.sourcePages, 3);
        assert.equal(entries.length, 3);
        assert.ok(entries.every(entry => entry.type === "event"));
        assert.equal(entries.reduce((sum, entry) => sum + entry.vector.time, 0), 4);
        assert.ok(entries.some(entry => dateOnly(core.Utils.resolveEntryDate(entry)) === "2026-05-03"));
    }),
    test("PersonProfile-style event dataset uses consumed and visible entries", () => {
        const { core, dv } = loadCore({ currentPath: "03 人物/人/小明.md" });
        const dataset = buildPersonProfileDataset(core, dv, { type: "event" });

        assert.equal(dataset.queryMetrics.sourcePages, 3);
        assert.equal(dataset.metrics.sourceEntries, 3);
        assert.equal(dataset.metrics.consumedEntries, 3);
        assert.equal(dataset.metrics.visibleEntries, 3);
        assert.ok(dataset.consumedEntries.every(entry => entry.type === "event"));
        assert.ok(dataset.availableLinks.every(link => !core.Utils.linkMatchesTarget(link.key, dv.current().file)));
        assert.equal(dataset.visibleEntries.reduce((sum, entry) => sum + entry.vector.time, 0), 4);
    }),
    test("PersonProfile-style transaction query uses linkedTo and keeps wallet links", () => {
        const { core } = loadCore({ currentPath: "03 人物/人/小明.md" });
        const entries = core.Query()
            .from({ linkedTo: true })
            .filter({ type: "journal", explicitTarget: true })
            .execute();
        assert.equal(entries.metrics.sourcePages, 3);
        assert.equal(entries.length, 1);
        assert.equal(entries[0].vector.money, 300);
        assert.ok(entries[0].meta.outlinks.includes("样例信用卡"));
    }),
    test("PersonProfile-style interaction narrows visible transactions only", () => {
        const { core, dv } = loadCore({ currentPath: "03 人物/人/小明.md" });
        const all = buildPersonProfileDataset(core, dv, { type: "transaction" });
        const walletFiltered = buildPersonProfileDataset(core, dv, {
            type: "transaction",
            interaction: { links: ["样例信用卡"], matchMode: "and" },
        });
        const missingFiltered = buildPersonProfileDataset(core, dv, {
            type: "transaction",
            interaction: { links: ["不存在的钱包"], matchMode: "and" },
        });

        assert.equal(all.metrics.consumedEntries, 1);
        assert.equal(all.metrics.visibleEntries, 1);
        assert.ok(all.availableLinks.some(link => link.key === "样例信用卡"));
        assert.ok(all.availableLinks.every(link => !core.Utils.linkMatchesTarget(link.key, dv.current().file)));
        assert.equal(walletFiltered.metrics.consumedEntries, all.metrics.consumedEntries);
        assert.equal(walletFiltered.metrics.visibleEntries, 1);
        assert.equal(missingFiltered.metrics.consumedEntries, all.metrics.consumedEntries);
        assert.equal(missingFiltered.metrics.visibleEntries, 0);
    }),
    test("shared View adapter maps entries to stable view items", () => {
        const { core } = loadCore({ currentPath: "03 人物/人/小明.md" });
        const entry = core.Query()
            .from({ linkedTo: true })
            .filter({ type: "journal", explicitTarget: true })
            .execute()[0];
        const item = core.Utils.entryToViewItem(entry);
        assert.equal(item.text, "借钱给 小明");
        assert.equal(item.path, "02 事件/小明借款.md");
        assert.equal(item.link.subpath, 60);
        assert.deepEqual(item.vec, [300, 1, 0]);
        assert.equal(item.wallet.display, "信用卡");
        assert.equal(item.ctime.toFormat("yyyy-MM-dd"), "2026-05-06");
    }),
    test("entryToViewItem keeps inline self links and filters trailing self links at render time", () => {
        const { core } = loadCore();
        const inlinePage = makePage({
            path: "01 Daily/2026-05-10.md",
            day: "2026-05-10",
            lists: [
                listItem(1, "Discuss [[Target]] with [[Other]] #work", ["#work"], [
                    child(2, "0, 2"),
                ]),
            ],
        });
        const trailingPage = makePage({
            path: "01 Daily/2026-05-11.md",
            day: "2026-05-11",
            lists: [
                listItem(1, "Discuss with [[Target]]", ["#work"], [
                    child(2, "0, 2"),
                ]),
            ],
        });
        const targetFile = { path: "03 People/Target.md", name: "Target" };
        const inlineEntry = core.UnifiedParser.parseListItems(inlinePage.file.lists, inlinePage)[0];
        const inlineItem = core.Utils.entryToViewItem(inlineEntry, { targetFile });
        const trailingEntry = core.UnifiedParser.parseListItems(trailingPage.file.lists, trailingPage)[0];
        const trailingItem = core.Utils.entryToViewItem(trailingEntry, { targetFile });

        assert.equal(inlineItem.displayText, "Discuss Target with Other");
        assert.deepEqual(inlineItem.displayParts.filter(part => part.type === "link").map(part => part.target), ["Target", "Other"]);
        assert.deepEqual(inlineEntry.displayParts.filter(part => part.type === "link").map(part => part.target), ["Target", "Other"]);
        assert.equal(trailingItem.displayText, "Discuss with");
        assert.deepEqual(trailingItem.displayParts.filter(part => part.type === "link").map(part => part.target), []);
    }),
    test("visual acceptance: person page keeps inline self links and business links clickable", () => {
        const { core } = loadCore();
        const ViewKit = loadViewKit();
        const page = makePage({
            path: "01 Daily/person-visual.md",
            day: "2026-05-10",
            lists: [
                listItem(1, "Discuss [[Target]] with [[ProjectX]] via [[WalletX]] #work", ["#work"], [
                    child(2, "1, 2"),
                ]),
            ],
        });
        const entry = core.UnifiedParser.parseListItems(page.file.lists, page)[0];
        const item = core.Utils.entryToViewItem(entry, { targetFile: { path: "03 People/Target.md", name: "Target" } });
        const html = ViewKit.renderDisplayParts(item.displayParts, { fallback: item.displayText });
        assert.ok(html.includes('href="Target"'));
        assert.ok(html.includes('href="ProjectX"'));
        assert.ok(html.includes('href="WalletX"'));
        assert.equal(item.displayText, "Discuss Target with ProjectX via WalletX");
    }),
    test("visual acceptance: project page keeps inline self links and other links clickable", () => {
        const { core } = loadCore();
        const ViewKit = loadViewKit();
        const page = makePage({
            path: "01 Daily/project-visual.md",
            day: "2026-05-10",
            lists: [
                listItem(1, "Build [[ProjectX]] with [[Target]] using [[WalletX]] #work", ["#work"], [
                    child(2, "2, 1"),
                ]),
            ],
        });
        const entry = core.UnifiedParser.parseListItems(page.file.lists, page)[0];
        const item = core.Utils.entryToViewItem(entry, { targetFile: { path: "04 Projects/ProjectX.md", name: "ProjectX" } });
        const html = ViewKit.renderDisplayParts(item.displayParts, { fallback: item.displayText });
        assert.ok(html.includes('href="ProjectX"'));
        assert.ok(html.includes('href="Target"'));
        assert.ok(html.includes('href="WalletX"'));
        assert.equal(item.displayText, "Build ProjectX with Target using WalletX");
    }),
    test("visual acceptance: diary page keeps ordinary event links visible", () => {
        const { core } = loadCore();
        const ViewKit = loadViewKit();
        const page = makePage({
            path: "01 Daily/diary-visual.md",
            day: "2026-05-10",
            lists: [
                listItem(1, "Review [[ProjectX]] plan #work", ["#work"], [
                    child(2, "1, 1"),
                ]),
            ],
        });
        const entry = core.UnifiedParser.parseListItems(page.file.lists, page)[0];
        const item = core.Utils.entryToViewItem(entry, { includeWallet: false });
        const html = ViewKit.renderDisplayParts(item.displayParts, { fallback: item.displayText });
        assert.equal(item.displayText, "Review ProjectX plan");
        assert.ok(html.includes('href="ProjectX"'));
        assert.ok(!html.includes("Review plan"));
    }),
    test("visual acceptance: debug fields expose raw, clean, outlinks, linksDetailed and displayParts", () => {
        const { core } = loadCore();
        const page = makePage({
            path: "01 Daily/debug-visual.md",
            day: "2026-05-10",
            lists: [
                listItem(1, "Debug [[ProjectX]] SOURCE:[[ReceiptX]] #work", ["#work"], [
                    child(2, "1, 1"),
                ]),
            ],
        });
        const entry = core.UnifiedParser.parseListItems(page.file.lists, page)[0];
        assert.equal(entry.rawText, "Debug [[ProjectX]] SOURCE:[[ReceiptX]] #work");
        assert.equal(entry.cleanText, "Debug");
        assert.deepEqual(entry.meta.outlinks, ["ProjectX", "ReceiptX"]);
        assert.deepEqual(entry.linksDetailed.map(link => ({ target: link.target, role: link.role })), [
            { target: "ProjectX", role: "object" },
            { target: "ReceiptX", role: "source" },
        ]);
        assert.deepEqual(entry.displayParts, [
            { type: "text", text: "Debug" },
            { type: "link", target: "ProjectX", label: "ProjectX", role: "object" },
        ]);
    }),
    test("C2 extracts existing block ids for source tracing without changing text", () => {
        const { core } = loadCore();
        const pageWithBlock = makePage({
            path: "01 日记/2026-05-09.md",
            day: "2026-05-09",
            lists: [
                listItem(1, "给 [[小明]] 送书 ^abc123", [], [
                    child(2, "1, 2"),
                ]),
            ],
        });
        const entry = core.UnifiedParser.parseListItems(pageWithBlock.file.lists, pageWithBlock)[0];
        assert.equal(entry.meta.blockId, "abc123");
        assert.equal(entry.cleanText, "给 送书");

        const item = core.Utils.entryToViewItem(entry);
        assert.equal(item.link.blockId, "abc123");
        assert.equal(item.link.subpath, 1);
    }),
    test("ProjectProfile-style event and transaction queries use the same linkedTo pipeline", () => {
        const { core } = loadCore({ currentPath: "02 项目/项目A.md" });
        const eventEntries = core.Query()
            .from({ linkedTo: true })
            .filter({ type: "event", explicitTarget: true })
            .execute();
        const journalEntries = core.Query()
            .from({ linkedTo: true })
            .filter({ type: "journal", explicitTarget: true })
            .execute();

        assert.equal(eventEntries.metrics.sourcePages, 2);
        assert.equal(eventEntries.length, 2);
        assert.equal(eventEntries.reduce((sum, entry) => sum + entry.vector.time, 0), 6);
        assert.equal(dateOnly(core.Utils.resolveEntryDate(eventEntries[0])), "2026-05-08");
        assert.equal(journalEntries.length, 1);
        assert.equal(journalEntries[0].vector.money, -80);
    }),
    test("ProjectProfile-style event dataset keeps project metrics in visible entries", () => {
        const { core, dv } = loadCore({ currentPath: "02 项目/项目A.md" });
        const dataset = buildProjectProfileDataset(core, dv, { type: "event" });
        const dated = buildProjectProfileDataset(core, dv, {
            type: "event",
            interaction: { startDate: "2026-05-08", endDate: "2026-05-08", sort: "date" },
        });

        assert.equal(dataset.queryMetrics.sourcePages, 2);
        assert.equal(dataset.metrics.sourceEntries, 2);
        assert.equal(dataset.metrics.consumedEntries, 2);
        assert.equal(dataset.metrics.visibleEntries, 2);
        assert.equal(dataset.visibleEntries.reduce((sum, entry) => sum + entry.vector.time, 0), 6);
        assert.ok(dataset.availableLinks.every(link => !core.Utils.linkMatchesTarget(link.key, dv.current().file)));
        assert.equal(dated.metrics.consumedEntries, dataset.metrics.consumedEntries);
        assert.equal(dated.metrics.visibleEntries, 1);
    }),
    test("ProjectProfile-style transaction dataset filters links without changing consumed entries", () => {
        const { core, dv } = loadCore({ currentPath: "02 项目/项目A.md" });
        const all = buildProjectProfileDataset(core, dv, { type: "transaction" });
        const walletFiltered = buildProjectProfileDataset(core, dv, {
            type: "transaction",
            interaction: { links: ["样例信用卡"], matchMode: "and" },
        });
        const missingFiltered = buildProjectProfileDataset(core, dv, {
            type: "transaction",
            interaction: { links: ["不存在的钱包"], matchMode: "and" },
        });

        assert.equal(all.metrics.consumedEntries, 1);
        assert.equal(all.metrics.visibleEntries, 1);
        assert.ok(all.availableLinks.some(link => link.key === "样例信用卡"));
        assert.ok(all.availableLinks.every(link => !core.Utils.linkMatchesTarget(link.key, dv.current().file)));
        assert.equal(walletFiltered.metrics.consumedEntries, all.metrics.consumedEntries);
        assert.equal(walletFiltered.metrics.visibleEntries, 1);
        assert.equal(missingFiltered.metrics.consumedEntries, all.metrics.consumedEntries);
        assert.equal(missingFiltered.metrics.visibleEntries, 0);
    }),
    test("WalletProfile-style event query uses linkedTo and excludes wallet transactions", () => {
        const { core } = loadCore({ currentPath: "03 人物/钱包/样例信用卡.md" });
        const entries = core.Query()
            .from({ linkedTo: true })
            .filter({ type: "event", explicitTarget: true })
            .execute();
        assert.equal(entries.metrics.sourcePages, 2);
        assert.equal(entries.length, 1);
        assert.equal(entries[0].cleanText, "检查 权益");
        assert.equal(entries[0].vector.emotion, 1);
        assert.ok(entries.every(entry => entry.type === "event"));
    }),
    test("WalletProfile-style event dataset uses ViewQuery without touching Wallet transactions", () => {
        const { core, dv } = loadCore({ currentPath: "03 人物/钱包/样例信用卡.md" });
        const dataset = buildWalletEventProfileDataset(core, dv);
        const tagged = buildWalletEventProfileDataset(core, dv, {
            interaction: { tags: ["财务"], matchMode: "and" },
        });
        const missing = buildWalletEventProfileDataset(core, dv, {
            interaction: { tags: ["不存在标签"], matchMode: "and" },
        });

        assert.equal(dataset.queryMetrics.sourcePages, 2);
        assert.equal(dataset.metrics.sourceEntries, 1);
        assert.equal(dataset.metrics.consumedEntries, 1);
        assert.equal(dataset.metrics.visibleEntries, 1);
        assert.equal(dataset.visibleEntries[0].cleanText, "检查 权益");
        assert.ok(dataset.availableLinks.every(link => !core.Utils.linkMatchesTarget(link.key, dv.current().file)));
        assert.equal(tagged.metrics.consumedEntries, dataset.metrics.consumedEntries);
        assert.equal(tagged.metrics.visibleEntries, 1);
        assert.equal(missing.metrics.consumedEntries, dataset.metrics.consumedEntries);
        assert.equal(missing.metrics.visibleEntries, 0);

        core.Wallet.resetParseCache();
        assert.equal(new core.Wallet({ path: "03 人物/钱包/样例信用卡.md" }).transactions.length, 5);
    }),
    test("DiaryProfile-style source includes current and linked diary pages", () => {
        const { core } = loadCore({ currentPath: "01 日记/2026-05-03.md" });
        const entries = core.Query()
            .from({ currentAndLinkedDiary: true })
            .execute();
        assert.equal(entries.metrics.sourcePages, 2);
        assert.ok(entries.some(entry => entry.sourcePath === "01 日记/2026-05-03.md" && entry.cleanText === "给 送书"));
        assert.ok(entries.some(entry => entry.sourcePath === "02 事件/小明借款.md" && entry.type === "journal" && entry.vector.money === 300));
    }),
    test("DiaryProfile-style dataset preserves currentAndLinkedDiary source", () => {
        const { core } = loadCore({ currentPath: "01 日记/2026-05-03.md" });
        const dataset = buildDiaryProfileDataset(core);
        const searched = buildDiaryProfileDataset(core, {
            interaction: { search: "送书", sort: "time" },
        });

        assert.equal(dataset.queryMetrics.sourcePages, 2);
        assert.ok(dataset.consumedEntries.some(entry => entry.sourcePath === "01 日记/2026-05-03.md"));
        assert.ok(dataset.consumedEntries.some(entry => entry.sourcePath === "02 事件/小明借款.md"));
        assert.ok(dataset.visibleEntries.some(entry => entry.type === "journal" && entry.vector.money === 300));
        assert.equal(searched.metrics.consumedEntries, dataset.metrics.consumedEntries);
        assert.equal(searched.metrics.visibleEntries, 1);
        assert.equal(searched.visibleEntries[0].cleanText, "给 送书");
    }),
    test("Wallet keeps the public API while sharing parsed list cache", () => {
        const { core, dv } = loadCore();
        core.Wallet.resetParseCache();
        const wallet = new core.Wallet({ path: "03 人物/钱包/样例信用卡.md" });
        assert.equal(wallet.name, "样例信用卡");
        assert.equal(wallet.transactions.length, 5);
        assert.equal(wallet.positiveBalance, 145);
        assert.equal(wallet.debt, -88);
        assert.equal(wallet.netWorth, 10057);
        assert.equal(wallet.transactions.filter(t => t.serviceDays > 0).length, 3);
        assert.equal(wallet.transactions.find(t => t.text === "余额调整")?.value, 500);
        assert.equal(wallet.transactions.find(t => t.text === "2026-05-04")?.repaymentRecords.length, 2);
        assert.deepEqual(core.Wallet.getParseMetrics(), { hits: 0, misses: 2, unifiedHits: 0, cachedPages: 2 });
        new core.Wallet({ path: "03 人物/钱包/样例信用卡.md" });
        assert.deepEqual(core.Wallet.getParseMetrics(), { hits: 2, misses: 2, unifiedHits: 2, cachedPages: 2 });
        dv.page("01 日记/2026-05-02.md").file.mtime = mockDateTime("2026-05-02 12:00");
        new core.Wallet({ path: "03 人物/钱包/样例信用卡.md" });
        assert.deepEqual(core.Wallet.getParseMetrics(), { hits: 3, misses: 3, unifiedHits: 3, cachedPages: 2 });
    }),
    test("A1 Wallet reuses UnifiedParser document cache before falling back to legacy parsing", () => {
        const { core } = loadCore();
        core.Wallet.resetParseCache();
        core.Query()
            .from({ paths: ["01 日记/2026-05-02.md"] })
            .debug(true)
            .execute();

        const wallet = new core.Wallet({ path: "03 人物/钱包/样例信用卡.md" });
        assert.equal(wallet.transactions.length, 5);
        assert.equal(wallet.positiveBalance, 145);
        assert.equal(wallet.debt, -88);
        assert.deepEqual(core.Wallet.getParseMetrics(), { hits: 1, misses: 1, unifiedHits: 1, cachedPages: 2 });
    }),
    test("Wallet cache miss keeps UnifiedParser display metadata", () => {
        const walletPage = makePage({
            path: "03 人物/钱包/TestWallet.md",
            name: "TestWallet",
            tags: ["#钱包"],
            frontmatter: { tags: ["钱包"] },
            inlinks: [{ path: "01 Daily/wallet-source.md" }],
        });
        walletPage.file.inlinks.values = walletPage.file.inlinks;
        const sourcePage = makePage({
            path: "01 Daily/wallet-source.md",
            day: "2026-05-10",
            lists: [
                listItem(1, "[[03 人物/钱包/TestWallet|Wallet]] Pay [[ProjectX]] #记账 SOURCE:[[Receipts/R1]]", ["#记账"], [
                    child(2, "-42, 0"),
                    child(3, "BILL:2026-05-10"),
                ]),
            ],
        });
        const { core } = loadCore({ extraPages: [walletPage, sourcePage] });
        core.Wallet.resetParseCache();
        const wallet = new core.Wallet({ path: "03 人物/钱包/TestWallet.md" });
        assert.equal(wallet.transactions.length, 1);
        const tx = wallet.transactions[0];
        assert.equal(tx.value, -42);
        assert.ok(tx.displayParts.some(part => part.type === "link" && part.target === "ProjectX"));
        assert.ok(!tx.displayParts.some(part => part.type === "link" && String(part.target).includes("TestWallet")));
        assert.deepEqual(tx.sourceLinks.map(link => link.target), ["Receipts/R1"]);
        assert.deepEqual(core.Wallet.getParseMetrics(), { hits: 0, misses: 1, unifiedHits: 0, cachedPages: 1 });
    }),
    test("visual acceptance: wallet page hides current wallet and keeps business/source links", () => {
        const walletPage = makePage({
            path: "03 People/Wallets/TestWallet.md",
            name: "TestWallet",
            tags: ["#钱包"],
            frontmatter: { tags: ["钱包"] },
            inlinks: [{ path: "01 Daily/wallet-visual.md" }],
        });
        walletPage.file.inlinks.values = walletPage.file.inlinks;
        const sourcePage = makePage({
            path: "01 Daily/wallet-visual.md",
            day: "2026-05-10",
            lists: [
                listItem(1, "[[03 People/Wallets/TestWallet|Wallet]] Pay [[ProjectX]] #记账 SOURCE:[[Receipts/R1]]", ["#记账"], [
                    child(2, "-42, 0"),
                    child(3, "BILL:2026-05-10"),
                ]),
            ],
        });
        const { core } = loadCore({ extraPages: [walletPage, sourcePage] });
        const ViewKit = loadViewKit();
        core.Wallet.resetParseCache();
        const wallet = new core.Wallet({ path: "03 People/Wallets/TestWallet.md" });
        const tx = wallet.transactions[0];
        const html = ViewKit.renderDisplayParts(tx.displayParts, { fallback: tx.displayText || tx.text });
        assert.ok(!html.includes("TestWallet"));
        assert.ok(html.includes('href="ProjectX"'));
        assert.deepEqual(tx.sourceLinks.map(link => link.target), ["Receipts/R1"]);
    }),
    test("collectSupertagPages reads object tags from frontmatter only", () => {
        const inlineProject = makePage({
            path: "Docs/InlineProjectNote.md",
            name: "InlineProjectNote",
            tags: ["#项目"],
            frontmatter: {},
        });
        const scopedInlineProject = makePage({
            path: "Projects/ScopedInlineOnly.md",
            name: "ScopedInlineOnly",
            tags: ["#项目"],
            frontmatter: {},
        });
        const frontmatterProject = makePage({
            path: "Projects/FrontmatterOnlyProject.md",
            name: "FrontmatterOnlyProject",
            tags: [],
            frontmatter: { tags: ["项目"] },
        });
        const legacyPerson = makePage({
            path: "People/LegacyPerson.md",
            name: "LegacyPerson",
            tags: [],
            frontmatter: { tags: ["人"] },
        });
        const inlineWallet = makePage({
            path: "Wallets/InlineWalletNote.md",
            name: "InlineWalletNote",
            tags: ["#钱包"],
            frontmatter: {},
        });
        const { core, dv } = loadCore({
            extraPages: [inlineProject, scopedInlineProject, frontmatterProject, legacyPerson, inlineWallet],
        });

        const projectNames = core.Utils.collectSupertagPages({ tag: "#项目", dv }).map(page => page.file.name);
        assert.ok(projectNames.includes("FrontmatterOnlyProject"));
        assert.ok(!projectNames.includes("InlineProjectNote"));
        assert.ok(!projectNames.includes("ScopedInlineOnly"));
        assert.deepEqual(
            core.Utils.collectSupertagPages({ tag: "项目", scope: "Projects", dv }).map(page => page.file.name),
            ["FrontmatterOnlyProject"],
        );

        const personNames = core.Utils.collectSupertagPages({ tag: ["人物", "人"], dv }).map(page => page.file.name);
        assert.ok(personNames.includes("LegacyPerson"));
        assert.ok(core.Utils.hasObjectSupertag(frontmatterProject, ["#项目"]));
        assert.ok(core.Utils.hasFrontmatterTag(frontmatterProject, "项目"));
        assert.equal(core.Utils.resolveObjectType(inlineProject), "笔记");
        assert.equal(core.Utils.isStructuredObjectPage(inlineProject), false);
        assert.ok(!core.Utils.collectWalletPages({ dv }).some(page => page.file.name === "InlineWalletNote"));
    }),
    test("path schema decoupling keeps legacy paths from becoming object identity", () => {
        const movedWallet = makePage({
            path: "Archive/Accounts/MovedWallet.md",
            name: "MovedWallet",
            tags: [],
            frontmatter: { tags: ["钱包"], "类型": "信用卡" },
        });
        const walletDoc = makePage({
            path: "03 人物/钱包/钱包脚本使用说明.md",
            name: "钱包脚本使用说明",
            tags: [],
            frontmatter: {},
        });
        const movedDiary = makePage({
            path: "Archive/2026/2026-05-09.md",
            name: "2026-05-09",
            tags: [],
            frontmatter: { tags: ["日记"] },
        });
        const globalService = makePage({
            path: "Resources/Subscriptions/会员.md",
            name: "会员",
            frontmatter: {
                "服役天数": 365,
                "值": -365,
                "创建时间": "2026-01-01",
                "标签": ["订阅"],
            },
        });
        const { core, dv } = loadCore({
            extraPages: [movedWallet, walletDoc, movedDiary, globalService],
        });

        assert.ok(core.Utils.collectWalletPages({ dv }).some(page => page.file.path === "Archive/Accounts/MovedWallet.md"));
        assert.ok(!core.Utils.collectWalletPages({ dv }).some(page => page.file.path === "03 人物/钱包/钱包脚本使用说明.md"));
        assert.equal(core.Utils.isWalletLinkTarget("Archive/Accounts/MovedWallet", { dv }), true);
        assert.equal(core.Utils.isWalletLinkTarget("03 人物/钱包/钱包脚本使用说明", { dv }), false);
        assert.equal(core.Utils.resolveObjectType(movedDiary), "日记");
        assert.equal(core.Utils.isStructuredObjectPage(walletDoc), false);

        const livingCostItems = core.Utils.collectLivingCostItems({ dv, includeWallets: false, asOfDate: "2026-05-09" });
        assert.ok(livingCostItems.some(item => item.path === "Resources/Subscriptions/会员.md"));
    }),
    test("path config helpers keep runtime defaults and legacy fallback separate", () => {
        const { core } = loadCore();
        const { CONFIG, Utils } = core;
        const originalDefaultWallets = CONFIG.defaultDestinations.wallets;

        try {
            assert.equal(Utils.getConfiguredPath("runtime", "scripts"), CONFIG.runtimePaths.scripts);
            assert.equal(Utils.getConfiguredPath("defaultDestination", "wallets"), CONFIG.defaultDestinations.wallets);
            assert.equal(Utils.getConfiguredPath("legacy", "wallets"), CONFIG.legacyPaths.wallets);
            assert.equal(Utils.pathWithinConfigured("runtime", "scripts", "11 scripts/Core/ViewKit.js"), true);

            CONFIG.defaultDestinations.wallets = "New Defaults/Wallets";
            const defaultOnlyPage = makePage({
                path: "New Defaults/Wallets/UntypedWallet.md",
                name: "UntypedWallet",
                frontmatter: {},
            });
            assert.equal(Utils.pathWithinConfigured("defaultDestination", "wallets", defaultOnlyPage.file.path), true);
            assert.equal(Utils.pathWithinConfigured("legacy", "wallets", defaultOnlyPage.file.path), false);
            assert.equal(Utils.isStructuredObjectPage(defaultOnlyPage), false);

            const legacyPath = `${CONFIG.legacyPaths.wallets}/LegacyWallet.md`;
            assert.equal(Utils.pathWithinConfigured("legacy", "wallets", legacyPath), true);
            assert.ok(Utils.describePathRoles(legacyPath).some(role => role.role === "legacy" && role.key === "wallets"));
        } finally {
            CONFIG.defaultDestinations.wallets = originalDefaultWallets;
        }
    }),
    test("collectObjectAtoms treats the current page as a generic linked atom anchor", () => {
        const { core, dv } = loadCore({ currentPath: "03 人物/人/小明.md" });
        const data = core.Utils.collectObjectAtoms(null, { dv });

        assert.equal(data.targetName, "小明");
        assert.ok(data.items.length >= 3);
        assert.ok(data.events.some(item => item.path === "01 日记/2026-05-01.md"));
        assert.ok(data.journals.some(item => item.path === "02 事件/小明借款.md"));
        assert.ok(data.metrics.totalTime > 0);
        assert.ok(data.relations.some(row => row.target.includes("样例信用卡")));
        assert.ok(Array.isArray(data.sources));
        assert.ok(Array.isArray(data.warnings));
    }),
    test("wallet summary helpers normalize field shapes without dropping wallets", () => {
        const noisePage = makePage({
            path: "03 人物/钱包/钱包脚本使用说明.md",
            name: "钱包脚本使用说明",
            tags: ["#钱包"],
            frontmatter: {},
        });
        const { core } = loadCore({ extraPages: [...makeWalletFieldFixtures(), noisePage] });
        core.Wallet.resetParseCache();
        const walletPages = core.Utils.collectWalletPages();
        const walletPageNames = walletPages.map(page => page.file.name);
        assert.ok(walletPageNames.includes("样例信用卡"));
        assert.ok(!walletPageNames.includes("钱包脚本使用说明"));

        const wallets = core.Utils.collectWallets();
        const bills = core.Utils.collectWalletBills(wallets);
        const summaries = core.Utils.collectWalletSummaries(wallets, { bills, today: "2026-05-04" });
        const byName = new Map(summaries.map(summary => [summary.name, summary]));

        assert.equal(summaries.length, wallets.length);
        assert.equal(byName.get("样例信用卡").tags.join(","), "信用卡");
        assert.equal(byName.get("字符串类型钱包").tags.join(","), "储蓄卡");
        assert.equal(byName.get("数组类型钱包").tags.join(","), "储蓄卡,备用");
        assert.deepEqual(byName.get("缺类型钱包").tags, []);
        assert.equal(byName.get("字符串类型钱包").creditLimit, 2500);
        assert.equal(byName.get("缺类型钱包").transactionCount, 0);
        assert.equal(byName.get("缺类型钱包").billCount, 0);
        assert.ok(summaries.every(summary => Array.isArray(summary.errors)));
    }),
    test("wallet summary helpers preserve future bills and balances", () => {
        const { core } = loadCore();
        core.Wallet.resetParseCache();
        const wallet = new core.Wallet({ path: "03 人物/钱包/样例信用卡.md" });
        const bills = core.Utils.collectWalletBills([wallet]);
        const summary = wallet.toSummary("2026-05-04", bills);

        assert.equal(bills.length, 2);
        assert.ok(bills.every(bill => bill.wallet === "样例信用卡" && Array.isArray(bill.tags)));
        assert.equal(summary.positiveBalance, 145);
        assert.equal(summary.debt, -88);
        assert.equal(summary.creditLimit, 10000);
        assert.equal(summary.netWorth, 57);
        assert.equal(summary.availableCredit, 10057);
        assert.equal(summary.futureInflow, 88);
        assert.equal(summary.futureOutflow, 0);
    }),
    test("WalletCollection-style summaries use wallet frontmatter supertag only", () => {
        const frontmatterWallet = makePage({
            path: "Wallets/FrontmatterWallet.md",
            name: "FrontmatterWallet",
            tags: [],
            frontmatter: { tags: ["钱包"], "类型": "储蓄卡", "余额": 123 },
        });
        const inlineWalletNote = makePage({
            path: "Wallets/InlineWalletNote.md",
            name: "InlineWalletNote",
            tags: ["#钱包"],
            frontmatter: {},
        });
        frontmatterWallet.file.inlinks.values = frontmatterWallet.file.inlinks;
        inlineWalletNote.file.inlinks.values = inlineWalletNote.file.inlinks;
        const { core, dv } = loadCore({ extraPages: [frontmatterWallet, inlineWalletNote] });

        const walletPages = core.Utils.collectWalletPages({ dv });
        assert.ok(walletPages.some(page => page.file.name === "FrontmatterWallet"));
        assert.ok(!walletPages.some(page => page.file.name === "InlineWalletNote"));

        const wallets = core.Utils.collectWallets({ dv });
        const summaries = core.Utils.collectWalletSummaries(wallets, { today: "2026-05-04" });
        assert.ok(summaries.some(summary => summary.name === "FrontmatterWallet"));
        assert.ok(!summaries.some(summary => summary.name === "InlineWalletNote"));
    }),
    test("analysis aggregation can explicitly exclude #转账 while Wallet keeps it", () => {
        const { core } = loadCore();
        const transfer = core.Query()
            .from({ scope: "01 日记" })
            .filter({ type: "journal", tags: ["#转账"] })
            .execute()[0];
        const analysisEntries = core.Query()
            .from({ scope: "01 日记" })
            .filter({ type: "journal", excludeTags: ["#转账"] })
            .execute();

        assert.equal(transfer.vector.money, 500);
        assert.ok(!analysisEntries.some(entry => entry.meta.tags.includes("转账")));
        assert.equal(analysisEntries.reduce((sum, entry) => sum + entry.vector.money, 0), -443);
    }),
    test("collectLivingCostItems merges file and Wallet LIFE sources with billing dates and path dedupe", () => {
        const { core, dv } = loadCore();
        core.Wallet.resetParseCache();
        const filePage = makePage({
            path: "02 事件/年度会员.md",
            name: "年度会员",
            tags: ["#记账"],
            frontmatter: {
                "值": -365,
                "服役天数": 365,
                "创建时间": "2026-05-01",
                "标签": ["订阅"],
            },
        });

        const items = core.Utils.collectLivingCostItems({
            dv,
            pages: [filePage],
            walletLinks: [{ path: "03 人物/钱包/样例信用卡.md" }],
            asOfDate: "2026-05-05",
        });
        const fileItem = items.find(item => item.source === "file" && item.path === "02 事件/年度会员.md");
        const lunch = items.find(item => item.source === "inline" && item.name === "午餐");

        assert.ok(fileItem);
        assert.equal(fileItem.cost, 365);
        assert.equal(fileItem.days, 365);
        assert.equal(fileItem.isActive, true);
        assert.ok(lunch);
        assert.equal(lunch.days, 30);
        assert.equal(dateOnly(lunch.start), "2026-05-02");
        assert.equal(lunch.billingDate, "2026-05-02");

        const duplicateFile = makePage({
            path: "01 日记/2026-05-02.md",
            name: "路径优先项",
            tags: ["#记账"],
            frontmatter: {
                "值": -999,
                "服役天数": 99,
                "创建时间": "2026-05-01",
                "标签": ["餐饮"],
            },
        });
        const deduped = core.Utils.collectLivingCostItems({
            dv,
            pages: [duplicateFile],
            walletLinks: [{ path: "03 人物/钱包/样例信用卡.md" }],
            asOfDate: "2026-05-05",
        });
        assert.ok(deduped.some(item => item.source === "file" && item.path === "01 日记/2026-05-02.md"));
        assert.ok(!deduped.some(item => item.source === "inline" && item.path === "01 日记/2026-05-02.md"));
    }),
    test("collectLivingCostItems compares planned retirement date with asOfDate", () => {
        const inlinks = [{ path: "01 日记/2026-05-06-life.md" }];
        inlinks.values = inlinks;
        const walletPage = makePage({
            path: "03 人物/钱包/退役测试卡.md",
            name: "退役测试卡",
            tags: ["#钱包"],
            frontmatter: { tags: ["钱包"] },
            inlinks,
        });
        const sourcePage = makePage({
            path: "01 日记/2026-05-06-life.md",
            name: "2026-05-06-life",
            ctime: "2026-05-06",
            day: "2026-05-06",
            lists: [
                listItem(1, "[[退役测试卡|信用卡]] spotify月度订阅 #记账 #订阅", ["#记账", "#订阅"], [
                    child(2, "-120"),
                    child(3, "LIFE:30@@"),
                ]),
                listItem(11, "[[退役测试卡|信用卡]] 训练耳机 #记账 #数码", ["#记账", "#数码"], [
                    child(12, "-900"),
                    child(13, "LIFE:365@90"),
                ]),
            ],
        });
        const { core, dv } = loadCore({ extraPages: [walletPage, sourcePage] });
        core.Wallet.resetParseCache();

        const early = core.Utils.collectLivingCostItems({
            dv,
            walletLinks: [{ path: "03 人物/钱包/退役测试卡.md" }],
            asOfDate: "2026-05-09",
        });
        const spotifyEarly = early.find(item => item.name === "spotify月度订阅");
        const headphoneEarly = early.find(item => item.name === "训练耳机");

        assert.equal(spotifyEarly.isRetired, false);
        assert.equal(spotifyEarly.isActive, true);
        assert.equal(dateOnly(spotifyEarly.retiredDate), "2026-06-05");
        assert.equal(spotifyEarly.remaining, 27);
        assert.equal(spotifyEarly.progressBaseDays, 30);
        assert.equal(spotifyEarly.todayCost, 4);
        assert.equal(headphoneEarly.isRetired, false);
        assert.equal(headphoneEarly.isActive, true);
        assert.equal(dateOnly(headphoneEarly.retiredDate), "2026-08-04");
        assert.equal(headphoneEarly.remaining, 87);
        assert.equal(headphoneEarly.progressBaseDays, 90);
        assert.equal(headphoneEarly.todayCost, 10);

        const retired = core.Utils.collectLivingCostItems({
            dv,
            walletLinks: [{ path: "03 人物/钱包/退役测试卡.md" }],
            asOfDate: "2026-06-05",
        });
        const spotifyRetired = retired.find(item => item.name === "spotify月度订阅");
        assert.equal(spotifyRetired.isRetired, true);
        assert.equal(spotifyRetired.isActive, false);
        assert.equal(spotifyRetired.remaining, 0);
        assert.equal(spotifyRetired.todayCost, 0);
    }),
    test("PersonCollection-style aggregation matches PersonProfile query totals", () => {
        const { core, dv } = loadCore({ currentPath: "03 人物/人/小明.md" });
        const collection = buildPersonCollectionData(core, dv);
        const xiaoMing = collection.peopleMap.get("小明");
        const profileEvents = core.Query()
            .from({ linkedTo: "03 人物/人/小明.md" })
            .filter({ type: "event", explicitTarget: "小明" })
            .execute();
        const profileTransactions = core.Query()
            .from({ linkedTo: "03 人物/人/小明.md" })
            .filter({ type: "journal", explicitTarget: "小明" })
            .execute();

        assert.equal(collection.entries.metrics.sourcePages, 3);
        assert.equal(xiaoMing.totalTime, profileEvents.reduce((sum, entry) => sum + entry.vector.time, 0));
        assert.equal(xiaoMing.netMoney, profileTransactions.reduce((sum, entry) => sum + entry.vector.money, 0));
        assert.equal(xiaoMing.count, profileEvents.length + profileTransactions.length);
        assert.equal(collection.peopleMap.get("妈妈").count, 0);
    }),
    test("PersonCollection-style supertag source ignores inline #人物 notes", () => {
        const personPage = makePage({
            path: "People/Alex.md",
            name: "Alex",
            tags: ["#人物"],
            frontmatter: { tags: ["人物"] },
            inlinks: [{ path: "Logs/alex.md" }],
        });
        personPage.file.inlinks.values = personPage.file.inlinks;
        const inlinePersonNote = makePage({
            path: "People/人物说明.md",
            name: "人物说明",
            tags: ["#人物"],
            frontmatter: {},
            inlinks: [{ path: "Logs/alex.md" }],
        });
        const sourcePage = makePage({
            path: "Logs/alex.md",
            day: "2026-05-09",
            lists: [
                listItem(1, "和 [[People/Alex|Alex]] 复盘 #工作 @2026-05-09", ["#工作"], [
                    child(2, "3, 2"),
                ]),
            ],
        });
        const { core, dv } = loadCore({ extraPages: [personPage, inlinePersonNote, sourcePage] });
        const collection = buildPersonCollectionData(core, dv, { personQuery: "#人物" });
        const alex = collection.peopleMap.get("Alex");

        assert.ok(alex);
        assert.equal(alex.totalTime, 3);
        assert.equal(alex.count, 1);
        assert.ok(!collection.peopleMap.has("人物说明"));
    }),
    test("PersonCollection-style default accepts legacy frontmatter tag 人", () => {
        const legacyPerson = makePage({
            path: "People/LegacyPerson.md",
            name: "LegacyPerson",
            tags: [],
            frontmatter: { tags: ["人"] },
        });
        const { core, dv } = loadCore({ extraPages: [legacyPerson] });
        const collection = buildPersonCollectionData(core, dv);

        assert.ok(collection.peopleMap.has("小明"));
        assert.ok(collection.peopleMap.has("LegacyPerson"));
    }),
    test("ProjectCollection-style aggregation uses project frontmatter supertag", () => {
        const inlineProjectNote = makePage({
            path: "02 项目/项目说明.md",
            name: "项目说明",
            tags: ["#项目"],
            frontmatter: {},
        });
        const frontmatterOnlyProject = makePage({
            path: "02 项目/仅头部项目.md",
            name: "仅头部项目",
            tags: [],
            frontmatter: {
                tags: ["项目"],
                "状态": "计划中",
            },
        });
        const { core, dv } = loadCore({ extraPages: [inlineProjectNote, frontmatterOnlyProject] });
        const collection = buildProjectCollectionData(core, dv);
        const project = collection.projectData.find(item => item.name === "项目A");

        assert.ok(project);
        assert.ok(collection.projectData.some(item => item.name === "仅头部项目"));
        assert.ok(!collection.projectData.some(item => item.name === "项目说明"));
        assert.equal(collection.entries.metrics.sourcePages, 2);
        assert.equal(project.time, 6);
        assert.equal(project.money, -80);
        assert.equal(project.count, 3);
        assert.equal(project.status, "进行中");
        assert.equal(project.targetEffort, 5);
        assert.equal(project.progress, 120);
        assert.deepEqual(project.tags, ["开发"]);
    }),
    test("ObjectSummary collects object summaries without treating inline tags as identity", () => {
        const inlinePersonNote = makePage({
            path: "People/人物说明.md",
            name: "人物说明",
            tags: ["#人物"],
            frontmatter: {},
            inlinks: [{ path: "Logs/alex.md" }],
        });
        const personPage = makePage({
            path: "People/Alex.md",
            name: "Alex",
            tags: ["#人物"],
            frontmatter: { tags: ["人物"] },
            inlinks: [{ path: "Logs/alex.md" }],
        });
        const sourcePage = makePage({
            path: "Logs/alex.md",
            day: "2026-05-09",
            lists: [
                listItem(1, "和 [[People/Alex|Alex]] 复盘 #工作 @2026-05-09", ["#工作"], [
                    child(2, "3, 2"),
                ]),
            ],
        });
        const { core, dv } = loadCore({ extraPages: [inlinePersonNote, personPage, sourcePage] });
        const objectPages = core.Utils.collectSupertagPages({ tag: "#人物", dv });
        const result = core.ObjectSummary.collect({
            objectPages,
            Query: core.Query,
            createSummary: page => ({ name: page.file.name, path: page.file.path, count: 0, totalTime: 0 }),
            accumulate(summary, entry) {
                summary.count += 1;
                summary.totalTime += entry.vector.time || 0;
            },
        });

        assert.ok(result.summaryMap.has("Alex"));
        assert.ok(!result.summaryMap.has("人物说明"));
        assert.equal(result.summaryMap.get("Alex").count, 1);
        assert.equal(result.summaryMap.get("Alex").totalTime, 3);
        assert.equal(result.sourcePaths.length, 1);
    }),
    test("A2 PersonCollection inverted index matches brute-force person matching", () => {
        const { core, dv } = loadCore({ currentPath: "03 人物/人/小明.md" });
        const brute = buildPersonCollectionData(core, dv);
        const indexed = buildPersonCollectionDataIndexed(core, dv);

        for (const [name, brutePerson] of brute.peopleMap) {
            const indexedPerson = indexed.peopleMap.get(name);
            assert.ok(indexedPerson, name);
            assert.equal(indexedPerson.totalTime, brutePerson.totalTime);
            assert.equal(indexedPerson.netMoney, brutePerson.netMoney);
            assert.equal(indexedPerson.count, brutePerson.count);
        }
    }),
];

let passed = 0;
let failed = 0;

for (const item of cases) {
    try {
        item.fn();
        passed += 1;
        console.log(`PASS ${item.name}`);
    } catch (error) {
        failed += 1;
        console.log(`FAIL ${item.name}`);
        console.log(`  ${error.stack || error.message}`);
    }
}

console.log("");
console.log(`Summary: ${passed} passed, ${failed} failed`);

if (failed > 0) {
    process.exitCode = 1;
}
