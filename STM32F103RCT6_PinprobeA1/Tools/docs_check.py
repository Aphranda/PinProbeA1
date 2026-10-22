#!/usr/bin/env python3
"""Check docs metadata, index coverage, naming, and Markdown references.

PinProbeA1 adaptation of the HDRT100 docs gate (``tools/docs_check/docs_check.py``).

Policy:
  * every Markdown under ``docs/`` must start with a level-1 title and carry the
    governance metadata block (Status / Domain / Canonical / Related / Last updated);
  * historical material (``docs/legacy/``), archived assets (``docs/archive/``) and
    delivery sources (``docs/reports/`` non-README Markdown) are exempt from the
    metadata and naming rules, because they are frozen originals or are rendered
    straight into customer A4/PDF deliverables;
  * every Markdown must be listed in ``docs/README.md`` or in the README of one of
    its parent directories ("index nearby" rule);
  * ``docs/<path>.md`` references must resolve, except inside legacy/ and reports/,
    where upstream/obsolete paths are expected and reported as warnings.

Usage:
    python Tools/docs_check.py
    python Tools/docs_check.py --strict-names
"""

from __future__ import annotations

import argparse
import datetime
import re
import sys
from dataclasses import dataclass, field
from pathlib import Path

REQUIRED_METADATA = ("Status:", "Domain:", "Canonical:", "Related:", "Last updated:")

ALLOWED_STATUS = {"Draft", "Active", "Frozen", "Deprecated"}

ALLOWED_PREFIXES = {
    "ARCH",
    "CONTROL",
    "INTERFACE",
    "SCPI",
    "MULTINODE",
    "OTA",
    "STORAGE",
    "HARDWARE",
    "TOOLS",
    "VALIDATION",
    "RELEASE",
    "DOCS",
    "PROJECT",
    "LEGACY",
}

ALLOWED_SUFFIXES = {
    "ARCHITECTURE",
    "DESIGN",
    "PLAN",
    "TODO",
    "TASK_PROGRESS",
    "CHECKLIST",
    "PLAYBOOK",
    "COMPARISON",
    "EVALUATION",
    "COMMANDS",
    "ANALYSIS",
    "CONSTRAINTS",
    "REGISTRY",
    "REVIEW",
}

# Directories whose non-README Markdown is exempt from metadata/naming/index rules.
EXEMPT_DIRS = ("legacy/", "archive/", "reports/")

# Directories where broken ``docs/`` references are warnings instead of failures.
LENIENT_REF_DIRS = ("legacy/", "reports/")

LEGACY_NAME_ALLOWLIST: set[str] = set()

DOC_REF_RE = re.compile(r"docs[\\/][^\s`'\"<>)\]]+?\.md")


@dataclass
class CheckResult:
    failures: list[str] = field(default_factory=list)
    warnings: list[str] = field(default_factory=list)

    def fail(self, message: str) -> None:
        self.failures.append(message)
        print(f"FAIL {message}")

    def warn(self, message: str) -> None:
        self.warnings.append(message)
        print(f"WARN {message}")

    def ok(self, message: str) -> None:
        print(f"OK   {message}")


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--root", type=Path, default=Path.cwd())
    parser.add_argument(
        "--strict-names",
        action="store_true",
        help="Fail non-conforming filenames instead of warning about legacy/delivery names.",
    )
    return parser.parse_args()


def read_text(path: Path) -> str:
    return path.read_text(encoding="utf-8", errors="ignore")


def rel_posix(path: Path, docs_dir: Path) -> str:
    return path.relative_to(docs_dir).as_posix()


def is_exempt(rel: str) -> bool:
    if Path(rel).name == "README.md":
        return False
    return any(rel.startswith(prefix) for prefix in EXEMPT_DIRS)


def is_lenient_ref(rel: str) -> bool:
    return any(rel.startswith(prefix) for prefix in LENIENT_REF_DIRS)


def is_allowed_name(name: str) -> bool:
    if name in LEGACY_NAME_ALLOWLIST or name == "README.md":
        return True
    if not name.endswith(".md"):
        return False

    stem = name[:-3]
    stem = re.sub(r"_\d+$", "", stem)
    parts = stem.split("_")
    if len(parts) < 2:
        return False

    if len(parts) >= 3 and "_".join(parts[-2:]) == "TASK_PROGRESS":
        suffix = "TASK_PROGRESS"
        prefix_parts = parts[:-2]
    else:
        suffix = parts[-1]
        prefix_parts = parts[:-1]

    if suffix not in ALLOWED_SUFFIXES:
        return False

    for width in range(min(3, len(prefix_parts)), 0, -1):
        prefix = "_".join(prefix_parts[:width])
        if prefix in ALLOWED_PREFIXES:
            return True
    return False


def check_metadata(path: Path, docs_dir: Path, text: str, result: CheckResult) -> None:
    lines = text.splitlines()
    if not lines or not lines[0].startswith("# "):
        result.fail(f"{path}: first line must be a level-1 Markdown title")
        return

    header_window = lines[1:8]
    for field_name in REQUIRED_METADATA:
        if not any(line.startswith(field_name) for line in header_window):
            result.fail(f"{path}: missing metadata field near top: {field_name}")

    status_line = next((line for line in header_window if line.startswith("Status:")), None)
    if status_line:
        status = status_line.split(":", 1)[1].strip()
        if status not in ALLOWED_STATUS:
            result.fail(f"{path}: invalid Status value: {status}")

    canonical_line = next((line for line in header_window if line.startswith("Canonical:")), None)
    if canonical_line:
        expected = f"`docs/{rel_posix(path, docs_dir)}`"
        if expected not in canonical_line:
            result.fail(f"{path}: Canonical must include {expected}")

    date_line = next((line for line in header_window if line.startswith("Last updated:")), None)
    if date_line:
        value = date_line.split(":", 1)[1].strip()
        match = re.fullmatch(r"(\d{4})-(\d{2})-(\d{2})", value)
        if not match:
            result.fail(f"{path}: Last updated must be YYYY-MM-DD, got: {value}")
        else:
            try:
                datetime.date(int(match.group(1)), int(match.group(2)), int(match.group(3)))
            except ValueError:
                result.fail(f"{path}: invalid Last updated date: {value}")


def check_index_coverage(docs_files: list[Path], docs_dir: Path, result: CheckResult) -> None:
    index_text = ""
    index_path = docs_dir / "README.md"
    if index_path.exists():
        index_text = read_text(index_path)
    else:
        result.fail("docs/README.md is missing")

    readme_cache: dict[Path, str] = {}

    def readme_text(path: Path) -> str:
        if path not in readme_cache:
            readme_cache[path] = read_text(path) if path.exists() else ""
        return readme_cache[path]

    for path in docs_files:
        rel = rel_posix(path, docs_dir)
        if is_exempt(rel):
            continue

        needles = [rel, path.name]
        if any(needle in index_text for needle in needles):
            continue

        # "index nearby": any parent directory README (up to docs/) may host the entry.
        listed = False
        ancestor = path.parent
        while ancestor != docs_dir.parent and ancestor != docs_dir.parent.parent:
            candidate = ancestor / "README.md"
            if candidate.exists() and candidate != path:
                text = readme_text(candidate)
                if any(needle in text for needle in needles):
                    listed = True
                    break
            if ancestor == docs_dir:
                break
            ancestor = ancestor.parent

        if not listed:
            result.fail(f"{path}: not listed in docs/README.md or any parent README")


def check_references(root: Path, scan_files: list[tuple[Path, str]], result: CheckResult) -> None:
    for path, rel in scan_files:
        text = read_text(path)
        lenient_broken = 0
        for match in DOC_REF_RE.finditer(text):
            ref = match.group(0).replace("\\", "/")
            # Placeholders and glob patterns are legitimate in index tables.
            if any(token in ref for token in ("<", ">", "*", "?")):
                continue
            if not (root / ref).exists():
                if is_lenient_ref(rel):
                    lenient_broken += 1
                else:
                    result.fail(f"{path}: broken docs reference: {match.group(0)}")
        if lenient_broken:
            result.warn(
                f"{path}: {lenient_broken} broken docs reference(s) expected for frozen material "
                "(upstream/obsolete paths); not itemised"
            )


def check_conflict_markers(path: Path, text: str, result: CheckResult) -> None:
    for marker in ("<<<<<<<", "=======", ">>>>>>>"):
        if marker in text:
            result.fail(f"{path}: conflict marker found: {marker}")


def check_names(docs_files: list[Path], docs_dir: Path, strict: bool, result: CheckResult) -> None:
    for path in docs_files:
        rel = rel_posix(path, docs_dir)
        if is_allowed_name(path.name):
            continue
        message = f"{path}: filename does not match docs naming rules"
        if strict and not is_exempt(rel):
            result.fail(message)
        else:
            result.warn(message)


def main() -> int:
    args = parse_args()
    root = args.root.resolve()
    docs_dir = root / "docs"

    result = CheckResult()

    if not docs_dir.exists():
        result.fail(f"docs directory not found: {docs_dir}")
        return 1

    docs_files = sorted(docs_dir.rglob("*.md"))
    scan_files: list[tuple[Path, str]] = []

    for path in docs_files:
        rel = rel_posix(path, docs_dir)
        text = read_text(path)
        check_conflict_markers(path, text, result)
        if not is_exempt(rel):
            check_metadata(path, docs_dir, text, result)
        scan_files.append((path, rel))

    check_names(docs_files, docs_dir, args.strict_names, result)
    check_index_coverage(docs_files, docs_dir, result)

    root_readme = root / "README.md"
    if root_readme.exists():
        scan_files.append((root_readme, ""))

    check_references(root, scan_files, result)

    if result.failures:
        print(f"SUMMARY FAIL failures={len(result.failures)} warnings={len(result.warnings)}")
        return 1

    result.ok(f"docs check passed: files={len(docs_files)} warnings={len(result.warnings)}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
