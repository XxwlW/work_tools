const path = require("path");

function dateTime(value) {
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

function withValues(items) {
    const arr = Array.from(items || []);
    arr.values = arr;
    return arr;
}

function listItem(line, text, tags = [], children = [], parent = null) {
    const item = {
        line,
        text,
        tags,
        children,
        parent,
    };
    for (const child of children) {
        if (child.parent == null) child.parent = line;
    }
    return item;
}

function child(line, text, tags = [], children = []) {
    return listItem(line, text, tags, children, null);
}

function page(spec) {
    const frontmatter = spec.frontmatter || {};
    const file = {
        path: spec.path,
        name: spec.name || path.basename(spec.path, ".md"),
        tags: spec.tags || [],
        frontmatter,
        lists: spec.lists || [],
        inlinks: withValues(spec.inlinks || []),
        outlinks: withValues(spec.outlinks || []),
        ctime: dateTime(spec.ctime || "2026-05-01"),
        mtime: dateTime(spec.mtime || spec.ctime || "2026-05-01"),
        day: spec.day ? dateTime(spec.day) : null,
        link: { path: spec.path },
    };
    return {
        ...spec.fields,
        file,
        tags: (spec.tags || []).map(t => t.replace(/^#/, "")),
    };
}

function makeFixtures() {
    const nestedJournalItem = listItem(21, "[[样例信用卡|信用卡]] 衣服 #衣物", ["#衣物"], [
        child(22, "-200, -2"),
        child(23, "BILL:2026-05-05"),
    ], 20);
    const nestedJournalGroup = listItem(20, "购物 #记账 #消费", ["#记账", "#消费"], [
        nestedJournalItem,
    ]);

    const pages = [
        page({
            path: "03 人物/人/小明.md",
            name: "小明",
            tags: ["#人"],
            frontmatter: {
                tags: ["人"],
            },
            inlinks: [
                { path: "01 日记/2026-05-01.md" },
                { path: "01 日记/2026-05-03.md" },
                { path: "02 事件/小明借款.md" },
            ],
        }),
        page({
            path: "03 人物/人/妈妈.md",
            name: "妈妈",
            tags: ["#人"],
            frontmatter: {
                tags: ["人"],
            },
        }),
        page({
            path: "03 人物/钱包/样例信用卡.md",
            name: "样例信用卡",
            tags: ["#钱包"],
            frontmatter: {
                tags: ["钱包"],
                "还款日": [15, 14, 15],
                "信用": 10000,
                "类型": ["信用卡"],
            },
            inlinks: [
                { path: "01 日记/2026-05-02.md" },
                { path: "01 日记/2026-05-04.md" },
            ],
        }),
        page({
            path: "01 日记/2026-05-01.md",
            day: "2026-05-01",
            lists: [
                listItem(1, "和 [[小明]] 跑步 #健康 @2026-05-03", ["#健康"], [
                    child(2, "2, 3"),
                ]),
            ],
        }),
        page({
            path: "01 日记/2026-05-02.md",
            day: "2026-05-02",
            lists: [
                listItem(10, "[[样例信用卡|信用卡]] 午餐 #记账 #餐饮", ["#记账", "#餐饮"], [
                    child(11, "-120, -1"),
                    child(12, "BILL:2026-05-02; LIFE:30; MULTI:3@120"),
                ]),
                nestedJournalGroup,
                nestedJournalItem,
                listItem(30, "[[样例信用卡|信用卡]] 余额调整 #记账 #转账", ["#记账", "#转账"], [
                    child(31, "500, 0"),
                ]),
            ],
        }),
        page({
            path: "01 日记/2026-05-03.md",
            day: "2026-05-03",
            frontmatter: {
                "值": "1, 2",
                "标签": ["#社交"],
                "关联": ["[[小明]]"],
                "创建时间": "2026-05-03",
                "链接日记": [{ path: "02 事件/小明借款.md" }],
            },
            lists: [
                listItem(40, "给 [[小明]] 送书 #礼物", ["#礼物"], [
                    child(41, "1, 2"),
                ]),
            ],
        }),
        page({
            path: "01 日记/2026-05-04.md",
            day: "2026-05-04",
            tags: ["#记账"],
            frontmatter: {
                "值": -88,
                "标签": ["#贷款"],
                "关联": ["[[样例信用卡]]"],
                "创建时间": "2026-05-04",
                "还款信息": "BILL:2026-05-04; MULTI:2@88",
                "服役天数": 8,
            },
            lists: [
                listItem(50, "[[样例信用卡|信用卡]] 咖啡 #记账 #饮品 LIFE:7", ["#记账", "#饮品"], [
                    child(51, "-35, 1"),
                ]),
                listItem(55, "检查 [[样例信用卡|信用卡]] 权益 #财务", ["#财务"], [
                    child(56, "0, 1"),
                ]),
            ],
        }),
        page({
            path: "02 事件/小明借款.md",
            day: "2026-05-06",
            lists: [
                listItem(60, "[[样例信用卡|信用卡]] 借钱给 [[小明]] #记账 #借款", ["#记账", "#借款"], [
                    child(61, "300, 1"),
                ]),
            ],
        }),
        page({
            path: "02 项目/项目A.md",
            name: "项目A",
            tags: ["#项目"],
            frontmatter: {
                tags: ["项目"],
                "标签": ["开发"],
                "状态": "进行中",
                "期望努力值": 5,
            },
            inlinks: [
                { path: "02 项目/项目A日志.md" },
                { path: "02 项目/项目A方案.md" },
            ],
        }),
        page({
            path: "02 项目/项目A日志.md",
            day: "2026-05-07",
            lists: [
                listItem(70, "推进 [[项目A]] 原型 #开发 @2026-05-08", ["#开发"], [
                    child(71, "4, 2"),
                ]),
                listItem(80, "[[样例信用卡|信用卡]] 购买素材 [[项目A]] #记账 #成本", ["#记账", "#成本"], [
                    child(81, "-80, 0"),
                ]),
            ],
        }),
        page({
            path: "02 项目/项目A方案.md",
            frontmatter: {
                "值": ["2"],
                "标签": ["#开发"],
                "项目": ["[[项目A]]"],
                "创建时间": "2026-04-12",
            },
        }),
    ];
    return pages;
}

function makeWalletFieldFixtures() {
    return [
        page({
            path: "03 人物/钱包/字符串类型钱包.md",
            name: "字符串类型钱包",
            tags: ["#钱包"],
            frontmatter: {
                tags: ["钱包"],
                "信用": "2500",
                "类型": "储蓄卡",
            },
        }),
        page({
            path: "03 人物/钱包/数组类型钱包.md",
            name: "数组类型钱包",
            tags: ["#钱包"],
            frontmatter: {
                tags: ["钱包"],
                "信用": "1500",
                "类型": ["储蓄卡", "备用"],
            },
        }),
        page({
            path: "03 人物/钱包/缺类型钱包.md",
            name: "缺类型钱包",
            tags: ["#钱包"],
            frontmatter: {
                tags: ["钱包"],
                "信用": 0,
            },
            inlinks: [
                { path: "02 事件/钱包无交易关联.md" },
            ],
        }),
        page({
            path: "02 事件/钱包无交易关联.md",
            name: "钱包无交易关联",
            lists: [
                listItem(1, "仅关联 [[缺类型钱包]] 的非账务记录 #财务", ["#财务"]),
            ],
        }),
    ];
}

function createMockDataview(options = {}) {
    const pages = [...makeFixtures(), ...(options.extraPages || [])];
    const byPath = new Map();
    const byName = new Map();
    for (const p of pages) {
        byPath.set(p.file.path, p);
        byName.set(p.file.name, p);
    }

    function wrap(items) {
        const arr = withValues(items);
        const originalFilter = arr.filter.bind(arr);
        arr.filter = predicate => wrap(originalFilter(predicate));
        const originalMap = arr.map.bind(arr);
        arr.map = mapper => withValues(originalMap(mapper));
        return arr;
    }

    const currentPath = options.currentPath || "03 人物/人/小明.md";

    return {
        pages(query) {
            if (!query) return wrap(pages);
            const trimmed = String(query).trim();
            const scopeMatch = trimmed.match(/^"(.+)"$/);
            if (scopeMatch) {
                const scope = scopeMatch[1].replace(/\\/g, "/");
                return wrap(pages.filter(p => p.file.path.startsWith(scope + "/") || p.file.path === scope));
            }
            if (trimmed.startsWith("#")) {
                const tag = trimmed.replace(/^#/, "");
                return wrap(pages.filter(p => (p.file.tags || []).some(t => t.replace(/^#/, "") === tag)));
            }
            return wrap(pages);
        },
        page(target) {
            if (!target) return null;
            const key = typeof target === "string" ? target : target.path;
            if (byPath.has(key)) return byPath.get(key);
            if (byName.has(key)) return byName.get(key);
            const noExt = key.replace(/\.md$/, "");
            if (byPath.has(`${noExt}.md`)) return byPath.get(`${noExt}.md`);
            return null;
        },
        current() {
            return byPath.get(currentPath) || null;
        },
        tryQuery(query) {
            return Promise.resolve({ values: this.pages(query) });
        },
    };
}

module.exports = {
    createMockDataview,
    makeFixtures,
    makeWalletFieldFixtures,
};
