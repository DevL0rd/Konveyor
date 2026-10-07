import re

ORDER = "AppletOrder"
MARK = "konveyorReplaced"
ADDED = "konveyorAdded"
APPLET = re.compile(r"^\[Containments\]\[(\d+)\]\[Applets\]\[(\d+)\]$")
CONTAINMENT = re.compile(r"^\[Containments\]\[(\d+)\]$")
GENERAL = re.compile(r"^\[Containments\]\[(\d+)\]\[General\]$")


def read_groups(path):
    groups, header, lines = [], None, []
    with open(path) as handle:
        for line in handle.read().splitlines():
            if line.startswith("[") and line.endswith("]"):
                groups.append((header, lines))
                header, lines = line, []
            else:
                lines.append(line)
    groups.append((header, lines))
    return groups


def write_groups(path, groups):
    out = []
    for header, lines in groups:
        if header is not None:
            out.append(header)
        out.extend(lines)
    with open(path, "w") as handle:
        handle.write("\n".join(out) + "\n")


def value(lines, key):
    for line in lines:
        if line.startswith(key + "="):
            return line.split("=", 1)[1]
    return None


def set_value(lines, key, new):
    kept = [line for line in lines if not line.startswith(key + "=")]
    if new is not None:
        position = next((i for i, line in enumerate(kept) if line == ""), len(kept))
        kept.insert(position, f"{key}={new}")
    return kept


def panel_ids(groups):
    return {CONTAINMENT.match(h).group(1) for h, lines in groups
            if h and CONTAINMENT.match(h) and value(lines, "plugin") == "org.kde.panel"}


def panel_applets(groups):
    panels = panel_ids(groups)
    for index, (header, lines) in enumerate(groups):
        match = APPLET.match(header or "")
        if match and match.group(1) in panels:
            yield index, header, lines


def applets_on(groups, panel):
    return [match.group(2) for match in (APPLET.match(header or "") for header, _ in groups) if match and match.group(1) == panel]


def next_applet_id(groups):
    return max((int(n) for header, _ in groups if header for n in re.findall(r"\[(\d+)\]", header)), default=0) + 1


def is_inside(header, parents):
    return bool(header) and any(header.startswith(parent + "[") for parent in parents)


def drop_from_order(groups, headers):
    dropped = {}
    for header in headers:
        match = APPLET.match(header)
        dropped.setdefault(match.group(1), set()).add(match.group(2))
    result = []
    for header, lines in groups:
        match = GENERAL.match(header or "")
        order = value(lines, ORDER)
        if match and match.group(1) in dropped and order is not None:
            lines = set_value(lines, ORDER, ";".join(i for i in order.split(";") if i and i not in dropped[match.group(1)]))
        result.append((header, lines))
    return result


def reorder(groups, panel, change):
    general = f"[Containments][{panel}][General]"
    existing = sorted(applets_on(groups, panel), key=int)
    result = []
    found = False
    for header, lines in groups:
        if header == general:
            found = True
            order = [i for i in (value(lines, ORDER) or ";".join(existing)).split(";") if i]
            lines = set_value(lines, ORDER, ";".join(change(order)))
        result.append((header, lines))
    if not found:
        result.append((general, [f"{ORDER}=" + ";".join(change(existing)), ""]))
    return result
