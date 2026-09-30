#include "controldriver.h"

using namespace Konveyor::Settings::Testing;

class TestSettingsControlsLookQml : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase() { QVERIFY(m_home.setUp()); }

    void controls_data();
    void controls() { runControl(m_home); }

private:
    SettingsHome m_home;
};

void TestSettingsControlsLookQml::controls_data()
{
    addControlColumns();
    const char *gradient = R"({"props":{"angle":180,"from":"#7f00ff","to":"#00b4ff"}})";
    addControl("ring off",
        {"LookPage", "Show focus ring", "switched", "[false]", "layout/focus-ring",
            R"({"children":["width","active-color","inactive-color","off"]})"});
    addControl("border on",
        {"LookPage", "Show border", "switched", "[true]", "layout/border",
            R"({"children":["width","active-color","inactive-color","urgent-color","on"]})"});
    addControl("ring width", {"LookPage", "Thickness", "edited", "[6.4]", "layout/focus-ring/width", R"({"args":[6]})"});
    addControl("border width", {"LookPage", "Thickness", "edited", "[2]", "layout/border/width", R"({"args":[2]})", nullptr, 1});
    addControl("ring gradient", {"LookPage", "Focused window", "chosen", R"(["gradient"])", "layout/focus-ring/active-gradient", gradient});
    addControl(
        "gradient drops color", {"LookPage", "Focused window", "chosen", R"(["gradient"])", "layout/focus-ring/active-color", "null"});
    addControl("gradient after color",
        {"LookPage", "Focused window", "chosen", R"(["gradient"])", "layout/focus-ring/active-color", "null",
            "layout { focus-ring { active-gradient from=\"#000\" to=\"#fff\"; active-color \"#123456\"; }; }\n"});
    addControl("ring theme",
        {"LookPage", "Other windows", "chosen", R"(["theme"])", "layout/focus-ring/inactive-color", R"({"args":["accent"]})"});
    addControl("ring color",
        {"LookPage", "Focused window", "chosen", R"(["color"])", "layout/focus-ring/active-color", R"({"args":["#7f00ff"]})"});
    addControl("urgent auto", {"LookPage", "Needs attention", "chosen", R"(["none"])", "layout/border/urgent-color", "null", nullptr, 1});
    addControl("picked color",
        {"LookPage", "Focused window", "edited", R"(["#ff000080"])", "layout/focus-ring/active-color", R"({"args":["#ff000080"]})"});
    addControl("gradient start",
        {"LookPage", "Focused window", "edited@1", R"(["#102030"])", "layout/focus-ring/active-gradient",
            R"({"props":{"angle":180,"from":"#102030","to":"#00b4ff"}})"});
    addControl("gradient end",
        {"LookPage", "Focused window", "edited@2", R"(["#405060"])", "layout/focus-ring/active-gradient",
            R"({"props":{"angle":180,"from":"#7f00ff","to":"#405060"}})"});
    addControl("gradient angle",
        {"LookPage", "Focused window", "edited@3", "[44.6]", "layout/focus-ring/active-gradient",
            R"({"props":{"angle":45,"from":"#7f00ff","to":"#00b4ff"}})"});
    addControl("gradient space",
        {"LookPage", "Focused window", "edited@4", R"([{"relative-to":"workspace-view","in":"oklab"}])",
            "layout/focus-ring/active-gradient",
            R"({"props":{"angle":180,"from":"#7f00ff","in":"oklab","relative-to":"workspace-view","to":"#00b4ff"}})"});
    addControl("tabs off",
        {"LookPage", "Show tabs beside tabbed columns", "switched", "[false]", "layout/tab-indicator", R"({"children":["off"]})"});
    addControl("tab side", {"LookPage", "Placement", "chosen", R"(["top"])", "layout/tab-indicator/position", R"({"args":["top"]})"});
    addControl("tab inside",
        {"LookPage", "^Inside the column", "toggle,toggled", "[]", "layout/tab-indicator/place-within-column", R"({"args":[]})"});
    addControl("tab outside",
        {"LookPage", "^Inside the column", "toggle,toggled", "[]", "layout/tab-indicator/place-within-column", "null",
            "layout { tab-indicator { place-within-column; }; }\n"});
    addControl("hide single tab",
        {"LookPage", "Hide when there's only one tab", "switched", "[true]", "layout/tab-indicator/hide-when-single-tab",
            R"({"args":[]})"});
    addControl("tab width", {"LookPage", "Thickness", "edited", "[5]", "layout/tab-indicator/width", R"({"args":[5]})", nullptr, 2});
    addControl("tab gap", {"LookPage", "Distance from the window", "edited", "[-4]", "layout/tab-indicator/gap", R"({"args":[-4]})"});
    addControl("tab length",
        {"LookPage", "Length", "edited", "[55.4]", "layout/tab-indicator/length", R"({"args":[],"props":{"total-proportion":0.55}})"});
    addControl(
        "tab spacing", {"LookPage", "Space between tabs", "edited", "[3]", "layout/tab-indicator/gaps-between-tabs", R"({"args":[3]})"});
    addControl("tab roundness", {"LookPage", "Roundness", "edited", "[2]", "layout/tab-indicator/corner-radius", R"({"args":[2]})"});
    addControl(
        "tab color", {"LookPage", "Showing tab", "chosen", R"(["theme"])", "layout/tab-indicator/active-color", R"({"args":["accent"]})"});
    addControl("hint off",
        {"LookPage", "Show where a dragged window will land", "switched", "[false]", "layout/insert-hint", R"({"children":["off"]})"});
    addControl("hint gradient", {"LookPage", "Hint color", "chosen", R"(["gradient"])", "layout/insert-hint/gradient", gradient});
    addControl("even corners",
        {"LookPage", "Corner roundness for every window", "edited", "[[4,4,4,4]]", "window-rule/geometry-corner-radius",
            R"({"args":[4]})"});
    addControl("uneven corners",
        {"LookPage", "Corner roundness for every window", "edited", "[[1,2.4,3,4]]", "window-rule/geometry-corner-radius",
            R"({"args":[1,2,3,4]})"});
    addControl("first corner rule",
        {"LookPage", "Corner roundness for every window", "edited", "[[8,8,8,8]]", "window-rule#1/geometry-corner-radius",
            R"({"args":[8]})", "window-rule { match app-id=\"a\"; }\n"});
    addControl("no clipping",
        {"LookPage", "Cut app content to the rounded shape", "switched", "[false]", "window-rule/clip-to-geometry", R"({"args":[false]})"});
    addControl("animations off", {"MotionPage", "Animate windows", "switched", "[false]", "animations", R"({"children":["off"]})"});
    addControl("slowdown", {"MotionPage", "Animation speed", "edited", "[2.46]", "animations/slowdown", R"({"args":[2.5]})"});
    addControl("open instantly", {"MotionPage", "Opening windows", "toggle,toggled", "[]", "animations/window-open/off", R"({"args":[]})"});
    addControl("open animated",
        {"MotionPage", "Opening windows", "toggle,toggled", "[]", "animations/window-open/off", "null",
            "animations { window-open { off; }; }\n"});
    addControl("spring",
        {"MotionPage", "^Moving windows", "chosen", R"(["spring"])", "animations/window-movement/spring",
            R"({"props":{"damping-ratio":1,"epsilon":0.0001,"stiffness":800}})",
            "animations { window-movement { duration-ms 200; curve \"linear\"; }; }\n"});
    addControl("curve",
        {"MotionPage", "^Opening windows", "chosen", R"(["easing"])", "animations/window-open/curve", R"({"args":["ease-out-cubic"]})"});
    addControl("bounciness",
        {"MotionPage", "Bounciness", "edited", "[0.456]", "animations/workspace-switch/spring",
            R"({"props":{"damping-ratio":0.46,"epsilon":0.0001,"stiffness":1000}})", nullptr, 4});
    addControl("stiffness",
        {"MotionPage", "Stiffness", "edited", "[0.4]", "animations/window-resize/spring",
            R"({"props":{"damping-ratio":1,"epsilon":0.0001,"stiffness":1}})", nullptr, 2});
    addControl(
        "duration", {"MotionPage", "Duration", "edited", "[321.6]", "animations/window-open/duration-ms", R"({"args":[322]})", nullptr, 3});
    addControl("named curve",
        {"MotionPage", "Curve", "chosen", R"(["linear"])", "animations/horizontal-view-movement/curve", R"({"args":["linear"]})"});
    addControl("custom curve",
        {"MotionPage", "Custom curve", "edited", "[[0.1,0.2,0.3,0.4]]", "animations/window-open/curve",
            R"({"args":["cubic-bezier",0.1,0.2,0.3,0.4]})", nullptr, 3});
    addControl("precision",
        {"MotionPage", "Finish precision", "chosen", "[0.01]", "animations/window-movement/spring",
            R"({"props":{"damping-ratio":1,"epsilon":0.01,"stiffness":800}})", nullptr, 1});
    addControl("touchpad off", {"TouchPage", "Use touchpad gestures", "switched", "[false]", "gestures/touchpad/off", R"({"args":[]})"});
    addControl("touchscreen on",
        {"TouchPage", "Use touchscreen gestures", "switched", "[true]", "gestures/touchscreen/off", "null",
            "gestures { touchscreen { off; }; }\n"});
    addControl("swipe fingers", {"TouchPage", "Swipe with", "chosen", "[4]", "gestures/touchpad/swipe-fingers", R"({"args":[4]})"});
    addControl("horizontal swipe",
        {"TouchPage", "Swipe left or right", "chosen", R"(["off"])", "gestures/touchscreen/horizontal-swipe", R"({"args":["off"]})",
            nullptr, 1});
    addControl("vertical swipe",
        {"TouchPage", "Swipe up or down", "chosen", R"(["off"])", "gestures/touchpad/vertical-swipe", R"({"args":["off"]})"});
    addControl(
        "natural swipe", {"TouchPage", "Natural swiping", "switched", "[false]", "gestures/touchpad/natural-swipe", R"({"args":[false]})"});
    addControl("window fingers",
        {"TouchPage", "Move windows with", "chosen", "[3]", "gestures/touchscreen/window-swipe-fingers", R"({"args":[3]})", nullptr, 1});
    addControl("window push",
        {"TouchPage", "Push the window left or right", "chosen", R"(["off"])", "gestures/touchpad/window-horizontal-swipe",
            R"({"args":["off"]})"});
    addControl("window lift",
        {"TouchPage", "Push the window up or down", "chosen", R"(["off"])", "gestures/touchpad/window-vertical-swipe",
            R"({"args":["off"]})"});
    addControl("pinch fingers", {"TouchPage", "Pinch with", "chosen", "[5]", "gestures/touchpad/pinch-fingers", R"({"args":[5]})"});
    addControl("pinch", {"TouchPage", "Pinch", "chosen", R"(["off"])", "gestures/touchscreen/pinch", R"({"args":["off"]})", nullptr, 1});
    addControl("long press",
        {"TouchPage", "Hold a title bar to move the window", "switched", "[false]", "gestures/touchscreen/long-press-to-move",
            R"({"args":[false]})", nullptr, 1});
    addControl(
        "hold time", {"TouchPage", "Hold for", "edited", "[650.4]", "gestures/touchscreen/long-press-ms", R"({"args":[650]})", nullptr, 1});
    addControl("three tap",
        {"TouchPage", "Tap with 3 fingers", "chosen", R"(["toggle-overview"])", "gestures/touchpad/three-finger-tap",
            R"({"args":["toggle-overview"]})"});
    addControl("four tap",
        {"TouchPage", "Tap with 4 fingers", "chosen", R"(["off"])", "gestures/touchscreen/four-finger-tap", R"({"args":["off"]})", nullptr,
            1});
    addControl("five tap",
        {"TouchPage", "Tap with 5 fingers", "chosen", R"(["cycle-width"])", "gestures/touchpad/five-finger-tap",
            R"({"args":["cycle-width"]})"});
}

QTEST_MAIN(TestSettingsControlsLookQml)
#include "test_settings_controls_look_qml.moc"
