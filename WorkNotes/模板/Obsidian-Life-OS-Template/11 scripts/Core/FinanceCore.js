/**
 * 财务核心逻辑库 (FinanceCore.js) v1.0
 *
 * 功能：
 * 1. 提供统一的配置 (CONFIG)
 * 2. 提供通用的辅助函数 (Utils)
 * 3. 提供标准的钱包类 (Wallet)
 *
 * 用法：
 * 在其他 DataviewJS 脚本中：
 * const core = {};
 * await dv.view("11 scripts/Core/FinanceCore", core);
 * const { CONFIG, Utils, Wallet } = core;
 */

// --- 1. 配置中心 ---
const CONFIG = {
    journalTag: "记账",
    frontmatterKeys: {
        repaymentTerms: "还款日", creditLimit: "信用", typeTags: "类型",
        transactionValue: "值", relatedLinks: "关联", creationTime: "创建时间",
        projectLinks: "项目", repaymentInfo: "还款信息", billingDate: "入账日",
        maxRows: "最大行数", startDate: "开始时间", endDate: "结束时间",
        serviceDays: "服役天数", englishType: "type"
    },
    defaultRepaymentTerms: [15, 14, 15],
    runtimePaths: {
        scripts: '11 scripts'
    },
    defaultDestinations: {
        scripts: '11 scripts',
        templates: '10 模板',
        diary: '01 日记',
        events: '02 事件',
        ledgers: '02 事件/021 账本',
        persons: '03 人物/人',
        projects: '04 项目',
        wallets: '03 人物/钱包'
    },
    legacyPaths: {
        templates: '10 模板',
        diary: '01 日记',
        events: '02 事件',
        ledgers: '02 事件/021 账本',
        persons: '03 人物/人',
        projects: '04 项目',
        wallets: '03 人物/钱包'
    }
};

// --- 2. 辅助函数库 ---
const Utils = {
    async getDvQuery(query) { return (await dv.tryQuery(query)).values; },

    // [New] 三维价值解析器：统一解析 [Money, Emotion, Time]
    parseValue(raw, isJournal) {
        let vec = [0, 0, 0]; // [Money, Emotion, Time]
        if (!raw && raw !== 0) return vec;

        let values = [];
        if (Array.isArray(raw)) {
            values = raw.map(n => Number(n)).filter(n => !isNaN(n));
        } else if (typeof raw === 'string') {
            // Clean: Remove tags, links, and brackets
            let clean = raw.replace(/#[^\s]+/g, '').replace(/\[\[.*?\]\]/g, '').replace(/[\[\]]/g, '').trim();
            const parts = clean.split(/[,，\s]+/).filter(p => p !== "");
            values = parts.map(p => Number(p)).filter(n => !isNaN(n));
        } else if (typeof raw === 'number') {
            values = [raw];
        }

        if (values.length === 0) return vec;

        if (isJournal) {
            // Context: Bill (Transaction) -> [Money, Emotion]
            vec[0] = values[0];      // Money
            vec[1] = values[1] || 0; // Emotion
            vec[2] = 0;              // Time
        } else {
            // Context: Event (Thing) -> [Time, Emotion]
            vec[0] = 0;              // Money
            vec[1] = values[1] || 0; // Emotion
            vec[2] = values[0];      // Time
        }
        return vec;
    },

    // [New] 智能数值解析器：支持 标量、向量、简写字符串
    parseAmount(raw) {
        if (typeof raw === 'number') return raw;
        if (Array.isArray(raw)) return raw[0]; // 数组取第一维
        if (typeof raw === 'string') {
            const trimmed = raw.trim();
            // 1. JSON 数组格式 [x, y, z]
            if (trimmed.startsWith('[') && trimmed.endsWith(']')) {
                try {
                    const arr = JSON.parse(trimmed);
                    if (Array.isArray(arr)) return Number(arr[0]) || 0;
                } catch (e) { /* ignore */ }
            }
            // 2. 逗号分隔格式 x, y, z (支持中英文逗号)
            if (trimmed.includes(',') || trimmed.includes('，')) {
                const parts = trimmed.split(/[,，]/);
                return Number(parts[0]) || 0;
            }
            // 3. 普通数字字符串
            return Number(raw) || 0;
        }
        return 0;
    },

    _asArray(value) {
        if (value == null) return [];
        if (Array.isArray(value)) return value;
        if (typeof value.array === "function") return value.array();
        if (Array.isArray(value.values)) return value.values;
        if (typeof value !== "string" && typeof value[Symbol.iterator] === "function") {
            try { return Array.from(value); } catch (e) { /* ignore */ }
        }
        return [value];
    },

    normalizeArrayField(value) {
        return this._asArray(value)
            .map(item => {
                if (item == null) return "";
                if (typeof item === "object" && item.path) return item.path;
                return String(item);
            })
            .map(item => item.trim())
            .filter(Boolean);
    },

    normalizeSupertagInput(value) {
        return this.normalizeArrayField(value)
            .flatMap(item => this.normalizeArrayField(item))
            .map(item => String(item || "").replace(/^#/, "").trim())
            .filter(Boolean);
    },

    hasObjectSupertag(page, tag) {
        const wanted = new Set(this.normalizeSupertagInput(tag));
        if (!wanted.size) return false;
        const fm = page?.file?.frontmatter || {};
        return this.normalizeSupertagInput(fm.tags)
            .some(item => wanted.has(item));
    },

    hasFrontmatterTag(page, tag) {
        return this.hasObjectSupertag(page, tag);
    },

    isWalletPage(page, options = {}) {
        return this.hasObjectSupertag(page, options.walletTag || "钱包")
            || this.hasFrontmatterType(page, ["钱包", "信用卡", "储蓄卡", "现金", "平台账户"]);
    },

    hasFrontmatterType(page, type) {
        const wanted = new Set(this.normalizeSupertagInput(type));
        if (!wanted.size) return false;
        const fm = page?.file?.frontmatter || {};
        return [
            ...this._normalizeFieldArray(fm[CONFIG.frontmatterKeys.typeTags]),
            ...this._normalizeFieldArray(fm[CONFIG.frontmatterKeys.englishType]),
        ].map(item => String(item || "").replace(/^#/, "").trim())
            .some(item => wanted.has(item));
    },

    _normalizePath(path) {
        return String(path || "").replace(/\\/g, "/").replace(/\.md$/, "").replace(/^\/+|\/+$/g, "");
    },

    _pathWithin(path, scope) {
        const normalizedPath = this._normalizePath(path);
        const normalizedScope = this._normalizePath(scope);
        return Boolean(normalizedPath && normalizedScope)
            && (normalizedPath === normalizedScope || normalizedPath.startsWith(`${normalizedScope}/`));
    },

    getConfiguredPath(role, key) {
        const groups = {
            runtime: CONFIG.runtimePaths,
            defaultDestination: CONFIG.defaultDestinations,
            legacy: CONFIG.legacyPaths,
        };
        return groups[role]?.[key] || "";
    },

    pathWithinConfigured(role, key, path) {
        return this._pathWithin(path, this.getConfiguredPath(role, key));
    },

    describePathRoles(path) {
        const roles = [];
        const groups = [
            ["runtime", CONFIG.runtimePaths],
            ["defaultDestination", CONFIG.defaultDestinations],
            ["legacy", CONFIG.legacyPaths],
        ];
        for (const [role, paths] of groups) {
            for (const [key, scope] of Object.entries(paths || {})) {
                if (this._pathWithin(path, scope)) roles.push({ role, key, scope });
            }
        }
        return roles;
    },

    _recordLegacyPathWarning(kind, pageOrPath, legacyPath) {
        const sourcePath = typeof pageOrPath === "string" ? pageOrPath : pageOrPath?.file?.path;
        if (!sourcePath) return;
        if (!this._legacyPathWarnings) this._legacyPathWarnings = [];
        const warning = {
            kind,
            sourcePath,
            legacyPath,
            message: `${kind}: ${sourcePath} matched legacy path ${legacyPath}; add frontmatter tags/type to decouple identity from folders.`,
        };
        this._legacyPathWarnings.push(warning);
        if (this._legacyPathWarnings.length > 200) this._legacyPathWarnings.shift();
    },

    getLegacyPathWarnings() {
        return [...(this._legacyPathWarnings || [])];
    },

    clearLegacyPathWarnings() {
        this._legacyPathWarnings = [];
    },

    _legacyTypeFromPath(path, options = {}) {
        const p = this._normalizePath(path);
        const legacy = CONFIG.legacyPaths;
        const checks = [
            ["记账", legacy.ledgers],
            ["日记", legacy.diary],
            ["事件", legacy.events],
            ["钱包", legacy.wallets],
            ["人物", legacy.persons],
            ["项目", legacy.projects],
            ["模板", legacy.templates],
        ];
        const hit = checks.find(([, scope]) => this._pathWithin(p, scope));
        if (!hit) return null;
        if (options.warn !== false) this._recordLegacyPathWarning(options.kind || "legacy-path-type", path, hit[1]);
        return hit[0];
    },

    collectSupertagPages(options = {}) {
        const sources = options.pages
            ? { pages: options.pages }
            : options.sources
                ? options.sources
                : options.scope
                    ? { scope: options.scope, maxPages: options.maxPages }
                    : { allowGlobal: true, maxPages: options.maxPages || 1200 };
        const pages = SourceResolver.resolve(sources);
        return this._asArray(pages)
            .filter(page => page?.file)
            .filter(page => this.hasObjectSupertag(page, options.tag || options.tags))
            .sort((a, b) => String(a.file.name || "").localeCompare(String(b.file.name || ""), "zh-Hans-CN"));
    },

    collectWalletPages(options = {}) {
        const sources = options.pages
            ? { pages: options.pages }
            : options.sources
                ? options.sources
                : options.scope
                    ? { scope: options.scope, maxPages: options.maxPages }
                    : { allowGlobal: true, maxPages: options.maxPages || 1200 };
        const pages = SourceResolver.resolve(sources);
        return this._asArray(pages)
            .filter(page => page?.file)
            .filter(page => this.isWalletPage(page, options))
            .sort((a, b) => String(a.file.name || "").localeCompare(String(b.file.name || ""), "zh-Hans-CN"));
    },

    collectWallets(options = {}) {
        return this.collectWalletPages(options)
            .map(page => {
                try {
                    return new Wallet(page.file.link || { path: page.file.path });
                } catch (error) {
                    console.error("Wallet load failed", page?.file?.path, error);
                    return null;
                }
            })
            .filter(wallet => wallet && wallet.name);
    },

    normalizeNumber(value, fallback = 0) {
        if (value == null || value === "") return fallback;
        if (typeof value === "number") return Number.isFinite(value) ? value : fallback;
        const parsed = this.parseAmount(value);
        return Number.isFinite(Number(parsed)) ? Number(parsed) : fallback;
    },

    getWalletIcon(name) {
        if (name.includes("支付宝") || name.includes("花呗") || name.includes("网商")) return "🔵";
        if (name.includes("微信")) return "🟢";
        if (name.includes("工") || name.includes("建") || name.includes("招") || name.includes("中") || name.includes("行")) return "🏛️";
        if (name.includes("京东") || name.includes("白条")) return "🔴";
        if (name.includes("现金")) return "💵";
        return "👛";
    },

    getRelativeTimeDesc(dateStr) {
        if (!dateStr) return "";
        const target = new Date(dateStr);
        const today = new Date(); today.setHours(0, 0, 0, 0); target.setHours(0, 0, 0, 0);
        const diff = Math.ceil((target - today) / (1000 * 60 * 60 * 24));
        if (diff < 0) return { text: `已过 ${Math.abs(diff)} 天`, type: 'past' };
        if (diff === 0) return { text: `今天`, type: 'urgent' };
        if (diff === 1) return { text: `明天`, type: 'warning' };
        return { text: `${diff} 天后`, type: 'future' };
    },

    parseRepaymentString(str) {
        if (!str || typeof str !== 'string') return [];
        return str.split("#").map(item => {
            let [dateString, valueString] = item.split("@");
            if (!dateString) return { date: null, value: Number(valueString) || 0 };
            let parsableDateString = dateString.trim();
            if (/^\d{8}$/.test(parsableDateString)) {
                parsableDateString = `${parsableDateString.substring(0, 4)}-${parsableDateString.substring(4, 6)}-${parsableDateString.substring(6, 8)}`;
            }
            const dateObj = new Date(parsableDateString);
            if (isNaN(dateObj.getTime())) return { date: null, value: Number(valueString) || 0 };
            return {
                date: `${dateObj.getFullYear()}-${String(dateObj.getMonth() + 1).padStart(2, '0')}-${String(dateObj.getDate()).padStart(2, '0')} ${String(dateObj.getHours()).padStart(2, '0')}:${String(dateObj.getMinutes()).padStart(2, '0')}`,
                value: Number(valueString)
            };
        });
    },

    parseInstallmentString(str, firstRepaymentDate, transactionValue) {
        if (!str || !str.toUpperCase().startsWith('MULTI:')) return null;
        const parts = str.substring(6).split(';');
        const [installmentsStr, totalAmountStr] = parts[0].split('@');
        const installments = Number(installmentsStr);
        const totalAmount = totalAmountStr === undefined ? transactionValue : Number(totalAmountStr);
        if (isNaN(installments) || installments <= 0 || isNaN(totalAmount)) return null;

        const totalCents = Math.round(totalAmount * 100);
        const baseInstallmentCents = Math.floor(totalCents / installments);
        const remainderCents = totalCents - (baseInstallmentCents * installments);
        let startDate = new Date(firstRepaymentDate);
        for (let i = 1; i < parts.length; i++) {
            const [key, value] = parts[i].split('=');
            if (key.trim().toLowerCase() === '开始日期' && value) {
                const parsedStartDate = new Date(value.trim());
                if (!isNaN(parsedStartDate.getTime())) startDate = parsedStartDate;
            }
        }
        const records = [];
        for (let i = 0; i < installments; i++) {
            const repaymentDate = new Date(startDate);
            repaymentDate.setMonth(startDate.getMonth() + i);
            let val = (i === 0) ? (baseInstallmentCents + remainderCents) / 100 : baseInstallmentCents / 100;
            records.push({
                date: `${repaymentDate.getFullYear()}-${String(repaymentDate.getMonth() + 1).padStart(2, '0')}-${String(repaymentDate.getDate()).padStart(2, '0')} ${String(new Date(firstRepaymentDate).getHours()).padStart(2, '0')}:${String(new Date(firstRepaymentDate).getMinutes()).padStart(2, '0')}`,
                value: Number(val.toFixed(2))
            });
        }
        return records;
    },

    calculateNextRepaymentDate(dateTimeString, s, e, r) {
        const d = new Date(dateTimeString);
        if (isNaN(d.getTime())) return null;
        let by = d.getFullYear(), bm = d.getMonth();
        if (d.getDate() >= s) bm += 1;
        const closeDate = new Date(by, bm, e);
        let repDate = new Date(closeDate.getTime());
        repDate.setDate(r);
        if (repDate.getTime() < closeDate.getTime()) repDate.setMonth(repDate.getMonth() + 1);
        repDate.setHours(d.getHours()); repDate.setMinutes(d.getMinutes());
        return `${repDate.getFullYear()}-${String(repDate.getMonth() + 1).padStart(2, '0')}-${String(repDate.getDate()).padStart(2, '0')} ${String(repDate.getHours()).padStart(2, '0')}:${String(repDate.getMinutes()).padStart(2, '0')}`;
    },

    removeTagsFromText(text, removeList) {
        if (!text || !removeList || removeList.length === 0) return text;
        const pattern = new RegExp(removeList.join("|"), "g");
        return text.replace(pattern, "").trim();
    },

    /**
     * 从文本中解析显式指定的事件时间 @日期
     * 支持格式: @YYYY-MM-DD, @YYYYMMDD, @YYYY/MM/DD
     * @param {string} text - 待解析的文本
     * @returns {{ date: Date, match: string } | null} - 解析结果或 null
     */
    parseEventDate(text) {
        if (!text || typeof text !== 'string') return null;

        // 正则匹配 @日期 格式
        // 1. @YYYY-MM-DD
        // 2. @YYYYMMDD
        // 3. @YYYY/MM/DD
        const dateRegex = /@(\d{4}[-/]\d{2}[-/]\d{2}|\d{8})/g;
        const match = dateRegex.exec(text);

        if (!match) return null;

        let dateStr = match[1];

        // 统一转换为 YYYY-MM-DD 格式
        if (/^\d{8}$/.test(dateStr)) {
            // YYYYMMDD -> YYYY-MM-DD
            dateStr = `${dateStr.substring(0, 4)}-${dateStr.substring(4, 6)}-${dateStr.substring(6, 8)}`;
        } else if (dateStr.includes('/')) {
            // YYYY/MM/DD -> YYYY-MM-DD
            dateStr = dateStr.replace(/\//g, '-');
        }

        const parsedDate = new Date(dateStr);
        if (isNaN(parsedDate.getTime())) return null;

        // 设置时间为当天的开始，避免时区问题
        parsedDate.setHours(0, 0, 0, 0);

        return {
            date: parsedDate,
            match: match[0] // 包括 @ 符号的完整匹配
        };
    },

    resolveDateValue(value) {
        if (!value) return null;
        if (value instanceof Date) return isNaN(value.getTime()) ? null : value;
        if (typeof value === 'object') {
            if (typeof value.toJSDate === 'function') {
                const d = value.toJSDate();
                return d && !isNaN(d.getTime()) ? d : null;
            }
            if (typeof value.toMillis === 'function') {
                const d = new Date(value.toMillis());
                return !isNaN(d.getTime()) ? d : null;
            }
            if (value.ts) {
                const d = new Date(value.ts);
                return !isNaN(d.getTime()) ? d : null;
            }
        }
        const d = new Date(value);
        return !isNaN(d.getTime()) ? d : null;
    },

    resolveBillDate(value) {
        const raw = this.resolveDateValue(value);
        if (!raw) return null;
        const d = new Date(raw.getTime());
        d.setHours(0, 0, 0, 0);
        return d;
    },

    formatDateKey(value) {
        const d = this.resolveBillDate(value);
        if (!d) return "";
        return [
            d.getFullYear(),
            String(d.getMonth() + 1).padStart(2, "0"),
            String(d.getDate()).padStart(2, "0"),
        ].join("-");
    },

    resolveEntryDateInfo(entry) {
        if (!entry) return null;
        const explicit = this.resolveDateValue(entry.meta?.explicitDate);
        if (explicit) return { date: explicit, source: "explicit" };

        const page = entry.sourcePage;
        if (!page || !page.file) return null;

        const fileDay = this.resolveDateValue(page.file.day);
        if (fileDay) return { date: fileDay, source: "file-day" };

        const created = this.resolveDateValue(page.file.frontmatter?.[CONFIG.frontmatterKeys.creationTime]);
        if (created) return { date: created, source: "frontmatter" };

        const ctime = this.resolveDateValue(page.file.ctime);
        if (ctime) return { date: ctime, source: "ctime-fallback" };

        return null;
    },

    resolveEntryDate(entry) {
        return this.resolveEntryDateInfo(entry)?.date || null;
    },

    toDateTime(value, dataview = (typeof dv !== "undefined" ? dv : null)) {
        const date = this.resolveDateValue(value);
        if (!date) return null;
        if (dataview?.luxon?.DateTime?.fromJSDate) return dataview.luxon.DateTime.fromJSDate(date);
        return {
            ts: date.getTime(),
            valueOf() { return this.ts; },
            toMillis() { return this.ts; },
            toFormat(format) {
                const yyyy = date.getFullYear();
                const MM = String(date.getMonth() + 1).padStart(2, "0");
                const dd = String(date.getDate()).padStart(2, "0");
                const HH = String(date.getHours()).padStart(2, "0");
                const mm = String(date.getMinutes()).padStart(2, "0");
                const ss = String(date.getSeconds()).padStart(2, "0");
                if (format === "MM-dd") return `${MM}-${dd}`;
                if (format === "yyyy-MM-dd") return `${yyyy}-${MM}-${dd}`;
                return `${yyyy}-${MM}-${dd} ${HH}:${mm}:${ss}`;
            },
        };
    },

    _targetCandidates(target) {
        const file = target?.file || target || (typeof dv !== "undefined" ? dv.current()?.file : null);
        const candidates = new Set();
        const add = value => {
            if (!value) return;
            const raw = String(value).replace(/\.md$/, "");
            candidates.add(raw);
            const parts = raw.split(/[\\/]/);
            candidates.add(parts[parts.length - 1]);
        };
        if (typeof file === "string") add(file);
        add(file?.name);
        add(file?.path);
        return candidates;
    },

    linkMatchesTarget(link, target) {
        const normalizedLink = String(link || "").replace(/\.md$/, "");
        if (!normalizedLink) return false;
        for (const candidate of this._targetCandidates(target)) {
            const normalizedCandidate = String(candidate || "").replace(/\.md$/, "");
            if (!normalizedCandidate) continue;
            if (normalizedLink === normalizedCandidate) return true;
            if (normalizedLink.endsWith(`/${normalizedCandidate}`)) return true;
        }
        return false;
    },

    parseWikiLinks(text) {
        const links = [];
        const linkRegex = /\[\[([^\]|]+)(?:\|([^\]]+))?\]\]/g;
        let match;
        while ((match = linkRegex.exec(String(text || ""))) !== null) {
            const target = String(match[1] || "").trim();
            if (!target) continue;
            const label = String(match[2] || target.split(/[\\/]/).pop() || target).trim();
            links.push({
                target,
                label,
                raw: match[0],
                order: links.length + 1,
                index: match.index,
            });
        }
        return links;
    },

    parseSourceLinks(text) {
        const links = [];
        const sourceRegex = /SOURCE:\s*(\[\[([^\]|]+)(?:\|([^\]]+))?\]\])/ig;
        let match;
        while ((match = sourceRegex.exec(String(text || ""))) !== null) {
            const target = String(match[2] || "").trim();
            if (!target) continue;
            const label = String(match[3] || target.split(/[\\/]/).pop() || target).trim();
            links.push({
                target,
                label,
                raw: match[0],
                linkRaw: match[1],
                role: "source",
                order: links.length + 1,
                index: match.index + match[0].indexOf(match[1]),
            });
        }
        return links;
    },

    isWalletLinkTarget(target, options = {}) {
        const raw = String(target || "").replace(/\.md$/, "");
        if (!raw) return false;
        const dataview = options.dv || (typeof dv !== "undefined" ? dv : null);
        const page = dataview?.page ? dataview.page(raw) : null;
        if (page?.file) return this.isWalletPage(page, options);
        const legacyHit = this._pathWithin(raw, CONFIG.legacyPaths.wallets);
        if (legacyHit) this._recordLegacyPathWarning("legacy-wallet-link", raw, CONFIG.legacyPaths.wallets);
        return legacyHit;
    },

    _cleanDisplayTextChunk(text) {
        return String(text || "")
            .replace(/(?:^|\s)\^[A-Za-z0-9_-]+\b/g, " ")
            .replace(/SOURCE:\s*$/ig, " ")
            .replace(/BILL:\s*[\d-]{8,10}/ig, " ")
            .replace(/LIFE:\d+(?:@@|@[^\s;,，]*)?/ig, " ")
            .replace(/MULTI:[^\s;,，]+/ig, " ")
            .replace(/@\d{4}[-/]?\d{2}[-/]?\d{2}|@\d{8}/g, " ")
            .replace(/#[^\s#]+/g, " ")
            .replace(/^[;,，\s]+|[;,，\s]+$/g, "")
            .replace(/\s+/g, " ")
            .trim();
    },

    _makeDisplayParts(rawText, linksDetailed = []) {
        const parts = [];
        const addText = text => {
            const clean = this._cleanDisplayTextChunk(text);
            if (!clean) return;
            const previous = parts[parts.length - 1];
            if (previous?.type === "text") previous.text = `${previous.text} ${clean}`.replace(/\s+/g, " ");
            else parts.push({ type: "text", text: clean });
        };

        const detailsByOrder = new Map((linksDetailed || []).map(link => [link.order, link]));
        const linkRegex = /\[\[([^\]|]+)(?:\|([^\]]+))?\]\]/g;
        let cursor = 0;
        let order = 0;
        let match;
        while ((match = linkRegex.exec(String(rawText || ""))) !== null) {
            order += 1;
            addText(String(rawText || "").slice(cursor, match.index));
            const detail = detailsByOrder.get(order);
            if (detail && !["wallet", "source"].includes(detail.role)) {
                parts.push({
                    type: "link",
                    target: detail.target,
                    label: detail.label,
                    role: detail.role || "object",
                });
            }
            cursor = linkRegex.lastIndex;
        }
        addText(String(rawText || "").slice(cursor));
        return parts;
    },

    _partsToText(parts = [], fallback = "") {
        const text = (parts || [])
            .map(part => part?.type === "link" ? part.label : part?.text)
            .filter(Boolean)
            .join(" ")
            .replace(/\s+/g, " ")
            .trim();
        return text || String(fallback || "").trim();
    },

    filterPartsForView(parts = [], context = null) {
        const source = context?.file || context;
        if (!Array.isArray(parts) || !source) return [...(parts || [])];

        const normalize = value => String(value || "")
            .replace(/\\/g, "/")
            .replace(/\.md$/i, "")
            .replace(/^\/+|\/+$/g, "")
            .toLowerCase()
            .trim();
        const basename = value => {
            const raw = String(value || "").replace(/\\/g, "/").replace(/\.md$/i, "");
            const pieces = raw.split("/").filter(Boolean);
            return pieces[pieces.length - 1] || raw;
        };

        let contextPath = "";
        let contextName = "";
        if (typeof source === "string") {
            contextPath = normalize(source);
            contextName = normalize(basename(source));
        } else {
            contextPath = normalize(source.path);
            contextName = normalize(source.name || basename(source.path));
        }
        if (!contextPath && !contextName) return [...parts];
        const contextIsWallet = /(^|\/)(钱包|wallets?)(\/|$)/i.test(contextPath)
            || this.hasObjectSupertag({ file: source }, "钱包");

        const targetMatchesContext = target => {
            const rawTarget = String(target || "").trim();
            const normalizedTarget = normalize(rawTarget);
            if (!normalizedTarget) return false;
            const targetHasPath = /[\\/]/.test(rawTarget);
            if (targetHasPath && contextPath) return normalizedTarget === contextPath;
            if (!targetHasPath && contextName) return normalizedTarget === contextName;
            if (contextPath) return normalizedTarget === contextPath;
            return contextName && normalizedTarget === contextName;
        };

        const isSelfLink = part => part?.type === "link"
            && part.role !== "wallet"
            && part.role !== "source"
            && targetMatchesContext(part.target || part.path || part.label);
        const hasFollowingBodyPart = (linkIndex) => {
            for (let i = linkIndex + 1; i < parts.length; i += 1) {
                if (parts[i]?.type === "text") {
                    if (/[a-zA-Z0-9\u4e00-\u9fa5]/.test(parts[i].text)) {
                        return true;
                    }
                } else if (parts[i]?.type === "link" && !isSelfLink(parts[i]) && parts[i].role !== "wallet") {
                    return true;
                }
            }
            return false;
        };

        return parts.map((part, index) => {
            if (!part || part.type !== "link") return part;
            if (part.role === "wallet" || part.role === "source") return part;
            if (!isSelfLink(part)) return part;
            if (contextIsWallet) return null;
            if (!hasFollowingBodyPart(index)) {
                return null;
            }
            return part; // 保持它作为链接的形态，不降级为纯文本
        }).filter(Boolean);
    },

    decorateEntryDisplay(entry, options = {}) {
        if (!entry) return entry;
        const target = options.targetFile || options.target || null;
        const displayRawText = entry.meta?.displayRawText || entry.rawText || entry.cleanText || "";
        const rawLinks = this.parseWikiLinks(displayRawText);
        const rawSourceLinks = this.parseSourceLinks(entry.rawText || "");
        const infoSourceLinks = this.parseSourceLinks(entry.meta?.sourceText || entry.meta?.info || "");
        const displaySourceLinks = this.parseSourceLinks(displayRawText);
        const sourceByIndex = new Set(displaySourceLinks.map(link => link.index));
        const sourceTargets = new Set([...rawSourceLinks, ...infoSourceLinks, ...displaySourceLinks].map(link => link.target));
        let walletAssigned = false;

        const detailed = rawLinks.map(link => {
            let role = "object";
            if (sourceByIndex.has(link.index)) {
                role = "source";
            } else if (this.linkMatchesTarget(link.target, target)) {
                role = "self";
            } else if (entry.type === "journal" && this.isWalletLinkTarget(link.target, options)) {
                role = "wallet";
                walletAssigned = true;
            }
            return { ...link, role };
        });

        for (const link of entry.meta?.outlinks || []) {
            const targetText = String(link || "").trim();
            if (!targetText) continue;
            if (sourceTargets.has(targetText)) continue;
            if (detailed.some(item => item.target === targetText)) continue;
            detailed.push({
                target: targetText,
                label: targetText.split(/[\\/]/).pop(),
                raw: `[[${targetText}]]`,
                role: entry.type === "journal" && !walletAssigned && this.isWalletLinkTarget(targetText, options) ? "wallet" : "object",
                order: detailed.length + 1,
                inherited: true,
            });
            if (detailed[detailed.length - 1].role === "wallet") walletAssigned = true;
        }

        for (const source of [...rawSourceLinks, ...infoSourceLinks]) {
            if (detailed.some(item => item.role === "source" && item.target === source.target)) continue;
            detailed.push({
                target: source.target,
                label: source.label,
                raw: source.raw,
                role: "source",
                order: detailed.length + 1,
            });
        }

        entry.linksDetailed = detailed;
        entry.meta.sourceLinks = detailed.filter(link => link.role === "source");
        entry.displayParts = this._makeDisplayParts(displayRawText || entry.rawText || entry.cleanText || "", detailed);
        entry.displayText = this._partsToText(entry.displayParts, entry.cleanText || entry.rawText || "");
        return entry;
    },

    walletFromEntry(entry, options = {}) {
        if (!entry || entry.type !== "journal") return null;
        const target = options.targetFile || options.target || (typeof dv !== "undefined" ? dv.current()?.file : null);
        const dataview = options.dv || (typeof dv !== "undefined" ? dv : null);
        const detailedWallet = (entry.linksDetailed || [])
            .find(link => link.role === "wallet" && !this.linkMatchesTarget(link.target, target));
        if (detailedWallet) {
            const walletPage = dataview?.page ? dataview.page(detailedWallet.target) : null;
            return {
                path: walletPage?.file?.path || detailedWallet.target,
                display: detailedWallet.label || walletPage?.file?.name || String(detailedWallet.target).split(/[\\/]/).pop(),
            };
        }
        const walletLink = (entry.meta?.outlinks || [])
            .find(link => !this.linkMatchesTarget(link, target) && this.isWalletLinkTarget(link, options));
        if (!walletLink) return null;
        const walletPage = dataview?.page ? dataview.page(walletLink) : null;
        return {
            path: walletPage?.file?.path || walletLink,
            display: walletPage?.file?.name || String(walletLink).split(/[\\/]/).pop(),
        };
    },

    entryToViewItem(entry, options = {}) {
        if (!entry) return null;
        const ctime = this.toDateTime(this.resolveEntryDate(entry), options.dv);
        if (!ctime) return null;
        const link = { path: entry.sourcePath };
        if (entry.lineIndex >= 0) link.subpath = entry.lineIndex;
        if (entry.meta?.blockId) link.blockId = entry.meta.blockId;
        const rawDisplayParts = entry.displayParts || [];
        const displayContext = options.targetFile || options.target || null;
        const displayParts = this.filterPartsForView(rawDisplayParts, displayContext);
        const displayFallback = rawDisplayParts.length > 0
            ? (entry.cleanText || "")
            : (entry.displayText || entry.cleanText || entry.rawText || entry.sourcePage?.file?.name || "");
        const displayText = this._partsToText(displayParts, displayFallback);
        return {
            text: displayText,
            cleanText: entry.cleanText || "",
            displayText,
            displayParts,
            linksDetailed: entry.linksDetailed || [],
            sourceLinks: (entry.linksDetailed || []).filter(link => link.role === "source"),
            link,
            path: entry.sourcePath,
            ctime,
            tags: entry.meta?.tags || [],
            vec: [
                entry.vector?.money || 0,
                entry.vector?.emotion || 0,
                entry.vector?.time || 0,
            ],
            wallet: options.includeWallet === false ? null : this.walletFromEntry(entry, options),
        };
    },

    walletBillToViewItem(bill, options = {}) {
        if (!bill) return null;
        const dateObj = this.resolveBillDate(bill.dateObj || bill.date);
        if (!dateObj) return null;

        const displayContext = options.targetFile || options.target || null;
        const rawDisplayParts = Array.isArray(bill.displayParts) ? bill.displayParts : [];
        const displayParts = this.filterPartsForView(rawDisplayParts, displayContext);
        const displayFallback = bill.displayText || bill.description || bill.text || bill.name || "";
        const displayText = this._partsToText(displayParts, displayFallback);
        const value = this.normalizeNumber(bill.value, 0);
        const sourcePath = bill.sourcePath || bill.source?.path || bill.link?.path || bill.path || "";
        const walletPath = bill.walletPath || bill.path || "";
        const link = { path: sourcePath };
        if (bill.blockId) link.blockId = bill.blockId;
        else if (bill.subpath != null) link.subpath = bill.subpath;
        else if (bill.lineIndex != null) link.subpath = bill.lineIndex;
        else if (bill.sourceLine != null) link.subpath = bill.sourceLine;

        return {
            ...bill,
            type: "wallet-bill",
            text: displayText,
            cleanText: bill.description || displayText,
            description: bill.description || displayText,
            displayText,
            displayParts,
            linksDetailed: bill.linksDetailed || [],
            sourceLinks: bill.sourceLinks || [],
            link,
            sourcePath,
            walletPath,
            path: walletPath,
            ctime: this.toDateTime(dateObj, options.dv),
            dateObj,
            date: this.formatDateKey(dateObj),
            value,
            vec: [value, 0, 0],
            wallet: bill.wallet || "",
            tags: this.normalizeArrayField(bill.tags).map(tag => String(tag).replace(/^#/, "")),
        };
    },

    walletBillsToViewItems(bills = [], options = {}) {
        return this._asArray(bills)
            .map(bill => this.walletBillToViewItem(bill, options))
            .filter(Boolean);
    },

    collectWalletBillViewItems(wallets = [], options = {}) {
        const bills = options.bills || this.collectWalletBills(wallets);
        return this.walletBillsToViewItems(bills, options);
    },

    upcomingWalletBillViewItems(bills = [], options = {}) {
        const today = this.resolveBillDate(options.today || new Date()) || this.resolveBillDate(new Date());
        return this.walletBillsToViewItems(bills, options)
            .filter(item => item.dateObj && (!today || item.dateObj >= today))
            .sort((a, b) => a.dateObj - b.dateObj || Math.abs(b.value) - Math.abs(a.value));
    },

    collectObjectAtoms(target = null, options = {}) {
        const dataview = options.dv || (typeof dv !== "undefined" ? dv : null);
        const targetPage = target?.file
            ? target
            : (target?.path && dataview?.page ? dataview.page(target.path) : null)
                || (typeof target === "string" && dataview?.page ? dataview.page(target) : null)
                || (dataview?.current ? dataview.current() : null);
        const targetFile = targetPage?.file || (target?.file ?? null);
        const targetPath = targetFile?.path || target?.path || (typeof target === "string" ? target : "");
        const targetName = targetFile?.name || String(targetPath || "").split(/[\\/]/).pop()?.replace(/\.md$/, "") || "";
        const source = options.sources
            || options.source
            || { linkedTo: targetPath || true, maxPages: options.maxPages };
        const filterRules = options.filters || options.filter || {};
        const rawEntries = options.entries
            ? this._asArray(options.entries)
            : Query().from(source).filter(filterRules).execute();
        const warnings = rawEntries.warnings || rawEntries.metrics?.warnings || [];
        const metrics = rawEntries.metrics || null;

        const atoms = rawEntries
            .map(entry => {
                const item = this.entryToViewItem(entry, {
                    targetFile,
                    target: targetFile,
                    includeWallet: options.includeWallet !== false,
                    dv: dataview,
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
            })
            .filter(Boolean);

        const isTransfer = item => (item.tags || []).includes("转账");
        const financeItems = atoms.filter(item => item.entryType === "journal" && !isTransfer(item));
        const emotionItems = atoms.filter(item => item.vec?.[1] !== 0);
        const totalMoney = financeItems.reduce((sum, item) => sum + (item.vec?.[0] || 0), 0);
        const totalIncome = financeItems.filter(item => (item.vec?.[0] || 0) > 0).reduce((sum, item) => sum + item.vec[0], 0);
        const totalExpense = financeItems.filter(item => (item.vec?.[0] || 0) < 0).reduce((sum, item) => sum + item.vec[0], 0);
        const totalTime = atoms.reduce((sum, item) => sum + (item.vec?.[2] || 0), 0);
        const avgEmotion = emotionItems.length
            ? emotionItems.reduce((sum, item) => sum + (item.vec?.[1] || 0), 0) / emotionItems.length
            : 0;

        const relationMap = new Map();
        const addRelation = (link, item) => {
            if (!link?.target) return;
            if (link.role === "source" || link.role === "self") return;
            if (targetFile && this.linkMatchesTarget(link.target, targetFile)) return;
            const key = String(link.target).replace(/\.md$/, "");
            if (!key) return;
            if (!relationMap.has(key)) {
                relationMap.set(key, {
                    target: link.target,
                    label: link.label || key.split(/[\\/]/).pop(),
                    role: link.role || "object",
                    count: 0,
                    money: 0,
                    time: 0,
                    emotion: 0,
                    lastDate: null,
                });
            }
            const rel = relationMap.get(key);
            rel.count += 1;
            rel.money += item.vec?.[0] || 0;
            rel.time += item.vec?.[2] || 0;
            rel.emotion += item.vec?.[1] || 0;
            const ts = item.ctime?.ts || item.ctime?.toMillis?.() || 0;
            if (!rel.lastDate || ts > rel.lastDate) rel.lastDate = ts;
        };

        for (const item of atoms) {
            for (const link of item.linksDetailed || []) addRelation(link, item);
        }

        const sourceMap = new Map();
        for (const item of atoms) {
            for (const link of item.sourceLinks || []) {
                const key = String(link.target || "").replace(/\.md$/, "");
                if (!key) continue;
                if (!sourceMap.has(key)) {
                    sourceMap.set(key, {
                        target: link.target,
                        label: link.label || key.split(/[\\/]/).pop(),
                        count: 0,
                        money: 0,
                        items: [],
                    });
                }
                const sourceRef = sourceMap.get(key);
                sourceRef.count += 1;
                sourceRef.money += item.vec?.[0] || 0;
                sourceRef.items.push(item);
            }
        }

        return {
            targetPage,
            targetFile,
            targetPath,
            targetName,
            entries: rawEntries,
            atoms,
            items: atoms,
            events: atoms.filter(item => item.entryType === "event"),
            journals: atoms.filter(item => item.entryType === "journal"),
            lifecycle: atoms.filter(item => item.lifeDays > 0),
            sources: Array.from(sourceMap.values()).sort((a, b) => b.count - a.count || Math.abs(b.money) - Math.abs(a.money)),
            relations: Array.from(relationMap.values()).sort((a, b) => b.count - a.count || Math.abs(b.money) - Math.abs(a.money)),
            metrics: {
                atomCount: atoms.length,
                eventCount: atoms.filter(item => item.entryType === "event").length,
                journalCount: atoms.filter(item => item.entryType === "journal").length,
                sourceCount: sourceMap.size,
                relationCount: relationMap.size,
                totalMoney,
                totalIncome,
                totalExpense,
                totalTime,
                avgEmotion,
                query: metrics,
            },
            warnings,
        };
    },

    /**
     * 情感银行模型 (Emotion Bank Model) + 幂律遗忘
     * - 同一天内多条记录取平均值（保持 -3 ~ +3 范围）
     * - 跨天进行累加，使用幂律遗忘：R(t) = R₀ / (1 + t/τ)^β
     *   前期衰减快，后期趋于平稳
     *
     * @param {Array} records - 记录数组 [{score: number, date: Date|LuxonDateTime}, ...]
     * @param {Object} options - 配置参数
     * @param {number} options.tau - 时间常数（天），默认 7
     * @param {number} options.beta - 衰减指数，默认 0.5
     * @param {number} options.negBias - 负面事件权重倍数，默认 1.5
     * @returns {number} 情感账户余额
     */
    calculateEmotionScore(records, options = {}) {
        const tau = options.tau ?? 7;      // 时间常数
        const beta = options.beta ?? 0.5;  // 衰减指数
        const negBias = options.negBias ?? 1.5;

        if (!records || records.length === 0) return 0;

        // 幂律衰减函数：R(t) = R₀ / (1 + t/τ)^β
        const powerLawDecay = (days) => {
            if (days <= 0) return 1;
            return 1 / Math.pow(1 + days / tau, beta);
        };

        // 1. 转换日期并按日期分组
        const dailyMap = new Map();

        for (const record of records) {
            if (record.score === 0 || record.score === undefined) continue;

            let eventDate;
            if (record.date?.ts) {
                eventDate = new Date(record.date.ts);
            } else if (record.date instanceof Date) {
                eventDate = record.date;
            } else if (typeof record.date === 'string') {
                eventDate = new Date(record.date);
            } else {
                continue;
            }
            if (isNaN(eventDate.getTime())) continue;

            const dateStr = `${eventDate.getFullYear()}-${String(eventDate.getMonth() + 1).padStart(2, '0')}-${String(eventDate.getDate()).padStart(2, '0')}`;

            if (!dailyMap.has(dateStr)) {
                dailyMap.set(dateStr, { date: eventDate, scores: [] });
            }
            dailyMap.get(dateStr).scores.push(record.score);
        }

        if (dailyMap.size === 0) return 0;

        // 2. 计算每天的平均分
        const dailyData = Array.from(dailyMap.values())
            .map(day => ({
                date: day.date,
                avgScore: day.scores.reduce((a, b) => a + b, 0) / day.scores.length
            }))
            .sort((a, b) => a.date - b.date);

        // 3. 幂律衰减累加（从今天往回看每一天的贡献）
        const now = new Date();
        let balance = 0;

        for (const day of dailyData) {
            const daysAgo = (now - day.date) / (1000 * 60 * 60 * 24);
            const decay = powerLawDecay(daysAgo);
            const impact = day.avgScore * (day.avgScore < 0 ? negBias : 1.0);
            balance += impact * decay;
        }

        return balance;
    },

    /**
     * 将情感余额映射为 UI 反馈（标签、颜色）
     * 情感银行模型下分数是累加值，范围更大
     *
     * @param {number} score - 情感余额
     * @returns {Object} { score, label, emoji, color }
     */
    getEmotionLabel(score) {
        if (score === null || score === undefined || isNaN(score)) {
            return { score: 0, label: "无记录", emoji: "😶", color: "#9CA3AF" };
        }

        const s = Number(score);
        const display = s.toFixed(1);

        // 阈值基于情感银行模型：累加值，范围更大
        if (s >= 15) return { score: display, label: "挚爱", emoji: "🌟", color: "#10B981" };
        if (s >= 8) return { score: display, label: "亲密", emoji: "😍", color: "#34D399" };
        if (s >= 3) return { score: display, label: "友好", emoji: "😃", color: "#60A5FA" };
        if (s >= 0.5) return { score: display, label: "熟悉", emoji: "🙂", color: "#93C5FD" };
        if (s >= -0.5) return { score: display, label: "中性", emoji: "😶", color: "#9CA3AF" };
        if (s >= -3) return { score: display, label: "疏远", emoji: "🙁", color: "#F59E0B" };
        if (s >= -8) return { score: display, label: "反感", emoji: "😠", color: "#EF4444" };
        return { score: display, label: "仇恨", emoji: "☠️", color: "#7F1D1D" };
    },

    fmtMoney(num) {
        return Utils.normalizeNumber(num, 0).toLocaleString('en-US', { minimumFractionDigits: 0, maximumFractionDigits: 0 });
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
        return Utils.escapeHtml(String(value ?? "").replace(/\[\[([^\]|]+)(?:\|([^\]]+))?\]\]/g, (_, path, display) => display || path));
    },

    renderCreditText(currentDebt, creditLimit) {
        if (!creditLimit || creditLimit === 0) return `<span class="text-muted">-</span>`;
        const usage = Math.abs(currentDebt);
        const percentage = Math.min(100, Math.max(0, (usage / creditLimit) * 100));
        let colorClass = "text-muted";
        if (percentage > 30) colorClass = "text-warning";
        if (percentage > 80) colorClass = "text-danger";
        return `<span class="${colorClass}" style="font-family:monospace;">${percentage.toFixed(0)}%</span>`;
    },

    exportToCSV(data, filename = "data.csv") {
        if (!data || !data.length) {
            new Notice("数据为空，无法导出");
            return;
        }
        const headers = ["描述", "日期", "金额", "标签", "路径"];
        const csvContent = [
            headers.join(","),
            ...data.map(t => {
                const dateStr = new Date(t.time).toISOString().split('T')[0];
                const row = [
                    `"${String(t.text).replace(/"/g, '""')}"`,
                    dateStr,
                    t.value,
                    `"${t.tags.join(';')}"`,
                    `"${t.path}"`
                ];
                return row.join(",");
            })
        ].join("\n");

        const blob = new Blob([csvContent], { type: "text/csv;charset=utf-8;" });
        const link = document.createElement("a");
        link.href = URL.createObjectURL(blob);
        link.download = filename;
        document.body.appendChild(link);
        link.click();
        document.body.removeChild(link);
    },

    /**
     * 从混合字符串中提取 LIFE:天数（支持与 BILL:/MULTI: 混写）
     * @param {string} str - 待解析文本 (如 "BILL:2025-11-25; LIFE:1000; MULTI:3")
     * @returns {{ days: number, remaining: string } | null}
     */
    parseLivingCost(str) {
        if (!str || typeof str !== 'string') return null;
        // 支持 LIFE:天数、LIFE:天数@@（寿终正寝）、LIFE:天数@实际天数、LIFE:天数@日期
        const match = str.match(/LIFE:(\d+)(@@|@([^\s;,，]*))?/i);
        if (!match) return null;
        const days = Number(match[1]);
        if (days <= 0) return null;

        let actualDays = null;
        let retiredDate = null;
        let isLifetime = false;
        const atSuffix = match[2]; // '@@', '@90', '@2026-12-31', or undefined

        if (atSuffix) {
            if (atSuffix === '@@') {
                isLifetime = true;
                actualDays = days; // 寿终正寝：实际 = 预期
            } else {
                const atValue = match[3]; // @ 后的内容
                if (/^\d+$/.test(atValue)) {
                    actualDays = Number(atValue); // 纯数字 → 实际服役天数
                } else if (atValue) {
                    // 含 - 或 / → 日期，复用 resolveDateValue + YYYYMMDD 预处理
                    let dateStr = atValue.trim();
                    if (/^\d{8}$/.test(dateStr)) {
                        dateStr = `${dateStr.slice(0, 4)}-${dateStr.slice(4, 6)}-${dateStr.slice(6, 8)}`;
                    } else {
                        dateStr = dateStr.replace(/\//g, '-');
                    }
                    const d = this.resolveDateValue(dateStr);
                    if (d) { retiredDate = d; retiredDate.setHours(0, 0, 0, 0); }
                }
            }
        }

        // 移除完整匹配（含 @ 后缀）并清理分隔符
        const fullMatch = match[0];
        const escaped = fullMatch.replace(/[.*+?^${}()|[\]\\]/g, '\\$&');
        const remaining = str
            .replace(new RegExp(`[;,，\\s]*${escaped}[;,，\\s]*`, 'i'), ';')
            .replace(/^[;,，\s]+|[;,，\s]+$/g, '')
            .trim();
        return { days, actualDays, retiredDate, isLifetime, remaining };
    },

    parseFinanceDirectives(str) {
        let remaining = (str || "").toString();
        const result = {
            billingDate: null,
            billingDateRaw: null,
            lifeDays: 0,
            actualDays: null,   // 新增：@ 后的实际天数
            retiredDate: null,  // 新增：@ 后的退役日期
            isLifetime: false,  // 新增：@@ 寿终正寝标记
            multiRule: null,
            remaining: "",
        };

        const billMatch = remaining.match(/BILL:\s*([\d-]{8,10})\s*(?:[;,，\s]+|$)/i);
        if (billMatch) {
            const bd = new Date(billMatch[1]);
            if (!isNaN(bd.getTime())) {
                result.billingDate = bd;
                result.billingDateRaw = billMatch[1];
            }
            remaining = remaining.replace(billMatch[0], "").trim();
        }

        const lc = this.parseLivingCost(remaining);
        if (lc) {
            result.lifeDays = lc.days;
            result.actualDays = lc.actualDays;
            result.retiredDate = lc.retiredDate;
            result.isLifetime = lc.isLifetime;
            remaining = lc.remaining;
        }

        const multiIndex = remaining.toUpperCase().indexOf("MULTI:");
        if (multiIndex >= 0) {
            result.multiRule = remaining.substring(multiIndex).split(/\s+/)[0].replace(/[;,，]+$/, "");
            remaining = remaining.replace(result.multiRule, "").trim();
        }

        result.remaining = remaining.replace(/^[;,，\s]+|[;,，\s]+$/g, "").trim();
        return result;
    },

    /**
     * 解析页面 Frontmatter 为标准化三维向量（Core 层统一入口）
     *
     * 核心职责：
     * 1. 结构化笔记检测（值/标签/关联 三要素）
     * 2. 空值安全处理（null/undefined → [0,0,0]）
     * 3. 记账/事件分类及全零过滤（记账类全零返回 null，事件类保留）
     *
     * @param {Object} page - Dataview page 对象
     * @returns {{ vec: number[], isJournal: boolean, tags: string[] } | null}
     *   返回 null 表示不应纳入（非结构化笔记 或 记账类全零）
     */
    parseWalletProtocol(textOrEntry) {
        let sourceText = "";
        let legacyMeta = null;

        if (textOrEntry && typeof textOrEntry === "object") {
            legacyMeta = textOrEntry.meta || null;
            sourceText = legacyMeta?.sourceText
                || [textOrEntry.rawText, legacyMeta?.info].filter(Boolean).join(" ");
        } else {
            sourceText = String(textOrEntry || "");
        }

        const directives = this.parseFinanceDirectives(sourceText);
        let remainingText = directives.remaining || "";
        const repaymentMatches = remainingText.match(/\d{4}(?:[-/]?\d{2}){2}@[-+]?\d+(?:\.\d+)?/g) || [];
        const repaymentRaw = repaymentMatches.length > 0 ? repaymentMatches.join("#") : null;
        if (repaymentMatches.length > 0) {
            for (const match of repaymentMatches) remainingText = remainingText.replace(match, "");
        }
        remainingText = remainingText.replace(/#+/g, "#")
            .replace(/^[#;,锛孿\s]+|[#;,锛孿\s]+$/g, "")
            .trim();

        const lifeDays = directives.lifeDays || legacyMeta?.lifeDays || 0;
        const retiredDate = directives.retiredDate || legacyMeta?.retiredDate || null;
        const installmentRaw = directives.multiRule || legacyMeta?.multiRule || null;
        return {
            raw: sourceText,
            billingDate: directives.billingDateRaw || (directives.billingDate ? this.formatDateKey(directives.billingDate) : null),
            billingDateObj: directives.billingDate || null,
            life: {
                days: lifeDays,
                actualDays: directives.actualDays ?? legacyMeta?.actualDays ?? null,
                retiredDate,
                isLifetime: directives.isLifetime || legacyMeta?.isLifetime || false,
            },
            installment: installmentRaw ? { raw: installmentRaw, mode: "installment" } : null,
            repayment: repaymentRaw ? { raw: repaymentRaw, records: this.parseRepaymentString(repaymentRaw) } : null,
            remainingText,
            directives,
        };
    },

    parseFrontmatterValue(page) {
        if (!page || !page.file) return null;
        const fm = page.file.frontmatter;
        if (!fm) return null;

        const fmVal = fm[CONFIG.frontmatterKeys.transactionValue];
        const isStructured = fmVal !== undefined || fm["标签"] !== undefined || fm["关联"] !== undefined || fm["项目"] !== undefined;
        if (!isStructured) return null;

        const tags = page.file.tags || [];
        const isJournal = tags.includes(`#${CONFIG.journalTag}`);
        const vec = (fmVal != null) ? this.parseValue(fmVal, isJournal) : [0, 0, 0];

        // 记账类全零无意义 → null；事件类保留（存在性有意义）
        if (isJournal && vec[0] === 0 && vec[1] === 0 && vec[2] === 0) return null;

        return { vec, isJournal, tags };
    },

    _normalizeFieldArray(value) {
        return this.normalizeArrayField(value);
    },

    _normalizeTagsFromPage(page) {
        const fm = page?.file?.frontmatter || {};
        return [
            ...(page?.file?.tags || []),
            ...this._normalizeFieldArray(page?.tags),
            ...this._normalizeFieldArray(fm.tags),
            ...this._normalizeFieldArray(fm["标签"]),
        ].map(t => String(t).replace(/^#/, "")).filter(Boolean);
    },

    _hasExplicitObjectType(page) {
        const fm = page?.file?.frontmatter || {};
        return this._normalizeFieldArray(fm[CONFIG.frontmatterKeys.typeTags]).length > 0
            || this._normalizeFieldArray(fm[CONFIG.frontmatterKeys.englishType]).length > 0;
    },

    _normalizeExplicitTypes(page) {
        const fm = page?.file?.frontmatter || {};
        return [
            ...this._normalizeFieldArray(fm[CONFIG.frontmatterKeys.typeTags]),
            ...this._normalizeFieldArray(fm[CONFIG.frontmatterKeys.englishType]),
        ].map(item => String(item || "").replace(/^#/, "").trim()).filter(Boolean);
    },

    _canonicalObjectType(rawType) {
        const type = String(rawType || "").replace(/^#/, "").trim();
        if (!type) return "";
        if (["信用卡", "储蓄卡", "现金", "平台账户"].includes(type)) return "钱包";
        if (type === "人") return "人物";
        if (type === "template") return "模板";
        return type;
    },

    resolveObjectType(page) {
        if (!page?.file) return "note";
        const explicit = this._normalizeExplicitTypes(page).map(type => this._canonicalObjectType(type));
        if (explicit.length > 0) return explicit[0];

        const fm = page.file.frontmatter || {};
        const tags = this._normalizeTagsFromPage(page);
        const objectTags = this.normalizeSupertagInput(fm.tags);
        if (tags.includes(CONFIG.journalTag)) return "记账";
        if (objectTags.includes("钱包")) return "钱包";
        if (objectTags.includes("人物") || objectTags.includes("人")) return "人物";
        if (objectTags.includes("项目")) return "项目";
        if (objectTags.includes("日记")) return "日记";
        if (objectTags.includes("事件")) return "事件";
        if (objectTags.includes("模板")) return "模板";
        const legacyType = this._legacyTypeFromPath(page.file.path, { kind: "legacy-resolve-object-type" });
        if (legacyType) return legacyType;
        return "笔记";
    },

    isStructuredObjectPage(page) {
        if (!page?.file) return false;
        if (this.hasObjectSupertag(page, ["钱包", "人物", "人", "项目", "日记", "事件"])) return true;
        if (this._hasExplicitObjectType(page)) return true;
        const fm = page.file.frontmatter || {};
        const hasStructuredFields = fm[CONFIG.frontmatterKeys.transactionValue] !== undefined
            || fm[CONFIG.frontmatterKeys.relatedLinks] !== undefined
            || fm[CONFIG.frontmatterKeys.projectLinks] !== undefined
            || fm[CONFIG.frontmatterKeys.serviceDays] !== undefined
            || fm["标签"] !== undefined;
        if (hasStructuredFields) return true;
        const hasContentSignals = (page.file.outlinks || []).length > 0 || (page.file.lists || []).length > 0;
        const legacyType = this._legacyTypeFromPath(page.file.path, { kind: "legacy-structured-object-page", warn: false });
        if (legacyType && hasContentSignals) {
            const legacyKey = {
                "日记": "diary",
                "事件": "events",
                "记账": "ledgers",
                "钱包": "wallets",
                "人物": "persons",
                "项目": "projects",
                "模板": "templates",
            }[legacyType];
            this._recordLegacyPathWarning("legacy-structured-object-page", page, CONFIG.legacyPaths[legacyKey]);
            return true;
        }
        return false;
    },

    _makeAnomaly(type, severity, message, sourcePath, lineIndex, sampleText) {
        return {
            type,
            severity,
            message,
            sourcePath: sourcePath || "",
            lineIndex: Number.isFinite(Number(lineIndex)) ? Number(lineIndex) : -1,
            sampleText: String(sampleText || "").slice(0, 160),
        };
    },

    _pageHasWalletLink(entry) {
        for (const link of entry.meta?.outlinks || []) {
            if (this.isWalletLinkTarget(link)) return true;
        }
        return false;
    },

    _firstDisplayLink(entry) {
        return (entry.linksDetailed || []).find(link => link && link.role !== "source");
    },

    detectEntryAnomalies(entry) {
        const anomalies = [];
        if (!entry) return anomalies;
        const sourcePath = entry.sourcePath;
        const lineIndex = entry.lineIndex;
        const sampleText = entry.cleanText || entry.rawText || entry.sourcePage?.file?.name || "";

        if (entry.type === "journal") {
            const hasWalletLink = this._pageHasWalletLink(entry);
            if (!hasWalletLink) {
                anomalies.push(this._makeAnomaly(
                    "journal-missing-wallet",
                    "warning",
                    "#记账 条目缺少钱包链接，钱包视图无法归属该流水。",
                    sourcePath,
                    lineIndex,
                    sampleText
                ));
            }
            const firstLink = this._firstDisplayLink(entry);
            if (hasWalletLink && firstLink && firstLink.role !== "wallet") {
                anomalies.push(this._makeAnomaly(
                    "journal-first-link-not-wallet",
                    "warning",
                    "#记账 条目的第一链接不是钱包；建议改成 [[钱包|展示名]] 描述 [[业务对象]]。",
                    sourcePath,
                    lineIndex,
                    sampleText
                ));
            }
            const isTransfer = entry.meta?.tags?.includes("转账");
            if (!isTransfer && (entry.vector?.money || 0) === 0) {
                anomalies.push(this._makeAnomaly(
                    "journal-zero-money",
                    "info",
                    "非 #转账 记账条目的 money 为 0，请确认是否是占位或遗漏金额。",
                    sourcePath,
                    lineIndex,
                    sampleText
                ));
            }
        }
        if (entry.type === "event") {
            const sourceTargets = new Set((entry.meta?.sourceLinks || []).map(link => link.target));
            const objectLinks = (entry.meta?.outlinks || []).filter(link => !sourceTargets.has(link));
            const hasValue = (entry.vector?.time || 0) !== 0 || (entry.vector?.emotion || 0) !== 0 || (entry.vector?.money || 0) !== 0;
            if (hasValue && objectLinks.length === 0) {
                anomalies.push(this._makeAnomaly(
                    "event-missing-link",
                    "info",
                    "事件条目缺少对象链接，已按兼容规则保留。",
                    sourcePath,
                    lineIndex,
                    sampleText
                ));
            }
        }

        const dateInfo = this.resolveEntryDateInfo(entry);
        if (!dateInfo?.date) {
            anomalies.push(this._makeAnomaly(
                "entry-missing-date",
                "warning",
                "条目无法解析日期。",
                sourcePath,
                lineIndex,
                sampleText
            ));
        } else if (dateInfo.source === "ctime-fallback") {
            anomalies.push(this._makeAnomaly(
                "ctime-fallback",
                "info",
                "条目日期回退到文件 ctime；建议为结构化文件补充 创建时间。",
                sourcePath,
                lineIndex,
                sampleText
            ));
        }

        return anomalies;
    },

    inspectObjectQuality(options = {}) {
        const dataview = options.dv || (typeof dv !== "undefined" ? dv : null);
        const sampleSize = Number(options.sampleSize || 8);
        const pages = options.pages
            || (options.sources ? SourceResolver.resolve(options.sources) : SourceResolver.resolve({ allowGlobal: true, maxPages: options.maxPages || 1000 }));

        const buckets = new Map();
        const add = (type, severity, message, page, lineIndex = -1, sampleText = "") => {
            const anomaly = this._makeAnomaly(type, severity, message, page?.file?.path, lineIndex, sampleText || page?.file?.name);
            if (!buckets.has(type)) buckets.set(type, { type, count: 0, samples: [] });
            const bucket = buckets.get(type);
            bucket.count += 1;
            if (bucket.samples.length < sampleSize) bucket.samples.push(anomaly);
        };

        const hasRelation = page => {
            const fm = page.file.frontmatter || {};
            const relationFields = [
                fm[CONFIG.frontmatterKeys.relatedLinks],
                fm[CONFIG.frontmatterKeys.projectLinks],
                fm["链接日记"],
            ];
            if (relationFields.some(v => this._normalizeFieldArray(v).length > 0)) return true;
            return (page.file.outlinks || []).length > 0;
        };
        const hasWikiLink = page => {
            if ((page.file.outlinks || []).length > 0) return true;
            const fmText = JSON.stringify(page.file.frontmatter || {});
            if (fmText.includes("[[")) return true;
            return (page.file.lists || []).some(item => String(item.text || "").includes("[["));
        };
        const hasTags = page => this._normalizeTagsFromPage(page).length > 0;
        const hasCreationTime = page => Boolean(page.file.frontmatter?.[CONFIG.frontmatterKeys.creationTime] || page.file.day);
        const hasBlockId = text => /\^[A-Za-z0-9_-]+/.test(String(text || ""));

        for (const page of pages || []) {
            if (!this.isStructuredObjectPage(page)) continue;
            if (!this._hasExplicitObjectType(page)) add("object-missing-type", "info", "结构化对象缺少 类型/type 字段。", page);
            if (!hasTags(page)) add("object-missing-tags", "info", "对象文件缺少 tags/标签。", page);
            if (!hasCreationTime(page)) add("object-missing-created-time", "info", "对象文件缺少 创建时间。", page);
            if (!hasRelation(page)) add("object-missing-relation", "info", "结构化文件缺少 关联/项目/链接日记 或 wikilink。", page);
            if (!hasWikiLink(page)) add("object-without-wikilink", "info", "对象文件没有 wikilink。", page);

            for (const item of page.file.lists || []) {
                const tags = (item.tags || []).map(t => String(t).replace(/^#/, ""));
                const referenced = String(item.text || "").includes("[[") || tags.includes(CONFIG.journalTag);
                if (referenced && !hasBlockId(item.text)) {
                    add("referenced-list-missing-block-id", "info", "被视图引用的列表项没有 ^block-id。", page, item.line, item.text);
                }
            }
        }

        return {
            totalPages: (pages || []).length,
            checkedPages: (pages || []).filter(page => this.isStructuredObjectPage(page)).length,
            buckets: Array.from(buckets.values()).sort((a, b) => b.count - a.count),
        };
    },

    collectLivingCostItems(options = {}) {
        const dataview = options.dv || (typeof dv !== "undefined" ? dv : null);
        const asOfDate = this.resolveDateValue(options.asOfDate) || new Date();
        asOfDate.setHours(0, 0, 0, 0);
        const filterTags = (options.filterTags || []).map(t => String(t).replace(/^#/, ""));
        const daysBetween = (a, b) => Math.floor((b.getTime() - a.getTime()) / (1000 * 60 * 60 * 24));
        const toDate = value => {
            const d = this.resolveDateValue(value);
            if (!d) return null;
            d.setHours(0, 0, 0, 0);
            return d;
        };
        const addDays = (date, days) => {
            if (!date || isNaN(date.getTime()) || !Number.isFinite(Number(days))) return null;
            const d = new Date(date);
            d.setDate(d.getDate() + Number(days));
            d.setHours(0, 0, 0, 0);
            return d;
        };
        const decorate = item => {
            const start = toDate(item.start);
            item.start = start || new Date(NaN);
            item.elapsed = start ? Math.max(0, daysBetween(start, asOfDate)) : 0;
            item.dailyCost = item.days > 0 ? item.cost / item.days : 0;

            // 退役信息处理（向前兼容：旧 item 无这些字段，默认 null/false）
            const rawActualDays = item.actualDays ?? null;
            const rawRetiredDate = toDate(item.retiredDate);
            item.isLifetime = item.isLifetime ?? false;

            // 从退役日期推算 actualDays，或从 actualDays 推算退役日期；最终用退役日期和 asOfDate 比对。
            let resolvedActualDays = rawActualDays;
            let resolvedRetiredDate = rawRetiredDate;
            if (rawRetiredDate && start && resolvedActualDays == null) {
                resolvedActualDays = Math.max(0, daysBetween(start, rawRetiredDate));
            }
            if (!resolvedRetiredDate && start && resolvedActualDays != null) {
                resolvedRetiredDate = addDays(start, resolvedActualDays);
            }

            const hasRetirementPlan = resolvedActualDays != null || !!resolvedRetiredDate;
            item.isRetired = hasRetirementPlan && (
                resolvedRetiredDate
                    ? resolvedRetiredDate.getTime() <= asOfDate.getTime()
                    : item.elapsed >= Number(resolvedActualDays)
            );
            item.actualDays = resolvedActualDays; // null=无声明; number=声明/结算的实际服役天数
            item.retiredDate = resolvedRetiredDate;
            item.hasRetirementPlan = hasRetirementPlan;
            item.progressBaseDays = hasRetirementPlan && resolvedActualDays > 0 ? resolvedActualDays : item.days;

            // isActive：无 @ → 永续服役（elapsed >= 1 即活跃，无天花板）
            //           有 @/@@ → 到退役日之前仍活跃，退役日当天起不活跃
            item.isActive = !item.isRetired && item.elapsed >= 1 && item.cost > 0 && item.days > 0;

            // 实际日均规则：
            //   在役且在预期范围内（elapsed ≤ days）：cost/days（与预期相同，尚未确定实际）
            //   在役且超出预期（elapsed > days）：cost/elapsed（实际摊薄，更低）
            //   声明退役计划（@/@@）：在退役日前按声明实际天数计，退役后作为最终结算值
            item.actualDailyCost = hasRetirementPlan
                ? (resolvedActualDays > 0 ? item.cost / resolvedActualDays : 0)
                : (item.elapsed > item.days
                    ? item.cost / item.elapsed   // 超预期：摊薄，低于预期单价
                    : item.dailyCost);           // 在预期内：与预期单价相同
            item.todayCost = item.isActive ? item.actualDailyCost : 0;

            // 进度：无退役计划时按预期天数；有退役计划时按声明实际服役天数
            const progressDays = item.isRetired ? resolvedActualDays : item.elapsed;
            item.progressDays = progressDays;
            item.progress = item.progressBaseDays > 0 ? progressDays / item.progressBaseDays : 0;
            // 剩余天数：有退役计划时是距离退役日；无退役计划时是距离预期天数
            item.remaining = item.isRetired ? 0 : (item.progressBaseDays - item.elapsed);
            const amortizedRate = hasRetirementPlan ? item.actualDailyCost : item.dailyCost;
            item.amortized = Math.min(progressDays, item.progressBaseDays) * amortizedRate;
            item.category = item.tags[0] || "未分类";
            return item;
        };

        const fileSources = options.sources
            || (options.sourceScope ? { scope: options.sourceScope, maxPages: options.maxPages } : null)
            || (options.scope ? { scope: options.scope, maxPages: options.maxPages } : null)
            || { allowGlobal: true, maxPages: options.maxPages || 1200 };
        const filePages = options.pages
            || (options.paths
                ? options.paths.map(p => dataview?.page(typeof p === "string" ? p : p.path)).filter(Boolean)
                : SourceResolver.resolve(fileSources));
        const fileItems = [];
        const fileItemPaths = new Set();
        for (const p of filePages || []) {
            const sd = Number(p?.[CONFIG.frontmatterKeys.serviceDays] ?? p?.file?.frontmatter?.[CONFIG.frontmatterKeys.serviceDays]);
            if (!sd || sd <= 0) continue;
            const fm = p.file.frontmatter || {};
            const cost = Math.abs(this.parseAmount(p[CONFIG.frontmatterKeys.transactionValue] ?? fm[CONFIG.frontmatterKeys.transactionValue]));
            const tags = this._normalizeFieldArray(p["标签"] ?? fm["标签"]).map(t => t.replace(/^#/, ""));
            const item = decorate({
                name: p.file.name,
                cost,
                days: sd,
                start: p[CONFIG.frontmatterKeys.creationTime] ?? fm[CONFIG.frontmatterKeys.creationTime] ?? p.file.ctime,
                tags,
                path: p.file.path,
                source: "file",
            });
            fileItems.push(item);
            fileItemPaths.add(item.path);
        }

        const inlineItems = [];
        if (options.includeWallets !== false) {
            const walletLinks = options.walletLinks
                || (dataview ? this.collectWalletPages({
                    dv: dataview,
                    scope: options.walletScope,
                    maxPages: options.maxPages || options.walletMaxPages,
                    walletTag: options.walletTag || "钱包",
                }).map(p => p.file.link || { path: p.file.path }) : []);
            const seenInline = new Set();
            for (const link of walletLinks || []) {
                const wallet = new Wallet(link);
                if (!wallet.name) continue;
                for (const t of wallet.transactions || []) {
                    if (!t.serviceDays || t.serviceDays <= 0) continue;
                    if (fileItemPaths.has(t.path)) continue;
                    const identity = `${t.path}:${t.text}:${t.serviceDays}:${t.value}`;
                    if (seenInline.has(identity)) continue;
                    seenInline.add(identity);
                    inlineItems.push(decorate({
                        name: t.displayText || t.text,
                        displayText: t.displayText || t.text,
                        displayParts: t.displayParts || [],
                        cost: Math.abs(t.value),
                        days: t.serviceDays,
                        start: t.billingDate || t.time,
                        tags: t.tags || [],
                        path: t.path,
                        sourcePath: t.path,
                        lineIndex: t.lineIndex ?? t.line ?? null,
                        blockId: t.blockId ?? null,
                        link: {
                            path: t.path,
                            blockId: t.blockId ?? null,
                            subpath: t.blockId ? null : (t.lineIndex ?? t.line ?? null)
                        },
                        source: "inline",
                        wallet: wallet.name,
                        walletPath: wallet.path,
                        billingDate: t.billingDate,
                        actualDays: t.actualDays ?? null,
                        retiredDate: t.retiredDate ?? null,
                        isLifetime: t.isLifetime ?? false,
                    }));
                }
            }
        }

        let items = [...fileItems, ...inlineItems];
        if (filterTags.length > 0) items = items.filter(i => i.tags.some(t => filterTags.includes(t)));
        return items.sort((a, b) => {
            if (a.isActive !== b.isActive) return a.isActive ? -1 : 1;
            return b.todayCost - a.todayCost;
        });
    },

    collectWalletBills(wallets = []) {
        const result = [];
        for (const wallet of this._asArray(wallets)) {
            const walletName = wallet?.name || "";
            const walletPath = wallet?.path || "";
            const transactions = Array.isArray(wallet?.transactions) ? wallet.transactions : [];
            for (const t of transactions) {
                const records = Array.isArray(t?.repaymentRecords) ? t.repaymentRecords : [];
                for (const r of records) {
                    const dateObj = this.resolveBillDate(r?.date);
                    result.push({
                        ...r,
                        date: dateObj ? this.formatDateKey(dateObj) : String(r?.date || ""),
                        dateObj,
                        value: this.normalizeNumber(r?.value, 0),
                        wallet: walletName,
                        path: walletPath,
                        sourcePath: t?.path || walletPath,
                        sourceLine: t?.lineIndex ?? t?.line ?? null,
                        lineIndex: t?.lineIndex ?? t?.line ?? null,
                        blockId: t?.blockId ?? null,
                        description: t?.displayText || t?.text || "",
                        displayText: t?.displayText || t?.text || "",
                        displayParts: t?.displayParts || [],
                        linksDetailed: t?.linksDetailed || [],
                        sourceLinks: t?.sourceLinks || [],
                        tags: this.normalizeArrayField(t?.tags).map(tag => tag.replace(/^#/, "")),
                    });
                }
            }
        }
        return result;
    },

    toWalletSummary(wallet, bills = [], today = new Date()) {
        const errors = [];
        const asOfDate = this.resolveBillDate(today) || this.resolveBillDate(new Date());
        const name = wallet?.name || "";
        const path = wallet?.path || "";
        if (!wallet) errors.push("wallet 对象为空");
        if (!name) errors.push("缺少钱包名称");

        const tags = this.normalizeArrayField(wallet?.tags).map(tag => tag.replace(/^#/, ""));
        const positiveBalance = this.normalizeNumber(wallet?.positiveBalance, 0);
        const debt = this.normalizeNumber(wallet?.debt, 0);
        const creditLimit = this.normalizeNumber(wallet?.creditLimit, 0);
        const transactions = Array.isArray(wallet?.transactions) ? wallet.transactions : [];
        if (wallet && !Array.isArray(wallet.transactions)) errors.push("transactions 不是数组，已按空数组处理");

        const walletBills = this._asArray(bills).filter(b => {
            if (!b) return false;
            if (path && b.path === path) return true;
            return name && b.wallet === name;
        });
        let futureInflow = 0;
        let futureOutflow = 0;
        for (const bill of walletBills) {
            const billDate = this.resolveBillDate(bill.dateObj || bill.date);
            if (!billDate) {
                errors.push(`无法解析账单日期: ${bill.description || bill.date || "未命名账单"}`);
                continue;
            }
            if (asOfDate && billDate >= asOfDate) {
                const value = this.normalizeNumber(bill.value, 0);
                if (value > 0) futureInflow += value;
                else if (value < 0) futureOutflow += value;
            }
        }

        const currentBalance = positiveBalance + debt;
        const netWorth = currentBalance;
        return {
            name: name || path || "未知钱包",
            path,
            tags,
            positiveBalance,
            debt,
            creditLimit,
            netWorth,
            currentBalance,
            availableCredit: creditLimit + currentBalance,
            futureInflow,
            futureOutflow,
            transactionCount: transactions.length,
            billCount: walletBills.length,
            errors: [...new Set(errors)],
        };
    },

    collectWalletSummaries(wallets = [], options = {}) {
        const walletList = this._asArray(wallets);
        const bills = options.bills || this.collectWalletBills(walletList);
        const today = options.today || new Date();
        return walletList.map(wallet => {
            try {
                return this.toWalletSummary(wallet, bills, today);
            } catch (error) {
                return {
                    name: wallet?.name || wallet?.path || "未知钱包",
                    path: wallet?.path || "",
                    tags: [],
                    positiveBalance: 0,
                    debt: 0,
                    creditLimit: 0,
                    netWorth: 0,
                    currentBalance: 0,
                    availableCredit: 0,
                    futureInflow: 0,
                    futureOutflow: 0,
                    transactionCount: 0,
                    billCount: 0,
                    errors: [error?.message || String(error)],
                };
            }
        });
    },

};

const ObjectSummary = {
    normalizeTarget(value) {
        return String(value || "").replace(/\.md$/, "");
    },

    defaultAliases(summary, page) {
        const file = page?.file || {};
        return [
            summary?.name,
            summary?.path,
            file.name,
            file.path,
            this.normalizeTarget(summary?.path || file.path),
        ].filter(Boolean);
    },

    buildOutlinkIndex(summaryMap, options = {}) {
        const index = new Map();
        const add = (candidate, name) => {
            const normalized = this.normalizeTarget(candidate);
            if (!normalized) return;
            if (!index.has(normalized)) index.set(normalized, new Set());
            index.get(normalized).add(name);
        };
        for (const [name, item] of summaryMap || []) {
            const aliases = typeof options.aliases === "function"
                ? options.aliases(item.summary, item.page)
                : this.defaultAliases(item.summary, item.page);
            for (const alias of aliases) add(alias, name);
        }
        return index;
    },

    findEntryTargets(entry, outlinkIndex, allowedNames, options = {}) {
        const allowed = allowedNames instanceof Set ? allowedNames : new Set(allowedNames || []);
        const matched = new Set();
        const addMatches = link => {
            const normalized = this.normalizeTarget(link);
            if (!normalized) return;
            const keys = [normalized, normalized.split("/").pop()].filter(Boolean);
            for (const key of keys) {
                const names = outlinkIndex.get(key);
                if (!names) continue;
                for (const name of names) {
                    if (allowed.has(name)) matched.add(name);
                }
            }
        };

        for (const link of entry?.meta?.outlinks || []) addMatches(link);
        if (matched.size === 0 && options.rawTextFallback !== false && entry?.rawText) {
            const linkRegex = /\[\[([^\]|]+)(?:\|[^\]]+)?\]\]/g;
            let match;
            while ((match = linkRegex.exec(entry.rawText)) !== null) addMatches(match[1]);
        }
        return matched;
    },

    collect(options = {}) {
        const pages = Utils._asArray(options.objectPages || options.pages);
        const summaryMap = new Map();
        const inlinkToObjects = new Map();

        for (const page of pages) {
            const file = page?.file || {};
            if (!file.name || !file.path) continue;
            const summary = typeof options.createSummary === "function"
                ? options.createSummary(page)
                : { name: file.name, path: file.path, link: file.link, page };
            const name = summary?.name || file.name;
            summary.name = name;
            summary.path = summary.path || file.path;
            summary.link = summary.link || file.link;
            summary.page = summary.page || page;
            summaryMap.set(name, { page, summary });

            for (const link of file.inlinks || []) {
                if (!link?.path) continue;
                if (!inlinkToObjects.has(link.path)) inlinkToObjects.set(link.path, new Set());
                inlinkToObjects.get(link.path).add(name);
            }
        }

        const sourcePaths = [...inlinkToObjects.keys()];
        const entries = options.entries || (
            sourcePaths.length > 0 && typeof options.Query === "function"
                ? options.Query().from({ paths: sourcePaths }).debug(true).execute()
                : []
        );
        const outlinkIndex = this.buildOutlinkIndex(summaryMap, options);
        const matches = new Map();

        for (const entry of entries || []) {
            const allowedNames = inlinkToObjects.get(entry?.sourcePath);
            if (!allowedNames) continue;
            const names = this.findEntryTargets(entry, outlinkIndex, allowedNames, options);
            if (!names.size) continue;
            matches.set(entry, names);
            for (const name of names) {
                const item = summaryMap.get(name);
                if (!item) continue;
                if (typeof options.accumulate === "function") {
                    options.accumulate(item.summary, entry, { name, page: item.page, names });
                }
            }
        }

        const summaries = [];
        for (const { summary, page } of summaryMap.values()) {
            summaries.push(typeof options.finalize === "function" ? options.finalize(summary, page) : summary);
        }

        return {
            summaries,
            summaryMap: new Map([...summaryMap].map(([name, item]) => [name, item.summary])),
            objectPages: pages,
            sourcePaths,
            inlinkToObjects,
            outlinkIndex,
            entries,
            matches,
        };
    },
};

// --- 3. 钱包类 (v10.1 Kernel) ---
class Wallet {
    static resetParseCache() {
        Wallet._listParseMetrics = { hits: 0, misses: 0, unifiedHits: 0 };
    }
    static getParseMetrics() {
        if (!Wallet._listParseMetrics) Wallet._listParseMetrics = { hits: 0, misses: 0, unifiedHits: 0 };
        return { ...Wallet._listParseMetrics, cachedPages: UnifiedParser._documentParseCache?.size || 0 };
    }
    static _standardEntriesToParsedItems(entries = []) {
        return entries
            .filter(entry => entry && entry.type === "journal" && entry.lineIndex >= 0)
            .map(entry => {
                const item = entry.meta?._originalItem || null;
                const info = item?.children?.[1]?.text || entry.meta?.info || "";
                const walletProtocol = Utils.parseWalletProtocol(entry);
                const directives = walletProtocol.directives || Utils.parseFinanceDirectives(`${entry.rawText || ""} ${info || ""}`);
                return {
                    type: "standard",
                    value: entry.vector?.money || 0,
                    text: entry.cleanText || "",
                    displayText: entry.displayText || entry.cleanText || "",
                    displayParts: entry.displayParts || [],
                    linksDetailed: entry.linksDetailed || [],
                    sourceLinks: entry.meta?.sourceLinks || [],
                    tags: entry.meta?.tags || [],
                    info,
                    protocol: { wallet: walletProtocol },
                    directives,
                    lifeDays: walletProtocol.life?.days || entry.meta?.lifeDays || directives.lifeDays || 0,
                    multiRule: walletProtocol.installment?.raw || entry.meta?.multiRule || directives.multiRule || null,
                    explicitDate: entry.meta?.explicitDate || null,
                    line: entry.lineIndex,
                    parentLine: entry.meta?.parentLine || null,
                    _item: item,
                    _entry: entry,
                };
            });
    }
    constructor(link) {
        this.link = link; this.path = link.path;
        const page = dv.page(this.path);
        if (!page || !page.file) return;
        const fm = page.file.frontmatter;
        this.name = page.file.name;
        this.repaymentTerms = fm[CONFIG.frontmatterKeys.repaymentTerms] || CONFIG.defaultRepaymentTerms;
        this.creditLimit = Utils.normalizeNumber(fm[CONFIG.frontmatterKeys.creditLimit], 0);
        this.tags = Utils.normalizeArrayField(fm[CONFIG.frontmatterKeys.typeTags]).map(t => t.replace(/^#/, ""));
        this.debt = 0; this.positiveBalance = 0; this.netWorth = 0; this.transactions = [];
        this.initialize();
    }
    initialize() { this.extractTransactions(); this.calculateAggregates(); }
    _getParsedListItems(src) {
        if (!Wallet._listParseMetrics) Wallet._listParseMetrics = { hits: 0, misses: 0, unifiedHits: 0 };
        const key = src.file.path;
        const unifiedCache = UnifiedParser._ensureDocumentCache();
        const unifiedCached = unifiedCache.get(key);
        const unifiedFingerprint = UnifiedParser._pageFingerprint(src);
        if (unifiedCached && unifiedCached.fingerprint === unifiedFingerprint) {
            Wallet._listParseMetrics.hits++;
            Wallet._listParseMetrics.unifiedHits++;
            return Wallet._standardEntriesToParsedItems(unifiedCached.entries);
        }

        const entries = UnifiedParser.parseDocument(src);
        Wallet._listParseMetrics.misses++;
        return Wallet._standardEntriesToParsedItems(entries);
    }
    _textTargetsWallet(text) {
        if (!text) return false;
        const walletBase = this.path.replace(/\.md$/, "");
        return text.includes(`[[${this.name}`) || text.includes(`[[${walletBase}`) || text.includes(`[[${this.path}`);
    }
    _parsedTargetsWallet(parsed, lists) {
        if (this._textTargetsWallet(parsed._item?.text)) return true;
        const lineIndex = new Map();
        for (const item of lists || []) lineIndex.set(item.line, item);
        let parentLine = parsed.parentLine;
        while (parentLine) {
            const parent = lineIndex.get(parentLine);
            if (!parent) break;
            if (this._textTargetsWallet(parent.text)) return true;
            parentLine = parent.parent;
        }
        return false;
    }
    _buildTransaction(parsed, src) {
        if (isNaN(parsed.value)) return null;
        const directiveSource = `${parsed.text || ""} ${parsed.info || ""}`.trim();
        const walletProtocol = parsed.protocol?.wallet || Utils.parseWalletProtocol(directiveSource);
        const directives = walletProtocol.directives || parsed.directives || Utils.parseFinanceDirectives(directiveSource);
        const cleanDirectiveText = (text) => (text || "")
            .replace(/BILL:\s*[\d-]{8,10}/ig, "")
            .replace(/LIFE:\d+(?:@@|@[^\s;,，]*)?/ig, "") // 含 @ 后缀
            .replace(/MULTI:[^\s;,，]+/ig, "")
            .replace(/^[;,，\s]+|[;,，\s]+$/g, "")
            .trim();

        const rawDisplayParts = parsed.displayParts || [];
        const displayParts = Utils.filterPartsForView(rawDisplayParts, { path: this.path, name: this.name, frontmatter: { tags: ["钱包"] } });
        const displayText = Utils._partsToText(displayParts, rawDisplayParts.length > 0 ? (parsed.text || "") : (parsed.displayText || parsed.text || ""));

        const t = {
            value: parsed.value,
            text: cleanDirectiveText(parsed.text) || src.file.name,
            displayText: cleanDirectiveText(displayText) || src.file.name,
            displayParts,
            linksDetailed: parsed.linksDetailed || [],
            sourceLinks: parsed.sourceLinks || [],
            path: src.file.path,
            line: parsed.line,
            lineIndex: parsed.line,
            parentLine: parsed.parentLine ?? null,
            blockId: parsed._entry?.meta?.blockId || null,
            tags: parsed.tags,
            time: src.file.frontmatter[CONFIG.frontmatterKeys.creationTime] || src.file.ctime,
            billingDate: src.file.frontmatter[CONFIG.frontmatterKeys.billingDate],
            serviceDays: 0,
            repaymentRecords: []
        };
        // BILL: 最高优先级 — 从内联 info 提取，覆盖 Frontmatter
        if (walletProtocol.billingDate) {
            t.billingDate = walletProtocol.billingDate;
            t.time = walletProtocol.billingDate;
        }
        if (walletProtocol.life?.days > 0 || parsed.lifeDays > 0) t.serviceDays = walletProtocol.life?.days || parsed.lifeDays;
        t.actualDays = walletProtocol.life?.actualDays ?? null;
        t.retiredDate = walletProtocol.life?.retiredDate ?? null;
        t.isLifetime = walletProtocol.life?.isLifetime ?? false;
        const repayInfo = walletProtocol.installment?.raw || walletProtocol.repayment?.raw || walletProtocol.remainingText || directives.multiRule || directives.remaining;
        if (parsed.tags.includes("贷款")) t.repaymentRecords = this._detRepay(t, repayInfo);
        return t;
    }
    extractTransactions() {
        const incomingLinks = dv.page(this.path).file.inlinks.values;
        const all = [];
        for (const link of incomingLinks) {
            const src = dv.page(link.path);
            if (!src || !src.file) continue;
            if (src.file.lists) {
                const parsed = this._getParsedListItems(src);
                for (const p of parsed) {
                    if (!['standard', 'nested', 'inherited'].includes(p.type)) continue;
                    if (!this._parsedTargetsWallet(p, src.file.lists)) continue;
                    const t = this._buildTransaction(p, src);
                    if (t) all.push(t);
                }
            }
            const fmResult = Utils.parseFrontmatterValue(src);
            if (fmResult && fmResult.isJournal) {
                const t = this._parseFm(src);
                if (t && !all.some(x => x.text === t.text && x.time === t.time)) all.push(t);
            }
        }
        this.transactions = all;
    }
    _parseFm(src) {
        const fm = src.file.frontmatter;
        const rawVal = fm[CONFIG.frontmatterKeys.transactionValue];
        if (rawVal == null) return null;
        const val = Utils.parseAmount(rawVal);
        const tags = Utils.normalizeArrayField(fm["标签"]).map(t => t.replace(/#/g, ""));
        const t = { value: Number(val), text: src.file.name, path: src.file.path, tags: tags, time: fm[CONFIG.frontmatterKeys.creationTime], billingDate: fm[CONFIG.frontmatterKeys.billingDate], repaymentRecords: [] };
        // 入账日覆盖交易时间（与内联 BILL: 一致）
        if (t.billingDate) t.time = t.billingDate;
        // 生活成本: Frontmatter 服役天数
        const sd = Number(fm[CONFIG.frontmatterKeys.serviceDays]);
        if (sd > 0) t.serviceDays = sd;
        const walletProtocol = Utils.parseWalletProtocol(fm[CONFIG.frontmatterKeys.repaymentInfo] || "");
        const directives = walletProtocol.directives || Utils.parseFinanceDirectives(fm[CONFIG.frontmatterKeys.repaymentInfo] || "");
        if (walletProtocol.billingDate) {
            t.billingDate = walletProtocol.billingDate;
            t.time = walletProtocol.billingDate;
        }
        if (walletProtocol.life?.days > 0) t.serviceDays = walletProtocol.life.days;
        if (walletProtocol.life?.actualDays != null) t.actualDays = walletProtocol.life.actualDays;
        if (walletProtocol.life?.retiredDate) t.retiredDate = walletProtocol.life.retiredDate;
        if (walletProtocol.life?.isLifetime) t.isLifetime = walletProtocol.life.isLifetime;
        const repayInfo = walletProtocol.installment?.raw || walletProtocol.repayment?.raw || walletProtocol.remainingText || directives.multiRule || directives.remaining;
        if (tags.includes("贷款")) t.repaymentRecords = this._detRepay(t, repayInfo);
        return t;
    }
    _detRepay(t, info) {
        let calcDate = t.billingDate || t.time;
        if (info && typeof info === 'string') {
            const billRegex = /BILL:\s*([\d-]{8,10})\s*(?:;|，|,|\s+|$)/i;
            const match = info.match(billRegex);
            if (match) {
                const dStr = match[1];
                const d = new Date(dStr);
                if (!isNaN(d.getTime())) { calcDate = dStr; }
                info = info.replace(match[0], "").trim();
            }
        }
        const next = Utils.calculateNextRepaymentDate(calcDate, this.repaymentTerms[0], this.repaymentTerms[1], this.repaymentTerms[2]);
        if (info && info.toUpperCase().startsWith('MULTI:')) {
            const inst = Utils.parseInstallmentString(info, next, t.value);
            if (inst) return inst;
        }
        const repaymentSchedule = String(info || "")
            .match(/\d{4}(?:[-/]?\d{2}){2}@[-+]?\d+(?:\.\d+)?/g);
        let recs = Utils.parseRepaymentString(repaymentSchedule?.join("#") || info);
        recs = recs.filter(r => r.date && r.value != 0);
        if (recs.length > 0) return recs;
        return t.value < 0 ? [{ date: next, value: t.value }] : [{ date: t.time, value: t.value }];
    }
    calculateAggregates() {
        const today = new Date(); today.setHours(0, 0, 0, 0);

        const toJSDate = (d) => {
            if (!d) return null;
            if (d instanceof Date) return d;
            if (typeof d === 'object' && d.ts) return new Date(d.ts);
            return new Date(d);
        };

        this.positiveBalance = this.transactions.filter(t => !t.repaymentRecords?.length).reduce((s, t) => s + t.value, 0);

        this.debt = this.transactions.filter(t => t.repaymentRecords?.length > 0).reduce((s, t) => {
            // Include ALL transactions in debt calculation, even future ones, to reflect true net position
            return s + t.value;
        }, 0);

        // Net Worth includes ALL assets (even future ones)
        this.netWorth = this.transactions.reduce((s, t) => s + t.value, 0) + this.creditLimit;
    }
    toSummary(today = new Date(), bills = null) {
        const walletBills = bills || Utils.collectWalletBills([this]);
        return Utils.toWalletSummary(this, walletBills, today);
    }
}

// --- 4. 数据管线 (Query Pipeline) ---
class StandardEntry {
    constructor(sourcePage, lineIndex, rawText) {
        this.sourcePath = sourcePage.file.path;
        this.sourcePage = sourcePage;
        this.lineIndex = lineIndex;
        this.type = 'event';
        this.rawText = rawText || "";
        this.cleanText = "";
        this.linksDetailed = [];
        this.displayParts = [];
        this.displayText = "";
        this.vector = { money: 0, emotion: 0, time: 0 };
        this.meta = {
            tags: [],
            outlinks: [],
            sourceLinks: [],
            explicitDate: null,
            lifeDays: 0,
            actualDays: null,
            retiredDate: null,
            isLifetime: false,
            multiRule: null,
            info: "",
            sourceText: "",
            displayRawText: "",
            anomalies: [],
            blockId: null,
            parentLine: null,
            _originalItem: null
        };
    }
}

class UnifiedParser {
    static _ensureDocumentCache() {
        if (!UnifiedParser._documentParseCache) UnifiedParser._documentParseCache = new Map();
        return UnifiedParser._documentParseCache;
    }

    static _pageFingerprint(page) {
        let frontmatterFingerprint = "";
        try {
            frontmatterFingerprint = JSON.stringify(page.file.frontmatter || {});
        } catch (e) {
            frontmatterFingerprint = "";
        }
        return [
            page.file.path,
            page.file.mtime?.ts || page.file.ctime?.ts || 0,
            page.file.lists?.length || 0,
            frontmatterFingerprint,
        ].join("|");
    }

    static parseDocument(page, options = {}) {
        if (!page || !page.file) return [];
        const cache = options.cache || UnifiedParser._ensureDocumentCache();
        const cacheKey = page.file.path;
        const fingerprint = this._pageFingerprint(page);
        const cached = cache.get(cacheKey);
        if (cached && cached.fingerprint === fingerprint) {
            if (options.metrics) options.metrics.cacheHits = (options.metrics.cacheHits || 0) + 1;
            return cached.entries;
        }
        if (options.metrics) options.metrics.cacheMisses = (options.metrics.cacheMisses || 0) + 1;

        let results = [];
        const fm = page.file.frontmatter;

        // 1. Frontmatter 虚拟项 — 使用 Core 层统一检测
        const fmResult = Utils.parseFrontmatterValue(page);
        if (fmResult) {
            const tags = (fm["标签"] || []).map(t => t.replace(/#/g, ""));
            const entry = new StandardEntry(page, -1, page.file.name);
            entry.cleanText = page.file.name;
            entry.meta.tags = tags;
            const addRelatedLinks = raw => {
                const relatedItems = Array.isArray(raw) ? raw : [raw];
                for (const rel of relatedItems) {
                    if (!rel) continue;
                    if (typeof rel === 'string') {
                        const m = rel.match(/\[\[([^\]|]+)(?:\|[^\]]+)?\]\]/);
                        entry.meta.outlinks.push((m ? m[1] : rel).trim());
                    } else if (rel.path) {
                        entry.meta.outlinks.push(rel.path.replace(/\.md$/, ""));
                    }
                }
            };
            addRelatedLinks(fm[CONFIG.frontmatterKeys.relatedLinks] || fm["关联"] || []);
            addRelatedLinks(fm[CONFIG.frontmatterKeys.projectLinks] || fm["项目"] || []);

            const cTime = fm[CONFIG.frontmatterKeys.creationTime] || (page.file.day ? page.file.day.toFormat('yyyy-MM-dd') : page.file.ctime.toFormat('yyyy-MM-dd'));
            entry.meta.explicitDate = new Date(cTime);

            entry.type = fmResult.isJournal ? 'journal' : 'event';

            entry.vector.money = fmResult.isJournal ? fmResult.vec[0] : 0;
            entry.vector.emotion = fmResult.vec[1] || 0;
            entry.vector.time = fmResult.vec[2] || 0;

            const multi = fm[CONFIG.frontmatterKeys.repaymentInfo];
            if (multi && typeof multi === 'string') {
                entry.meta.info = Utils.parseWalletProtocol(multi).remainingText || multi;
            }

            Utils.decorateEntryDisplay(entry);
            results.push(entry);
        }

        // 2. 解析列表项
        if (page.file.lists && page.file.lists.length > 0) {
            results.push(...this.parseListItems(page.file.lists, page));
        }

        cache.set(cacheKey, { fingerprint, entries: results });
        return results;
    }

    static parseListItems(lists, page) {
        const lineIndexMap = new Map();
        for (const item of lists) lineIndexMap.set(item.line, item);

        const results = [];
        const journalTag = `#${CONFIG.journalTag}`;
        const normalizedJournalTag = journalTag.replace(/#/g, "");
        const normalizeText = value => String(value || "").replace(/\s+/g, " ").trim();
        const blockIdRegex = /(?:^|\s)\^([A-Za-z0-9_-]+)\b/;
        const stripBlockId = value => normalizeText(String(value || "").replace(/(?:^|\s)\^[A-Za-z0-9_-]+\b/g, " "));
        const extractBlockId = value => {
            const match = String(value || "").match(blockIdRegex);
            return match ? match[1] : null;
        };
        const unique = values => {
            const seen = new Set();
            const out = [];
            for (const value of values || []) {
                const clean = String(value || "").trim();
                if (!clean || seen.has(clean)) continue;
                seen.add(clean);
                out.push(clean);
            }
            return out;
        };
        const stripDirectives = text => String(text || "")
            .replace(/SOURCE:\s*\[\[[^\]]+\]\]/ig, " ")
            .replace(/BILL:\s*[\d-]{8,10}/ig, " ")
            .replace(/LIFE:\d+(?:@@|@[^\s;,，]*)?/ig, " ")
            .replace(/MULTI:[^\s;,，]+/ig, " ")
            .replace(/@\d{4}[-/]?\d{2}[-/]?\d{2}|@\d{8}/g, " ");
        const inlineTagRegex = /#[^\s#[\]]+/g;
        const stripInlineTags = text => String(text || "").replace(inlineTagRegex, " ");
        const normalizeInlineTag = tag => String(tag || "")
            .replace(/^#/, "")
            .replace(/\[\[[\s\S]*$/, "")
            .replace(/[\[\]]/g, "")
            .trim();
        const cleanDirectiveText = text => normalizeText(String(text || "")
            .replace(/BILL:\s*[\d-]{8,10}/ig, "")
            .replace(/LIFE:\d+(?:@@|@[^\s;,，]*)?/ig, "") // 含 @ 后缀
            .replace(/MULTI:[^\s;,，]+/ig, "")
            .replace(/SOURCE:\s*\[\[[^\]]+\]\]/ig, "")
            .replace(/@\d{4}[-/]?\d{2}[-/]?\d{2}/g, "")
            .replace(/^[;,，\s]+|[;,，\s]+$/g, ""));
        const extractTags = candidate => unique([
            ...(candidate?.tags || []),
            ...Array.from(String(candidate?.text || "").matchAll(inlineTagRegex)).map(match => match[0]),
        ].map(normalizeInlineTag).filter(Boolean));
        const sourceIndexSet = text => new Set(Utils.parseSourceLinks(text || "").map(link => link.index));
        const linkDetails = text => {
            const sourceIndexes = sourceIndexSet(text);
            return Utils.parseWikiLinks(text || "").map(link => ({
                ...link,
                target: String(link.target || "").trim(),
                isSource: sourceIndexes.has(link.index),
            })).filter(link => link.target);
        };
        const objectLinkDetails = text => linkDetails(text).filter(link => !link.isSource);
        const sourceLinkDetails = text => Utils.parseSourceLinks(text || "");
        const linkTargets = links => unique((links || []).map(link => link.target));
        const itemDisplayRaw = candidate => cleanDirectiveText(stripInlineTags(stripDirectives(stripBlockId(candidate?.text || ""))));
        const itemSemanticText = candidate => normalizeText(stripInlineTags(stripDirectives(stripBlockId(candidate?.text || "")))
            .replace(/\[\[[^\]|]+(?:\|[^\]]+)?\]\]/g, " ")
            .replace(/[\[\]]/g, " "));
        const linkLabelText = text => normalizeText(stripInlineTags(stripDirectives(stripBlockId(text || "")))
            .replace(/\[\[([^\]|]+)(?:\|([^\]]+))?\]\]/g, (_, target, alias) => alias || String(target).split(/[\\/]/).pop())
            .replace(/[\[\]]/g, " "));
        const looksLikeValue = text => {
            let clean = stripInlineTags(stripDirectives(text || ""))
                .replace(/\[\[.*?\]\]/g, "")
                .replace(/[\[\]]/g, "")
                .trim();
            if (!clean) return false;
            clean = clean.replace(/^[（(]\s*|\s*[）)]$/g, "").trim();
            return /^-?\d+(?:\.\d+)?(?:\s*[,，\s]\s*-?\d+(?:\.\d+)?){0,2}$/.test(clean);
        };
        const firstValueChild = candidate => (candidate?.children || []).find(child => child && looksLikeValue(child.text));
        const hasDirectValue = candidate => Boolean(firstValueChild(candidate));
        const contextAncestors = candidate => {
            const ancestors = [];
            let parentLine = candidate?.parent;
            while (parentLine != null) {
                const parentItem = lineIndexMap.get(parentLine);
                if (!parentItem || hasDirectValue(parentItem)) break;
                ancestors.push(parentItem);
                parentLine = parentItem.parent;
            }
            return ancestors;
        };
        const nearestContextLinks = (ancestors, ownLinks) => {
            if (ownLinks.length > 0) return [];
            for (const ancestor of ancestors) {
                const links = objectLinkDetails(ancestor.text);
                if (links.length > 0) return links;
            }
            return [];
        };
        const nearestPlainAncestorDescription = ancestors => {
            for (const ancestor of ancestors) {
                if (objectLinkDetails(ancestor.text).length > 0) return "";
                // 区分两种父节点：
                //   分组容器：子项中有"完整子事件"（有直接值的子项）→ 可提供 prefix（如「取旅游」）
                //   叙事容器：子项全是纯叶子文字（无任何直接值子项）→ 父自身被缺省，不传描述
                const nonValueChildren = (ancestor.children || []).filter(c => c && !looksLikeValue(c.text));
                if (nonValueChildren.length > 0 && !nonValueChildren.some(c => hasDirectValue(c))) return "";
                const desc = itemSemanticText(ancestor);
                if (desc) return desc;
            }
            return "";
        };
        const nearestAncestorBusinessDescription = ancestors => {
            for (const ancestor of ancestors) {
                const desc = itemSemanticText(ancestor);
                if (desc) return desc;
            }
            return "";
        };
        const childInfoRaw = (candidate, valueChild) => (candidate?.children || [])
            .filter(child => child && child.line !== valueChild?.line)
            .filter(child => !looksLikeValue(child.text))
            .filter(child => !firstValueChild(child))
            .map(child => child.text || "")
            .filter(Boolean)
            .join(" ");
        const sourceFileTarget = () => String(page?.file?.path || "").replace(/\.md$/i, "");
        const sourceFileLabel = () => page?.file?.name || sourceFileTarget().split(/[\\/]/).pop() || "";
        const sourceFileDisplayRaw = () => {
            const target = sourceFileTarget();
            const label = sourceFileLabel();
            return target ? `[[${target}|${label}]]` : label;
        };
        const makeDisplayRawText = (prefix, ownRaw, inheritedLinks) => normalizeText([
            prefix,
            ownRaw,
            ...(inheritedLinks || []).map(link => link.raw || `[[${link.target}]]`),
        ].filter(Boolean).join(" "));
        const infoCleanText = raw => cleanDirectiveText(linkLabelText(raw));
        const vectorHasValue = vector => (vector?.money || 0) !== 0 || (vector?.emotion || 0) !== 0 || (vector?.time || 0) !== 0;

        const buildEntry = item => {
            if (!item || looksLikeValue(item.text)) return null;
            const valueChild = firstValueChild(item);
            const explicitValue = Boolean(valueChild);

            const ancestors = contextAncestors(item);
            const ownObjectLinks = objectLinkDetails(item.text);
            const inheritedObjectLinks = nearestContextLinks(ancestors, ownObjectLinks);
            const objectLinks = unique([
                ...linkTargets(ownObjectLinks),
                ...linkTargets(inheritedObjectLinks),
            ]);
            const infoRaw = childInfoRaw(item, valueChild);
            const sourceLinks = unique([
                ...linkTargets(sourceLinkDetails(item.text)),
                ...linkTargets(sourceLinkDetails(infoRaw)),
            ]);
            const tags = unique([
                ...extractTags(item),
                ...ancestors.flatMap(ancestor => extractTags(ancestor)),
            ]);

            const ownSemantic = itemSemanticText(item);
            const ownLabel = linkLabelText(item.text);
            const journalContext = tags.includes(normalizedJournalTag);
            const isJournal = journalContext;
            const isJournalCandidate = candidate => extractTags(candidate).includes(normalizedJournalTag);
            // 「完整普通子事件」：有直接数值子项、且不是记账语境的非数值子项。
            // 记账子项可以挂在普通叙事父节点下，但不应阻止父节点自身缺省成普通事件。
            const hasCompleteEventChild = candidate => (candidate?.children || [])
                .some(c => c && !looksLikeValue(c.text) && hasDirectValue(c) && !isJournalCandidate(c));
            const hasEventLikeChildren = hasCompleteEventChild(item);
            // 值缺省只回答「这个候选原子没有显式值时，能不能补 0」。
            const valueDefaulted = !isJournal && !explicitValue && !hasEventLikeChildren;
            if (isJournal && !explicitValue) return null;
            if (!explicitValue && !valueDefaulted) return null;

            // 叙事型父节点抑制：若父节点有自身语义文字 + 无完整子事件，
            // 说明父节点是「叙事事件」，子项是其详情，不应作为独立事件产生条目。
            // 父先缺省（在父节点上），子项不拼不产生。
            if (!isJournal && !explicitValue && item.parent != null) {
                const parentItem = lineIndexMap.get(item.parent);
                if (parentItem) {
                    const parentSemantic = itemSemanticText(parentItem);
                    const parentHasEventLikeChildren = hasCompleteEventChild(parentItem);
                    if (parentSemantic && !parentHasEventLikeChildren) return null;
                }
            }

            let prefixDescription = "";
            let ownDescription = "";
            let displayPrefixDescription = "";
            const hasNonValueChildren = (item.children || []).some(child => child && !looksLikeValue(child.text));
            const linkOnlyTerminalAtom = !ownSemantic && objectLinks.length > 0 && !hasNonValueChildren;
            let descriptionDefaulted = false;
            if (isJournal) {
                const inheritedDescription = nearestAncestorBusinessDescription(ancestors);
                descriptionDefaulted = explicitValue && !ownSemantic && !inheritedDescription && objectLinks.length > 0;
                ownDescription = ownSemantic || inheritedDescription || (descriptionDefaulted ? sourceFileLabel() : "");
                displayPrefixDescription = ownSemantic ? "" : inheritedDescription;
            } else {
                prefixDescription = ownSemantic ? "" : nearestPlainAncestorDescription(ancestors);
                // 描述缺省只回答「已成原子的候选没有描述时，用不用源文件链接兜底」。
                descriptionDefaulted = !prefixDescription
                    && !ownSemantic
                    && objectLinks.length > 0
                    && (explicitValue || (valueDefaulted && linkOnlyTerminalAtom));
                if (ownSemantic) {
                    ownDescription = valueDefaulted && ownObjectLinks.length > 0 ? ownLabel : ownSemantic;
                } else if (descriptionDefaulted) {
                    ownDescription = sourceFileLabel();
                } else {
                    ownDescription = prefixDescription ? ownLabel : "";
                }
                displayPrefixDescription = prefixDescription;
            }

            const cleanText = cleanDirectiveText([prefixDescription, ownDescription].filter(Boolean).join(" "));
            if (!cleanText) return null;
            if (valueDefaulted && objectLinks.length === 0) return null;

            const entry = new StandardEntry(page, item.line, item.text);
            entry.meta.parentLine = item.parent;
            entry.meta._originalItem = item;
            entry.meta.blockId = extractBlockId(item.text);
            entry.meta.tags = tags;
            entry.meta.outlinks = unique([...objectLinks, ...sourceLinks]);
            entry.meta.sourceText = [item.text, infoRaw].filter(Boolean).join(" ");
            entry.meta.info = stripBlockId(infoCleanText(infoRaw));
            entry.meta.displayRawText = descriptionDefaulted
                ? sourceFileDisplayRaw()
                : makeDisplayRawText(displayPrefixDescription, itemDisplayRaw(item), inheritedObjectLinks);
            entry.meta.businessDescription = stripBlockId(cleanText);
            entry.cleanText = stripBlockId(cleanText).replace(/^[;,，\s]+|[;,，\s]+$/g, '').replace(/\s+/g, ' ');
            entry.type = isJournal ? 'journal' : 'event';

            const combinedForRegex = entry.meta.sourceText;
            const dateMatch = Utils.parseEventDate(combinedForRegex);
            if (dateMatch) entry.meta.explicitDate = dateMatch.date;

            const rawVal = valueChild?.text || "";
            const vec = explicitValue ? Utils.parseValue(rawVal, isJournal) : [0, 0, 0];
            if (isJournal) {
                entry.vector.money = vec[0];
                entry.vector.emotion = vec[1] || 0;
                entry.vector.time = 0;
            } else if (valueDefaulted) {
                entry.vector.money = 0;
                entry.vector.emotion = 0;
                entry.vector.time = 0;
                entry.meta.valueDefaulted = true;
            } else {
                entry.vector.money = 0;
                entry.vector.emotion = vec[1] || 0;
                entry.vector.time = vec[2] || 0;
            }
            if (!vectorHasValue(entry.vector) && !valueDefaulted) return null;
            if (entry.type === "event" && objectLinks.length === 0) entry.meta.anomalies.push("missing-link");

            Utils.decorateEntryDisplay(entry);
            return entry;
        };

        for (const item of lists) {
            const entry = buildEntry(item);
            if (entry) results.push(entry);
        }
        return results;
    }
}

class SourceResolver {
    static normalize(sources = {}) {
        if (!sources) return {};
        if (typeof sources === 'string') return { scope: sources };
        if (Array.isArray(sources)) return { paths: sources };

        const normalized = { ...sources };
        if (typeof normalized.scope === 'string') {
            normalized.scope = normalized.scope.trim().replace(/^"|"$/g, '');
        }
        if (normalized.path && !normalized.paths) normalized.paths = [normalized.path];
        if (normalized.paths && !Array.isArray(normalized.paths)) normalized.paths = [normalized.paths];
        if (normalized.excludeTag && !normalized.excludeTags) normalized.excludeTags = [normalized.excludeTag];
        if (normalized.excludeTags && !Array.isArray(normalized.excludeTags)) normalized.excludeTags = [normalized.excludeTags];
        return normalized;
    }

    static hasExplicitSource(sources) {
        return Boolean(
            sources.scope ||
            sources.current ||
            sources.currentAndLinkedDiary ||
            sources.linkedTo ||
            sources.linkedByTag ||
            (sources.pages && sources.pages.length > 0) ||
            (sources.paths && sources.paths.length > 0)
        );
    }

    static toArray(value) {
        if (!value) return [];
        if (Array.isArray(value)) return Array.from(value);
        if (value.values && Array.isArray(value.values)) return Array.from(value.values);
        return Array.from(value);
    }

    static resolve(sources) {
        return this.resolveWithMeta(sources).pages;
    }

    static _linkToPath(link) {
        if (!link) return null;
        if (typeof link === 'string') {
            const match = link.match(/\[\[([^\]|]+)(?:\|[^\]]+)?\]\]/);
            return (match ? match[1] : link).replace(/\.md$/, "");
        }
        if (link.path) return link.path;
        return null;
    }

    static _linkedDiaryPages(currentPage) {
        const pagesByPath = new Map();
        const addPage = page => {
            if (page?.file?.path) pagesByPath.set(page.file.path, page);
        };
        addPage(currentPage);
        if (!currentPage?.file) return Array.from(pagesByPath.values());

        const rawLinks = currentPage["链接日记"] || currentPage.file.frontmatter?.["链接日记"] || [];
        const linkItems = Array.isArray(rawLinks) ? rawLinks : [rawLinks];
        for (const link of linkItems) {
            const target = this._linkToPath(link);
            if (!target) continue;
            addPage(dv.page(target));
        }

        const currentCandidates = new Set([
            currentPage.file.path,
            currentPage.file.path.replace(/\.md$/, ""),
            currentPage.file.name,
        ]);
        for (const page of this.toArray(dv.pages())) {
            if (!page?.file || page.file.path === currentPage.file.path) continue;
            const related = page["链接日记"] || page.file.frontmatter?.["链接日记"] || [];
            const relatedItems = Array.isArray(related) ? related : [related];
            const hit = relatedItems.some(link => {
                const target = this._linkToPath(link);
                return target && currentCandidates.has(target.replace(/\.md$/, ""));
            });
            if (hit) addPage(page);
        }

        return Array.from(pagesByPath.values());
    }

    static resolveWithMeta(sources = {}) {
        const normalized = this.normalize(sources);
        const warnings = [];
        const hasSource = this.hasExplicitSource(normalized);
        const strictGuard = normalized.strictSourceGuard === true || normalized.sourceGuard === "strict";
        let linkedToResolvedAsBase = false;

        if (!hasSource && normalized.allowGlobal !== true) {
            if (strictGuard) {
                warnings.push("Source guard: Query has no explicit source. Strict mode returns an empty result; add from({ scope }), from({ linkedTo:true }) or allowGlobal:true.");
                return { pages: [], warnings, sources: normalized, hasSource };
            }
            warnings.push("Source guard: Query has no explicit source. Transition mode still runs dv.pages(), but this call should add from({ scope }) or allowGlobal:true.");
        }

        let basePages;
        if (normalized.pages) {
            basePages = this.toArray(normalized.pages).filter(Boolean);
        } else if (normalized.paths) {
            basePages = normalized.paths.map(p => dv.page(typeof p === 'string' ? p : p.path)).filter(Boolean);
        } else if (normalized.currentAndLinkedDiary) {
            basePages = this._linkedDiaryPages(dv.current());
        } else if (normalized.current) {
            basePages = [dv.current()].filter(p => p && p.file);
        } else if (normalized.scope) {
            basePages = this.toArray(dv.pages(`"${normalized.scope}"`));
        } else if (normalized.linkedTo && !normalized.linkedByTag) {
            const sourcePath = normalized.linkedTo === true ? dv.current()?.file?.path : (normalized.linkedTo.path || normalized.linkedTo);
            const tp = dv.page(sourcePath);
            if (tp && tp.file) {
                basePages = this.toArray(tp.file.inlinks).map(link => dv.page(link.path)).filter(p => p && p.file);
                linkedToResolvedAsBase = true;
            } else {
                warnings.push(`SourceResolver: linkedTo target not found: ${sourcePath || "(current page)"}`);
                basePages = [];
            }
        } else {
            basePages = this.toArray(dv.pages());
        }

        if (normalized.excludeTags && normalized.excludeTags.length > 0) {
            const excludes = normalized.excludeTags.map(t => String(t).replace(/#/g, ""));
            basePages = basePages.filter(p => {
                const pageTags = (p.file.tags || []).map(t => t.replace(/#/g, ""));
                return !excludes.some(ext => pageTags.includes(ext));
            });
        }

        if (normalized.linkedByTag) {
            const tagPages = this.toArray(dv.pages(normalized.linkedByTag));
            let inlinkPaths = new Set();
            for (const tp of tagPages) {
                for (const link of this.toArray(tp.file.inlinks)) inlinkPaths.add(link.path);
            }
            basePages = basePages.filter(p => inlinkPaths.has(p.file.path));
        }

        if (normalized.linkedTo && !linkedToResolvedAsBase) {
            const sourcePath = normalized.linkedTo === true ? dv.current()?.file?.path : (normalized.linkedTo.path || normalized.linkedTo);
            const tp = dv.page(sourcePath);
            let inlinkPaths = new Set();
            if (tp && tp.file) {
                for (const link of this.toArray(tp.file.inlinks)) inlinkPaths.add(link.path);
            } else {
                warnings.push(`SourceResolver: linkedTo target not found: ${sourcePath || "(current page)"}`);
            }
            basePages = basePages.filter(p => inlinkPaths.has(p.file.path));
        }

        if (normalized.maxPages && basePages.length > normalized.maxPages) {
            warnings.push(`SourceResolver: sourcePages ${basePages.length} exceeds maxPages ${normalized.maxPages}.`);
        }

        return { pages: basePages, warnings, sources: normalized, hasSource };
    }
}

class FilterChain {
    constructor(rules) { this.rules = rules || {}; }

    static _getLinkedPageTags(link) {
        if (!FilterChain._targetTagPageCache) FilterChain._targetTagPageCache = new Map();
        const key = String(link || "");
        const page = dv.page(key);
        if (!page?.file) return [];
        const fingerprint = [
            page.file.path,
            page.file.mtime?.ts || page.file.ctime?.ts || 0,
            (page.file.tags || []).join(","),
        ].join("|");
        const cached = FilterChain._targetTagPageCache.get(key);
        if (cached && cached.fingerprint === fingerprint) return cached.tags;
        const tags = page.file.tags || [];
        FilterChain._targetTagPageCache.set(key, { fingerprint, tags });
        return tags;
    }

    static isSameOrAfter(d1, d2) {
        if (!d1 || !d2) return false;
        const _d1 = new Date(d1); _d1.setHours(0, 0, 0, 0);
        const _d2 = new Date(d2); _d2.setHours(0, 0, 0, 0);
        return _d1 >= _d2;
    }
    static isSameOrBefore(d1, d2) {
        if (!d1 || !d2) return false;
        const _d1 = new Date(d1); _d1.setHours(0, 0, 0, 0);
        const _d2 = new Date(d2); _d2.setHours(0, 0, 0, 0);
        return _d1 <= _d2;
    }

    test(entry) {
        if (this.rules.type && entry.type !== this.rules.type) return false;

        if (this.rules.excludeTags && this.rules.excludeTags.length > 0) {
            const excludeClean = this.rules.excludeTags.map(t => t.replace(/#/g, ""));
            if (excludeClean.some(t => entry.meta.tags.includes(t))) return false;
        }

        if (this.rules.tags && this.rules.tags.length > 0) {
            const includeClean = this.rules.tags.map(t => t.replace(/#/g, ""));
            if (!includeClean.some(t => entry.meta.tags.includes(t))) return false;
        }

        if (this.rules.explicitTarget) {
            const currentFile = dv.current()?.file;
            const rawTarget = this.rules.explicitTarget === true ? currentFile?.name : this.rules.explicitTarget;
            const targetCandidates = new Set([rawTarget].filter(Boolean));
            if (this.rules.explicitTarget === true && currentFile) {
                targetCandidates.add(currentFile.path);
                targetCandidates.add(currentFile.path?.replace(/\.md$/, ""));
            }
            const normalizeTarget = (value) => String(value || "").replace(/\.md$/, "");
            const targetHit = entry.meta.outlinks.some(link => {
                const normalizedLink = normalizeTarget(link);
                for (const candidate of targetCandidates) {
                    if (!candidate) continue;
                    const normalizedCandidate = normalizeTarget(candidate);
                    if (link === candidate || normalizedLink === normalizedCandidate) return true;
                    if (normalizedLink.endsWith(`/${normalizedCandidate}`)) return true;
                }
                return false;
            });
            const textHit = [...targetCandidates].some(target => {
                if (!target) return false;
                return entry.cleanText === target || entry.rawText.indexOf(`[[${target}`) !== -1;
            });
            if (!targetHit && !textHit) {
                return false;
            }
        }

        if (this.rules.targetTag) {
            let hit = false;
            const targetTagRaw = this.rules.targetTag.replace(/#/g, '');
            for (const link of entry.meta.outlinks) {
                const tags = FilterChain._getLinkedPageTags(link);
                if (tags.some(t => t.replace(/#/g, '') === targetTagRaw)) {
                    hit = true; break;
                }
            }
            if (!hit) return false;
        }

        const eDate = Utils.resolveEntryDate(entry);
        if (this.rules.startDate && !FilterChain.isSameOrAfter(eDate, this.rules.startDate)) return false;
        if (this.rules.endDate && !FilterChain.isSameOrBefore(eDate, this.rules.endDate)) return false;

        return true;
    }
}

class DataQuery {
    constructor() { this._sources = {}; this._filters = {}; this._debug = false; }
    from(sources) { this._sources = { ...this._sources, ...SourceResolver.normalize(sources) }; return this; }
    filter(rules) { this._filters = { ...this._filters, ...(rules || {}) }; return this; }
    debug(enabled = true) { this._debug = enabled !== false; return this; }
    static _now() {
        if (typeof performance !== 'undefined' && performance && typeof performance.now === 'function') return performance.now();
        return Date.now();
    }
    _attachDebug(entries, metrics, warnings) {
        metrics.warnings = warnings;
        Object.defineProperty(entries, "metrics", { value: metrics, configurable: true });
        Object.defineProperty(entries, "warnings", { value: warnings, configurable: true });
        return entries;
    }
    execute() {
        const startedAt = DataQuery._now();
        const resolved = SourceResolver.resolveWithMeta(this._sources);
        const pages = resolved.pages;
        const warnings = [...resolved.warnings];
        const metrics = {
            sourcePages: pages.length,
            parsedEntries: 0,
            filteredEntries: 0,
            frontmatterEntries: 0,
            listEntries: 0,
            skippedEntries: 0,
            elapsedMs: 0,
            cacheHits: 0,
            cacheMisses: 0,
            cacheHitRate: 0,
            anomalies: [],
            warnings: [],
        };

        const entries = [];
        let sourceListItems = 0;
        for (const p of pages) {
            sourceListItems += p.file?.lists?.length || 0;
            entries.push(...UnifiedParser.parseDocument(p, { metrics }));
        }

        metrics.parsedEntries = entries.length;
        metrics.frontmatterEntries = entries.filter(e => e.lineIndex === -1).length;
        metrics.listEntries = entries.length - metrics.frontmatterEntries;
        metrics.skippedEntries = Math.max(0, sourceListItems - metrics.listEntries);
        metrics.anomalies = entries.flatMap(entry => Utils.detectEntryAnomalies(entry));
        const cacheReads = metrics.cacheHits + metrics.cacheMisses;
        metrics.cacheHitRate = cacheReads > 0 ? Math.round((metrics.cacheHits / cacheReads) * 10000) / 10000 : 0;

        const chain = new FilterChain(this._filters);
        const filtered = entries.filter(e => chain.test(e));
        metrics.filteredEntries = filtered.length;
        metrics.elapsedMs = Math.round((DataQuery._now() - startedAt) * 100) / 100;
        return this._attachDebug(filtered, metrics, warnings);
    }
}

const Query = () => new DataQuery();

// --- 5. 导出 ---
if (typeof input !== 'undefined' && input && typeof input === 'object') {
    input.CONFIG = CONFIG;
    input.Utils = Utils;
    input.ObjectSummary = ObjectSummary;
    input.Wallet = Wallet;
    input.Query = Query;
    input.StandardEntry = StandardEntry;
    input.UnifiedParser = UnifiedParser;
    input.SourceResolver = SourceResolver;
    input.FilterChain = FilterChain;
    input.DataQuery = DataQuery;
}
