#!/usr/bin/env python3
"""Convert a Markdown document to fixed-page GTS A4 portrait HTML.

This follows the page model used by the A4 daily-report template:
each physical page is a .page containing .page-header, .main and .page-footer.
"""

from __future__ import annotations

import argparse
import html
import re
from dataclasses import dataclass
from pathlib import Path


MAX_UNITS = 45.0
TRAILING_MERGE_UNITS = 52.0


@dataclass
class Block:
    kind: str
    text: str = ""
    level: int = 0
    headers: list[str] | None = None
    rows: list[list[str]] | None = None
    items: list[str] | None = None
    lines: list[str] | None = None


@dataclass
class Page:
    title: str
    subtitle: str
    blocks: list[Block]


def inline_md(text: str) -> str:
    text = re.sub(r"\[([^\]]+)\]\([^)]+\)", r"\1", text)
    escaped = html.escape(text)
    escaped = re.sub(r"\*\*([^*]+)\*\*", r"<strong>\1</strong>", escaped)
    escaped = re.sub(r"`([^`]+)`", r"<code>\1</code>", escaped)
    escaped = escaped.replace("禁止修改", '<span class="danger-text">禁止修改</span>')
    return escaped


def extract_logo(template_text: str) -> str:
    match = re.search(r"<g id=\"gts-logo\">.*?</g>", template_text, re.S)
    if not match:
        raise ValueError("Cannot find gts-logo in template.")
    return match.group(0)


def is_table_separator(line: str) -> bool:
    if not re.match(r"^\|(.+)\|\s*$", line):
        return False
    cells = [c.strip() for c in line.strip().strip("|").split("|")]
    return all(re.match(r"^:?-{3,}:?$", c) for c in cells)


def parse_table(lines: list[str], start: int) -> tuple[Block, int]:
    table_lines: list[str] = []
    i = start
    while i < len(lines) and re.match(r"^\|(.+)\|\s*$", lines[i]):
        if not is_table_separator(lines[i]):
            table_lines.append(lines[i])
        i += 1

    parsed = [
        [cell.strip() for cell in row.strip().strip("|").split("|")]
        for row in table_lines
    ]
    headers = parsed[0] if parsed else []
    rows = parsed[1:] if len(parsed) > 1 else []
    return Block(kind="table", headers=headers, rows=rows), i


def parse_markdown(path: Path) -> tuple[str, list[Block]]:
    lines = path.read_text(encoding="utf-8").splitlines()
    blocks: list[Block] = []
    title = path.stem
    i = 0
    while i < len(lines):
        line = lines[i]
        stripped = line.strip()

        if not stripped or stripped == "---":
            i += 1
            continue

        if stripped.startswith("```"):
            lang = stripped[3:].strip()
            code_lines: list[str] = []
            i += 1
            while i < len(lines) and not lines[i].strip().startswith("```"):
                code_lines.append(lines[i])
                i += 1
            if i < len(lines):
                i += 1
            blocks.append(Block(kind="code", text=lang, lines=code_lines))
            continue

        heading = re.match(r"^(#{1,6})\s+(.+)$", line)
        if heading:
            level = len(heading.group(1))
            text = heading.group(2).strip()
            if level == 1:
                title = re.sub(r"`", "", text)
                blocks.append(Block(kind="doc_title", text=text, level=level))
            else:
                blocks.append(Block(kind="heading", text=text, level=level))
            i += 1
            continue

        if re.match(r"^\|(.+)\|\s*$", line):
            table, i = parse_table(lines, i)
            blocks.append(table)
            continue

        if line.startswith(">"):
            quote_lines: list[str] = []
            while i < len(lines) and lines[i].startswith(">"):
                quote_lines.append(lines[i].lstrip(">").strip())
                i += 1
            blocks.append(Block(kind="quote", text=" ".join(quote_lines)))
            continue

        if re.match(r"^\d+\.\s+", line) or re.match(r"^[-*]\s+", line):
            kind = "ol" if re.match(r"^\d+\.\s+", line) else "ul"
            items: list[str] = []
            while i < len(lines):
                m = re.match(r"^\d+\.\s+(.+)$", lines[i]) if kind == "ol" else re.match(r"^[-*]\s+(.+)$", lines[i])
                if not m:
                    break
                items.append(m.group(1).strip())
                i += 1
            blocks.append(Block(kind=kind, items=items))
            continue

        paragraph: list[str] = [stripped]
        i += 1
        while i < len(lines):
            nxt = lines[i].strip()
            if not nxt or nxt == "---" or nxt.startswith("#") or nxt.startswith(">") or nxt.startswith("```") or re.match(r"^\|(.+)\|\s*$", nxt) or re.match(r"^\d+\.\s+", nxt) or re.match(r"^[-*]\s+", nxt):
                break
            paragraph.append(nxt)
            i += 1
        blocks.append(Block(kind="p", text=" ".join(paragraph)))

    return title, blocks


def table_row_units(headers: list[str], row: list[str]) -> float:
    width = max(1, len(headers))
    chars = sum(len(c) for c in row)
    code_penalty = sum(c.count("`") for c in row) * 0.06
    return 0.62 + chars / (92.0 * min(width, 4)) + code_penalty


def block_units(block: Block) -> float:
    if block.kind == "heading":
        return 1.8 if block.level == 2 else 1.25
    if block.kind == "doc_title":
        return 0.0
    if block.kind == "p":
        return 0.95 + len(block.text) / 125.0
    if block.kind == "quote":
        return 1.25 + len(block.text) / 115.0
    if block.kind in ("ul", "ol"):
        return 1.0 + sum(0.85 + len(item) / 110.0 for item in (block.items or []))
    if block.kind == "code":
        return 1.1 + len(block.lines or []) * 0.45
    if block.kind == "table":
        rows = block.rows or []
        headers = block.headers or []
        return 1.35 + sum(table_row_units(headers, row) for row in rows)
    return 1.0


def split_table(block: Block, remaining: float) -> tuple[Block, Block | None]:
    headers = block.headers or []
    rows = block.rows or []
    min_rows = 1
    used = 2.0
    take = 0
    capacity = max(8.0, remaining)
    for row in rows:
        row_u = table_row_units(headers, row)
        if take >= min_rows and used + row_u > capacity:
            break
        used += row_u
        take += 1
    take = max(1, take)
    first = Block(kind="table", headers=headers, rows=rows[:take])
    rest_rows = rows[take:]
    rest = Block(kind="table", headers=headers, rows=rest_rows) if rest_rows else None
    return first, rest


def split_code(block: Block, remaining: float) -> tuple[Block, Block | None]:
    lines = block.lines or []
    capacity = max(6, int((remaining - 1.6) / 0.58))
    take = max(4, min(capacity, len(lines)))
    first = Block(kind="code", text=block.text, lines=lines[:take])
    rest_lines = lines[take:]
    rest = Block(kind="code", text=block.text, lines=rest_lines) if rest_lines else None
    return first, rest


def extract_cover_note(blocks: list[Block]) -> tuple[str, list[Block]]:
    remaining: list[Block] = []
    cover_note = ""
    consumed_note = False
    for block in blocks:
        if block.kind == "doc_title":
            remaining.append(block)
            continue
        if not consumed_note and block.kind == "quote" and "文档更新时间" in block.text:
            cover_note = block.text
            consumed_note = True
            continue
        remaining.append(block)
    return cover_note, remaining


def paginate(title: str, blocks: list[Block]) -> list[Page]:
    pages: list[Page] = []
    current: list[Block] = []
    current_title = title
    current_subtitle = "客户外发资料"
    used = 0.0
    section_title = ""

    def flush() -> None:
        nonlocal current, used, current_title, current_subtitle
        if current:
            pages.append(Page(current_title, current_subtitle, current))
        current = []
        used = 0.0

    pending = list(blocks)
    while pending:
        block = pending.pop(0)
        if block.kind == "doc_title":
            continue

        if block.kind == "heading" and block.level == 2:
            section_title = re.sub(r"`", "", block.text)
            lookahead = [block]
            for next_block in pending:
                if next_block.kind == "heading" and next_block.level == 2:
                    break
                lookahead.append(next_block)
            section_units = sum(block_units(item) for item in lookahead)
            if current and (section_title == "修订记录" or used + section_units > MAX_UNITS):
                flush()
            if not current:
                current_title = section_title
                current_subtitle = title
            current.append(block)
            used += block_units(block)
            continue

        if (
            block.kind == "heading"
            and block.level == 3
            and re.sub(r"`", "", block.text) == "示例"
            and current
            and current_title == "设备信息"
            and section_title == "系统指令"
        ):
            flush()
            current_title = section_title
            current_subtitle = title

        if block.kind == "heading" and block.level > 2 and current and pending:
            next_block = pending[0]
            if next_block.kind != "heading" and used + block_units(block) + block_units(next_block) > MAX_UNITS:
                flush()
                current_title = section_title or title
                current_subtitle = f"{title}（续）"

        units = block_units(block)
        if used + units <= MAX_UNITS:
            current.append(block)
            used += units
            continue

        if block.kind == "table" and len(block.rows or []) > 1:
            first, rest = split_table(block, MAX_UNITS - used)
            if used + block_units(first) <= MAX_UNITS or not current:
                current.append(first)
                flush()
            else:
                flush()
                current_title = section_title or title
                current_subtitle = f"{title}（续）"
                pending.insert(0, block)
                continue
            if rest:
                current_title = section_title or title
                current_subtitle = f"{title}（续）"
                pending.insert(0, rest)
            continue

        if block.kind == "code" and len(block.lines or []) > 8:
            first, rest = split_code(block, MAX_UNITS - used)
            if used + block_units(first) <= MAX_UNITS or not current:
                current.append(first)
                flush()
            else:
                flush()
                pending.insert(0, block)
                continue
            if rest:
                pending.insert(0, rest)
            continue

        flush()
        current_title = section_title or title
        current_subtitle = f"{title}（续）"
        current.append(block)
        used = units

    flush()
    return pages


def merge_trailing_sparse_page(pages: list[Page]) -> list[Page]:
    if len(pages) < 2:
        return pages

    if len(pages) >= 3 and pages[-1].title == "修订记录":
        previous = pages[-3]
        sparse = pages[-2]
        combined_units = sum(block_units(block) for block in previous.blocks + sparse.blocks)
        if combined_units <= TRAILING_MERGE_UNITS:
            title = previous.title if previous.title == sparse.title else f"{previous.title} / {sparse.title}"
            merged = Page(title=title, subtitle=previous.subtitle, blocks=previous.blocks + sparse.blocks)
            return pages[:-3] + [merged, pages[-1]]

    previous = pages[-2]
    last = pages[-1]
    if last.title == "修订记录":
        return pages
    combined_units = sum(block_units(block) for block in previous.blocks + last.blocks)
    if combined_units > TRAILING_MERGE_UNITS:
        return pages

    title = previous.title if previous.title == last.title else f"{previous.title} / {last.title}"
    merged = Page(title=title, subtitle=previous.subtitle, blocks=previous.blocks + last.blocks)
    return pages[:-2] + [merged]


def block_to_html(block: Block) -> str:
    if block.kind == "heading":
        if block.level == 2:
            return f'<div class="section-title">{inline_md(block.text)}</div>'
        level = min(max(block.level + 1, 3), 5)
        return f"<h{level}>{inline_md(block.text)}</h{level}>"
    if block.kind == "p":
        return f"<p>{inline_md(block.text)}</p>"
    if block.kind == "quote":
        return f"<div class=\"note-box\">{inline_md(block.text)}</div>"
    if block.kind in ("ul", "ol"):
        tag = block.kind
        items = "\n".join(f"<li>{inline_md(item)}</li>" for item in (block.items or []))
        return f'<{tag} class="small-list">\n{items}\n</{tag}>'
    if block.kind == "code":
        lang = html.escape(block.text)
        code = html.escape("\n".join(block.lines or []))
        return f'<pre data-lang="{lang}"><code>{code}</code></pre>'
    if block.kind == "table":
        headers = block.headers or []
        rows = block.rows or []
        ths = "".join(f"<th>{inline_md(h)}</th>" for h in headers)
        trs = []
        for row in rows:
            cells = row + [""] * (len(headers) - len(row))
            tds = "".join(f"<td>{inline_md(c)}</td>" for c in cells[: len(headers)])
            trs.append(f"<tr>{tds}</tr>")
        return '<table class="compact">\n<thead><tr>' + ths + "</tr></thead>\n<tbody>\n" + "\n".join(trs) + "\n</tbody>\n</table>"
    return ""


def page_html(page: Page, index: int, total: int, doc_date: str, doc_kind: str, footer_name: str) -> str:
    body = "\n".join(block_to_html(block) for block in page.blocks)
    return f"""<!-- P{index} -->
<div class="page">
  <div class="page-header">
    <div>
      <h1>{inline_md(page.title)}</h1>
      <div class="subtitle">{inline_md(page.subtitle)}</div>
    </div>
    <svg class="logo" viewBox="4400 6500 18750 3520" role="img" aria-label="GTS"><use href="#gts-logo"></use></svg>
    <div class="doc-meta">
      <strong>{html.escape(doc_date)}</strong>
      {html.escape(doc_kind)}<br>
      PinProbe A1
    </div>
  </div>
  <div class="main">
{body}
  </div>
  <div class="page-footer">
    <span>Accurate · Simple · Fast · Agile | General Test Systems Inc.</span>
    <span>{html.escape(footer_name)} | {index} / {total}</span>
  </div>
</div>"""


def build_html(
    title: str,
    pages: list[Page],
    logo: str,
    doc_date: str,
    cover_note: str = "",
    cover_image: Path | None = None,
    doc_kind: str = "SCPI Reference",
    cover_subtitle: str = "通信指令 · 设备控制 · 异常处理",
    footer_name: str = "PinProbe A1 文档",
) -> str:
    total = len(pages) + 1
    cover_product = "PinProbe A1"
    cover_doc_name = title
    if cover_doc_name.startswith(cover_product):
        cover_doc_name = cover_doc_name[len(cover_product):].strip()
    cover_img_html = ""
    if cover_image is not None:
        cover_img_html = f'<div class="cover-image-wrap"><img src="{html.escape(cover_image.resolve().as_uri())}" alt="PinProbe A1 箱体示意图"></div>'
    cover = f"""<!-- P1 Cover -->
<div class="page cover-page">
  <div class="page-header cover-header">
    <div class="cover-header-text">
      <div class="cover-doc-label">客户交付文档</div>
      <div class="cover-doc-subtitle">Customer Delivery</div>
    </div>
    <svg class="logo" viewBox="4400 6500 18750 3520" role="img" aria-label="GTS"><use href="#gts-logo"></use></svg>
    <div class="doc-meta">
      <strong>{html.escape(doc_date)}</strong>
      {html.escape(doc_kind)}<br>
      PinProbe A1
    </div>
  </div>
  <div class="main cover-main">
    <div class="cover-layout">
      <div class="cover-copy">
        <div class="cover-kicker">{inline_md(cover_product)}</div>
        <div class="cover-title">{inline_md(cover_doc_name)}</div>
        <div class="cover-divider"></div>
        <div class="cover-subtitle">{html.escape(cover_subtitle)}</div>
        <div class="cover-info-line">
          <span>客户联调资料</span>
          <span>SCPI / UART</span>
          <span>{html.escape(doc_date)}</span>
        </div>
        <div class="cover-note">{inline_md(cover_note) if cover_note else "本文用于客户侧上位机联调、设备控制与维护参考。"}</div>
      </div>
      {cover_img_html}
    </div>
  </div>
  <div class="page-footer">
    <span>Accurate · Simple · Fast · Agile | General Test Systems Inc.</span>
    <span>{html.escape(footer_name)} | 1 / {total}</span>
  </div>
</div>"""
    rendered_pages = [cover]
    rendered_pages.extend(
        page_html(page, i + 2, total, doc_date, doc_kind, footer_name)
        for i, page in enumerate(pages)
    )
    return f"""<!DOCTYPE html>
<html lang="zh-CN">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>{html.escape(title)}</title>
<style>
  * {{ box-sizing:border-box; margin:0; padding:0; -webkit-print-color-adjust:exact; print-color-adjust:exact; }}
  body {{
    font-family:"Microsoft YaHei","SimSun","DengXian",Arial,sans-serif;
    background:#d7d9df;
    color:#1f2328;
    padding:18px;
    line-height:1.42;
  }}
  @page {{ size:A4 portrait; margin:0; }}
  .page {{
    width:210mm;
    height:297mm;
    margin:0 auto 18px;
    background:#fff;
    box-shadow:0 3px 14px rgba(0,0,0,.16);
    display:flex;
    flex-direction:column;
    overflow:hidden;
    break-after:page;
    page-break-after:always;
  }}
  .page:last-child {{ margin-bottom:0; }}
  .page-header {{
    padding:10mm 16mm 5mm;
    border-bottom:5px solid #d9e1ef;
    display:flex;
    justify-content:space-between;
    align-items:flex-start;
    gap:18px;
  }}
  .page-header .logo {{ height:28px; width:auto; display:block; overflow:visible; }}
  .svg-defs {{ position:absolute; width:0; height:0; overflow:hidden; }}
  .doc-meta {{ text-align:right; font-size:10.5px; color:#687381; white-space:nowrap; }}
  .doc-meta strong {{ display:block; color:#1f3864; font-size:13px; margin-bottom:3px; }}
  .main {{ flex:1; padding:6mm 16mm 6mm; overflow:hidden; }}
  .page-footer {{
    padding:3mm 16mm 4mm;
    border-top:5px solid #d9e1ef;
    color:#8b949e;
    font-size:9px;
    display:flex;
    justify-content:space-between;
  }}
  h1 {{ font-size:23px; color:#172033; letter-spacing:0; margin-bottom:3px; }}
  h3 {{ font-size:13px; color:#1f3864; margin:7px 0 5px; }}
  h4, h5 {{ font-size:12px; color:#374151; margin:6px 0 4px; }}
  p {{ font-size:10.7px; margin:4px 0 6px; }}
  .subtitle {{ color:#5b6573; font-size:12px; }}
  .section-title {{
    font-size:14px;
    color:#1f3864;
    font-weight:700;
    padding-bottom:4px;
    border-bottom:2px solid #1f3864;
    margin-bottom:6px;
  }}
  table {{ width:100%; border-collapse:collapse; font-size:10.3px; border:1.5px solid #2f3542; margin:6px 0 8px; }}
  th {{
    background:#1f3864;
    color:#fff;
    text-align:left;
    padding:5px 7px;
    border:1px solid #182b4c;
    font-weight:700;
  }}
  th code {{
    color:#fff;
    background:rgba(255,255,255,.16);
    border-color:rgba(255,255,255,.28);
    font-weight:700;
  }}
  td {{ padding:4px 7px; border:1px solid #c8d1dc; vertical-align:top; }}
  tbody tr:nth-child(even) td {{ background:#f5f8fc; }}
  code {{
    font-family:Consolas,"Courier New",monospace;
    font-size:9.5px;
    background:#f5f8fc;
    border:1px solid #dce3ed;
    border-radius:3px;
    padding:0 3px;
  }}
  pre {{
    white-space:pre-wrap;
    word-break:break-word;
    font-size:10px;
    background:#f5f8fc;
    border:1px solid #dce3ed;
    padding:6px 7px;
    margin:5px 0 7px;
  }}
  pre code {{ border:0; background:transparent; padding:0; font-size:10px; }}
  .note-box {{
    padding:7px 10px;
    font-size:10.8px;
    background:#f7fbf5;
    border-left:4px solid #2e7d32;
    margin:5px 0 7px;
  }}
  .danger-text {{ color:#c1121f; font-weight:800; }}
  .small-list {{ font-size:10.8px; color:#2f3542; padding-left:17px; margin:4px 0 7px; }}
  li {{ margin:1px 0; }}
  .chips {{ display:grid; grid-template-columns:repeat(4,1fr); gap:8px; margin:0 0 9px; }}
  .chip {{ border:1px solid #d9e1ef; border-left:4px solid #1f3864; padding:6px 8px; min-height:44px; background:#f8fafc; }}
  .chip:nth-child(2) {{ border-left-color:#2e7d32; }}
  .chip:nth-child(3) {{ border-left-color:#d97706; }}
  .chip:nth-child(4) {{ border-left-color:#6a1b9a; }}
  .chip .k {{ display:block; color:#687381; font-size:9.5px; margin-bottom:1px; }}
  .chip .v {{ display:block; color:#111827; font-size:12px; font-weight:700; }}
  .cover-page .page-header {{ padding-top:11mm; padding-bottom:8mm; align-items:center; }}
  .cover-header-text {{ min-width:40mm; }}
  .cover-doc-label {{ font-size:14px; color:#172033; font-weight:800; }}
  .cover-doc-subtitle {{ margin-top:3px; color:#687381; font-size:10px; }}
  .cover-page .logo {{ height:30px; }}
  .cover-main {{ justify-content:center; padding-top:8mm; padding-bottom:7mm; }}
  .cover-layout {{ display:flex; flex-direction:column; gap:11mm; justify-content:center; height:100%; }}
  .cover-copy {{ text-align:left; }}
  .cover-kicker {{ font-size:18px; color:#1f3864; font-weight:800; margin-bottom:4px; }}
  .cover-title {{ font-size:31px; line-height:1.24; color:#172033; font-weight:800; margin-bottom:12px; max-width:142mm; }}
  .cover-divider {{ width:86px; height:4px; background:#1f3864; margin:0 0 14px; }}
  .cover-subtitle {{ font-size:14px; color:#1f3864; margin-bottom:13px; }}
  .cover-info-line {{ display:flex; flex-wrap:wrap; gap:8px 20px; color:#374151; font-size:11px; font-weight:700; margin-bottom:10px; }}
  .cover-info-line span {{ padding-left:9px; border-left:3px solid #d9e1ef; }}
  .cover-info-line span:first-child {{ border-left-color:#1f3864; }}
  .cover-note {{ margin-top:0; padding:8px 10px; border-left:4px solid #2e7d32; background:#f7fbf5; color:#2f3542; font-size:11px; line-height:1.6; }}
  .cover-image-wrap {{ height:122mm; display:flex; align-items:center; justify-content:center; overflow:hidden; }}
  .cover-image-wrap img {{ max-width:100%; max-height:122mm; object-fit:contain; display:block; }}
  @media print {{
    html, body {{
      width:210mm;
      min-width:210mm;
      max-width:210mm;
      background:#fff;
      padding:0;
      overflow:visible;
      -webkit-print-color-adjust:exact;
      print-color-adjust:exact;
    }}
    .page {{
      width:210mm !important;
      height:297mm !important;
      min-height:297mm !important;
      margin:0;
      box-shadow:none;
      break-after:page;
      page-break-after:always;
      transform:none !important;
    }}
  }}
</style>
</head>
<body>
<svg class="svg-defs" aria-hidden="true" focusable="false" width="0" height="0">
  <defs>
{logo}
  </defs>
</svg>
{chr(10).join(rendered_pages)}
</body>
</html>
"""


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--markdown", required=True, type=Path)
    parser.add_argument("--template", required=True, type=Path)
    parser.add_argument("--html", required=True, type=Path)
    parser.add_argument("--date", default="2026-07-27")
    parser.add_argument("--cover-image", type=Path)
    parser.add_argument("--doc-kind", default="SCPI Reference")
    parser.add_argument("--cover-subtitle", default="通信指令 · 设备控制 · 异常处理")
    parser.add_argument("--footer-name", default="PinProbe A1 文档")
    args = parser.parse_args()

    title, blocks = parse_markdown(args.markdown)
    logo = extract_logo(args.template.read_text(encoding="utf-8"))
    cover_note, content_blocks = extract_cover_note(blocks)
    pages = merge_trailing_sparse_page(paginate(title, content_blocks))
    output = build_html(
        title,
        pages,
        logo,
        args.date,
        cover_note,
        args.cover_image,
        args.doc_kind,
        args.cover_subtitle,
        args.footer_name,
    )
    args.html.write_text(output, encoding="utf-8", newline="\n")
    print(f"Wrote {args.html} ({len(pages) + 1} pages)")


if __name__ == "__main__":
    main()
