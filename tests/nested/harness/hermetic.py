import os
from pathlib import Path

from nested import REPO, build_dir

VARIABLES = ("HOME", "XDG_CONFIG_HOME", "XDG_DATA_HOME", "XDG_STATE_HOME", "XDG_CACHE_HOME", "XDG_RUNTIME_DIR")


def inside(path, directories):
    return any(path == directory or path.startswith(directory + "/") for directory in directories)


def environment(process):
    entries = (process / "environ").read_bytes().decode(errors="replace").split("\0")
    return dict(entry.split("=", 1) for entry in entries if "=" in entry)


def used_files(process):
    files = {os.readlink(fd) for fd in (process / "fd").iterdir()}
    for line in (process / "maps").read_text().splitlines():
        fields = line.split(None, 5)
        if len(fields) == 6:
            files.add(fields[5])
    return files


def leaks():
    root = os.environ["KONVEYOR_TEST_ROOT"]
    outside = [directory for directory in os.environ["KONVEYOR_OUTSIDE_DIRS"].split(":") if directory]
    allowed = [str(REPO), str(build_dir())]
    session = os.getsid(0)
    found = []
    for process in Path("/proc").iterdir():
        try:
            if not process.name.isdigit() or os.getsid(int(process.name)) != session:
                continue
            name = (process / "comm").read_text().strip()
            variables = environment(process)
            found += [f"{name} has {key}={variables.get(key)}" for key in VARIABLES if not inside(variables.get(key, ""), [root])]
            found += [f"{name} uses {path}" for path in sorted(used_files(process)) if inside(path, outside) and not inside(path, allowed)]
        except (OSError, ProcessLookupError):
            continue
    return found
