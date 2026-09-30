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
