#!/usr/bin/env python3
from __future__ import annotations

import argparse
import re
import shutil
import subprocess
import tempfile
from pathlib import Path


EDGE_CANDIDATES = [
    Path(r"C:\Program Files (x86)\Microsoft\Edge\Application\msedge.exe"),
    Path(r"C:\Program Files\Microsoft\Edge\Application\msedge.exe"),
]


def edge_path() -> Path:
    for candidate in EDGE_CANDIDATES:
        if candidate.exists():
            return candidate
    raise FileNotFoundError("Microsoft Edge was not found")


def pdf_page_count(path: Path) -> int:
    data = path.read_bytes()
    return len(re.findall(rb"/Type\s*/Page\b", data))


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--html", required=True, type=Path)
    parser.add_argument("--pdf", required=True, type=Path)
    args = parser.parse_args()

    edge = edge_path()
    user_data_dir = Path(tempfile.mkdtemp(prefix="edge-a4-pdf-"))
    try:
        args.pdf.parent.mkdir(parents=True, exist_ok=True)
        cmd = [
            str(edge),
            "--headless=new",
            "--disable-gpu",
            "--no-first-run",
            "--disable-extensions",
            "--allow-file-access-from-files",
            "--run-all-compositor-stages-before-draw",
            "--no-pdf-header-footer",
            "--print-to-pdf-no-header",
            f"--user-data-dir={user_data_dir}",
            f"--print-to-pdf={args.pdf.resolve()}",
            args.html.resolve().as_uri(),
        ]
        subprocess.run(cmd, check=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    finally:
        shutil.rmtree(user_data_dir, ignore_errors=True)

    if not args.pdf.exists() or args.pdf.stat().st_size == 0:
        raise RuntimeError(f"PDF was not created: {args.pdf}")
    print(f"Wrote {args.pdf} ({pdf_page_count(args.pdf)} pages)")


if __name__ == "__main__":
    main()
