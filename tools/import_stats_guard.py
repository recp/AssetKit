#!/usr/bin/env python3
"""Run assetkit_import_stats and compare importer shape against a JSON baseline.

The C tool intentionally prints benchmark timings plus importer shape as TSV.
This wrapper makes the stable part of that output useful as a regression guard:
primitive type counts, owned/accessor index path counts, index component
histogram, index count, and owned index bytes.

Examples:

  tools/import_stats_guard.py --write-baseline /tmp/assetkit-baseline.json \
    /path/to/model.dae /path/to/model.gltf /path/to/model.obj

  tools/import_stats_guard.py --baseline /tmp/assetkit-baseline.json \
    --ignore-time /path/to/model.dae /path/to/model.gltf /path/to/model.obj
"""

from __future__ import annotations

import argparse
import json
import subprocess
import sys
from dataclasses import dataclass
from pathlib import Path
from typing import Iterable


SHAPE_FIELDS = (
    "prims",
    "points",
    "lines",
    "triangles",
    "polygons",
    "owned",
    "accessor",
    "u8",
    "u16",
    "u32",
    "indices",
    "owned_bytes",
)

TIME_FIELDS = ("median_ms",)


@dataclass(slots=True)
class Diff:
    key: str
    field: str
    expected: object
    actual: object


def read_paths_file(path: str) -> list[str]:
    out: list[str] = []
    for raw in Path(path).read_text(encoding="utf-8").splitlines():
        line = raw.strip()
        if not line or line.startswith("#"):
            continue
        out.append(line)
    return out


def stats_bin_default() -> str:
    repo = Path(__file__).resolve().parents[1]
    candidate = repo / "build" / "assetkit_import_stats"
    return str(candidate)


def run_stats(stats_bin: str, paths: list[str], iterations: int, warmup: int) -> list[dict[str, object]]:
    cmd = [stats_bin, "-n", str(iterations), "-w", str(warmup), *paths]
    proc = subprocess.run(cmd, check=False, text=True, capture_output=True)
    if proc.stderr:
        sys.stderr.write(proc.stderr)
    if proc.returncode != 0:
        raise SystemExit(proc.returncode)
    sys.stdout.write(proc.stdout)
    return parse_tsv(proc.stdout)


def parse_tsv(text: str) -> list[dict[str, object]]:
    lines = [line for line in text.splitlines() if line.strip()]
    if not lines:
        return []
    header = lines[0].split("\t")
    rows: list[dict[str, object]] = []
    for line in lines[1:]:
        parts = line.split("\t")
        if len(parts) != len(header):
            raise SystemExit(f"Malformed TSV row: {line}")
        row: dict[str, object] = dict(zip(header, parts))
        for field in ("iters", *SHAPE_FIELDS):
            if field in row:
                row[field] = int(row[field])
        for field in ("min_ms", "avg_ms", "median_ms", "max_ms"):
            row[field] = float(row[field])
        rows.append(row)
    return rows


def row_key(row: dict[str, object], mode: str) -> str:
    if mode == "path":
        return str(row["path"])
    return str(row["file"])


def rows_by_key(rows: Iterable[dict[str, object]], mode: str) -> dict[str, dict[str, object]]:
    keyed: dict[str, dict[str, object]] = {}
    for row in rows:
        key = row_key(row, mode)
        if key in keyed:
            raise SystemExit(f"Duplicate baseline key {key!r}; rerun with --key path")
        keyed[key] = row
    return keyed


def write_baseline(path: str, rows: list[dict[str, object]], key_mode: str) -> None:
    data = {
        "version": 1,
        "key": key_mode,
        "fields": [*SHAPE_FIELDS, *TIME_FIELDS],
        "rows": rows_by_key(rows, key_mode),
    }
    Path(path).write_text(json.dumps(data, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    print(f"Wrote baseline: {path}", flush=True)


def load_baseline(path: str) -> tuple[str, dict[str, dict[str, object]]]:
    data = json.loads(Path(path).read_text(encoding="utf-8"))
    if int(data.get("version", 0)) != 1:
        raise SystemExit(f"Unsupported baseline version in {path}")
    key_mode = str(data.get("key") or "file")
    rows = data.get("rows")
    if not isinstance(rows, dict):
        raise SystemExit(f"Invalid baseline rows in {path}")
    return key_mode, rows


def compare_rows(
    current_rows: list[dict[str, object]],
    baseline_path: str,
    requested_key_mode: str,
    ignore_time: bool,
    time_tolerance_pct: float,
) -> int:
    baseline_key_mode, expected = load_baseline(baseline_path)
    key_mode = requested_key_mode or baseline_key_mode
    current = rows_by_key(current_rows, key_mode)
    diffs: list[Diff] = []

    for key, expected_row in expected.items():
        actual_row = current.get(key)
        if actual_row is None:
            diffs.append(Diff(key, "<row>", "present", "missing"))
            continue

        baseline_fields = expected_row.get("fields")
        if isinstance(baseline_fields, list):
            shape_fields = tuple(field for field in SHAPE_FIELDS if field in baseline_fields)
        else:
            shape_fields = tuple(field for field in SHAPE_FIELDS if field in expected_row)

        for field in shape_fields:
            if int(actual_row[field]) != int(expected_row[field]):
                diffs.append(Diff(key, field, expected_row[field], actual_row[field]))

        if not ignore_time and "median_ms" in expected_row:
            expected_ms = float(expected_row["median_ms"])
            actual_ms = float(actual_row["median_ms"])
            allowed = expected_ms * (1.0 + time_tolerance_pct / 100.0)
            if actual_ms > allowed:
                diffs.append(Diff(key, "median_ms", f"<={allowed:.3f}", f"{actual_ms:.3f}"))

    for key in current:
        if key not in expected:
            diffs.append(Diff(key, "<row>", "missing", "present"))

    if diffs:
        for diff in diffs:
            print(
                f"DIFF key={diff.key!r} field={diff.field} "
                f"expected={diff.expected!r} actual={diff.actual!r}",
                file=sys.stderr,
            )
        print(f"IMPORT_STATS_GUARD FAIL diffs={len(diffs)}", file=sys.stderr)
        return 1

    print(f"IMPORT_STATS_GUARD OK rows={len(current)}", flush=True)
    return 0


def parse_args(argv: list[str]) -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("paths", nargs="*", help="Input asset paths")
    parser.add_argument("--paths-file", help="Newline-delimited input asset path list")
    parser.add_argument("--stats-bin", default=stats_bin_default(), help="Path to assetkit_import_stats")
    parser.add_argument("--iterations", "-n", type=int, default=7)
    parser.add_argument("--warmup", "-w", type=int, default=2)
    parser.add_argument("--baseline", help="JSON baseline to compare against")
    parser.add_argument("--write-baseline", help="Write JSON baseline from current run")
    parser.add_argument("--key", choices=("file", "path"), default=None)
    parser.add_argument("--ignore-time", action="store_true", help="Compare stable shape only")
    parser.add_argument("--time-tolerance-pct", type=float, default=35.0)
    return parser.parse_args(argv)


def main(argv: list[str]) -> int:
    args = parse_args(argv)
    paths = list(args.paths)
    if args.paths_file:
        paths.extend(read_paths_file(args.paths_file))
    if not paths:
        raise SystemExit("No input paths. Pass paths or --paths-file.")
    if args.iterations <= 0 or args.warmup < 0:
        raise SystemExit("Iterations must be positive and warmup must be non-negative.")

    rows = run_stats(args.stats_bin, paths, args.iterations, args.warmup)
    if args.write_baseline:
        write_baseline(args.write_baseline, rows, args.key or "file")
    if args.baseline:
        return compare_rows(
            rows,
            args.baseline,
            args.key,
            args.ignore_time,
            args.time_tolerance_pct,
        )
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))
