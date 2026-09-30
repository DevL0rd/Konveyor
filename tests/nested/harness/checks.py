import json
import os
import subprocess
import traceback
from pathlib import Path

from kwinsession import konveyor
from nested import REPO, build_dir


class Checks:
    def __init__(self):
        self.problems = []

    def expect(self, condition, message):
        print(("ok       " if condition else "PROBLEM  ") + message)
        if not condition:
            self.problems.append(message)
        return condition

    def equal(self, actual, expected, message):
        return self.expect(actual == expected, message if actual == expected else f"{message}: expected {expected!r}, got {actual!r}")

    def run(self, *steps):
        for step in steps:
            print(f"--- {step.__name__}")
            try:
                step(self)
            except Exception:
                self.problems.append(f"{step.__name__} raised")
                traceback.print_exc()
        for problem in self.problems:
            print("  PROBLEM " + problem)
        print("RESULT:", "FAIL" if self.problems else "PASS")


def cli(*arguments):
    return subprocess.run([str(build_dir() / "bin" / "konveyor"), *arguments], capture_output=True, text=True, timeout=60)


def config_path():
    return Path(os.environ["XDG_CONFIG_HOME"]) / "konveyor" / "config.kdl"


def default_config():
    return (REPO / "data" / "default-config.kdl").read_text()


def load_config(text):
    config_path().write_text(text)
    return konveyor("LoadConfigFile", "") == ""


def notifications():
    path = Path(os.environ["KONVEYOR_NOTIFICATIONS_LOG"])
    return [json.loads(line) for line in path.read_text().splitlines()] if path.exists() else []
