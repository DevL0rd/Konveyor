MODIFIER_CODES = {"Super": 125, "Ctrl": 29, "Alt": 56, "Shift": 42}

KEY_CODES = {
    "Minus": 12, "Equal": 13, "BracketLeft": 26, "BracketRight": 27, "Comma": 51,
    "Period": 52, "Slash": 53, "Space": 57, "Semicolon": 39, "Apostrophe": 40,
    "Grave": 41, "Backslash": 43, "Escape": 1, "Tab": 15, "Return": 28,
    "BackSpace": 14, "Delete": 111, "Insert": 110, "Home": 102, "End": 107,
    "Print": 99,
    "1": 2, "2": 3, "3": 4, "4": 5, "5": 6, "6": 7, "7": 8, "8": 9, "9": 10, "0": 11,
    "minus": 12, "equal": 13, "Q": 16, "W": 17, "E": 18, "R": 19, "T": 20, "Y": 21,
    "U": 22, "I": 23, "O": 24, "P": 25, "bracketleft": 26, "bracketright": 27,
    "Return": 28, "A": 30, "S": 31, "D": 32, "F": 33, "G": 34, "H": 35, "J": 36,
    "K": 37, "L": 38, "semicolon": 39, "apostrophe": 40, "grave": 41, "backslash": 43,
    "Z": 44, "X": 45, "C": 46, "V": 47, "B": 48, "N": 49, "M": 50, "comma": 51,
    "period": 52, "slash": 53, "space": 57, "F1": 59, "F2": 60, "F3": 61, "F4": 62,
    "F5": 63, "F6": 64, "F7": 65, "F8": 66, "F9": 67, "F10": 68, "F11": 87, "F12": 88,
    "Home": 102, "Up": 103, "Page_Up": 104, "Prior": 104, "Left": 105, "Right": 106,
    "End": 107, "Down": 108, "Page_Down": 109, "Next": 109, "Insert": 110,
    "Delete": 111, "Escape": 1, "Tab": 15, "BackSpace": 14, "Print": 99,
}


def evdev_codes(label):
    """Map a Konveyor bind label such as "Super+Ctrl+Left" to evdev codes."""
    parts = [p for p in label.split("+") if p]
    if not parts:
        return None
    modifiers = []
    for part in parts[:-1]:
        if part not in MODIFIER_CODES:
            return None
        modifiers.append(MODIFIER_CODES[part])
    name = parts[-1]
    key = KEY_CODES.get(name)
    if key is None:
        key = next((code for spelling, code in KEY_CODES.items() if spelling.lower() == name.lower()), None)
    if key is None:
        return None
    return modifiers, key
