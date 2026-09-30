#include "controldriver.h"

using namespace Konveyor::Settings::Testing;

namespace
{

const char *Rule
    = "window-rule {\n    match app-id=\"^a$\"\n}\nworkspace \"mail\"\nmonitor-profile \"wide\" { match aspect-ratio-above=2.0; }\n";
const char *Editor = R"({"rulePath":"window-rule"})";

Control rule(const char *label, const char *signal, const char *arguments, const char *path, const char *expected,
    const char *config = Rule, int nth = 0)
{
    return {"RuleEditorPage", label, signal, arguments, path, expected, config, nth, Editor};
}

}

class TestSettingsControlsRulesQml : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase() { QVERIFY(m_home.setUp()); }

    void controls_data();
    void controls() { runControl(m_home); }

private:
    SettingsHome m_home;
};

void TestSettingsControlsRulesQml::controls_data()
{
    addControlColumns();
    addControl("tile", rule("Tiling", "chosen", R"(["tile"])", "window-rule/manage", R"({"args":[true]})"));
    addControl("leave to kde", rule("Tiling", "chosen", R"(["kde"])", "window-rule/manage", R"({"args":[false]})"));
    addControl(
        "tiling default", rule("Tiling", "chosen", R"(["default"])", "window-rule/manage", "null", "window-rule {\n    manage true\n}\n"));
    addControl("open floating", rule("Open floating", "chosen", R"(["yes"])", "window-rule/open-floating", R"({"args":[true]})"));
    addControl("open maximized", rule("Open maximized", "chosen", R"(["no"])", "window-rule/open-maximized", R"({"args":[false]})"));
    addControl("open to edges",
        rule("Open maximized to the edges", "chosen", R"(["yes"])", "window-rule/open-maximized-to-edges", R"({"args":[true]})"));
    addControl("open fullscreen unset",
        rule("Open fullscreen", "chosen", R"(["unset"])", "window-rule/open-fullscreen", "null",
            "window-rule {\n    open-fullscreen true\n}\n"));
    addControl("open focused", rule("Take focus when it opens", "chosen", R"(["no"])", "window-rule/open-focused", R"({"args":[false]})"));
    addControl("workspace", rule("Workspace", "currentIndex=1;activated", "[0]", "window-rule/open-on-workspace", R"({"args":["mail"]})"));
    addControl("workspace default",
        rule("Workspace", "currentIndex=0;activated", "[0]", "window-rule/open-on-workspace", "null",
            "window-rule {\n    open-on-workspace \"mail\"\n}\nworkspace \"mail\"\n"));
    addControl("monitor", rule("Monitor", "picked", R"(["DP-1"])", "window-rule/open-on-output", R"({"args":["DP-1"]})"));
    addControl("any monitor",
        rule("Monitor", "picked", R"([""])", "window-rule/open-on-output", "null", "window-rule {\n    open-on-output \"DP-1\"\n}\n"));
    addControl("app width", rule("Width", "chosen", R"(["auto"])", "window-rule/default-column-width", R"({"children":[]})"));
    addControl("set width", rule("Width", "chosen", R"(["size"])", "window-rule/default-column-width", R"({"children":["proportion"]})"));
    addControl("default width",
        rule("Width", "chosen", R"(["default"])", "window-rule/default-column-width", "null",
            "window-rule {\n    default-column-width {}\n}\n"));
    addControl("height",
        rule("Height", "edited", R"([{"kind":"fixed","value":500}])", "window-rule/default-window-height", R"({"children":["fixed"]})"));
    addControl(
        "column style", rule("Column style", "chosen", R"(["tabbed"])", "window-rule/default-column-display", R"({"args":["tabbed"]})"));
    addControl("grouping",
        rule("Other windows from this app", "chosen", R"(["stack"])", "window-rule/group-app-windows", R"({"args":["stack"]})"));
    addControl("placement", rule("New windows", "chosen", R"(["column"])", "window-rule/new-window-placement", R"({"args":["column"]})"));
    addControl(
        "float extra", rule("Float its extra windows", "chosen", R"(["yes"])", "window-rule/float-child-windows", R"({"args":[true]})"));
    addControl("force resize", rule("Force resizing", "chosen", R"(["yes"])", "window-rule/force-resizable", R"({"args":[true]})"));
    addControl(
        "stack limit", rule("Most stacked per column", "chosen", R"(["custom"])", "window-rule/max-rows-per-column", R"({"args":[3]})"));
    addControl("stack limit up",
        rule("Most stacked per column", "increase;valueModified", "[]", "window-rule/max-rows-per-column", R"({"args":[4]})",
            "window-rule {\n    max-rows-per-column 3\n}\n"));
    addControl("pin end", rule("Pin its column", "chosen", R"(["end"])", "window-rule/column-position", R"({"args":["end"]})"));
    addControl("floating position",
        rule("Floating position", "chosen", R"(["custom"])", "window-rule/default-floating-position",
            R"({"args":[],"props":{"relative-to":"top-left","x":32,"y":32}})"));
    addControl("floating across",
        rule("Floating position", "increase;valueModified", "[]", "window-rule/default-floating-position",
            R"({"props":{"relative-to":"bottom","x":18,"y":20}})",
            "window-rule {\n    default-floating-position x=10 y=20 relative-to=\"bottom\"\n}\n"));
    addControl("min width", rule("Minimum width", "chosen", R"(["limit"])", "window-rule/min-width", R"({"args":[800]})"));
    addControl("max height up",
        rule("Maximum height", "increase;valueModified", "[]", "window-rule/max-height", R"({"args":[950]})",
            "window-rule {\n    max-height 900\n}\n"));
    addControl("no max width",
        rule("Maximum width", "chosen", R"(["none"])", "window-rule/max-width", "null", "window-rule {\n    max-width 700\n}\n"));
    addControl("min height", rule("Minimum height", "chosen", R"(["limit"])", "window-rule/min-height", R"({"args":[800]})"));
    addControl("opacity", rule("Opacity", "chosen", R"(["custom"])", "window-rule/opacity", R"({"args":[0.9]})"));
    addControl("opacity default",
        rule("Opacity", "chosen", R"(["default"])", "window-rule/opacity", "null", "window-rule {\n    opacity 0.5\n}\n"));
    addControl("corners", rule("Rounded corners", "edited", "[[3,3,3,3]]", "window-rule/geometry-corner-radius", R"({"args":[3]})"));
    addControl(
        "mixed corners", rule("Rounded corners", "edited", "[[1,2,3,4]]", "window-rule/geometry-corner-radius", R"({"args":[1,2,3,4]})"));
    addControl("inherit corners",
        rule("Rounded corners", "edited", "[null]", "window-rule/geometry-corner-radius", "null",
            "window-rule {\n    geometry-corner-radius 2\n}\n"));
    addControl(
        "clip", rule("Clip the window to its corners", "chosen", R"(["yes"])", "window-rule/clip-to-geometry", R"({"args":[true]})"));
    addControl("custom ring", rule("Custom focus ring", "switched", "[true]", "window-rule/focus-ring", R"({"children":[]})"));
    addControl("no custom border",
        rule("Custom border", "switched", "[false]", "window-rule/border", "null", "window-rule {\n    border { on; }\n}\n"));
    addControl("show border",
        rule("Show it", "chosen", R"(["yes"])", "window-rule/border", R"({"children":["width","on"]})",
            "window-rule {\n    border { off; width 2; }\n}\n", 1));
    addControl("hide ring",
        rule("Show it", "chosen", R"(["no"])", "window-rule/focus-ring", R"({"children":["off"]})",
            "window-rule {\n    focus-ring {}\n}\n"));
    addControl("inherit ring width",
        rule("Width", "chosen", R"(["inherit"])", "window-rule/focus-ring/width", "null", "window-rule {\n    focus-ring { width 3; }\n}\n",
            1));
    addControl("border width", rule("Width", "chosen", R"(["custom"])", "window-rule/border/width", R"({"args":[4]})", Rule, 2));
    addControl("ring paint",
        rule("Focused window", "chosen", R"(["theme"])", "window-rule/focus-ring/active-color", R"({"args":["accent"]})",
            "window-rule {\n    focus-ring {}\n}\n"));
    addControl("ring paint inherit",
        rule("Needs attention", "chosen", R"(["none"])", "window-rule/border/urgent-color", "null",
            "window-rule {\n    border { urgent-color \"#ff0000\"; }\n}\n"));
    addControl("any app", rule("App", "chosen", R"(["any"])", "window-rule/match", R"({"props":{}})"));
    addControl("pick app",
        rule("App", "picked", R"(["org.kde.dolphin"])", "window-rule/match", R"({"props":{"app-id":"^org\\.kde\\.dolphin$"}})"));
    addControl("title", rule("Title", "edited", R"(["^x$"])", "window-rule/match", R"({"props":{"app-id":"^a$","title":"^x$"}})"));
    addControl("profile",
        rule("Monitor profile", "currentIndex=1;activated", "[0]", "window-rule/match",
            R"({"props":{"app-id":"^a$","monitor-profile":"^wide$"}})"));
    addControl("floating state",
        rule("Window state", "chosen@3", R"(["yes"])", "window-rule/match", R"({"props":{"app-id":"^a$","is-floating":true}})"));
    addControl("is-focused state",
        rule("Window state", "chosen@0", R"(["yes"])", "window-rule/match", R"({"props":{"app-id":"^a$","is-focused":true}})"));
    addControl("is-active state",
        rule("Window state", "chosen@1", R"(["no"])", "window-rule/match", R"({"props":{"app-id":"^a$","is-active":false}})"));
    addControl("is-active-in-column state",
        rule("Window state", "chosen@2", R"(["yes"])", "window-rule/match", R"({"props":{"app-id":"^a$","is-active-in-column":true}})"));
    addControl("is-urgent state",
        rule("Window state", "chosen@4", R"(["yes"])", "window-rule/match", R"({"props":{"app-id":"^a$","is-urgent":true}})"));
    addControl("startup state",
        rule("Window state", "chosen@5", R"(["no"])", "window-rule/match", R"({"props":{"app-id":"^a$","at-startup":false}})"));
    addControl("add match", rule("^Or also match…", "clicked", "[]", "window-rule/match#1", R"({"args":[],"props":{}})"));
    addControl("add exclude", rule("^Except windows that…", "clicked", "[]", "window-rule/exclude", R"({"args":[],"props":{}})"));
    addControl("remove match", rule("=window-rule/match", "removeRequested", "[]", "window-rule/match", "null"));
    addControl("delete rule", rule("^Delete this rule", "clicked", "[]", "window-rule", "null"));
    const char *two = "window-rule { match app-id=\"^a$\"; }\nwindow-rule { match app-id=\"^b$\"; }\n";
    addControl("add rule", {"RulesPage", "^Add rule", "clicked", "[]", "window-rule#2", R"({"children":["match"]})", two});
    addControl("rule up", {"RulesPage", "*", "movedUp@1", "[]", "window-rule/match", R"({"props":{"app-id":"^b$"}})", two});
    addControl("rule down", {"RulesPage", "*", "movedDown@0", "[]", "window-rule/match", R"({"props":{"app-id":"^b$"}})", two});
    addControl("rule deleted", {"RulesPage", "*", "deleted@0", "[]", "window-rule#1", "null", two});
}

QTEST_MAIN(TestSettingsControlsRulesQml)
#include "test_settings_controls_rules_qml.moc"
