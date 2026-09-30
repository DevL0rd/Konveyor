#!/usr/bin/env python3
import re
import unittest

from completeness import REPO, CompletenessCase, cpp_literals, files, read, strip_cpp_comments

LITERAL = r'QStringLiteral\("([a-z0-9-]+)"\)'
REGISTRATIONS = [
    (re.compile(r"\binsert\(\s*" + LITERAL), "{}"),
    (re.compile(r"(?<![\w.])[a-z]+Of\(\s*" + LITERAL), "{}"),
    (re.compile(r"\bnode\.name\s*==\s*QLatin1String\(\"([a-z0-9-]+)\"\)"), "{}"),
    (re.compile(r"\{\s*" + LITERAL + r",\s*&\w+::\w+\s*\}"), "{}"),
    (re.compile(r"\bdecodeEdgeScroll\(\s*\w+,\s*" + LITERAL), "{}"),
    (re.compile(r"\baddStateHandlers\(\s*table,\s*" + LITERAL), "{}-color"),
    (re.compile(r"\baddStateHandlers\(\s*table,\s*" + LITERAL), "{}-gradient"),
    (re.compile(r"\baddPaintHandlers\(\s*table,\s*" + LITERAL), "{}"),
    (re.compile(r"\baddPaintHandlers\(\s*table,\s*" + LITERAL + r",\s*" + LITERAL), None),
]


def accepted_names():
    names = set()
    for path in files(REPO / "src" / "core" / "config", ["*.cpp"]):
        text = strip_cpp_comments(read(path))
        for pattern, template in REGISTRATIONS:
            for match in pattern.finditer(text):
                names.add(template.format(match.group(1)) if template else match.group(2))
    return names


def test_inputs():
    literals = []
    for path in files(REPO / "tests" / "unit" / "config", ["*.cpp", "*.h"]):
        literals.extend(literal for literal in cpp_literals(read(path)) if re.search(r"[{};=\n]", literal))
    return "\n".join(literals)


def used(name, inputs):
    return re.search(r"(?:^|[\s{;])" + re.escape(name) + r"(?=[\s=;{}]|$)", inputs, re.M) is not None


class TestConfigNodesTested(CompletenessCase):
    def test_scan_finds_known_nodes(self):
        names = accepted_names()
        for name in ("gaps", "window-open", "active-color", "urgent-gradient", "trigger-height", "is-urgent", "width-above",
                     "min-height", "cooldown-ms", "gradient", "include", "proportion"):
            self.assertIn(name, names)

    def test_every_config_node_is_used_by_a_config_test(self):
        inputs = test_inputs()
        coverage = {name: used(name, inputs) for name in accepted_names()}
        self.assertAllTested("config node and property names", coverage, "untested-config-nodes.txt",
                             "Load a config that uses each one in a test under tests/unit/config.")


if __name__ == "__main__":
    unittest.main()
