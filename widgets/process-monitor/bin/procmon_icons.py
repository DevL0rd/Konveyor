import glob
import os
import time

DESKTOP_DIRS = [
    "/usr/share/applications", "/usr/local/share/applications",
    os.path.expanduser("~/.local/share/applications"),
    "/var/lib/flatpak/exports/share/applications",
    os.path.expanduser("~/.local/share/flatpak/exports/share/applications"),
]
ICON_ALIASES = {
    "chrome": "google-chrome", "chromium": "chromium", "msedge": "microsoft-edge",
    "brave": "brave-browser", "code-oss": "code-oss", "codium": "vscodium",
    "firefox-bin": "firefox", "thunderbird-bin": "thunderbird",
    "steamwebhelper": "steam", "wineserver": "wine", "telegram-desktop": "telegram",
}
_EXEC_SKIP = {"env", "flatpak", "sh", "bash", "run", "--", "gtk-launch"}
GENERIC_COMMS = {
    "python", "python2", "python3", "node", "nodejs", "java", "electron",
    "sh", "bash", "dash", "zsh", "fish", "perl", "ruby", "mono", "dotnet",
    "wrapper", "lua", "php", "deno", "bun",
}

_icon_map = None
_steam_apps = {}
_icon_map_when = 0.0
_icon_resolved = {}
_identity_cache = {}


def _parse_desktop(path):
    icon = exe = tryexe = wm = name = ""
    in_entry = False
    try:
        for ln in open(path, encoding="utf-8", errors="ignore"):
            ln = ln.strip()
            if ln.startswith("["):
                in_entry = (ln == "[Desktop Entry]")
                continue
            if not in_entry or "=" not in ln:
                continue
            k, _, v = ln.partition("=")
            v = v.strip()
            if k == "Icon" and not icon: icon = v
            elif k == "Exec" and not exe: exe = v
            elif k == "TryExec" and not tryexe: tryexe = v
            elif k == "StartupWMClass" and not wm: wm = v
            elif k == "Name" and not name: name = v
    except OSError:
        return None
    return icon, exe, tryexe, wm, name


def _exec_basename(exe):
    for t in exe.split():
        if not t or t.startswith("%") or t.startswith("-") or "=" in t.split("/")[0]:
            continue
        b = os.path.basename(t)
        if b and b not in _EXEC_SKIP:
            return b
    return ""


def build_icon_map():
    m = {}
    steam = {}
    for d in DESKTOP_DIRS:
        for path in glob.glob(os.path.join(d, "*.desktop")):
            r = _parse_desktop(path)
            if not r or not r[0]:
                continue
            icon, exe, tryexe, wm, name = r
            keys = {os.path.basename(path)[:-8].lower()}
            if exe:
                b = _exec_basename(exe)
                if b: keys.add(b.lower())
            if tryexe:
                keys.add(os.path.basename(tryexe).lower())
            if wm:
                keys.add(wm.lower())
            marker = "steam://rungameid/"
            if marker in exe:
                app_id = exe.split(marker, 1)[1].split()[0].strip("\"'")
                if app_id.isdigit():
                    steam[app_id] = (name, icon or "steam_icon_" + app_id)
            for k in keys:
                m.setdefault(k, icon)
    return m, steam


def refresh_icon_map(now):
    global _icon_map, _steam_apps, _icon_map_when
    if _icon_map is not None and now - _icon_map_when <= 300:
        return False
    _icon_map, _steam_apps = build_icon_map()
    _icon_map_when = now
    _icon_resolved.clear()
    _identity_cache.clear()
    return True


def resolve_icon(comm):
    icon = _icon_resolved.get(comm)
    if icon is not None:
        return icon
    if _icon_map is None:
        refresh_icon_map(time.monotonic())
    c = comm.lower()
    icon = ICON_ALIASES.get(c) or ("" if c in GENERIC_COMMS else _icon_map.get(c, "")) or ""
    _icon_resolved[comm] = icon
    return icon


def process_identity(pid, fallback_name, fallback_icon):
    cache_key = (pid, fallback_name, fallback_icon)
    cached = _identity_cache.get(cache_key)
    if cached is not None:
        return cached
    app_id = ""
    try:
        with open(f"/proc/{pid}/environ", "rb") as f:
            env = f.read()
        for item in env.split(b"\0"):
            if item.startswith(b"SteamAppId=") or item.startswith(b"STEAM_COMPAT_APP_ID="):
                value = item.split(b"=", 1)[1].decode(errors="ignore")
                if value.isdigit():
                    app_id = value
                    if item.startswith(b"SteamAppId="):
                        break
    except OSError:
        pass
    if app_id:
        steam_name, steam_icon = _steam_apps.get(app_id, ("", "steam_icon_" + app_id))
        identity = (steam_name or fallback_name, steam_icon or fallback_icon)
    else:
        identity = (fallback_name, fallback_icon)
    _identity_cache[cache_key] = identity
    return identity
