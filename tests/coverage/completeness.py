import re
import unittest
from pathlib import Path

REPO = Path(__file__).resolve().parents[2]
HERE = Path(__file__).resolve().parent

CPP_LITERAL = re.compile(r'R"([^(\s]*)\((.*?)\)\1"|"((?:[^"\\\n]|\\.)*)"', re.S)
CPP_COMMENT = re.compile(r'//[^\n]*|/\*.*?\*/|R"([^(\s]*)\(.*?\)\1"|"(?:[^"\\\n]|\\.)*"|\'(?:[^\'\\\n]|\\.)*\'', re.S)


def read(path):
    return Path(path).read_text(encoding="utf-8")


def strip_cpp_comments(text):
    return CPP_COMMENT.sub(lambda match: match.group(0) if match.group(0)[0] in "\"'R" else " ", text)


def cpp_literals(text):
    literals = []
    for match in CPP_LITERAL.finditer(strip_cpp_comments(text)):
        literals.append(match.group(2) if match.group(2) is not None else bytes(match.group(3), "utf-8").decode("unicode_escape"))
    return literals


def cpp_function_body(text, name):
    match = re.search(r"\b" + re.escape(name) + r"\s*\([^;{]*?\)[^;{]*\{", text)
    if not match:
        raise AssertionError(f"could not find the function {name}")
    depth = 0
    for index in range(match.end() - 1, len(text)):
        depth += {"{": 1, "}": -1}.get(text[index], 0)
        if depth == 0:
            return match.start(), index + 1
    raise AssertionError(f"unbalanced braces in {name}")


def without_functions(text, names):
    for name in names:
        start, end = cpp_function_body(text, name)
        text = text[:start] + text[end:]
    return text


def files(root, patterns):
    found = []
    for pattern in patterns:
        found.extend(path for path in Path(root).rglob(pattern) if path.is_file())
    return sorted(set(found))


def mentions_word(text, word):
    return re.search(r"(?<![\w-])" + re.escape(word) + r"(?![\w-])", text) is not None


def mentions_quoted(text, word):
    return re.search(r"[\"'`]" + re.escape(word) + r"[\"'`]", text) is not None


def load_allowlist(name):
    path = HERE / name
    lines = [line.strip() for line in read(path).splitlines()] if path.exists() else []
    return [line for line in lines if line]


class CompletenessCase(unittest.TestCase):
    maxDiff = None

    def assertAllTested(self, kind, coverage, allowlist, hint):
        self.assertTrue(coverage, f"found no {kind} at all, so the scan of the sources is broken")
        listed = load_allowlist(allowlist)
        duplicates = sorted({item for item in listed if listed.count(item) > 1})
        untested = sorted(name for name, tested in coverage.items() if not tested)
        print(f"\n{len(untested)} of {len(coverage)} {kind} are not covered (listed in tests/coverage/{allowlist}):")
        for name in untested:
            print(f"  {name}")
        problems = []
        if duplicates:
            problems.append(f"tests/coverage/{allowlist} lists these more than once: {', '.join(duplicates)}")
        new = [name for name in untested if name not in listed]
        if new:
            problems.append(f"these {kind} are not covered: {', '.join(new)}. {hint}")
        fixed = sorted(name for name in set(listed) if coverage.get(name))
        if fixed:
            problems.append(f"these {kind} are covered now, so remove them from tests/coverage/{allowlist}: {', '.join(fixed)}")
        gone = sorted(name for name in set(listed) if name not in coverage)
        if gone:
            problems.append(f"tests/coverage/{allowlist} lists {kind} that no longer exist: {', '.join(gone)}")
        if listed != sorted(listed):
            problems.append(f"keep tests/coverage/{allowlist} sorted")
        if problems:
            self.fail("\n".join(problems))
