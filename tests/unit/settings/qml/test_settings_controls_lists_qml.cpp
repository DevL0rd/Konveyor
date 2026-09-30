#include "controldriver.h"

using namespace Konveyor::Settings::Testing;

namespace
{

const char *Profiles = "monitor-profile \"a\" {}\nmonitor-profile \"b\" {}\noutput \"DP-1\"\n";
const char *Wide = "monitor-profile \"wide\" {\n    match aspect-ratio-above=2.0\n}\n";
const char *Workspaces = "workspace \"a\"\nworkspace \"b\" {\n    open-on-output \"DP-1\"\n}\n";

Control profile(const char *label, const char *signal, const char *arguments, const char *path, const char *expected,
    const char *config = Wide, int nth = 0)
{
    return {"MonitorProfilePage", label, signal, arguments, path, expected, config, nth, R"({"profilePath":"monitor-profile"})"};
}

Control output(const char *label, const char *signal, const char *arguments, const char *path, const char *expected, const char *config)
{
    return {"OutputOverridePage", label, signal, arguments, path, expected, config, 0, R"({"outputPath":"output"})"};
}

}

class TestSettingsControlsListsQml : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase() { QVERIFY(m_home.setUp()); }

    void controls_data();
    void controls() { runControl(m_home); }

private:
    SettingsHome m_home;
};

void TestSettingsControlsListsQml::controls_data()
{
    addControlColumns();
    addControl("profile down", {"MonitorsPage", "a", "movedDown", "[]", "monitor-profile", R"({"args":["b"]})", Profiles});
    addControl("profile up", {"MonitorsPage", "b", "movedUp", "[]", "monitor-profile", R"({"args":["b"]})", Profiles});
    addControl("profile deleted", {"MonitorsPage", "a", "deleted", "[]", "monitor-profile#1", "null", Profiles});
    addControl(
        "profile added", {"MonitorsPage", "*", "named", R"(["c"])", "monitor-profile#2", R"({"args":["c"],"children":[]})", Profiles});
    addControl("override deleted", {"MonitorsPage", "DP-1", "deleted", "[]", "output", "null", Profiles});
    addControl("override added",
        {"MonitorsPage", "* | ^Add override", "picked | clicked", R"(["HDMI-A-1"] | [])", "output#1",
            R"({"args":["HDMI-A-1"],"children":[]})", Profiles});
    addControl("profile renamed", profile("*", "named", R"(["ultra"])", "monitor-profile", R"({"args":["ultra"],"children":["match"]})"));
    addControl("wider than", profile("Wider than", "changed", "[2.4]", "monitor-profile/match", R"({"props":{"aspect-ratio-above":2.4}})"));
    addControl("narrower than",
        profile(
            "Narrower than", "changed", "[3]", "monitor-profile/match", R"({"props":{"aspect-ratio-above":2,"aspect-ratio-below":3}})"));
    addControl("wide pixels",
        profile("More pixels wide than", "changed", "[2000]", "monitor-profile/match",
            R"({"props":{"aspect-ratio-above":2,"width-above":2000}})"));
    addControl("narrow pixels",
        profile("Fewer pixels wide than", "changed", "[5000]", "monitor-profile/match",
            R"({"props":{"aspect-ratio-above":2,"width-below":5000}})"));
    addControl("tall pixels",
        profile("More pixels tall than", "changed", "[900]", "monitor-profile/match",
            R"({"props":{"aspect-ratio-above":2,"height-above":900}})"));
    addControl("short pixels off",
        profile("Fewer pixels tall than", "changed", "[null]", "monitor-profile/match", R"({"props":{}})",
            "monitor-profile \"p\" {\n    match height-below=2000\n}\n"));
    addControl("monitor name",
        profile("Monitor name", "picked", R"(["DP-2"])", "monitor-profile/match", R"({"props":{"aspect-ratio-above":2,"name":"^DP-2$"}})"));
    addControl("any monitor name",
        profile("Monitor name", "chosen", R"(["any"])", "monitor-profile/match", R"({"props":{}})",
            "monitor-profile \"p\" {\n    match name=\"^DP\"\n}\n"));
    addControl("condition removed", profile("=monitor-profile/match", "removeRequested", "[]", "monitor-profile/match", "null"));
    addControl(
        "condition added", profile("^Or also match…", "clicked", "[]", "monitor-profile/match#1", R"({"props":{"aspect-ratio-above":2}})"));
    addControl("customize gaps", profile("Gaps", "toggle@Customize here;toggled", "[]", "monitor-profile/layout/gaps", R"({"args":[16]})"));
    addControl("inherit gaps",
        profile("Gaps", "toggle@Customized here;toggled", "[]", "monitor-profile/layout/gaps", "null",
            "monitor-profile \"p\" {\n    layout {\n        gaps 3\n    }\n}\n"));
    addControl("profile gaps",
        profile("Gaps", "edited", "[7]", "monitor-profile/layout/gaps", R"({"args":[7]})",
            "monitor-profile \"p\" {\n    layout {\n        gaps 3\n    }\n}\n"));
    addControl("profile flag off",
        profile("Center a lone window", "switched", "[false]", "monitor-profile/layout/always-center-single-column", R"({"args":[false]})",
            "monitor-profile \"p\" {\n    layout {\n        always-center-single-column\n    }\n}\n"));
    addControl("profile ring",
        profile("Show focus ring", "toggle@Customize here;toggled", "[]", "monitor-profile/layout/focus-ring",
            R"({"children":["on","width","active-color","inactive-color","urgent-color"]})"));
    addControl("profile struts",
        profile("Reserved screen edges", "toggle@Customize here;toggled", "[]", "monitor-profile/layout/struts",
            R"({"children":["left","right","top","bottom"]})"));
    addControl("own corners",
        output("Own hot corners", "switched", "[true]", "output/hot-corners", R"({"children":["top-left"]})", "output \"DP-1\"\n"));
    addControl("no own corners",
        output("Own hot corners", "switched", "[false]", "output/hot-corners", "null", "output \"DP-1\" { hot-corners { top-left; }; }\n"));
    addControl("corners off here",
        output("Hot corners on this monitor", "switched", "[false]", "output/hot-corners", R"({"children":["top-left","off"]})",
            "output \"DP-1\" { hot-corners { top-left; }; }\n"));
    addControl("corners on here",
        output("Hot corners on this monitor", "switched", "[true]", "output/hot-corners", R"({"children":["top-left"]})",
            "output \"DP-1\" { hot-corners { top-left; off; }; }\n"));
    addControl("corner here",
        output("=blockPath:output/hot-corners", "toggle", R"(["top-right"])", "output/hot-corners",
            R"({"children":["top-left","top-right"]})", "output \"DP-1\" { hot-corners { top-left; }; }\n"));
    addControl("output gaps",
        output("Gaps", "toggle@Customize here;toggled", "[]", "output/layout/gaps", R"({"args":[16]})", "output \"DP-1\"\n"));
    addControl("workspace added", {"WorkspacesPage", "*", "nameChosen", R"(["c"])", "workspace#2", R"({"args":["c"]})", Workspaces});
    addControl("workspace renamed",
        {"WorkspacesPage", "^Rename | *", "clicked | nameChosen", R"([] | ["bee"])", "workspace#1",
            R"({"args":["bee"],"children":["open-on-output"]})", Workspaces, 1});
    addControl("workspace up", {"WorkspacesPage", "^Move up", "clicked", "[]", "workspace", R"({"args":["b"]})", Workspaces, 1});
    addControl("workspace down", {"WorkspacesPage", "^Move down", "clicked", "[]", "workspace", R"({"args":["b"]})", Workspaces});
    addControl("workspace removed", {"WorkspacesPage", "^Remove", "clicked", "[]", "workspace", R"({"args":["b"]})", Workspaces});
    addControl(
        "workspace any output", {"WorkspacesPage", "=workspace#1", "picked", R"([""])", "workspace#1/open-on-output", "null", Workspaces});
    addControl("workspace output",
        {"WorkspacesPage", "=workspace", "picked", R"(["HDMI-A-1"])", "workspace/open-on-output", R"({"args":["HDMI-A-1"]})", Workspaces});
    addControl("workspace gaps",
        {"WorkspacesPage", "=workspace | Gaps", "expanded=true | toggle@Customize here;toggled", "[] | []", "workspace/layout/gaps",
            R"({"args":[16]})", Workspaces});
    addControl("mod key", {"ShortcutsPage", "Key that “Mod” stands for", "picked", R"(["Alt"])", "input/mod-key", R"({"args":["Alt"]})"});
    const char *add = R"(openFor;keyName="Mod+F7";actionNode={"name":"spawn","args":[" kitty "]};cooldown=150;repeat=false;save)";
    addControl("bind added",
        {"ShortcutsPage", "*", add, "[null]", "binds/Mod+F7", R"({"props":{"cooldown-ms":150,"repeat":false},"children":["spawn"]})"});
    addControl("bind arguments trimmed", {"ShortcutsPage", "*", add, "[null]", "binds/Mod+F7/spawn", R"({"args":["kitty"]})"});
    addControl("bind renamed",
        {"ShortcutsPage", "=binds/Mod+C | *", R"(editRequested | keyName="Mod+Shift+C";save)", "[] | []", "binds/Mod+C", "null"});
    addControl("bind kept action",
        {"ShortcutsPage", "=binds/Mod+C | *", R"(editRequested | keyName="Mod+Shift+C";save)", "[] | []", "binds/Mod+Shift+C",
            R"({"children":["center-column"]})"});
    const char *clash = R"(openFor;keyName="Super+C";actionNode={"name":"close-window","args":[]};save)";
    addControl("clash replaced", {"ShortcutsPage", "*", clash, "[null]", "binds/Mod+C", "null"});
    addControl("clash written", {"ShortcutsPage", "*", clash, "[null]", "binds/Super+C", R"({"children":["close-window"]})"});
    addControl("hidden bind",
        {"ShortcutsPage", "*", R"(openFor;keyName="Mod+F8";actionNode={"name":"center-column"};hideFromCheatsheet=true;save)", "[null]",
            "binds/Mod+F8", R"({"props":{"hotkey-overlay-title":null}})"});
    addControl("titled bind",
        {"ShortcutsPage", "*", R"(openFor;keyName="Mod+F8";actionNode={"name":"center-column"};cheatsheetTitle=" Middle ";save)", "[null]",
            "binds/Mod+F8", R"({"props":{"hotkey-overlay-title":"Middle"}})"});
    addControl("bind removed", {"ShortcutsPage", "=binds/Mod+C", "removeRequested", "[]", "binds/Mod+C", "null"});
    addControl("binds restored",
        {"ShortcutsPage", "^Restore default shortcuts?", "accepted", "[]", "binds/Mod+X", "null",
            "binds {\n    Mod+X { spawn \"x\"; }\n}\n"});
    addControl("defaults back",
        {"ShortcutsPage", "^Restore default shortcuts?", "accepted", "[]", "binds/Mod+C", R"({"children":["center-column"]})",
            "binds {\n}\n"});
}

QTEST_MAIN(TestSettingsControlsListsQml)
#include "test_settings_controls_lists_qml.moc"
