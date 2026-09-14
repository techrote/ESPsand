#!/usr/bin/env python3
"""Run the same deterministic checks used by GitHub Actions."""

from __future__ import annotations

import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def run(label: str, command: list[str]) -> None:
    print(f"\n== {label} ==")
    subprocess.run(command, cwd=ROOT, check=True)


def main() -> int:
    python = sys.executable
    run("format", [python, "tools/format.py", "--check"])
    run("host tests", [python, "-m", "platformio", "test", "-e", "native"])
    run("ESP32-S3 firmware compile", [python, "-m", "platformio", "run", "-e", "esp32s3"])
    run(
        "ESP32-S3 minimal bringup compile",
        [python, "-m", "platformio", "run", "-e", "esp32s3_bringup"],
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
