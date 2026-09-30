import os
import re
from pathlib import Path

HEADER = re.compile(r"\[([^\]]*)\]")


def config_path(name):
    if os.path.isabs(name):
        return Path(name)
    return Path(os.environ["XDG_CONFIG_HOME"]) / name


def read(path):
    groups = {}
    current = groups.setdefault((), {})
    if not Path(path).exists():
        return {}
    for line in Path(path).read_text().splitlines():
        if line.startswith("[") and line.endswith("]"):
            current = groups.setdefault(tuple(HEADER.findall(line)), {})
        elif "=" in line and not line.startswith("#"):
            key, value = line.split("=", 1)
            current[key] = value
    return {group: entries for group, entries in groups.items() if entries}


def write(path, groups):
    blocks = []
    for group, entries in groups.items():
        if not entries:
            continue
        lines = ["".join(f"[{name}]" for name in group)] if group else []
        lines += [f"{key}={value}" for key, value in entries.items()]
        blocks.append("\n".join(lines))
    Path(path).parent.mkdir(parents=True, exist_ok=True)
    Path(path).write_text("\n\n".join(blocks) + "\n" if blocks else "")


def set_entry(path, group, key, value):
    groups = read(path)
    entries = groups.setdefault(tuple(group), {})
    if value is None:
        entries.pop(key, None)
    else:
        entries[key] = value
    write(path, groups)


def entry(path, group, key, default=""):
    return read(path).get(tuple(group), {}).get(key, default)


def parse_arguments(arguments):
    options = {"group": [], "file": None, "key": None, "default": "", "delete": False, "value": None}
    index = 0
    while index < len(arguments):
        argument = arguments[index]
        if argument in ("--file", "--key", "--default", "--type"):
            options[argument[2:]] = arguments[index + 1]
            index += 2
            continue
        if argument == "--group":
            options["group"].append(arguments[index + 1])
            index += 2
            continue
        if argument == "--delete":
            options["delete"] = True
        elif argument != "--notify":
            options["value"] = argument
        index += 1
    return options


def kwriteconfig(arguments):
    options = parse_arguments(arguments)
    value = None if options["delete"] else options["value"]
    set_entry(config_path(options["file"]), options["group"], options["key"], value)
    return 0


def kreadconfig(arguments):
    options = parse_arguments(arguments)
    print(entry(config_path(options["file"]), options["group"], options["key"], options["default"]))
    return 0
