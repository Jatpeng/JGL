from __future__ import annotations

import argparse
import os
from dataclasses import dataclass
from datetime import datetime
from pathlib import Path
import re


ROOT = Path(__file__).resolve().parents[1]
DOCS_ROOT = ROOT / "docs"
WIKI_INDEX = Path("wiki/index.md")

CATEGORY_ORDER = [
    ("root", "总览"),
    ("engine", "引擎与渲染"),
    ("sections", "编辑器与功能专题"),
    ("api", "API 文档"),
    ("other", "其他"),
]


@dataclass
class PageDoc:
    rel_path: Path
    title: str
    summary: str
    category_key: str
    category_name: str
    modified_at: datetime


def iter_markdown_files() -> list[Path]:
    paths: list[Path] = []
    for path in DOCS_ROOT.rglob("*.md"):
        rel_path = path.relative_to(DOCS_ROOT)
        if rel_path.parts and rel_path.parts[0] == "wiki":
            continue
        paths.append(path)
    return sorted(paths)


def detect_category(rel_path: Path) -> tuple[str, str]:
    if len(rel_path.parts) == 1:
        if rel_path.name == "README.md":
            return ("other", "其他")
        return ("root", "总览")

    top_level = rel_path.parts[0]
    for category_key, category_name in CATEGORY_ORDER:
        if top_level == category_key:
            return (category_key, category_name)
    return ("other", "其他")


def extract_title(text: str, fallback: str) -> str:
    for line in text.splitlines():
        stripped = line.strip()
        if stripped.startswith("# "):
            return stripped[2:].strip()
    return fallback


def normalize_inline_markdown(text: str) -> str:
    text = re.sub(r"`([^`]+)`", r"\1", text)
    text = re.sub(r"\[([^\]]+)\]\([^)]+\)", r"\1", text)
    text = re.sub(r"!\[[^\]]*\]\([^)]+\)", "", text)
    return " ".join(text.split()).strip()


def extract_summary(text: str) -> str:
    in_code_block = False
    paragraph_lines: list[str] = []

    def flush_paragraph() -> str:
        summary = normalize_inline_markdown(" ".join(paragraph_lines))
        if summary:
            return summary[:140].rstrip(" ，。；;,:") + ("…" if len(summary) > 140 else "")
        return ""

    for line in text.splitlines():
        stripped = line.strip()

        if stripped.startswith("```"):
            in_code_block = not in_code_block
            if paragraph_lines:
                summary = flush_paragraph()
                if summary:
                    return summary
                paragraph_lines.clear()
            continue

        if in_code_block:
            continue

        if not stripped:
            if paragraph_lines:
                summary = flush_paragraph()
                if summary:
                    return summary
                paragraph_lines.clear()
            continue

        if stripped.startswith("#"):
            if paragraph_lines:
                summary = flush_paragraph()
                if summary:
                    return summary
                paragraph_lines.clear()
            continue

        if (
            stripped.startswith("- ")
            or stripped.startswith("* ")
            or stripped.startswith("> ")
            or stripped.startswith("|")
            or re.match(r"\d+\.\s", stripped)
        ):
            if paragraph_lines:
                summary = flush_paragraph()
                if summary:
                    return summary
                paragraph_lines.clear()
            continue

        paragraph_lines.append(stripped)

    if paragraph_lines:
        return flush_paragraph()

    return "暂无摘要。"


def build_page_docs() -> list[PageDoc]:
    pages: list[PageDoc] = []
    for path in iter_markdown_files():
        rel_path = path.relative_to(DOCS_ROOT)
        text = path.read_text(encoding="utf-8")
        category_key, category_name = detect_category(rel_path)
        pages.append(
            PageDoc(
                rel_path=rel_path,
                title=extract_title(text, rel_path.stem),
                summary=extract_summary(text),
                category_key=category_key,
                category_name=category_name,
                modified_at=datetime.fromtimestamp(path.stat().st_mtime),
            )
        )
    return pages


def category_sort_key(page: PageDoc) -> tuple[int, str]:
    order_index = next(
        (index for index, (category_key, _) in enumerate(CATEGORY_ORDER) if category_key == page.category_key),
        len(CATEGORY_ORDER),
    )
    return (order_index, page.title.lower())


def relative_link(target_rel_path: Path) -> str:
    start_path = DOCS_ROOT / WIKI_INDEX
    target_path = DOCS_ROOT / target_rel_path
    return Path(os.path.relpath(target_path, start=start_path.parent)).as_posix()


def render_category_section(category_key: str, category_name: str, pages: list[PageDoc]) -> list[str]:
    category_pages = [page for page in pages if page.category_key == category_key]
    if not category_pages:
        return []

    lines = [f"### {category_name}", ""]
    for page in sorted(category_pages, key=lambda item: item.title.lower()):
        lines.append(f"- [{page.title}]({relative_link(page.rel_path)}) - {page.summary}")
    lines.append("")
    return lines


def render_recent_updates(pages: list[PageDoc], count: int = 8) -> list[str]:
    recent_pages = sorted(pages, key=lambda item: item.modified_at, reverse=True)[:count]
    lines = ["## 最近更新", ""]
    for page in recent_pages:
        lines.append(
            f"- {page.modified_at.strftime('%Y-%m-%d')} [{page.title}]({relative_link(page.rel_path)}) - {page.summary}"
        )
    lines.append("")
    return lines


def render_all_pages_table(pages: list[PageDoc]) -> list[str]:
    lines = [
        "## 全量索引",
        "",
        "| 页面 | 分类 | 路径 |",
        "| --- | --- | --- |",
    ]
    for page in sorted(pages, key=category_sort_key):
        lines.append(
            f"| [{page.title}]({relative_link(page.rel_path)}) | {page.category_name} | `{page.rel_path.as_posix()}` |"
        )
    lines.append("")
    return lines


def render_wiki_markdown(pages: list[PageDoc]) -> str:
    category_count_text = "、".join(
        f"{category_name}{sum(1 for page in pages if page.category_key == category_key)}篇"
        for category_key, category_name in CATEGORY_ORDER
        if any(page.category_key == category_key for page in pages)
    )

    lines = [
        "# JGL Wiki",
        "",
        "> 本页由 `tools/generate_wiki_docs.py` 自动生成，用于把 `docs/` 下的 Markdown 文档组织成可浏览的知识库入口。",
        "",
        f"当前已收录 {len(pages)} 篇文档，覆盖 {category_count_text}。",
        "",
        "## 快速入口",
        "",
        "- [文档索引](../index.md)",
        "- [Wiki 使用与维护](./contributing.md)",
        "- [API Docs 说明](../README.md)",
        "",
        "## 分类导航",
        "",
    ]

    for category_key, category_name in CATEGORY_ORDER:
        lines.extend(render_category_section(category_key, category_name, pages))

    lines.extend(render_recent_updates(pages))
    lines.extend(render_all_pages_table(pages))
    return "\n".join(lines).rstrip() + "\n"


def generate_wiki(output_path: Path) -> None:
    output_path.parent.mkdir(parents=True, exist_ok=True)
    pages = build_page_docs()
    output_path.write_text(render_wiki_markdown(pages), encoding="utf-8")


def main() -> None:
    parser = argparse.ArgumentParser(description="Generate a markdown wiki index for JGL docs.")
    parser.add_argument(
        "--output",
        default=str(DOCS_ROOT / WIKI_INDEX),
        help="Output path for the generated wiki index markdown file.",
    )
    args = parser.parse_args()

    generate_wiki(Path(args.output))


if __name__ == "__main__":
    main()
