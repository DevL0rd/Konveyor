#!/usr/bin/env python3
import re
import unittest

from completeness import REPO, CompletenessCase, cpp_literals, files, mentions_word, read
from test_config_nodes_tested import accepted_names

SETTINGS_QML = REPO / "src" / "settings" / "qml"
PAGES = SETTINGS_QML / "catalog" / "Pages.js"
TEST_ROOTS = [REPO / "tests" / "unit", REPO / "tests" / "nested"]
SETTINGS_TESTS = REPO / "tests" / "unit" / "settings"
JS_LITERAL = re.compile(r'"((?:[^"\\\n]|\\.)*)"|\'((?:[^\'\\\n]|\\.)*)\'')


def pages():
    return dict(re.findall(r'id:\s*"([a-z]+)".*?file:\s*"pages/(\w+)\.qml"', read(PAGES), re.S))


def written_keys():
    config = accepted_names()
    keys = set()
    for path in files(SETTINGS_QML, ["*.qml", "*.js"]):
        for match in JS_LITERAL.finditer(read(path)):
            literal = match.group(1) if match.group(1) is not None else match.group(2)
            keys.update(segment for segment in literal.split("/") if segment in config)
    return keys


def settings_test_literals():
    literals = []
    for path in files(SETTINGS_TESTS, ["*.cpp", "*.h"]):
        literals.extend(cpp_literals(read(path)))
    for path in files(SETTINGS_TESTS, ["*.py", "*.qml", "*.js"]):
        literals.append(read(path))
    return "\n".join(literals)


class TestSettingsTested(CompletenessCase):
    def test_scan_finds_pages_and_keys(self):
        self.assertEqual(pages().get("layout"), "LayoutPage")
        self.assertIn("focus-follows-mouse", written_keys())

    def test_every_settings_page_is_loaded_by_a_test(self):
        tests = "\n".join(read(path) for root in TEST_ROOTS for path in files(root, ["*.cpp", "*.h", "*.py", "*.qml"]))
        coverage = {page: mentions_word(tests, file) for page, file in pages().items()}
        self.assertAllTested("Settings pages", coverage, "untested-settings-pages.txt",
                             "Load each page's QML file by name in a test and check it shows without QML warnings.")

    def test_every_settings_key_is_written_by_a_test(self):
        literals = settings_test_literals()
        coverage = {key: mentions_word(literals, key) for key in written_keys()}
        self.assertAllTested("config keys the Settings UI writes", coverage, "untested-settings-keys.txt",
                             "Drive the Settings UI to write each one in a test under tests/unit/settings and check the KDL it saves.")


if __name__ == "__main__":
    unittest.main()
