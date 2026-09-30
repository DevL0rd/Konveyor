#include "controldriver.h"

using namespace Konveyor::Settings::Testing;

class TestSettingsControlsQml : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase() { QVERIFY(m_home.setUp()); }

    void controls_data();
    void controls() { runControl(m_home); }

private:
    SettingsHome m_home;
};

void TestSettingsControlsQml::controls_data()
{
    addControlColumns();
    const char *empty = "";
    addControl(
        "hide widgets off", {"PlasmaPage", "Hide desktop widgets behind windows", "switched", "[false]", "hide-desktop-widgets", "null"});
    addControl("hide widgets on",
        {"PlasmaPage", "Hide desktop widgets behind windows", "switched", "[true]", "hide-desktop-widgets", R"({"args":[]})", empty});
    addControl("fill panels off",
        {"PlasmaPage", "Stretch panels while a window is maximized", "switched", "[false]", "fill-panels-on-maximize", "null"});
    addControl("fill panels on",
        {"PlasmaPage", "Stretch panels while a window is maximized", "switched", "[true]", "fill-panels-on-maximize", R"({"args":[]})",
            empty});
    addControl("no minimize", {"PlasmaPage", "Don't allow minimizing", "switched", "[true]", "disable-minimize", R"({"args":[]})"});
    addControl("mute failures",
        {"PlasmaPage", "Notify me when the config has an error", "switched", "[false]", "config-notification/disable-failed",
            R"({"args":[]})"});
    addControl("notify failures",
        {"PlasmaPage", "Notify me when the config has an error", "switched", "[true]", "config-notification/disable-failed", "null",
            "config-notification { disable-failed; }\n"});
    addControl("keep minimized",
        {"ExperimentsPage", "Prevent minimizing on focus change", "switched", "[true]", "experiments/prevent-fullscreen-minimize",
            R"({"args":[]})"});
    addControl("keep fullscreen",
        {"ExperimentsPage", "Prevent leaving fullscreen on focus change", "switched", "[true]", "experiments/prevent-fullscreen-exit",
            R"({"args":[]})"});
    addControl("back and forth",
        {"WorkspacesPage", "Go back when switching to the current workspace", "switched", "[true]", "input/workspace-auto-back-and-forth",
            R"({"args":[]})"});
    addControl("empty above",
        {"WorkspacesPage", "Keep an empty workspace above the first", "switched", "[true]", "layout/empty-workspace-above-first",
            R"({"args":[]})"});
    addControl(
        "focus follows", {"MousePage", "Focus follows the mouse", "switched", "[true]", "input/focus-follows-mouse", R"({"args":[]})"});
    addControl("warp nearest",
        {"MousePage", "Move the pointer with keyboard focus", "chosen", R"(["nearest"])", "input/warp-mouse-to-focus",
            R"({"args":[],"props":{}})"});
    addControl("warp center",
        {"MousePage", "Move the pointer with keyboard focus", "chosen", R"(["center-xy"])", "input/warp-mouse-to-focus",
            R"({"props":{"mode":"center-xy"}})"});
    addControl("warp always",
        {"MousePage", "Move the pointer with keyboard focus", "chosen", R"(["center-xy-always"])", "input/warp-mouse-to-focus",
            R"({"props":{"mode":"center-xy-always"}})"});
    addControl("warp off",
        {"MousePage", "Move the pointer with keyboard focus", "chosen", R"(["off"])", "input/warp-mouse-to-focus", "null",
            "input { warp-mouse-to-focus; }\n"});
    addControl("title bar",
        {"MousePage", "Dragging a tiled window's title bar", "chosen", R"(["move-window"])", "gestures/titlebar-drag",
            R"({"args":["move-window"]})"});
    addControl("resize tiled",
        {"MousePage", "Resize tiled windows by dragging their edges", "switched", "[true]", "gestures/resize-tiled-windows",
            R"({"args":[]})"});
    addControl("corners on",
        {"MousePage", "Open the overview from screen corners", "switched", "[true]", "gestures/hot-corners", R"({"children":[]})"});
    addControl("corners off",
        {"MousePage", "Open the overview from screen corners", "switched", "[false]", "gestures/hot-corners",
            R"({"children":["top-right","off"]})", "gestures { hot-corners { top-right; }; }\n"});
    addControl("corner added",
        {"MousePage", "Active corners", "toggled", R"(["top-right"])", "gestures/hot-corners", R"({"children":["top-left","top-right"]})",
            "gestures {\n}\n"});
    addControl("corner removed",
        {"MousePage", "Active corners", "toggled", R"(["top-left"])", "gestures/hot-corners", R"({"children":["top-right"]})",
            "gestures { hot-corners { top-left; top-right; }; }\n"});
    addControl("last corner off",
        {"MousePage", "Active corners", "toggled", R"(["top-left"])", "gestures/hot-corners", R"({"children":["top-left","off"]})",
            "gestures { hot-corners { top-left; }; }\n"});
    addControl(
        "edge width", {"MousePage", "Edge width", "edited", "[40.4]", "gestures/dnd-edge-view-scroll/trigger-width", R"({"args":[40]})"});
    addControl("edge height",
        {"MousePage", "Edge height", "edited", "[0]", "gestures/dnd-edge-workspace-switch/trigger-height", R"({"args":[0]})"});
    addControl("scroll delay",
        {"MousePage", "Wait before scrolling", "edited", "[120]", "gestures/dnd-edge-view-scroll/delay-ms", R"({"args":[120]})"});
    addControl("switch delay",
        {"MousePage", "Wait before scrolling", "edited", "[300]", "gestures/dnd-edge-workspace-switch/delay-ms", R"({"args":[300]})",
            nullptr, 1});
    addControl(
        "scroll speed", {"MousePage", "Top speed", "edited", "[1500]", "gestures/dnd-edge-view-scroll/max-speed", R"({"args":[1500]})"});
    addControl("switch speed",
        {"MousePage", "Top speed", "edited", "[900]", "gestures/dnd-edge-workspace-switch/max-speed", R"({"args":[900]})", nullptr, 1});
    addControl("gaps", {"LayoutPage", "Gaps", "edited", "[12.4]", "layout/gaps", R"({"args":[12]})"});
    addControl("width presets",
        {"LayoutPage", "Width presets", "edited", R"([[{"kind":"proportion","value":0.5},{"kind":"fixed","value":800}]])",
            "layout/preset-column-widths", R"({"children":["proportion","fixed"]})"});
    addControl("height presets", {"LayoutPage", "Height presets", "edited", "[[]]", "layout/preset-window-heights", R"({"children":[]})"});
    addControl("struts",
        {"LayoutPage", "Reserved screen edges", "edited", R"([{"left":10,"right":0,"top":5.6,"bottom":0}])", "layout/struts/top",
            R"({"args":[6]})"});
    addControl("background",
        {"LayoutPage", "Workspace background", "edited", R"(["#112233"])", "layout/background-color", R"({"args":["#112233"]})"});
    addControl("new column",
        {"LayoutPage", "Where new windows open", "chosen", R"(["left"])", "layout/new-column-position", R"({"args":["left"]})"});
    addControl("placement",
        {"LayoutPage", "Give each new window", "chosen", R"(["stack"])", "layout/new-window-placement", R"({"args":["stack"]})"});
    addControl("grouping",
        {"LayoutPage", "Windows from an app that's already open", "chosen", R"(["off"])", "layout/group-app-windows",
            R"({"args":["off"]})"});
    addControl(
        "max rows", {"LayoutPage", "Most stacked windows per column", "edited", "[4]", "layout/max-rows-per-column", R"({"args":[4]})"});
    addControl("float extra on",
        {"LayoutPage", "Float an app's extra windows", "switched", "[true]", "layout/float-child-windows", R"({"args":[]})"});
    addControl("float extra off",
        {"LayoutPage", "Float an app's extra windows", "switched", "[false]", "layout/float-child-windows", R"({"args":[false]})",
            "layout { float-child-windows; }\n"});
    addControl("starting width",
        {"LayoutPage", "Starting width", "edited", R"([{"kind":"fixed","value":700}])", "layout/default-column-width",
            R"({"children":["fixed"]})"});
    addControl(
        "app picks width", {"LayoutPage", "Starting width", "edited", "[null]", "layout/default-column-width", R"({"children":[]})"});
    addControl("tabbed",
        {"LayoutPage", "How a column shows several windows", "chosen", R"(["tabbed"])", "layout/default-column-display",
            R"({"args":["tabbed"]})"});
    addControl("centering",
        {"LayoutPage", "Keep the focused column centered", "chosen", R"(["always"])", "layout/center-focused-column",
            R"({"args":["always"]})"});
    addControl("center lone",
        {"LayoutPage", "Center a lone window", "switched", "[true]", "layout/always-center-single-column", R"({"args":[]})"});
    addControl("center lone off",
        {"LayoutPage", "Center a lone window", "switched", "[false]", "layout/always-center-single-column", "null",
            "layout { always-center-single-column; }\n"});
    addControl("expand lone off",
        {"LayoutPage", "Expand a lone window", "switched", "[false]", "layout/always-expand-single-column", R"({"args":[false]})"});
    addControl("expand lone on",
        {"LayoutPage", "Expand a lone window", "switched", "[true]", "layout/always-expand-single-column", R"({"args":[true]})",
            "layout { always-expand-single-column false; }\n"});
    addControl(
        "remember sizes", {"LayoutPage", "Remember window sizes", "switched", "[true]", "layout/remember-window-sizes", R"({"args":[]})"});
    addControl("forget sizes",
        {"LayoutPage", "Remember window sizes", "switched", "[false]", "layout/remember-window-sizes", R"({"args":[false]})",
            "layout { remember-window-sizes; }\n"});
    addControl("remember positions",
        {"LayoutPage", "Remember floating window positions", "switched", "[true]", "layout/remember-window-positions", R"({"args":[]})"});
}

QTEST_MAIN(TestSettingsControlsQml)
#include "test_settings_controls_qml.moc"
