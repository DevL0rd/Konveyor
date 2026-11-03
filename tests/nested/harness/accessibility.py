import os
import subprocess
import time

import gi

gi.require_version("Atspi", "2.0")
from gi.repository import Atspi

from kwinsession import wait_for
from sessionguard import popen


class Accessibility:
    def __init__(self):
        self.launcher = subprocess.Popen(["/usr/lib/at-spi-bus-launcher", "--launch-immediately"])
        time.sleep(1)
        self.registry = subprocess.Popen(["/usr/lib/at-spi2-registryd", "--use-gnome-session"])
        time.sleep(1)
        Atspi.init()

    def close(self):
        for process in (self.registry, self.launcher):
            process.terminate()
            process.wait(timeout=10)

    def launch(self, *command):
        return popen(command, env={**os.environ, "QT_LINUX_ACCESSIBILITY_ALWAYS_ON": "1"})

    def application(self, name):
        desktop = Atspi.get_desktop(0)
        for index in range(max(0, desktop.get_child_count())):
            child = desktop.get_child_at_index(index)
            if child.get_name() == name:
                return child
        return None

    def descendants(self, node):
        for index in range(node.get_child_count()):
            child = node.get_child_at_index(index)
            if child is None:
                continue
            yield child
            yield from self.descendants(child)

    def buttons_after(self, application, label, prefix):
        seen = False
        found = []
        for node in self.descendants(application):
            if node.get_role_name() == "label" and node.get_name() == label:
                seen = True
            elif seen and node.get_role_name() == "button" and (node.get_name() or "").startswith(prefix):
                found.append(node)
        return found

    def press(self, node):
        return Atspi.Action.do_action(node, 0)

    def wait_for_application(self, name, timeout=60):
        return wait_for(lambda: self.application(name) is not None, timeout, 0.5)
