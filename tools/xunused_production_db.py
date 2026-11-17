import json
import sys
from pathlib import Path

source, target, root, build = (Path(argument).resolve() for argument in sys.argv[1:5])
tests = [root / "tests", build / "tests"]
entries = json.loads(source.read_text())
kept = [
    entry
    for entry in entries
    if not any(Path(entry["directory"], entry["file"]).resolve().is_relative_to(folder) for folder in tests)
]
target.write_text(json.dumps(kept))
