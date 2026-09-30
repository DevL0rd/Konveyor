#include "config/loader.h"
#include "values/configvalues.h"
#include "values/livematching.h"

#include <QTest>

using namespace Konveyor;
using namespace Konveyor::Settings;

class TestSettingsValues : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void globalDefaults();
    void globalCustomized();
    void animationKinds();
    void layoutDefaults();
    void layoutCustomized();
    void paintValues();
    void sizeAndColorValues();
    void scopedLayoutPerScope_data();
    void scopedLayoutPerScope();
    void profileForOutput();
    void rulesMatchLiveWindows();

private:
    static Config::Config load(const QString &text)
    {
        const auto loaded = Config::loadString(text, QStringLiteral("config.kdl"));
        if (!loaded) {
            qWarning("%s", qPrintable(loaded.error().toString()));
            return {};
        }
        return loaded->config;
    }
};

void TestSettingsValues::globalDefaults()
{
    const QVariantMap values = globalValues(Config::Config());
    QCOMPARE(values.size(), 25);
    for (const QString &key : {QStringLiteral("hide-desktop-widgets"), QStringLiteral("fill-panels-on-maximize"),
             QStringLiteral("disable-minimize"), QStringLiteral("experiments/prevent-fullscreen-minimize"),
             QStringLiteral("experiments/prevent-fullscreen-exit"), QStringLiteral("config-notification/disable-failed"),
             QStringLiteral("input/focus-follows-mouse"), QStringLiteral("input/warp-mouse-to-focus"),
             QStringLiteral("input/workspace-auto-back-and-forth"), QStringLiteral("gestures/resize-tiled-windows")}) {
        QVERIFY2(values.value(key).typeId() == QMetaType::Bool && !values.value(key).toBool(), qPrintable(key));
    }
    QCOMPARE(values.value(QStringLiteral("input/warp-mouse-to-focus/mode")).toString(), QString());
    QCOMPARE(values.value(QStringLiteral("gestures/titlebar-drag")).toString(), QStringLiteral("scroll-view"));
    const QVariantMap corners = values.value(QStringLiteral("gestures/hot-corners")).toMap();
    QCOMPARE(corners.value(QStringLiteral("enabled")).toBool(), true);
    QCOMPARE(corners.value(QStringLiteral("top-left")).toBool(), true);
    QCOMPARE(corners.value(QStringLiteral("bottom-right")).toBool(), false);
    const QVariantMap touchpad = values.value(QStringLiteral("gestures/touchpad")).toMap();
    QCOMPARE(touchpad.size(), 15);
    QCOMPARE(touchpad.value(QStringLiteral("natural-swipe")).toBool(), true);
    QCOMPARE(values.value(QStringLiteral("animations/enabled")).toBool(), true);
    QCOMPARE(values.value(QStringLiteral("animations/slowdown")).toDouble(), 1.0);
}

void TestSettingsValues::globalCustomized()
{
    const QVariantMap values = globalValues(load(QStringLiteral(R"(
hide-desktop-widgets
fill-panels-on-maximize
disable-minimize
experiments { prevent-fullscreen-minimize; prevent-fullscreen-exit; }
config-notification { disable-failed; }
input {
    focus-follows-mouse
    warp-mouse-to-focus mode="center-xy-always"
    workspace-auto-back-and-forth
    mod-key "Alt"
}
gestures {
    dnd-edge-view-scroll { trigger-width 40; delay-ms 90; max-speed 900; }
    dnd-edge-workspace-switch { trigger-height 30; delay-ms 70; max-speed 800; }
    hot-corners { top-right; bottom-left; }
    resize-tiled-windows
    titlebar-drag "move-window"
    touchpad {
        swipe-fingers 4
        pinch-fingers 3
        horizontal-swipe "off"
        vertical-swipe "off"
        pinch "off"
        window-swipe-fingers 3
        window-horizontal-swipe "off"
        window-vertical-swipe "off"
        three-finger-tap "toggle-overview"
        four-finger-tap "off"
        five-finger-tap "kontrol-panel"
    }
    touchscreen { off; long-press-ms 800; }
}
)")));
    for (const QString &key : {QStringLiteral("hide-desktop-widgets"), QStringLiteral("fill-panels-on-maximize"),
             QStringLiteral("disable-minimize"), QStringLiteral("experiments/prevent-fullscreen-minimize"),
             QStringLiteral("experiments/prevent-fullscreen-exit"), QStringLiteral("config-notification/disable-failed"),
             QStringLiteral("input/focus-follows-mouse"), QStringLiteral("input/warp-mouse-to-focus"),
             QStringLiteral("input/workspace-auto-back-and-forth"), QStringLiteral("gestures/resize-tiled-windows")}) {
        QVERIFY2(values.value(key).toBool(), qPrintable(key));
    }
    QCOMPARE(values.value(QStringLiteral("input/warp-mouse-to-focus/mode")).toString(), QStringLiteral("center-xy-always"));
    QCOMPARE(values.value(QStringLiteral("input/mod-key")).toString(), QStringLiteral("Alt"));
    QCOMPARE(values.value(QStringLiteral("gestures/titlebar-drag")).toString(), QStringLiteral("move-window"));
    const QVariantMap view = values.value(QStringLiteral("gestures/dnd-edge-view-scroll")).toMap();
    QCOMPARE(view.value(QStringLiteral("trigger")).toDouble(), 40.0);
    QCOMPARE(view.value(QStringLiteral("delay-ms")).toInt(), 90);
    QCOMPARE(view.value(QStringLiteral("max-speed")).toDouble(), 900.0);
    QCOMPARE(values.value(QStringLiteral("gestures/dnd-edge-workspace-switch")).toMap().value(QStringLiteral("trigger")).toDouble(), 30.0);
    const QVariantMap corners = values.value(QStringLiteral("gestures/hot-corners")).toMap();
    QCOMPARE(corners.value(QStringLiteral("top-left")).toBool(), false);
    QCOMPARE(corners.value(QStringLiteral("top-right")).toBool(), true);
    QCOMPARE(corners.value(QStringLiteral("bottom-left")).toBool(), true);
    const QVariantMap touchpad = values.value(QStringLiteral("gestures/touchpad")).toMap();
    QCOMPARE(touchpad.value(QStringLiteral("swipe-fingers")).toInt(), 4);
    QCOMPARE(touchpad.value(QStringLiteral("pinch-fingers")).toInt(), 3);
    QCOMPARE(touchpad.value(QStringLiteral("horizontal-swipe")).toString(), QStringLiteral("off"));
    QCOMPARE(touchpad.value(QStringLiteral("vertical-swipe")).toString(), QStringLiteral("off"));
    QCOMPARE(touchpad.value(QStringLiteral("pinch")).toString(), QStringLiteral("off"));
    QCOMPARE(touchpad.value(QStringLiteral("window-swipe-fingers")).toInt(), 3);
    QCOMPARE(touchpad.value(QStringLiteral("window-horizontal-swipe")).toString(), QStringLiteral("off"));
    QCOMPARE(touchpad.value(QStringLiteral("window-vertical-swipe")).toString(), QStringLiteral("off"));
    QCOMPARE(touchpad.value(QStringLiteral("three-finger-tap")).toString(), QStringLiteral("toggle-overview"));
    QCOMPARE(touchpad.value(QStringLiteral("four-finger-tap")).toString(), QStringLiteral("off"));
    QCOMPARE(touchpad.value(QStringLiteral("five-finger-tap")).toString(), QStringLiteral("kontrol-panel"));
    const QVariantMap touchscreen = values.value(QStringLiteral("gestures/touchscreen")).toMap();
    QCOMPARE(touchscreen.value(QStringLiteral("enabled")).toBool(), false);
    QCOMPARE(touchscreen.value(QStringLiteral("long-press-ms")).toInt(), 800);
}

void TestSettingsValues::animationKinds()
{
    const QVariantMap values = globalValues(load(QStringLiteral(R"(
animations {
    slowdown 2.5
    workspace-switch { spring damping-ratio=0.8 stiffness=600 epsilon=0.001; }
    window-open { duration-ms 200; curve "cubic-bezier" 0.1 0.2 0.3 0.4; }
    window-resize { off; }
    window-movement { duration-ms 100; curve "ease-out-expo"; }
}
)")));
    QCOMPARE(values.value(QStringLiteral("animations/slowdown")).toDouble(), 2.5);
    const QVariantMap spring = values.value(QStringLiteral("animations/workspace-switch")).toMap();
    QCOMPARE(spring.value(QStringLiteral("kind")).toString(), QStringLiteral("spring"));
    QCOMPARE(spring.value(QStringLiteral("damping-ratio")).toDouble(), 0.8);
    QCOMPARE(spring.value(QStringLiteral("stiffness")).toDouble(), 600.0);
    QCOMPARE(spring.value(QStringLiteral("epsilon")).toDouble(), 0.001);
    const QVariantMap easing = values.value(QStringLiteral("animations/window-open")).toMap();
    QCOMPARE(easing.value(QStringLiteral("kind")).toString(), QStringLiteral("easing"));
    QCOMPARE(easing.value(QStringLiteral("duration-ms")).toInt(), 200);
    QCOMPARE(easing.value(QStringLiteral("curve")).toString(), QStringLiteral("cubic-bezier"));
    QCOMPARE(easing.value(QStringLiteral("bezier")).toList(), (QVariantList {0.1, 0.2, 0.3, 0.4}));
    QCOMPARE(values.value(QStringLiteral("animations/window-resize")).toMap().value(QStringLiteral("enabled")).toBool(), false);
    QCOMPARE(values.value(QStringLiteral("animations/window-movement")).toMap().value(QStringLiteral("curve")).toString(),
        QStringLiteral("ease-out-expo"));
    QVERIFY(values.contains(QStringLiteral("animations/horizontal-view-movement")));
}

void TestSettingsValues::layoutDefaults()
{
    const QVariantMap values = layoutValues(Config::Layout());
    QCOMPARE(values.size(), 22);
    QCOMPARE(values.value(QStringLiteral("center-focused-column")).toString(), QStringLiteral("never"));
    QCOMPARE(values.value(QStringLiteral("new-column-position")).toString(), QStringLiteral("right"));
    QCOMPARE(values.value(QStringLiteral("always-expand-single-column")).toBool(), true);
    QCOMPARE(values.value(QStringLiteral("always-center-single-column")).toBool(), false);
    QCOMPARE(values.value(QStringLiteral("float-child-windows")).toBool(), false);
    QCOMPARE(values.value(QStringLiteral("remember-window-sizes")).toBool(), false);
    QCOMPARE(values.value(QStringLiteral("remember-window-positions")).toBool(), false);
    QCOMPARE(values.value(QStringLiteral("empty-workspace-above-first")).toBool(), false);
    QCOMPARE(values.value(QStringLiteral("default-column-display")).toString(), QStringLiteral("normal"));
    QCOMPARE(values.value(QStringLiteral("new-window-placement")).toString(), QStringLiteral("column"));
    QCOMPARE(values.value(QStringLiteral("tab-indicator")).toMap().size(), 12);
    QCOMPARE(values.value(QStringLiteral("insert-hint")).toMap().value(QStringLiteral("enabled")).toBool(), true);
}

void TestSettingsValues::layoutCustomized()
{
    const QVariantMap values = layoutValues(load(QStringLiteral(R"(
layout {
    gaps 7
    center-focused-column "on-overflow"
    new-column-position "left"
    always-center-single-column
    always-expand-single-column false
    empty-workspace-above-first
    remember-window-sizes
    remember-window-positions true
    group-app-windows "stack"
    max-rows-per-column 5
    new-window-placement "stack"
    float-child-windows
    default-column-display "tabbed"
    preset-column-widths { proportion 0.5; fixed 800; }
    default-column-width { fixed 640; }
    preset-window-heights { proportion 0.25; }
    struts { left 1; right 2; top 3; bottom 4; }
    focus-ring { off; width 2; active-color "#ff0000"; }
    border { on; width 3; inactive-color "accent"; }
    tab-indicator { hide-when-single-tab; place-within-column; gap 6; width 5; length total-proportion=0.5; position "top"; gaps-between-tabs 2; corner-radius 3; }
    insert-hint { off; color "#00ff0080"; }
    background-color "#123456"
}
)"))
            .layout);
    QCOMPARE(values.value(QStringLiteral("gaps")).toDouble(), 7.0);
    QCOMPARE(values.value(QStringLiteral("center-focused-column")).toString(), QStringLiteral("on-overflow"));
    QCOMPARE(values.value(QStringLiteral("new-column-position")).toString(), QStringLiteral("left"));
    for (const QString &key : {QStringLiteral("always-center-single-column"), QStringLiteral("empty-workspace-above-first"),
             QStringLiteral("remember-window-sizes"), QStringLiteral("remember-window-positions"), QStringLiteral("float-child-windows")}) {
        QVERIFY2(values.value(key).toBool(), qPrintable(key));
    }
    QCOMPARE(values.value(QStringLiteral("always-expand-single-column")).toBool(), false);
    QCOMPARE(values.value(QStringLiteral("group-app-windows")).toString(), QStringLiteral("stack"));
    QCOMPARE(values.value(QStringLiteral("max-rows-per-column")).toInt(), 5);
    QCOMPARE(values.value(QStringLiteral("new-window-placement")).toString(), QStringLiteral("stack"));
    QCOMPARE(values.value(QStringLiteral("default-column-display")).toString(), QStringLiteral("tabbed"));
    const QVariantList widths = values.value(QStringLiteral("preset-column-widths")).toList();
    QCOMPARE(widths.size(), 2);
    QCOMPARE(widths.at(1).toMap().value(QStringLiteral("kind")).toString(), QStringLiteral("fixed"));
    QCOMPARE(values.value(QStringLiteral("default-column-width")).toMap().value(QStringLiteral("value")).toDouble(), 640.0);
    QCOMPARE(values.value(QStringLiteral("preset-window-heights")).toList().size(), 1);
    const QVariantMap struts = values.value(QStringLiteral("struts")).toMap();
    QCOMPARE(struts.value(QStringLiteral("bottom")).toDouble(), 4.0);
    const QVariantMap ring = values.value(QStringLiteral("focus-ring")).toMap();
    QCOMPARE(ring.value(QStringLiteral("enabled")).toBool(), false);
    QCOMPARE(ring.value(QStringLiteral("width")).toDouble(), 2.0);
    QCOMPARE(ring.value(QStringLiteral("active")).toMap().value(QStringLiteral("color")).toString(), QStringLiteral("#ffff0000"));
    const QVariantMap border = values.value(QStringLiteral("border")).toMap();
    QCOMPARE(border.value(QStringLiteral("enabled")).toBool(), true);
    QCOMPARE(border.value(QStringLiteral("inactive")).toMap().value(QStringLiteral("source")).toString(), QStringLiteral("accent"));
    const QVariantMap tab = values.value(QStringLiteral("tab-indicator")).toMap();
    QCOMPARE(tab.value(QStringLiteral("hide-when-single-tab")).toBool(), true);
    QCOMPARE(tab.value(QStringLiteral("place-within-column")).toBool(), true);
    QCOMPARE(tab.value(QStringLiteral("position")).toString(), QStringLiteral("top"));
    QCOMPARE(tab.value(QStringLiteral("length")).toDouble(), 0.5);
    QCOMPARE(tab.value(QStringLiteral("gaps-between-tabs")).toDouble(), 2.0);
    QCOMPARE(tab.value(QStringLiteral("corner-radius")).toDouble(), 3.0);
    const QVariantMap hint = values.value(QStringLiteral("insert-hint")).toMap();
    QCOMPARE(hint.value(QStringLiteral("enabled")).toBool(), false);
    QCOMPARE(hint.value(QStringLiteral("paint")).toMap().value(QStringLiteral("color")).toString(), QStringLiteral("#8000ff00"));
    QCOMPARE(values.value(QStringLiteral("background-color")).toString(), QStringLiteral("#ff123456"));
}

void TestSettingsValues::paintValues()
{
    const Config::Layout layout = load(QStringLiteral(R"(
layout {
    focus-ring {
        active-gradient from="#000000" to="#ffffff" angle=45 relative-to="workspace-view" in="oklch longer hue"
        inactive-color "window-text"
        urgent-gradient from="#101010" to="#202020" in="oklab"
    }
}
)"))
                                      .layout;
    const QVariantMap active = paintValue(layout.focusRing.active).toMap();
    QCOMPARE(active.value(QStringLiteral("source")).toString(), QStringLiteral("color"));
    const QVariantMap gradient = active.value(QStringLiteral("gradient")).toMap();
    QCOMPARE(gradient.value(QStringLiteral("from")).toString(), QStringLiteral("#ff000000"));
    QCOMPARE(gradient.value(QStringLiteral("to")).toString(), QStringLiteral("#ffffffff"));
    QCOMPARE(gradient.value(QStringLiteral("angle")).toDouble(), 45.0);
    QCOMPARE(gradient.value(QStringLiteral("relative-to")).toString(), QStringLiteral("workspace-view"));
    QCOMPARE(gradient.value(QStringLiteral("in")).toString(), QStringLiteral("oklch longer hue"));
    QCOMPARE(paintValue(layout.focusRing.inactive).toMap().value(QStringLiteral("source")).toString(), QStringLiteral("window-text"));
    const QVariantMap urgent = paintValue(layout.focusRing.urgent).toMap().value(QStringLiteral("gradient")).toMap();
    QCOMPARE(urgent.value(QStringLiteral("in")).toString(), QStringLiteral("oklab"));
    QCOMPARE(urgent.value(QStringLiteral("relative-to")).toString(), QStringLiteral("window"));
    QVERIFY(!paintValue(std::nullopt).isValid());
    for (const QString &keyword :
        {QStringLiteral("focus"), QStringLiteral("hover"), QStringLiteral("window"), QStringLiteral("inactive-text")}) {
        const Config::Layout themed = load(QStringLiteral("layout { border { active-color \"%1\"; }; }").arg(keyword)).layout;
        QCOMPARE(paintValue(themed.border.active).toMap().value(QStringLiteral("source")).toString(), keyword);
    }
}

void TestSettingsValues::sizeAndColorValues()
{
    QVERIFY(!sizeValue(std::nullopt).isValid());
    const QVariantMap proportion = sizeValue(Config::PresetSize(Config::Proportion {0.25})).toMap();
    QCOMPARE(proportion.value(QStringLiteral("kind")).toString(), QStringLiteral("proportion"));
    QCOMPARE(proportion.value(QStringLiteral("value")).toDouble(), 0.25);
    const QVariantMap fixed = sizeValue(Config::PresetSize(Config::Fixed {300})).toMap();
    QCOMPARE(fixed.value(QStringLiteral("kind")).toString(), QStringLiteral("fixed"));
    QCOMPARE(fixed.value(QStringLiteral("value")).toDouble(), 300.0);
    QVERIFY(!colorValue(QColor()).isValid());
    QCOMPARE(colorValue(QColor(1, 2, 3, 4)).toString(), QStringLiteral("#04010203"));
}

void TestSettingsValues::scopedLayoutPerScope_data()
{
    QTest::addColumn<QString>("kind");
    QTest::addColumn<QString>("name");
    QTest::addColumn<int>("gaps");
    QTest::newRow("global") << QString() << QString() << 1;
    QTest::newRow("layout") << QStringLiteral("layout") << QString() << 1;
    QTest::newRow("unknown kind") << QStringLiteral("window-rule") << QStringLiteral("DP-1") << 1;
    QTest::newRow("output") << QStringLiteral("output") << QStringLiteral("DP-1") << 2;
    QTest::newRow("output other case") << QStringLiteral("output") << QStringLiteral("dp-1") << 2;
    QTest::newRow("output without layout") << QStringLiteral("output") << QStringLiteral("HDMI-A-1") << 1;
    QTest::newRow("output unknown") << QStringLiteral("output") << QStringLiteral("DP-9") << 1;
    QTest::newRow("profile") << QStringLiteral("monitor-profile") << QStringLiteral("Wide") << 3;
    QTest::newRow("profile is exact") << QStringLiteral("monitor-profile") << QStringLiteral("wide") << 5;
    QTest::newRow("workspace") << QStringLiteral("workspace") << QStringLiteral("Mail") << 4;
    QTest::newRow("workspace other case") << QStringLiteral("workspace") << QStringLiteral("MAIL") << 4;
}

void TestSettingsValues::scopedLayoutPerScope()
{
    QFETCH(QString, kind);
    QFETCH(QString, name);
    QFETCH(int, gaps);
    const Config::Config config = load(QStringLiteral(R"(
layout { gaps 1; center-focused-column "always"; }
output "DP-1" { layout { gaps 2; }; }
output "HDMI-A-1" { hot-corners { off; }; }
monitor-profile "Wide" { match aspect-ratio-above=2.0; layout { gaps 3; }; }
monitor-profile "wide" { match aspect-ratio-above=3.0; layout { gaps 5; }; }
workspace "mail" { layout { gaps 4; }; }
)"));
    const Config::Layout layout = scopedLayout(config, kind, name);
    QCOMPARE(layout.gaps, double(gaps));
    QCOMPARE(layoutValues(layout).value(QStringLiteral("center-focused-column")).toString(), QStringLiteral("always"));
}

void TestSettingsValues::profileForOutput()
{
    const Config::Config config = load(QStringLiteral(R"(
monitor-profile "portrait" { match aspect-ratio-below=1.0; }
monitor-profile "named" { match name="^HDMI"; }
)"));
    const auto output = [](const QString &name, int width, int height) {
        return QVariantMap {{QStringLiteral("name"), name},
            {QStringLiteral("logical"), QVariantMap {{QStringLiteral("width"), width}, {QStringLiteral("height"), height}}}};
    };
    QCOMPARE(profileNameFor(config, output(QStringLiteral("DP-1"), 1080, 1920)), QStringLiteral("portrait"));
    QCOMPARE(profileNameFor(config, output(QStringLiteral("HDMI-A-1"), 1920, 1080)), QStringLiteral("named"));
    QCOMPARE(profileNameFor(config, output(QStringLiteral("DP-1"), 1920, 1080)), QString());
}

void TestSettingsValues::rulesMatchLiveWindows()
{
    const Config::Config config = load(QStringLiteral(R"(
monitor-profile "portrait" { match aspect-ratio-below=1.0; }
window-rule { match app-id="^firefox$"; }
window-rule { match is-floating=true; }
window-rule { match monitor-profile="portrait"; }
window-rule { match app-id="kate" is-focused=true; exclude title="scratch"; }
)"));
    const QVariantList windows {
        QVariantMap {{QStringLiteral("app_id"), QStringLiteral("firefox")}, {QStringLiteral("title"), QStringLiteral("a")},
            {QStringLiteral("workspace_id"), 1}},
        QVariantMap {{QStringLiteral("app_id"), QStringLiteral("kate")}, {QStringLiteral("title"), QStringLiteral("notes")},
            {QStringLiteral("is_focused"), true}, {QStringLiteral("is_floating"), true}, {QStringLiteral("workspace_id"), 2}},
        QVariantMap {{QStringLiteral("app_id"), QStringLiteral("kate")}, {QStringLiteral("title"), QStringLiteral("scratch")},
            {QStringLiteral("is_focused"), true}, {QStringLiteral("workspace_id"), 2}},
    };
    const QVariantList workspaces {QVariantMap {{QStringLiteral("id"), 1}, {QStringLiteral("output"), QStringLiteral("DP-1")}},
        QVariantMap {{QStringLiteral("id"), 2}, {QStringLiteral("output"), QStringLiteral("DP-2")}}};
    const QVariantList outputs {
        QVariantMap {{QStringLiteral("name"), QStringLiteral("DP-1")},
            {QStringLiteral("logical"), QVariantMap {{QStringLiteral("width"), 1920}, {QStringLiteral("height"), 1080}}}},
        QVariantMap {{QStringLiteral("name"), QStringLiteral("DP-2")},
            {QStringLiteral("logical"), QVariantMap {{QStringLiteral("width"), 1080}, {QStringLiteral("height"), 1920}}}},
    };
    const auto titles = [&](int rule) {
        QStringList result;
        for (const QVariant &window : windowsMatchingRule(config, config.windowRules.at(rule), windows, workspaces, outputs)) {
            result.append(window.toMap().value(QStringLiteral("title")).toString());
        }
        return result;
    };
    QCOMPARE(titles(0), QStringList {QStringLiteral("a")});
    QCOMPARE(titles(1), QStringList {QStringLiteral("notes")});
    QCOMPARE(titles(2), (QStringList {QStringLiteral("notes"), QStringLiteral("scratch")}));
    QCOMPARE(titles(3), QStringList {QStringLiteral("notes")});
    QVERIFY(windowsMatchingRule(config, config.windowRules.at(0), windows, {}, {}).size() == 1);
}

QTEST_GUILESS_MAIN(TestSettingsValues)
#include "test_settings_values.moc"
