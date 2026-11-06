#!/usr/bin/env python3
"""Generate dependency-free SVG charts for the focused JESON result set."""

from __future__ import annotations

import csv
import html
from dataclasses import dataclass
from pathlib import Path
from typing import Callable


ROOT = Path(__file__).resolve().parent.parent
DOM_SOURCE = ROOT / "docs/results/apple-arm64-2026-11-06-summary.csv"
TYPED_SOURCE = ROOT / "docs/results/apple-arm64-2026-11-06-typed-summary.csv"
OUTPUT = ROOT / "docs/results/charts"


@dataclass(frozen=True)
class Metric:
    filename: str
    title: str
    subtitle: str
    value: Callable[[dict[str, str]], float]
    display: Callable[[float], str]
    lower_is_better: bool = True
    fixed_max: float | None = None


DOM_METRICS = (
    Metric(
        "selected-binary-size.svg",
        "Binary size",
        "Stripped parse + statistics executable • lower is better",
        lambda row: float(row["stripped_bytes"]),
        lambda value: f"{value / 1024:.1f} KiB",
    ),
    Metric(
        "selected-conformance.svg",
        "JSON conformance",
        "Correct checks out of 136 • higher is better",
        lambda row: 100.0 * float(row["conformance_correct"]) / float(row["conformance_total"]),
        lambda value: f"{value:.1f}%",
        lower_is_better=False,
        fixed_max=100.0,
    ),
    Metric(
        "selected-parse-time.svg",
        "DOM parse time",
        "Canada + CITM + Twitter • best of 10 • lower is better",
        lambda row: float(row["parse_ms"]),
        lambda value: f"{value:.1f} ms",
    ),
    Metric(
        "selected-parse-memory.svg",
        "DOM parse peak allocation",
        "Sum of the three per-dataset peaks • lower is better",
        lambda row: float(row["parse_peak_bytes"]) / (1024.0 * 1024.0),
        lambda value: f"{value:.1f} MiB",
    ),
    Metric(
        "selected-stringify-time.svg",
        "Stringify time",
        "Canada + CITM + Twitter • best of 10 • lower is better",
        lambda row: float(row["stringify_ms"]),
        lambda value: f"{value:.1f} ms",
    ),
    Metric(
        "selected-prettify-time.svg",
        "Pretty-print time",
        "Canada + CITM + Twitter • best of 10 • lower is better",
        lambda row: float(row["prettify_ms"]),
        lambda value: f"{value:.1f} ms",
    ),
)

TYPED_METRICS = (
    Metric(
        "selected-typed-parse-time.svg",
        "JSON to application model",
        "Representative embedded config • best of 5 × 100,000 • lower is better",
        lambda row: float(row["parse_us"]),
        lambda value: f"{value:.3f} µs",
    ),
    Metric(
        "selected-typed-parse-memory.svg",
        "Application-model parse peak",
        "Parser + returned model heap peak • lower is better",
        lambda row: float(row["peak_bytes"]),
        lambda value: f"{value / 1024:.2f} KiB" if value >= 1024 else f"{value:.0f} B",
    ),
    Metric(
        "selected-typed-binary-size.svg",
        "Application-model executable size",
        "Stripped parse + semantic-check executable • lower is better",
        lambda row: float(row["stripped_bytes"]),
        lambda value: f"{value / 1024:.1f} KiB",
    ),
)


def load_rows(source_path: Path) -> list[dict[str, str]]:
    with source_path.open(newline="", encoding="utf-8") as source:
        return list(csv.DictReader(source))


def render(metric: Metric, rows: list[dict[str, str]]) -> str:
    values = [(row["library"], metric.value(row)) for row in rows]
    values.sort(key=lambda item: item[1], reverse=not metric.lower_is_better)

    width = 1000
    height = max(450, 125 + len(values) * 48 + 70)
    label_x = 32
    bar_x = 280
    bar_width = 570
    first_y = 125
    row_height = 48
    maximum = metric.fixed_max or max(value for _, value in values)

    lines = [
        f'<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 {width} {height}" role="img">',
        f"  <title>{html.escape(metric.title)}</title>",
        f'  <rect width="{width}" height="{height}" rx="12" fill="#ffffff" stroke="#e5e7eb"/>',
        f'  <text x="32" y="46" font-family="Arial, sans-serif" font-size="28" font-weight="700" fill="#111827">{html.escape(metric.title)}</text>',
        f'  <text x="32" y="72" font-family="Arial, sans-serif" font-size="15" fill="#6b7280">{html.escape(metric.subtitle)}</text>',
    ]

    for index, (library, value) in enumerate(values):
        y = first_y + index * row_height
        ratio = value / maximum if maximum else 0.0
        current_width = max(3.0, ratio * bar_width)
        if library == "JESON CompactDocument":
            color = "#f97316"
        elif library in ("JESON Value", "JESON typed"):
            color = "#f59e0b"
        else:
            color = "#3b82f6"
        lines.extend(
            (
                f'  <text x="{label_x}" y="{y + 16}" font-family="Arial, sans-serif" font-size="15" font-weight="600" fill="#1f2937">{html.escape(library)}</text>',
                f'  <rect x="{bar_x}" y="{y}" width="{bar_width}" height="24" rx="6" fill="#f3f4f6"/>',
                f'  <rect x="{bar_x}" y="{y}" width="{current_width:.1f}" height="24" rx="6" fill="{color}"/>',
                f'  <text x="870" y="{y + 17}" font-family="monospace" font-size="14" font-weight="600" fill="#374151">{html.escape(metric.display(value))}</text>',
            )
        )

    lines.extend(
        (
            f'  <circle cx="32" cy="{height - 27}" r="6" fill="#f97316"/>',
            f'  <text x="46" y="{height - 22}" font-family="Arial, sans-serif" font-size="13" fill="#6b7280">JESON modes</text>',
            f'  <circle cx="145" cy="{height - 27}" r="6" fill="#3b82f6"/>',
            f'  <text x="159" y="{height - 22}" font-family="Arial, sans-serif" font-size="13" fill="#6b7280">Comparison libraries</text>',
            f'  <text x="968" y="{height - 22}" text-anchor="end" font-family="Arial, sans-serif" font-size="13" fill="#6b7280">Apple arm64 • AppleClang 21 • 2026-11-06</text>',
            "</svg>",
            "",
        )
    )
    return "\n".join(lines)


def main() -> None:
    OUTPUT.mkdir(parents=True, exist_ok=True)
    dom_rows = load_rows(DOM_SOURCE)
    typed_rows = load_rows(TYPED_SOURCE)
    for metric in DOM_METRICS:
        (OUTPUT / metric.filename).write_text(render(metric, dom_rows), encoding="utf-8")
    for metric in TYPED_METRICS:
        (OUTPUT / metric.filename).write_text(render(metric, typed_rows), encoding="utf-8")


if __name__ == "__main__":
    main()
