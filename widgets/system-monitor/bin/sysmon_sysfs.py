import os
import re

ASLEEP = ("suspended", "suspending")


def read(path, cast=int):
    try:
        with open(path) as f:
            return cast(f.read().strip())
    except (OSError, ValueError):
        return None


def pci_slot(dev):
    """The device's PCI address, e.g. "0000:03:00.0". None if it has none."""
    try:
        with open(os.path.join(dev, "uevent")) as f:
            return re.search(r"^PCI_SLOT_NAME=(.+)$", f.read(), re.M).group(1)
    except (OSError, AttributeError):
        return None


def hwdb_name(dev):
    """The device's name in the udev hardware database, e.g. "Navi 32 [Radeon
    RX 7700 XT / 7800 XT]" -> "Radeon RX 7700 XT / 7800 XT". None if udev
    doesn't know it."""
    slot = pci_slot(dev)
    if slot is None:
        return None
    try:
        with open("/run/udev/data/+pci:" + slot) as f:
            model = re.search(r"^E:ID_MODEL_FROM_DATABASE=(.+)$", f.read(), re.M).group(1)
    except (OSError, AttributeError):
        return None
    brand = re.search(r"\[(.+)\]", model)
    return brand.group(1) if brand else model


def awake(dev):
    return read(os.path.join(dev, "power", "runtime_status"), str) not in ASLEEP
