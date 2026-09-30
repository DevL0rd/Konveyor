#!/usr/bin/env python3
import argparse
import json
import sys
from pathlib import Path

REPO = Path(__file__).resolve().parents[2]


def area(name):
    parts = Path(name).parts
    depth = 3 if parts[:2] in (("src", "core"), ("src", "effect"), ("src", "settings")) else 2
    return "/".join(parts[:depth][:-1] if len(parts) <= depth else parts[:depth])


def load(export):
    totals = {}
    for entry in json.loads(Path(export).read_text())["data"][0]["files"]:
        name = Path(entry["filename"]).resolve()
        if not name.is_relative_to(REPO / "src"):
            continue
        summary = entry["summary"]
        group = totals.setdefault(area(str(name.relative_to(REPO))), {"lines": [0, 0], "branches": [0, 0], "functions": [0, 0]})
        for key in group:
            group[key][0] += summary[key]["covered"]
            group[key][1] += summary[key]["count"]
    return dict(sorted(totals.items()))


def percent(pair):
    return 100.0 * pair[0] / pair[1] if pair[1] else 100.0


def cell(pair):
    return f"{percent(pair):.1f}% ({pair[0]}/{pair[1]})"


def table(totals):
    overall = {key: [sum(group[key][0] for group in totals.values()), sum(group[key][1] for group in totals.values())]
               for key in ("lines", "branches", "functions")}
    rows = ["| C++ directory | Lines | Branches | Functions |", "|---|---:|---:|---:|"]
    for name, group in totals.items():
        rows.append(f"| {name} | {cell(group['lines'])} | {cell(group['branches'])} | {cell(group['functions'])} |")
    rows.append(f"| **Total** | {cell(overall['lines'])} | {cell(overall['branches'])} | {cell(overall['functions'])} |")
    return rows, overall


def main():
    parser = argparse.ArgumentParser(description="Summarize llvm-cov output for Konveyor and check it against the thresholds")
    parser.add_argument("--export", required=True, help="llvm-cov export -summary-only output")
    parser.add_argument("--thresholds", required=True)
    parser.add_argument("--markdown")
    arguments = parser.parse_args()
    rows, overall = table(load(arguments.export))
    text = "\n".join(["## C++ coverage", "", *rows, ""])
    print(text)
    if arguments.markdown:
        Path(arguments.markdown).write_text(text)
    minimum = json.loads(Path(arguments.thresholds).read_text())["cpp"]
    failed = False
    for key in ("lines", "branches"):
        measured = percent(overall[key])
        if measured + 1e-9 < minimum[key]:
            print(f"C++ {key} coverage {measured:.2f}% is below the {minimum[key]:.2f}% threshold in {arguments.thresholds}")
            failed = True
        else:
            print(f"C++ {key} coverage {measured:.2f}% meets the {minimum[key]:.2f}% threshold")
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main())
