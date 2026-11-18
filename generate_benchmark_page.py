#!/usr/bin/env python3

import argparse
import html
import re
from datetime import datetime, timezone
from pathlib import Path


NUMBER = r"[0-9]+(?:\.[0-9]+)?(?:[eE][+-]?[0-9]+)?"


def parse_results(text: str) -> dict:
    result = {"seq_runs": {}, "par_runs": {}}

    header = re.search(
        r"Элементов:\s*(\d+),\s*запусков:\s*(\d+),\s*потоков:\s*(\d+)",
        text,
    )
    if header:
        result["size"], result["runs"], result["threads"] = map(
            int, header.groups()
        )

    for kind, run, seconds in re.findall(
        rf"^(seq|par) запуск (\d+):\s*({NUMBER})\s*с$", text, re.MULTILINE
    ):
        result[f"{kind}_runs"][int(run)] = float(seconds)

    for kind, seconds in re.findall(
        rf"^(seq|par) среднее:\s*({NUMBER})\s*с$", text, re.MULTILINE
    ):
        result[f"{kind}_average"] = float(seconds)

    speedup = re.search(rf"^Ускорение:\s*({NUMBER})(?:x)?$", text, re.MULTILINE)
    if speedup:
        result["speedup"] = float(speedup.group(1))

    return result


def format_number(value: float | None, suffix: str = "") -> str:
    if value is None:
        return "—"
    return f"{value:.6g}{suffix}"


def build_page(
    result: dict,
    required_speedup: float,
    run_url: str,
    raw_output: str,
) -> str:
    speedup = result.get("speedup")
    passed = speedup is not None and speedup >= required_speedup
    status_class = "passed" if passed else "failed"
    status_text = "Порог пройден" if passed else "Порог не пройден"
    timestamp = datetime.now(timezone.utc).strftime("%Y-%m-%d %H:%M UTC")

    rows = []
    run_numbers = sorted(set(result["seq_runs"]) | set(result["par_runs"]))
    for run in run_numbers:
        seq = result["seq_runs"].get(run)
        par = result["par_runs"].get(run)
        pair_speedup = seq / par if seq is not None and par else None
        rows.append(
            "<tr>"
            f"<td>{run}</td>"
            f"<td>{format_number(seq)}</td>"
            f"<td>{format_number(par)}</td>"
            f"<td>{format_number(pair_speedup, 'x')}</td>"
            "</tr>"
        )

    run_link = ""
    if run_url:
        safe_url = html.escape(run_url, quote=True)
        run_link = f'<a class="run-link" href="{safe_url}">Открыть запуск CI</a>'

    raw_details = ""
    if speedup is None:
        raw_details = (
            "<details open><summary>Вывод benchmark</summary>"
            f"<pre>{html.escape(raw_output)}</pre></details>"
        )

    return f"""<!doctype html>
<html lang="ru">
<head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>Parallel quicksort benchmark</title>
  <style>
    :root {{ color-scheme: light dark; font-family: system-ui, sans-serif; }}
    body {{ max-width: 920px; margin: 0 auto; padding: 32px 20px; }}
    h1 {{ margin-bottom: 8px; }}
    .meta {{ color: #777; margin-bottom: 24px; }}
    .status {{ padding: 12px 16px; border-radius: 8px; font-weight: 700; }}
    .passed {{ background: #1f883d22; color: #1f883d; }}
    .failed {{ background: #cf222e22; color: #cf222e; }}
    .cards {{ display: grid; grid-template-columns: repeat(auto-fit, minmax(150px, 1fr)); gap: 12px; margin: 24px 0; }}
    .card {{ border: 1px solid #8885; border-radius: 8px; padding: 14px; }}
    .card span {{ display: block; color: #777; font-size: 0.85rem; }}
    .card strong {{ font-size: 1.25rem; }}
    table {{ width: 100%; border-collapse: collapse; margin: 24px 0; }}
    th, td {{ border-bottom: 1px solid #8885; padding: 10px; text-align: right; }}
    th:first-child, td:first-child {{ text-align: left; }}
    tfoot {{ font-weight: 700; }}
    .run-link {{ display: inline-block; margin-top: 8px; }}
    pre {{ overflow-x: auto; padding: 16px; background: #8882; border-radius: 8px; }}
  </style>
</head>
<body>
  <h1>Parallel quicksort benchmark</h1>
  <div class="meta">Обновлено: {timestamp}<br>{run_link}</div>
  <div class="status {status_class}">{status_text}</div>

  <div class="cards">
    <div class="card"><span>Элементов</span><strong>{result.get('size', '—')}</strong></div>
    <div class="card"><span>Запусков</span><strong>{result.get('runs', '—')}</strong></div>
    <div class="card"><span>Потоков</span><strong>{result.get('threads', '—')}</strong></div>
    <div class="card"><span>Требуется</span><strong>{required_speedup:g}x</strong></div>
  </div>

  <table>
    <thead><tr><th>Запуск</th><th>Sequential, с</th><th>Parallel, с</th><th>Ускорение</th></tr></thead>
    <tbody>{''.join(rows)}</tbody>
    <tfoot><tr><td>Среднее</td><td>{format_number(result.get('seq_average'))}</td><td>{format_number(result.get('par_average'))}</td><td>{format_number(speedup, 'x')}</td></tr></tfoot>
  </table>
  {raw_details}
</body>
</html>
"""


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("results", type=Path)
    parser.add_argument("output", type=Path)
    parser.add_argument("--required-speedup", type=float, default=0.0)
    parser.add_argument("--run-url", default="")
    args = parser.parse_args()

    raw_output = args.results.read_text(encoding="utf-8")
    args.output.mkdir(parents=True, exist_ok=True)
    page = build_page(
        parse_results(raw_output),
        args.required_speedup,
        args.run_url,
        raw_output,
    )
    (args.output / "index.html").write_text(page, encoding="utf-8")


if __name__ == "__main__":
    main()
