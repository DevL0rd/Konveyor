#!/usr/bin/env python3
import json
import sys
from pathlib import Path

import dbus
import dbus.service
from dbus.mainloop.glib import DBusGMainLoop
from gi.repository import GLib

INTERFACE = "org.freedesktop.Notifications"


class Notifications(dbus.service.Object):
    def __init__(self, bus, log):
        super().__init__(bus, "/org/freedesktop/Notifications")
        self.log = log
        self.next_id = 1

    @dbus.service.method(INTERFACE, in_signature="susssasa{sv}i", out_signature="u")
    def Notify(self, app_name, replaces_id, app_icon, summary, body, actions, hints, timeout):
        entry = {"app": str(app_name), "icon": str(app_icon), "summary": str(summary), "body": str(body)}
        with self.log.open("a") as log:
            log.write(json.dumps(entry) + "\n")
        self.next_id += 1
        return dbus.UInt32(self.next_id - 1)

    @dbus.service.method(INTERFACE, in_signature="u")
    def CloseNotification(self, notification_id):
        self.NotificationClosed(notification_id, dbus.UInt32(3))

    @dbus.service.method(INTERFACE, out_signature="as")
    def GetCapabilities(self):
        return ["body", "actions", "icon-static"]

    @dbus.service.method(INTERFACE, out_signature="ssss")
    def GetServerInformation(self):
        return "konveyor-tests", "Konveyor", "1", "1.2"

    @dbus.service.signal(INTERFACE, signature="uu")
    def NotificationClosed(self, notification_id, reason):
        pass


def main():
    log = Path(sys.argv[1])
    DBusGMainLoop(set_as_default=True)
    bus = dbus.SessionBus()
    name = dbus.service.BusName(INTERFACE, bus)
    server = Notifications(bus, log)
    Path(f"{log}.ready").touch()
    GLib.MainLoop().run()
    del name, server


if __name__ == "__main__":
    main()
