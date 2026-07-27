#!/usr/bin/env python3
from __future__ import annotations

import argparse
import re
import subprocess
import tempfile
from pathlib import Path

from PIL import Image, ImageDraw


EDGE_CANDIDATES = [
    Path(r"C:\Program Files (x86)\Microsoft\Edge\Application\msedge.exe"),
    Path(r"C:\Program Files\Microsoft\Edge\Application\msedge.exe"),
]


def edge_path() -> Path:
    for candidate in EDGE_CANDIDATES:
        if candidate.exists():
            return candidate
    raise FileNotFoundError("Microsoft Edge was not found")


def extract_pages(html_text: str) -> tuple[str, str, list[str]]:
    head = html_text.split("<body>", 1)[0] + "<body>\n"
    defs_match = re.search(r'<svg class="svg-defs".*?</svg>', html_text, re.S)
    defs = defs_match.group(0) if defs_match else ""
    pages = re.findall(r'<!-- P\d+.*?</div>\s*(?=<!-- P\d+|</body>)', html_text, re.S)
    return head, defs, pages


def screenshot(edge: Path, html_file: Path, png_file: Path) -> None:
    user_data_dir = Path(tempfile.mkdtemp(prefix="edge-a4-shot-"))
    args = [
        str(edge),
        "--headless=new",
        "--disable-gpu",
        "--no-first-run",
        "--disable-extensions",
        "--hide-scrollbars",
        "--allow-file-access-from-files",
        f"--user-data-dir={user_data_dir}",
        "--window-size=900,1250",
        f"--screenshot={png_file.resolve()}",
        html_file.resolve().as_uri(),
    ]
    subprocess.run(args, check=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)


def make_contact_sheet(images: list[tuple[int, Path]], output: Path) -> None:
    thumbs: list[tuple[int, Image.Image]] = []
    for page_no, path in images:
        img = Image.open(path).convert("RGB")
        img.thumbnail((260, 365))
        thumbs.append((page_no, img.copy()))

    cols = 4
    cell_w, cell_h = 290, 410
    rows = (len(thumbs) + cols - 1) // cols
    sheet = Image.new("RGB", (cols * cell_w, rows * cell_h), "white")
    draw = ImageDraw.Draw(sheet)
    for idx, (page_no, img) in enumerate(thumbs):
        x = (idx % cols) * cell_w + 15
        y = (idx // cols) * cell_h + 30
        draw.text((x, y - 22), f"P{page_no}", fill=(31, 56, 100))
        sheet.paste(img, (x, y))
    sheet.save(output)


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--html", required=True, type=Path)
    parser.add_argument("--out-dir", required=True, type=Path)
    parser.add_argument("--pages", required=True, help="Comma-separated page numbers")
    args = parser.parse_args()

    args.out_dir.mkdir(parents=True, exist_ok=True)
    text = args.html.read_text(encoding="utf-8")
    head, defs, pages = extract_pages(text)
    selected = [int(p.strip()) for p in args.pages.split(",") if p.strip()]
    edge = edge_path()

    shots: list[tuple[int, Path]] = []
    for page_no in selected:
        if page_no < 1 or page_no > len(pages):
            continue
        page_html = args.out_dir / f"p{page_no:02d}.html"
        page_png = args.out_dir / f"p{page_no:02d}.png"
        page_html.write_text(head + defs + "\n" + pages[page_no - 1] + "\n</body></html>", encoding="utf-8")
        screenshot(edge, page_html, page_png)
        shots.append((page_no, page_png))

    sheet = args.out_dir / "contact_sheet.png"
    make_contact_sheet(shots, sheet)
    print(sheet)


if __name__ == "__main__":
    main()
