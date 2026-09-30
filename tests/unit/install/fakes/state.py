import json
import os

MODIFIERS = {"Meta": 0x10000000, "Ctrl": 0x04000000, "Alt": 0x08000000, "Shift": 0x02000000}
NAMED_KEYS = {"Meta": 0x01000022, "Alt": 0x01000023, "Tab": 0x01000001, "Space": 0x20}


def path(name):
    return os.path.join(os.environ["STUB_DIR"], name)


def load(name, default):
    if not os.path.exists(path(f"{name}.json")):
        return default
    with open(path(f"{name}.json")) as handle:
        return json.load(handle)


def save(name, value):
    with open(path(f"{name}.json"), "w") as handle:
        json.dump(value, handle, indent=1, sort_keys=True)


def record(argv, stdin=None):
    entry = {"argv": argv}
    if stdin is not None:
        entry["stdin"] = stdin
    if os.environ.get("STUB_INSIDE"):
        entry["inside"] = os.environ["STUB_INSIDE"]
    with open(path("calls.jsonl"), "a") as log:
        log.write(json.dumps(entry) + "\n")


def key_code(text):
    *modifiers, key = text.split("+")
    if key in NAMED_KEYS:
        code = NAMED_KEYS[key]
    elif key.startswith("F") and key[1:].isdigit():
        code = 0x01000030 + int(key[1:]) - 1
    else:
        code = ord(key.upper())
    for modifier in modifiers:
        code |= MODIFIERS[modifier]
    return code


def key_text(code):
    parts = [name for name, bit in MODIFIERS.items() if code & bit]
    key = code & ~sum(MODIFIERS.values())
    named = {value: name for name, value in NAMED_KEYS.items()}
    if key in named:
        parts.append(named[key])
    elif 0x01000030 <= key < 0x01000060:
        parts.append(f"F{key - 0x01000030 + 1}")
    else:
        parts.append(chr(key))
    return "+".join(parts)


def shortcuts():
    return load("kglobalaccel", {})


def action_name(component, action):
    return f"{component}\t{action}"


def holders(store, code, skip=None):
    return [name for name, entry in store.items() if name != skip and code in entry["keys"]]


def assign(store, name, codes):
    store[name]["keys"] = [code for code in codes if not holders(store, code, skip=name)]
