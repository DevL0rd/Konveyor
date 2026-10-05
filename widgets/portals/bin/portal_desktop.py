import json
import os

HOME = os.path.expanduser("~")
XDG_DATA_HOME = os.environ.get("XDG_DATA_HOME", os.path.join(HOME, ".local/share"))
XDG_DATA_DIRS = os.environ.get("XDG_DATA_DIRS", "/usr/local/share:/usr/share").split(":")

APP_DIRS = [os.path.join(XDG_DATA_HOME, "applications")] + \
           [os.path.join(d, "applications") for d in XDG_DATA_DIRS] + \
           [os.path.join(XDG_DATA_HOME, "flatpak/exports/share/applications"),
            "/var/lib/flatpak/exports/share/applications"]

STATE_DIR = os.path.join(XDG_DATA_HOME, "Plasma-App-Portal")


def parse_desktop(path):
    """Minimal [Desktop Entry] parser -> dict (first section only)."""
    out, in_entry = {}, False
    try:
        with open(path, encoding="utf-8", errors="replace") as f:
            for line in f:
                line = line.rstrip("\n")
                if line.startswith("["):
                    in_entry = line.strip() == "[Desktop Entry]"
                    continue
                if in_entry and "=" in line and not line.startswith("#"):
                    k, _, v = line.partition("=")
                    out.setdefault(k.strip(), v.strip())   # keep first (un-localized)
    except OSError:
        return None
    return out


def resolve_desktop(desktop_id):
    for d in APP_DIRS:
        p = os.path.join(d, desktop_id)
        if os.path.isfile(p):
            return p, parse_desktop(p)
    return None, None


def load_json(path):
    try:
        with open(path) as f:
            return json.load(f)
    except (OSError, ValueError):
        return {}


def load_object(path):
    data = load_json(path)
    return data if isinstance(data, dict) else {}
