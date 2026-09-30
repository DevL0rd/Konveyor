from kwinsession import qdbus

BTN_LEFT = 0x110
BTN_RIGHT = 0x111
BTN_MIDDLE = 0x112
MISSING_NODE = "/sys/devices/virtual/input/konveyor-test/event-missing"


def call(method, *arguments):
    return qdbus("org.kde.KonveyorTest", "/Touchpad", f"org.kde.KonveyorTest.Touchpad.{method}", *(str(argument).lower() if isinstance(argument, bool) else str(argument) for argument in arguments))


def add(sys_path=MISSING_NODE, lmr_tap_button_map=False):
    call("Add", sys_path, lmr_tap_button_map)


def remove():
    call("Remove")


def swipe(fingers, dx=0, dy=0, steps=30, cancel=False):
    call("Swipe", fingers, float(dx), float(dy), steps, cancel)


def pinch(fingers, scale, steps=20, cancel=False):
    call("Pinch", fingers, float(scale), steps, cancel)


def scroll(horizontal, delta):
    call("Scroll", horizontal, float(delta))


def click(button):
    call("Button", button, True)
    call("Button", button, False)
