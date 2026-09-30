#!/usr/bin/env python3
import re
import unittest

from completeness import REPO, CompletenessCase, cpp_literals, files, read, strip_cpp_comments, without_functions

ENGINE = REPO / "src" / "core" / "layout" / "engine"
LAYOUT_TESTS = REPO / "tests" / "unit" / "layout"
SMOKE_FILE = "test_layout_actions.cpp"
SMOKE_FUNCTIONS = ["simpleActionNames", "directionalPrefixes", "compositorActionNames", "directionalActionNames",
                   "everyNamedMonitorActionSucceeds", "actionsWithArgumentsSucceed", "badArgumentsAreReported"]
NAME = r'"([a-z0-9-]+)"'


def literal_list(text):
    return re.findall(NAME, text)


def engine_actions():
    names = set()
    for path in files(ENGINE, ["*actions*.cpp"]):
        text = strip_cpp_comments(read(path))
        names.update(re.findall(r"\badd\w*\(\s*(?:table,\s*)?(?:QStringLiteral\()?" + NAME, text))
        for pair in re.findall(r"\?\s*" + NAME + r"\s*:\s*" + NAME, text):
            names.update(pair)
        for array in re.findall(r"names\[\]\s*=\s*\{([^}]*)\}", text):
            names.update(literal_list(array))
        directions = re.search(r"directionNames\(\)\s*\{[^}]*?\{([^}]*)\}", text)
        for prefix in re.findall(r"\baddDirectionalActions\(\s*table,\s*QStringLiteral\(" + NAME, text):
            names.update(prefix + direction for direction in literal_list(directions.group(1)))
    return {name for name in names if not name.endswith("-")}


def config_actions():
    text = strip_cpp_comments(read(REPO / "src" / "core" / "config" / "binds.cpp"))
    simple = re.search(r"kSimpleActions\[\]\s*=\s*\{([^}]*)\}", text).group(1)
    special = re.search(r"kSpecialActions\[\]\s*=\s*\{(.*?)\};", text, re.S).group(1)
    return set(literal_list(simple)) | set(re.findall(r"\{\s*" + NAME + r",", special))


def behaviour_test_literals():
    literals = set()
    for path in files(LAYOUT_TESTS, ["*.cpp", "*.h"]):
        text = strip_cpp_comments(read(path))
        if path.name == SMOKE_FILE:
            text = without_functions(text, SMOKE_FUNCTIONS)
        literals.update(cpp_literals(text))
    return literals


class TestActionsTested(CompletenessCase):
    def test_scan_matches_the_config_parser(self):
        engine = engine_actions()
        self.assertIn("focus-monitor-next", engine)
        self.assertIn("close-overview", engine)
        missing = sorted(config_actions() - engine)
        self.assertEqual(missing, [], "the config parser accepts actions that the scan of the engine tables did not find")

    def test_every_action_has_a_behaviour_test(self):
        literals = behaviour_test_literals()
        coverage = {name: name in literals for name in engine_actions()}
        self.assertAllTested("layout actions", coverage, "untested-actions.txt",
                             f"Add a test under tests/unit/layout that performs each one and checks what it did "
                             f"(the smoke lists in {SMOKE_FILE} do not count).")


if __name__ == "__main__":
    unittest.main()
