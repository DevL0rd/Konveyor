#!/usr/bin/env python3
import argparse
import atexit
import json
import os
import subprocess
import sys
import uuid
from pathlib import Path

REPO = Path(__file__).resolve().parents[2]
SOURCES = "sources.json"


def tracked_sources():
    listed = subprocess.run(["git", "ls-files", "--cached", "--others", "--exclude-standard", "--", "widgets", "src", "extras",
                             "install.sh", "uninstall.sh", ":!widgets/shared/common"],
                            cwd=REPO, capture_output=True, text=True, check=True).stdout.split()
    sources = []
    for name in listed:
        path = REPO / name
        if not path.is_file():
            continue
        with path.open("rb") as handle:
            first = handle.readline()
        if name.endswith(".py") or (first.startswith(b"#!") and b"python" in first):
            sources.append(name)
    return sorted(sources)


def start(directory):
    wanted = set(json.loads((Path(directory) / SOURCES).read_text()))
    hits = {}
    monitoring = sys.monitoring
    tool = monitoring.COVERAGE_ID
    monitoring.use_tool_id(tool, "konveyor-coverage")

    def on_line(code, line):
        filename = code.co_filename
        if filename in wanted:
            hits.setdefault(filename, set()).add(line)
        elif not filename.startswith("<"):
            resolved = os.path.realpath(filename)
            if resolved in wanted:
                hits.setdefault(resolved, set()).add(line)
        return monitoring.DISABLE

    def save():
        monitoring.set_events(tool, 0)
        if not hits:
            return
        payload = {str(Path(name).relative_to(REPO)): sorted(lines) for name, lines in hits.items()}
        target = Path(directory) / f"hits-{os.getpid()}-{uuid.uuid4().hex}.json"
        target.write_text(json.dumps(payload))

    monitoring.register_callback(tool, monitoring.events.LINE, on_line)
    monitoring.set_events(tool, monitoring.events.LINE)
    atexit.register(save)
    os.register_at_fork(after_in_child=hits.clear)


def executable_lines(path):
    code = compile(path.read_bytes(), str(path), "exec", dont_inherit=True)
    lines = set()
    pending = [code]
    while pending:
        current = pending.pop()
        lines.update(line for _, _, line in current.co_lines() if line)
        pending.extend(constant for constant in current.co_consts if hasattr(constant, "co_lines"))
    return lines


def merge(directory):
    hits = {}
    for record in Path(directory).glob("hits-*.json"):
        for name, lines in json.loads(record.read_text()).items():
            hits.setdefault(name, set()).update(lines)
    return hits


def summarize(directory):
    hits = merge(directory)
    files = {}
    for name in tracked_sources():
        executable = executable_lines(REPO / name)
        covered = executable & hits.get(name, set())
        files[name] = {"lines": len(executable), "covered": len(covered), "missing": sorted(executable - covered)}
    return files


def percent(covered, total):
    return 100.0 * covered / total if total else 100.0


def table(files):
    rows = ["| Python file | Lines | Covered | Line % |", "|---|---:|---:|---:|"]
    for name, entry in files.items():
        rows.append(f"| {name} | {entry['lines']} | {entry['covered']} | {percent(entry['covered'], entry['lines']):.1f} |")
    total = sum(entry["lines"] for entry in files.values())
    covered = sum(entry["covered"] for entry in files.values())
    rows.append(f"| **Total** | {total} | {covered} | {percent(covered, total):.2f} |")
    return rows, percent(covered, total)


def report(arguments):
    files = summarize(arguments.data)
    rows, overall = table(files)
    text = "\n".join(["## Python line coverage", "", *rows, ""])
    print(text)
    if arguments.markdown:
        Path(arguments.markdown).write_text(text)
    if arguments.json:
        Path(arguments.json).write_text(json.dumps({"lines": overall, "files": files}, indent=1))
    minimum = json.loads(Path(arguments.thresholds).read_text())["python"]["lines"]
    if overall + 1e-9 < minimum:
        print(f"Python line coverage {overall:.2f}% is below the {minimum:.2f}% threshold in {arguments.thresholds}")
        return 1
    print(f"Python line coverage {overall:.2f}% meets the {minimum:.2f}% threshold")
    return 0


def prepare(arguments):
    directory = Path(arguments.data)
    directory.mkdir(parents=True, exist_ok=True)
    (directory / SOURCES).write_text(json.dumps([str((REPO / name).resolve()) for name in tracked_sources()]))
    return 0


def main():
    parser = argparse.ArgumentParser(description="Line coverage for Konveyor's Python scripts")
    commands = parser.add_subparsers(dest="command", required=True)
    prepare_parser = commands.add_parser("prepare", help="list the scripts to trace before the tests run")
    prepare_parser.add_argument("--data", required=True)
    prepare_parser.set_defaults(run=prepare)
    report_parser = commands.add_parser("report", help="merge the hits and check them against the threshold")
    report_parser.add_argument("--data", required=True)
    report_parser.add_argument("--thresholds", required=True)
    report_parser.add_argument("--markdown")
    report_parser.add_argument("--json")
    report_parser.set_defaults(run=report)
    arguments = parser.parse_args()
    return arguments.run(arguments)


if __name__ == "__main__":
    sys.exit(main())
