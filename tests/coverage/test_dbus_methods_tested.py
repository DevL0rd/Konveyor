#!/usr/bin/env python3
import re
import unittest

from completeness import REPO, CompletenessCase, files, read, strip_cpp_comments

INTERFACES = ["org.kde.Konveyor", "org.devl0rd.KontrolPanel"]
TEST_ROOTS = [REPO / "tests" / "unit", REPO / "tests" / "nested", REPO / "tests" / "install"]
TEST_PATTERNS = ["*.cpp", "*.h", "*.py", "*.sh", "*.qml"]


def interface_members():
    members = {}
    for path in files(REPO / "src", ["*.h"]):
        text = strip_cpp_comments(read(path))
        declared = re.search(r'Q_CLASSINFO\("D-Bus Interface",\s*"([^"]+)"\)', text)
        if not declared or declared.group(1) not in INTERFACES:
            continue
        owner = re.search(r"\bclass\s+(\w+)", text).group(1)
        for name in re.findall(r"\bQ_SCRIPTABLE\s+[\w:<>]+\s+(\w+)\s*\(", text):
            members[f"{declared.group(1)}.{name}"] = (declared.group(1), owner, name)
    return members


def called(interface, owner, name, tests):
    call = re.compile(r"[.>(]" + name + r"\b|[\"']" + name + r"[\"'(]")
    return any((interface in text or owner in text) and call.search(text) for text in tests)


class TestDBusMethodsTested(CompletenessCase):
    def test_scan_finds_both_interfaces(self):
        found = {interface for interface, _, _ in interface_members().values()}
        self.assertEqual(found, set(INTERFACES))

    def test_every_dbus_method_is_called_by_a_test(self):
        tests = [read(path) for root in TEST_ROOTS for path in files(root, TEST_PATTERNS)]
        coverage = {key: called(*member, tests) for key, member in interface_members().items()}
        self.assertAllTested("D-Bus methods and signals", coverage, "untested-dbus-methods.txt",
                             "Call each one from a unit or nested test that names its interface or class.")


if __name__ == "__main__":
    unittest.main()
