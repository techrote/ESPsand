#!/usr/bin/env python3
"""Cross-platform clang-format wrapper used by local development and CI."""

from __future__ import annotations

import argparse
import shutil
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
SOURCE_ROOTS = ("src", "lib", "test")
SOURCE_SUFFIXES = {".c", ".cc", ".cpp", ".h", ".hh", ".hpp"}


def source_files() -> list[Path]:
    files: list[Path] = []
    for relative_root in SOURCE_ROOTS:
        source_root = ROOT / relative_root
        if not source_root.exists():
            continue
        files.extend(
            path
            for path in source_root.rglob("*")
            if path.is_file() and path.suffix.lower() in SOURCE_SUFFIXES
        )
    return sorted(files)


def main() -> int:
    parser = argparse.ArgumentParser()
    mode = parser.add_mutually_exclusive_group(required=True)
    mode.add_argument("--check", action="store_true", help="fail when formatting differs")
    mode.add_argument("--write", action="store_true", help="rewrite source files in place")
    args = parser.parse_args()

    formatter = shutil.which("clang-format")
    if formatter is None:
        print(
            "clang-format was not found. Install requirements-dev.txt first.",
            file=sys.stderr,
        )
        return 2

    files = source_files()
    if not files:
        print("No C/C++ source files found.")
        return 0

    command = [formatter]
    if args.check:
        command.extend(["--dry-run", "--Werror"])
    else:
        command.append("-i")
    command.extend(str(path) for path in files)

    print(f"{'checking' if args.check else 'formatting'} {len(files)} source files")
    return subprocess.run(command, cwd=ROOT, check=False).returncode


if __name__ == "__main__":
    raise SystemExit(main())
