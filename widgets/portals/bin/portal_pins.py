import json
import mimetypes
import os
import re
import sys
import urllib.parse

from portal_desktop import HOME, STATE_DIR, load_json, resolve_desktop

HIDDEN = os.path.join(STATE_DIR, "hidden.json")
FOLDERS = os.path.join(STATE_DIR, "folders.json")    # key -> last-launched epoch (apps + games)
SIDEBAR = os.path.join(STATE_DIR, "sidebar.json")


def load_hidden():
    data = load_json(HIDDEN)
    return sorted(data) if isinstance(data, list) else []


def set_hidden(desktop_id, hidden):
    key = desktop_id[len("applications:"):] if desktop_id.startswith("applications:") else desktop_id
    key = key[:-len(".desktop")] if key.endswith(".desktop") else key
    current = set(load_hidden())
    if hidden:
        current.add(key)
    else:
        current.discard(key)
    os.makedirs(STATE_DIR, exist_ok=True)
    tmp = HIDDEN + ".tmp"
    with open(tmp, "w") as f:
        json.dump(sorted(current), f)
    os.replace(tmp, HIDDEN)
    return 0


def load_folders():
    data = load_json(FOLDERS)
    folders = []
    for entry in data if isinstance(data, list) else []:
        if isinstance(entry, dict) and entry.get("id") and isinstance(entry.get("apps"), list):
            folders.append({"id": str(entry["id"]), "name": str(entry.get("name") or ""),
                            "apps": [str(app) for app in entry["apps"] if app]})
    return folders


def save_folders(folders):
    folders = [folder for folder in folders if folder["apps"]]
    os.makedirs(STATE_DIR, exist_ok=True)
    tmp = FOLDERS + ".tmp"
    with open(tmp, "w") as f:
        json.dump(folders, f)
    os.replace(tmp, FOLDERS)
    return 0


def take_out(folders, app_ids):
    for folder in folders:
        folder["apps"] = [app for app in folder["apps"] if app not in app_ids]


def folder_create(folder_id, name, app_ids):
    folders = [folder for folder in load_folders() if folder["id"] != folder_id]
    take_out(folders, app_ids)
    folders.append({"id": folder_id, "name": name, "apps": list(dict.fromkeys(app_ids))})
    return save_folders(folders)


def folder_add(folder_id, app_id):
    folders = load_folders()
    target = next((folder for folder in folders if folder["id"] == folder_id), None)
    if target is None:
        sys.stderr.write("no such folder: %s\n" % folder_id)
        return 1
    take_out(folders, [app_id])
    target["apps"].append(app_id)
    return save_folders(folders)


def folder_remove(app_id):
    folders = load_folders()
    take_out(folders, [app_id])
    return save_folders(folders)


def folder_rename(folder_id, name):
    folders = load_folders()
    target = next((folder for folder in folders if folder["id"] == folder_id), None)
    if target is None:
        sys.stderr.write("no such folder: %s\n" % folder_id)
        return 1
    target["name"] = name
    return save_folders(folders)


def folder_delete(folder_id):
    return save_folders([folder for folder in load_folders() if folder["id"] != folder_id])


def clean_sidebar(data):
    pins, seen = [], set()
    for entry in data if isinstance(data, list) else []:
        if not isinstance(entry, dict) or entry.get("kind") not in ("app", "path"):
            continue
        pin_id = entry.get("id")
        if not isinstance(pin_id, str) or not pin_id or (entry["kind"], pin_id) in seen:
            continue
        seen.add((entry["kind"], pin_id))
        pins.append({"kind": entry["kind"], "id": pin_id, "name": str(entry.get("name") or "")})
    return pins


def load_sidebar():
    return clean_sidebar(load_json(SIDEBAR))


def save_sidebar(pins):
    if pins == load_sidebar():
        return 0
    os.makedirs(STATE_DIR, exist_ok=True)
    tmp = SIDEBAR + ".tmp"
    with open(tmp, "w") as f:
        json.dump(pins, f)
    os.replace(tmp, SIDEBAR)
    return 0


USER_DIR_ICONS = {"DESKTOP": "user-desktop", "DOWNLOAD": "folder-download", "TEMPLATES": "folder-templates",
                  "PUBLICSHARE": "folder-public", "DOCUMENTS": "folder-documents", "MUSIC": "folder-music",
                  "PICTURES": "folder-pictures", "VIDEOS": "folder-videos"}


def user_dir_icons():
    icons = {os.path.realpath(HOME): "user-home"}
    config = os.environ.get("XDG_CONFIG_HOME", os.path.join(HOME, ".config"))
    try:
        with open(os.path.join(config, "user-dirs.dirs")) as f:
            for line in f:
                match = re.match(r'XDG_(\w+)_DIR="(.*)"', line.strip())
                if match and match.group(1) in USER_DIR_ICONS:
                    path = os.path.realpath(match.group(2).replace("$HOME", HOME))
                    if path != os.path.realpath(HOME):
                        icons[path] = USER_DIR_ICONS[match.group(1)]
    except OSError:
        pass
    return icons


def resolve_pin(pin):
    out = dict(pin)
    if pin["kind"] == "app":
        path, entry = resolve_desktop(pin["id"] + ".desktop")
        out["missing"] = not entry
        out["name"] = (entry or {}).get("Name") or pin["name"] or pin["id"]
        out["icon"] = (entry or {}).get("Icon") or "application-x-executable"
        return out
    local = urllib.parse.unquote(urllib.parse.urlparse(pin["id"]).path) if pin["id"].startswith("file://") else pin["id"]
    is_dir = os.path.isdir(local)
    out["missing"] = not os.path.exists(local)
    out["folder"] = is_dir
    out["path"] = local
    out["name"] = pin["name"] or os.path.basename(local.rstrip("/")) or local
    if is_dir:
        out["icon"] = user_dir_icons().get(os.path.realpath(local), "folder")
    else:
        mime = mimetypes.guess_type(local)[0] or ""
        out["icon"] = mime.replace("/", "-") if mime else "text-x-generic"
        out["generic"] = mime.split("/")[0] + "-x-generic" if mime else "text-x-generic"
    return out


def sidebar_set(text):
    try:
        data = json.loads(text)
    except ValueError:
        sys.stderr.write("sidebar: invalid JSON\n")
        return 1
    if not isinstance(data, list):
        sys.stderr.write("sidebar: expected a list\n")
        return 1
    return save_sidebar(clean_sidebar(data))
