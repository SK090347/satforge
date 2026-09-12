"""Thin subprocess wrapper around the satforge CLI."""

from __future__ import annotations

import os
import shutil
import subprocess
from dataclasses import dataclass
from pathlib import Path
from typing import List, Optional


@dataclass
class SatResult:
    status: str  # "SAT", "UNSAT", "UNKNOWN", "ERROR"
    model: Optional[List[int]] = None
    raw: str = ""
    returncode: int = 0


def _find_binary() -> Path:
    env = os.environ.get("SATFORGE_BIN")
    if env:
        p = Path(env)
        if p.is_file():
            return p
    which = shutil.which("satforge")
    if which:
        return Path(which)
    # Relative to repo: build/satforge
    here = Path(__file__).resolve().parents[2]
    for candidate in (here / "build" / "satforge", here / "build" / "Release" / "satforge"):
        if candidate.is_file():
            return candidate
    raise FileNotFoundError(
        "satforge binary not found. Build with `make build` or set SATFORGE_BIN."
    )


def solve_dimacs(text: str, *, binary: Optional[Path] = None) -> SatResult:
    bin_path = binary or _find_binary()
    proc = subprocess.run(
        [str(bin_path)],
        input=text,
        text=True,
        capture_output=True,
        timeout=60,
    )
    return _parse_output(proc.stdout, proc.returncode, proc.stderr)


def solve_file(path: str | Path, *, binary: Optional[Path] = None) -> SatResult:
    bin_path = binary or _find_binary()
    proc = subprocess.run(
        [str(bin_path), str(path)],
        text=True,
        capture_output=True,
        timeout=60,
    )
    return _parse_output(proc.stdout, proc.returncode, proc.stderr)


def _parse_output(stdout: str, returncode: int, stderr: str = "") -> SatResult:
    status = "UNKNOWN"
    model: Optional[List[int]] = None
    for line in stdout.splitlines():
        line = line.strip()
        if line.startswith("s "):
            token = line[2:].strip().upper()
            if "UNSAT" in token:
                status = "UNSAT"
            elif "SAT" in token:
                status = "SAT"
        elif line.startswith("v "):
            nums = [int(x) for x in line[2:].split() if x != "0"]
            model = nums
    if returncode == 10:
        status = "SAT"
    elif returncode == 20:
        status = "UNSAT"
    elif returncode not in (0, 10, 20) and status == "UNKNOWN":
        return SatResult(status="ERROR", raw=stdout + stderr, returncode=returncode)
    return SatResult(status=status, model=model, raw=stdout, returncode=returncode)
