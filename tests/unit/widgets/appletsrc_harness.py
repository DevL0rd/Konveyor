import importlib.machinery
import importlib.util
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path


SERVICE = Path(__file__).resolve().parents[3] / "widgets" / "service"


class AppletsrcCase(unittest.TestCase):
    script = None
    fixture = ""

    def setUp(self):
        loader = importlib.machinery.SourceFileLoader(self.script.name.replace("-", "_"), str(self.script))
        spec = importlib.util.spec_from_loader(loader.name, loader)
        self.module = importlib.util.module_from_spec(spec)
        loader.exec_module(self.module)
        self.temporary = tempfile.TemporaryDirectory()
        self.addCleanup(self.temporary.cleanup)
        self.path = Path(self.temporary.name) / "appletsrc"
        self.path.write_text(self.fixture)

    def main(self, *arguments, code=0):
        result = subprocess.run([sys.executable, str(self.script), *arguments], capture_output=True, text=True)
        self.assertEqual(result.returncode, code, result.stderr)
        return result
