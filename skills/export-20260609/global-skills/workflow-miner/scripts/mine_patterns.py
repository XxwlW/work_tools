#!/usr/bin/env python3
"""First-pass repeated-work scanner for user-provided text files.

This script intentionally contains no personal paths, project names, or private
examples. Pass one or more exported conversation/task files as arguments.
"""

from __future__ import annotations

import argparse
import json
import re
from collections import Counter
from pathlib import Path


CATEGORY_KEYWORDS = {
    "code-and-debugging": [
        "bug",
        "fix",
        "test",
        "build",
        "error",
        "refactor",
        "run",
        "deploy",
        "CI",
    ],
    "documents-and-writing": [
        "PRD",
        "proposal",
        "requirements",
        "draft",
        "rewrite",
        "polish",
        "document",
    ],
    "design-and-prototype": [
        "UI",
        "UX",
        "prototype",
        "wireframe",
        "mockup",
        "Figma",
        "layout",
    ],
    "research-and-summary": [
        "research",
        "summarize",
        "analysis",
        "compare",
        "competitor",
        "market",
    ],
    "data-and-evaluation": [
        "dataset",
        "metrics",
        "evaluate",
        "benchmark",
        "report",
        "score",
    ],
    "automation": [
        "automate",
        "schedule",
        "monitor",
        "reminder",
        "script",
        "repeat",
        "batch",
    ],
}

PRIVATE_PATTERNS = [
    re.compile(r"/Users/[^\s`'\"]+"),
    re.compile(r"[A-Za-z0-9._%+-]+@[A-Za-z0-9.-]+\.[A-Za-z]{2,}"),
    re.compile(r"https?://[^\s)>'\"]+"),
    re.compile(r"(?i)(api[_-]?key|token|secret|password)\s*[:=]\s*[^\s]+"),
]


def read_inputs(paths: list[Path]) -> dict[str, str]:
    texts: dict[str, str] = {}
    for path in paths:
        if path.is_dir():
            for child in sorted(path.rglob("*")):
                if child.is_file() and child.suffix.lower() in {".txt", ".md", ".json", ".jsonl"}:
                    texts[str(child)] = child.read_text(encoding="utf-8", errors="ignore")
        else:
            texts[str(path)] = path.read_text(encoding="utf-8", errors="ignore")
    return texts


def count_private_markers(text: str) -> int:
    return sum(len(pattern.findall(text)) for pattern in PRIVATE_PATTERNS)


def score_categories(texts: dict[str, str]) -> dict[str, dict[str, object]]:
    results: dict[str, dict[str, object]] = {}
    combined = "\n".join(texts.values())
    lower = combined.lower()

    for category, keywords in CATEGORY_KEYWORDS.items():
        hits = Counter()
        for keyword in keywords:
            count = lower.count(keyword.lower())
            if count:
                hits[keyword] = count
        if hits:
            results[category] = {
                "score": sum(hits.values()),
                "keywords": dict(hits.most_common()),
            }

    return dict(sorted(results.items(), key=lambda item: (-int(item[1]["score"]), item[0])))


def repeated_phrases(texts: dict[str, str], limit: int) -> list[tuple[str, int]]:
    counter: Counter[str] = Counter()
    for text in texts.values():
        lines = [line.strip() for line in text.splitlines()]
        for line in lines:
            if 12 <= len(line) <= 120 and not line.startswith(("#", "---", "```")):
                normalized = re.sub(r"\s+", " ", line)
                counter[normalized] += 1
    return [(phrase, count) for phrase, count in counter.most_common(limit) if count > 1]


def build_report(texts: dict[str, str], limit: int) -> dict[str, object]:
    return {
        "files_scanned": len(texts),
        "private_marker_count": sum(count_private_markers(text) for text in texts.values()),
        "category_scores": score_categories(texts),
        "repeated_phrases": repeated_phrases(texts, limit),
    }


def render_markdown(report: dict[str, object]) -> str:
    lines = [
        "# Workflow Mining Scan",
        "",
        f"- Files scanned: {report['files_scanned']}",
        f"- Potential private markers found: {report['private_marker_count']}",
        "",
        "## Category Scores",
        "",
    ]
    category_scores = report["category_scores"]
    if isinstance(category_scores, dict) and category_scores:
        for category, data in category_scores.items():
            keywords = ", ".join(f"{key}={value}" for key, value in data["keywords"].items())
            lines.append(f"- {category}: score={data['score']} | {keywords}")
    else:
        lines.append("- No category signals found.")

    lines.extend(["", "## Repeated Phrases", ""])
    phrases = report["repeated_phrases"]
    if isinstance(phrases, list) and phrases:
        for phrase, count in phrases:
            lines.append(f"- {count}x: {phrase}")
    else:
        lines.append("- No repeated phrases found.")

    lines.extend(
        [
            "",
            "Use this scan only as a starting point. Review evidence manually before creating reusable assets.",
        ]
    )
    return "\n".join(lines) + "\n"


def main() -> int:
    parser = argparse.ArgumentParser(description="Mine repeated workflow signals from provided text files.")
    parser.add_argument("paths", nargs="+", type=Path, help="Text, Markdown, JSON, JSONL files or directories to scan")
    parser.add_argument("--limit", type=int, default=20, help="Maximum repeated phrases to show")
    parser.add_argument("--json", action="store_true", help="Emit JSON")
    args = parser.parse_args()

    texts = read_inputs(args.paths)
    report = build_report(texts, args.limit)
    if args.json:
        print(json.dumps(report, ensure_ascii=False, indent=2))
    else:
        print(render_markdown(report))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
