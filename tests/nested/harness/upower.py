#!/usr/bin/env python3
import sys

import dbus
import dbus.service
from dbus.mainloop.glib import DBusGMainLoop
from gi.repository import GLib

SERVICE = "org.freedesktop.UPower"
PATH = "/org/freedesktop/UPower"
PROPERTIES = "org.freedesktop.DBus.Properties"
CONTROL = "org.konveyor.FakeUPower"


class FakeUPower(dbus.service.Object):
    def __init__(self, bus, on_battery):
        super().__init__(bus, PATH)
        self.on_battery = on_battery

    @dbus.service.method(PROPERTIES, in_signature="ss", out_signature="v")
    def Get(self, interface, name):
        return dbus.Boolean(self.on_battery)

    @dbus.service.signal(PROPERTIES, signature="sa{sv}as")
    def PropertiesChanged(self, interface, changed, invalidated):
        pass

    @dbus.service.method(CONTROL, in_signature="b")
    def Set(self, on_battery):
        self.on_battery = bool(on_battery)
        self.PropertiesChanged(SERVICE, {"OnBattery": dbus.Boolean(self.on_battery)}, [])

    @dbus.service.method(CONTROL, in_signature="b")
    def Invalidate(self, on_battery):
        self.on_battery = bool(on_battery)
        self.PropertiesChanged(SERVICE, {}, ["OnBattery"])

    @dbus.service.method(CONTROL, in_signature="b")
    def Unrelated(self, on_battery):
        self.PropertiesChanged("org.freedesktop.UPower.Device", {"OnBattery": dbus.Boolean(on_battery)}, [])

    @dbus.service.method(CONTROL, in_signature="b")
    def OtherProperty(self, value):
        self.PropertiesChanged(SERVICE, {"LidIsClosed": dbus.Boolean(value)}, [])


def main():
    DBusGMainLoop(set_as_default=True)
    bus = dbus.SystemBus()
    name = dbus.service.BusName(SERVICE, bus)
    service = FakeUPower(bus, sys.argv[1] == "battery")
    GLib.MainLoop().run()
    del name, service


if __name__ == "__main__":
    main()
