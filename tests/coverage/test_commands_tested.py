#!/usr/bin/env python3
import ast
import re
import unittest
from pathlib import Path

from completeness import REPO, CompletenessCase, files, mentions_quoted, mentions_word, read, strip_cpp_comments

TEST_ROOTS = [REPO / "tests" / "unit", REPO / "tests" / "nested", REPO / "tests" / "install"]
TEST_PATTERNS = ["*.cpp", "*.h", "*.py", "*.sh", "*.qml"]
COMMAND = re.compile(r"^(?:--)?[a-z][a-z0-9-]+$")
CLI_MARKER = re.compile(r"konveyor\W{1,6}(?:msg|validate|restore-shortcuts)\b|bin/konveyor\b|KONVEYOR_CLI\b")


def scripts():
    found = [REPO / "src" / "cheatsheet" / "konveyor-cheatsheet.in"]
    for folder in [path for path in (REPO / "widgets").rglob("*") if path.is_dir() and path.name in ("bin", "service")]:
        found.extend(path for path in folder.iterdir() if path.is_file() and read(path).startswith("#!"))
    return sorted(found)


def names_in(node):
    return {child.id for child in ast.walk(node) if isinstance(child, ast.Name)}


def argument_names(function):
    tainted = {argument.arg for argument in function.args.args} | {"sys"}
    for _ in range(3):
        for child in ast.walk(function):
            if isinstance(child, ast.Assign) and names_in(child.value) & tainted:
                tainted |= {target.id for target in child.targets if isinstance(target, ast.Name)}
    return tainted


def strings_in(node):
    return {child.value for child in ast.walk(node) if isinstance(child, ast.Constant) and isinstance(child.value, str)}


def is_argument(node, tainted):
    while isinstance(node, (ast.Subscript, ast.Attribute)):
        node = node.value
    return isinstance(node, ast.Name) and node.id in tainted


def is_literal(node):
    if isinstance(node, (ast.Tuple, ast.List, ast.Set)):
        return all(is_literal(element) for element in node.elts)
    return isinstance(node, ast.Constant)


def dispatch_table(node, handlers):
    return isinstance(node, ast.Dict) and node.values and all(isinstance(value, ast.Name) and value.id in handlers
                                                              for value in node.values)


def compared_strings(function, handlers):
    tainted = argument_names(function)
    words = set()
    for child in ast.walk(function):
        if isinstance(child, ast.Compare):
            operands = [child.left, *child.comparators]
            if any(is_argument(operand, tainted) for operand in operands):
                words.update(*(strings_in(operand) for operand in operands if is_literal(operand)))
        elif dispatch_table(child, handlers):
            words.update(key.value for key in child.keys if isinstance(key, ast.Constant))
    return {word for word in words if isinstance(word, str) and COMMAND.match(word)}


def python_commands(path):
    tree = ast.parse(read(path))
    functions = {node.name: node for node in tree.body if isinstance(node, ast.FunctionDef)}
    if "main" not in functions:
        return set()
    parsers = [functions["main"]]
    tainted = argument_names(functions["main"])
    for call in ast.walk(functions["main"]):
        if isinstance(call, ast.Call) and isinstance(call.func, ast.Name) and call.func.id in functions:
            whole = len(call.args) == 1 and (isinstance(call.args[0], ast.Name)
                                             or (isinstance(call.args[0], ast.Subscript) and isinstance(call.args[0].slice, ast.Slice)))
            if whole and names_in(call.args[0]) & tainted:
                parsers.append(functions[call.func.id])
    return set().union(*(compared_strings(function, set(functions)) for function in parsers))


def shell_commands(path):
    words = set()
    for block in re.findall(r"\bcase\b.*?\bin\b(.*?)\besac\b", read(path), re.S):
        for arm in re.findall(r"^\s*([\w|-]+)\)", block, re.M):
            words.update(word for word in arm.split("|") if COMMAND.match(word))
    return words


def script_commands():
    commands = {}
    for path in scripts():
        name = path.name.removesuffix(".in")
        found = python_commands(path) if "python" in read(path).splitlines()[0] else shell_commands(path)
        for word in found:
            commands[f"{name} {word}"] = (name, word)
    return commands


def cli_commands():
    text = strip_cpp_comments(read(REPO / "src" / "cli" / "main.cpp"))
    commands = {}
    for word in re.findall(r'command\s*==\s*QLatin1String\("([a-z-]+)"\)', text):
        commands[f"konveyor {word}"] = word
    for word in re.findall(r'\{QStringLiteral\("([a-z-]+)"\),\s*[\[{]', text):
        commands[f"konveyor msg {word}"] = word
    for word in re.findall(r'QCommandLineOption\s+\w+\(\s*\{?QStringLiteral\("[a-z]"\),\s*QStringLiteral\("([a-z-]+)"\)', text):
        commands[f"konveyor --{word}"] = "--" + word
    for word in re.findall(r'QCommandLineOption\s+\w+\(\s*QStringLiteral\("([a-z-]+)"\)', text):
        commands[f"konveyor --{word}"] = "--" + word
    return commands


def exercised(word, texts):
    return any(mentions_word(text, word) if word.startswith("--") else mentions_quoted(text, word) for text in texts)


class TestCommandsTested(CompletenessCase):
    def setUp(self):
        self.tests = [read(path) for root in TEST_ROOTS for path in files(root, TEST_PATTERNS)]

    def test_scan_finds_known_commands(self):
        self.assertIn("portal-games --set-art", script_commands())
        self.assertIn("panel-launcher migrate", script_commands())
        self.assertIn("linux-plasma-screen-rotate toggle", script_commands())
        self.assertIn("konveyor msg focused-window", cli_commands())
        self.assertIn("konveyor --json", cli_commands())

    def test_every_script_command_is_run_by_a_test(self):
        coverage = {}
        for key, (script, word) in script_commands().items():
            texts = [text for text in self.tests if mentions_word(text, script) or mentions_word(text, Path(script).stem)]
            coverage[key] = exercised(word, texts)
        self.assertAllTested("script commands", coverage, "untested-commands.txt",
                             "Run the script with each one from a test that names the script.")

    def test_every_cli_command_is_run_by_a_test(self):
        texts = [text for text in self.tests if CLI_MARKER.search(text)]
        coverage = {key: exercised(word, texts) for key, word in cli_commands().items()}
        self.assertAllTested("konveyor CLI commands", coverage, "untested-cli-commands.txt",
                             "Run the konveyor CLI with each one from a test.")


if __name__ == "__main__":
    unittest.main()
