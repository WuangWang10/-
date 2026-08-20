#!/usr/bin/env python3
"""Render the simulation CSV as a dependency-free SVG plot."""

import csv
import math
import sys
from pathlib import Path


def load_csv(path: Path):
    with path.open(newline="", encoding="utf-8") as source:
        return list(csv.DictReader(source))


def polyline(rows, x_key, y_key, transform):
    return " ".join(
        f"{transform(float(row[x_key]), float(row[y_key]))[0]:.2f},"
        f"{transform(float(row[x_key]), float(row[y_key]))[1]:.2f}"
        for row in rows
    )


def main():
    if len(sys.argv) != 3:
        raise SystemExit("Usage: plot_results.py INPUT.csv OUTPUT.svg")
    input_path, output_path = map(Path, sys.argv[1:])
    rows = load_csv(input_path)
    if not rows:
        raise SystemExit("CSV contains no data")

    width, height, margin = 900, 700, 70
    xs = [float(row[key]) for row in rows for key in ("ref_x", "x")]
    ys = [float(row[key]) for row in rows for key in ("ref_y", "y")]
    x_min, x_max = min(xs), max(xs)
    y_min, y_max = min(ys), max(ys)
    span = max(x_max - x_min, y_max - y_min) * 1.1
    center_x, center_y = (x_min + x_max) / 2, (y_min + y_max) / 2
    x_min, x_max = center_x - span / 2, center_x + span / 2
    y_min, y_max = center_y - span / 2, center_y + span / 2

    def transform(x, y):
        px = margin + (x - x_min) / (x_max - x_min) * (width - 2 * margin)
        py = height - margin - (y - y_min) / (y_max - y_min) * (height - 2 * margin)
        return px, py

    reference = polyline(rows, "ref_x", "ref_y", transform)
    actual = polyline(rows, "x", "y", transform)
    start_x, start_y = transform(float(rows[0]["x"]), float(rows[0]["y"]))
    end_x, end_y = transform(float(rows[-1]["x"]), float(rows[-1]["y"]))
    rmse = math.sqrt(
        sum(float(row["position_error"]) ** 2 for row in rows) / len(rows)
    )

    svg = f'''<svg xmlns="http://www.w3.org/2000/svg" width="{width}" height="{height}" viewBox="0 0 {width} {height}">
<rect width="100%" height="100%" fill="#ffffff"/>
<text x="{margin}" y="36" font-family="sans-serif" font-size="22" fill="#17212b">2D LQR circular trajectory tracking</text>
<text x="{margin}" y="58" font-family="sans-serif" font-size="13" fill="#52606d">RMSE: {rmse:.4f} m</text>
<rect x="{margin}" y="{margin}" width="{width - 2 * margin}" height="{height - 2 * margin}" fill="#f8fafb" stroke="#c7d0d9"/>
<polyline points="{reference}" fill="none" stroke="#e4572e" stroke-width="3" stroke-dasharray="8 6"/>
<polyline points="{actual}" fill="none" stroke="#177e89" stroke-width="2.5"/>
<circle cx="{start_x:.2f}" cy="{start_y:.2f}" r="6" fill="#f3a712"/>
<circle cx="{end_x:.2f}" cy="{end_y:.2f}" r="6" fill="#177e89"/>
<line x1="650" y1="35" x2="685" y2="35" stroke="#e4572e" stroke-width="3" stroke-dasharray="8 6"/>
<text x="693" y="40" font-family="sans-serif" font-size="13" fill="#17212b">reference</text>
<line x1="650" y1="55" x2="685" y2="55" stroke="#177e89" stroke-width="3"/>
<text x="693" y="60" font-family="sans-serif" font-size="13" fill="#17212b">point mass</text>
<text x="{width / 2}" y="{height - 18}" text-anchor="middle" font-family="sans-serif" font-size="14">x (m)</text>
<text x="20" y="{height / 2}" transform="rotate(-90 20 {height / 2})" text-anchor="middle" font-family="sans-serif" font-size="14">y (m)</text>
</svg>'''
    output_path.parent.mkdir(parents=True, exist_ok=True)
    output_path.write_text(svg, encoding="utf-8")


if __name__ == "__main__":
    main()
