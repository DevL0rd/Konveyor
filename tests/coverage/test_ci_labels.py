#!/usr/bin/env python3
import json
import os
import re
import subprocess
import unittest
from pathlib import Path

from completeness import REPO, CompletenessCase, read

WORKFLOW = REPO / ".github" / "workflows" / "ci.yml"


def ci_lines():
    workflow = read(WORKFLOW)
    lines = workflow.splitlines()
    for script in dict.fromkeys(re.findall(r"\b(tests/[\w/.-]+\.sh)\b", workflow)):
        lines += read(REPO / script).splitlines()
    return lines


def labels_run_by_ci():
    labels = set()
    for line in ci_lines():
        labels.update(re.findall(r"\bctest\b.*?\s-L\s+[\"']?\^?([\w-]+)", line))
        for script in re.findall(r"tools/(?:check|coverage)\.sh\b(.*)", line):
            labels.add("unit")
            if "--nested" in script.split():
                labels.add("nested")
    return labels


def registered_tests():
    listing = subprocess.run(["ctest", "--test-dir", os.environ["KONVEYOR_BUILD_DIR"], "--show-only=json-v1"],
                             capture_output=True, text=True, check=True).stdout
    data = json.loads(listing)
    graph = data["backtraceGraph"]
    tests = {}
    for test in data["tests"]:
        defined_in = Path(graph["files"][graph["nodes"][test["backtrace"]]["file"]])
        if not defined_in.is_relative_to(REPO / "tests"):
            continue
        properties = {entry["name"]: entry["value"] for entry in test.get("properties", [])}
        tests[test["name"]] = set(properties.get("LABELS", []))
    return tests


class TestCiLabels(CompletenessCase):
    def test_ci_runs_the_unit_label(self):
        self.assertIn("unit", labels_run_by_ci())

    def test_every_ctest_test_runs_in_ci(self):
        run = labels_run_by_ci()
        coverage = {name: bool(labels & run) for name, labels in registered_tests().items()}
        self.assertAllTested("ctest tests", coverage, "not-run-in-ci.txt",
                             f"No CI job runs them. Give each one a label that .github/workflows/ci.yml runs (it runs {', '.join(sorted(run))}).")


if __name__ == "__main__":
    unittest.main()
