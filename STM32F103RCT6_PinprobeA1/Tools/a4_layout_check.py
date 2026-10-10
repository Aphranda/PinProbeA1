#!/usr/bin/env python3
"""Estimate rendered A4 page height for the GTS fixed-page report template.

The template renders every page as a fixed 210x297mm `.page` with
`overflow:hidden`, so any page whose content exceeds the main area is
silently clipped. This script models the CSS of `Tools/md_to_gts_a4_pages.py`
(font sizes, line heights, paddings, column widths, chip cards, note/todo rows
and the bottom conclusion bar) and reports the estimated height of every
generated page, flagging pages at risk of clipping.

Usage:
    python Tools/a4_layout_check.py <report.md>

The model is approximate: treat a page within ~10mm of the budget as "must be
rebalanced with an explicit <!-- page-break -->".
"""

from __future__ import annotations

import importlib.util
import math
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
GENERATOR = ROOT / "Tools" / "md_to_gts_a4_pages.py"

SPEC = importlib.util.spec_from_file_location("gts", GENERATOR)
gts = importlib.util.module_from_spec(SPEC)
sys.modules["gts"] = gts
SPEC.loader.exec_module(gts)

MM_PER_PX = 25.4 / 96.0
CONTENT_W = 178.0          # 210mm - 2 x 16mm page padding
PAGE_H = 297.0
HEADER_H = 27.0
FOOTER_H = 11.5
MAIN_H = PAGE_H - HEADER_H - FOOTER_H
BOXES = {"note", "todo", "coord"}


def char_units(text: str) -> float:
    """Weighted character count: CJK = 1.0, ASCII = 0.52."""
    return sum(1.0 if ord(ch) > 0x2000 else 0.52 for ch in text)


def wrap_lines(text: str, width_mm: float, font_px: float) -> int:
    capacity = max(1.0, width_mm / (font_px * MM_PER_PX))
    return max(1, math.ceil(char_units(text) / capacity))


def text_mm(text: str, font_px: float, width_mm: float, line_px: float = 1.42) -> float:
    return wrap_lines(text, width_mm, font_px) * font_px * line_px * MM_PER_PX


def table_height_mm(headers: list[str], rows: list[list[str]]) -> float:
    ncols = max(1, len(headers))
    text_w = CONTENT_W / ncols - 3.8
    total = wrap_lines(" ".join(headers), text_w, 10.3) * 10.3 * 1.42 * MM_PER_PX + 2.7
    for row in rows:
        lines = 1
        for cell in row:
            lines = max(lines, wrap_lines(cell, text_w, 10.3))
        total += lines * 10.3 * 1.42 * MM_PER_PX + 2.2
    return total + 4.2


def chips_height_mm(items: list[str]) -> float:
    col_w = (CONTENT_W - 3 * 2.1) / 4 - 4.2
    total = 0.0
    for start in range(0, max(1, len(items)), 4):
        height = 0.0
        for item in items[start:start + 4]:
            _, _, value = item.partition("|")
            height = max(height, 2.5 + 4.0 + wrap_lines(value.strip(), col_w, 12.0) * 12.0 * 1.25 * MM_PER_PX)
        total += height + 2.1
    return total + 3.2


def box_height_mm(text: str, width_mm: float) -> float:
    return text_mm(text, 10.8, width_mm - 3.0) + 3.7 + 3.2


def image_height_mm(src: str) -> float:
    path = src.replace("file:///", "")
    if path.lower().endswith(".svg"):
        try:
            from urllib.parse import unquote

            source = Path(unquote(path)).read_text(encoding="utf-8")
            viewbox = re.search(r'viewBox="[\d.\s]*?([\d.]+)\s+([\d.]+)"', source)
            if viewbox:
                vw, vh = float(viewbox.group(1)), float(viewbox.group(2))
                return min(CONTENT_W * (vh / vw), 82.0) + 2.6 + 4.2
        except Exception:
            pass
        return 45.0
    try:
        from PIL import Image

        with Image.open(path) as im:
            w, h = im.size
    except Exception:
        w = h = 1
    return min(CONTENT_W * (h / w), 82.0) + 2.6 + 4.2


def block_height_mm(block) -> float:
    if block.kind == "heading":
        return {2: 7.6, 3: 7.3}.get(block.level, 6.6)
    if block.kind == "p":
        return text_mm(block.text, 10.7, CONTENT_W) + 2.6
    if block.kind == "quote":
        return box_height_mm(block.text, CONTENT_W)
    if block.kind in BOXES:
        return box_height_mm(block.text, CONTENT_W)
    if block.kind == "chips":
        return chips_height_mm(block.items or [])
    if block.kind == "bar":
        return text_mm(block.text, 11.5, CONTENT_W - 5.0) + 3.7 + 2.1
    if block.kind == "svg":
        source = "\n".join(block.lines or [])
        viewbox = re.search(r'viewBox="0 0 ([\d.]+) ([\d.]+)"', source)
        width = re.search(r'width="([\d.]+)%"', source)
        if not viewbox:
            return 85.0
        vw, vh = float(viewbox.group(1)), float(viewbox.group(2))
        pct = float(width.group(1)) if width else 100.0
        return (pct / 100.0) * CONTENT_W * (vh / vw) + 2.6 + 4.2 + 4.0
    if block.kind in ("ul", "ol"):
        return 2.9 + sum(text_mm(item, 10.8, CONTENT_W - 5.0) + 1.1 for item in (block.items or []))
    if block.kind == "code":
        h = 2.2 * MM_PER_PX + 6.4
        for line in block.lines or []:
            h += wrap_lines(line, CONTENT_W - 4.0, 10.0) * 10.0 * 1.42 * MM_PER_PX
        return h
    if block.kind == "image":
        return image_height_mm(block.src) + 5.0
    if block.kind == "table":
        return table_height_mm(block.headers or [], block.rows or [])
    return 0.0


def page_blocks_height_mm(blocks: list) -> tuple[float, list[tuple[str, float]]]:
    parts: list[tuple[str, float]] = []
    total = 0.0
    i = 0
    while i < len(blocks):
        block = blocks[i]
        nxt = blocks[i + 1] if i + 1 < len(blocks) else None
        if nxt is not None and block.kind in BOXES and nxt.kind in BOXES:
            w_note = (CONTENT_W - 2.6) * 1.15 / 2
            w_todo = (CONTENT_W - 2.6) * 0.85 / 2
            height = max(box_height_mm(block.text, w_note), box_height_mm(nxt.text, w_todo))
            parts.append(("summary-row", height))
            total += height
            i += 2
            continue
        height = block_height_mm(block)
        parts.append((block.kind, height))
        total += height
        i += 1
    return total, parts


def main() -> None:
    md = Path(sys.argv[1] if len(sys.argv) > 1 else "")
    if not md.exists():
        raise SystemExit(f"markdown not found: {md}")
    title, blocks = gts.parse_markdown(md)
    _, content = gts.extract_cover_note(blocks)
    pages = gts.merge_trailing_sparse_page(gts.paginate(title, content))
    worst = 0.0
    for i, page in enumerate(pages, 2):
        total, parts = page_blocks_height_mm(page.blocks)
        worst = max(worst, total)
        flag = "OVERFLOW" if total > MAIN_H else ("tight" if total > MAIN_H - 12 else "ok")
        print(f"P{i:<3} {total:6.1f}mm / {MAIN_H:.0f}mm  {flag:<9} {page.title}")
        for kind, height in parts:
            if height > 30:
                print(f"       big block: {kind} {height:.1f}mm")
    print(f"\nmain area budget = {MAIN_H:.1f}mm ; worst page = {worst:.1f}mm")


if __name__ == "__main__":
    main()
