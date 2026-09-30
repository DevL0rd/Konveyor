#include "configtesthelpers.h"

#include <QTest>

using namespace Konveyor::Config;
using namespace Konveyor::Config::Testing;

namespace
{

using Reader = bool (*)(const Config &);

struct FlagCase
{
    const char *text;
    const char *name;
    Reader read;
    bool inverted;
};

const FlagCase kFlags[] = {
    {"%1", "hide-desktop-widgets", [](const Config &c) { return c.hideDesktopWidgets; }, false},
    {"%1", "fill-panels-on-maximize", [](const Config &c) { return c.fillPanelsOnMaximize; }, false},
    {"%1", "disable-minimize", [](const Config &c) { return c.disableMinimize; }, false},
    {"experiments { %1; }", "prevent-fullscreen-minimize", [](const Config &c) { return c.experiments.preventFullscreenMinimize; }, false},
    {"experiments { %1; }", "prevent-fullscreen-exit", [](const Config &c) { return c.experiments.preventFullscreenExit; }, false},
    {"config-notification { %1; }", "disable-failed", [](const Config &c) { return c.configNotificationDisableFailed; }, false},
    {"input { %1; }", "focus-follows-mouse", [](const Config &c) { return c.input.focusFollowsMouse; }, false},
    {"input { %1; }", "workspace-auto-back-and-forth", [](const Config &c) { return c.input.workspaceAutoBackAndForth; }, false},
    {"layout { %1; }", "always-center-single-column", [](const Config &c) { return c.layout.alwaysCenterSingleColumn; }, false},
    {"layout { %1; }", "always-expand-single-column", [](const Config &c) { return c.layout.alwaysExpandSingleColumn; }, false},
    {"layout { %1; }", "remember-window-sizes", [](const Config &c) { return c.layout.rememberWindowSizes; }, false},
    {"layout { %1; }", "remember-window-positions", [](const Config &c) { return c.layout.rememberWindowPositions; }, false},
    {"layout { %1; }", "float-child-windows", [](const Config &c) { return c.layout.floatChildWindows; }, false},
    {"layout { %1; }", "empty-workspace-above-first", [](const Config &c) { return c.layout.emptyWorkspaceAboveFirst; }, false},
    {"layout { tab-indicator { %1; }; }", "hide-when-single-tab", [](const Config &c) { return c.layout.tabIndicator.hideWhenSingleTab; },
        false},
    {"layout { tab-indicator { %1; }; }", "place-within-column", [](const Config &c) { return c.layout.tabIndicator.placeWithinColumn; },
        false},
    {"gestures { %1; }", "resize-tiled-windows", [](const Config &c) { return c.gestures.resizeTiledWindows; }, false},
    {"gestures { touchpad { %1; }; }", "natural-swipe", [](const Config &c) { return c.gestures.touchpad.naturalSwipe; }, false},
    {"gestures { touchpad { %1; }; }", "off", [](const Config &c) { return c.gestures.touchpad.enabled; }, true},
    {"gestures { touchscreen { %1; }; }", "natural-swipe", [](const Config &c) { return c.gestures.touchscreen.naturalSwipe; }, false},
    {"gestures { touchscreen { %1; }; }", "off", [](const Config &c) { return c.gestures.touchscreen.enabled; }, true},
    {"gestures { touchscreen { %1; }; }", "long-press-to-move", [](const Config &c) { return c.gestures.touchscreen.longPressToMove; },
        false},
    {"gestures { hot-corners { %1; }; }", "off", [](const Config &c) { return c.gestures.hotCorners.enabled; }, true},
    {"gestures { hot-corners { %1; }; }", "top-right", [](const Config &c) { return c.gestures.hotCorners.topRight; }, false},
    {"gestures { hot-corners { %1; }; }", "bottom-left", [](const Config &c) { return c.gestures.hotCorners.bottomLeft; }, false},
    {"gestures { hot-corners { %1; }; }", "bottom-right", [](const Config &c) { return c.gestures.hotCorners.bottomRight; }, false},
    {"animations { %1; }", "off", [](const Config &c) { return c.animations.enabled; }, true},
    {"animations { window-open { %1; }; }", "off", [](const Config &c) { return c.animations.windowOpen.enabled; }, true},
    {"animations { workspace-switch { %1; }; }", "off", [](const Config &c) { return c.animations.workspaceSwitch.enabled; }, true},
    {"animations { horizontal-view-movement { %1; }; }", "off", [](const Config &c) { return c.animations.horizontalViewMovement.enabled; },
        true},
    {"animations { window-movement { %1; }; }", "off", [](const Config &c) { return c.animations.windowMovement.enabled; }, true},
    {"animations { window-resize { %1; }; }", "off", [](const Config &c) { return c.animations.windowResize.enabled; }, true},
};

QString flagText(const FlagCase &flag, const QString &node)
{
    return QString::fromUtf8(flag.text).arg(node);
}

QString rowName(const FlagCase &flag, const char *form)
{
    return flagText(flag, QString::fromUtf8(flag.name) + QLatin1Char(' ') + QString::fromLatin1(form));
}

const QStringList &ruleBooleans()
{
    static const QStringList names {QStringLiteral("open-maximized"), QStringLiteral("open-maximized-to-edges"),
        QStringLiteral("open-fullscreen"), QStringLiteral("open-floating"), QStringLiteral("open-focused"), QStringLiteral("manage"),
        QStringLiteral("clip-to-geometry"), QStringLiteral("force-resizable"), QStringLiteral("float-child-windows")};
    return names;
}

}

class TestConfigFlags : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void flagForms_data();
    void flagForms();
    void flagsRejectOtherValues_data();
    void flagsRejectOtherValues();
    void touchpadRejectsTouchscreenFlags();
    void warpMouseTakesNoArguments();
    void ruleBooleansNeedAValue_data();
    void ruleBooleansNeedAValue();
    void ruleBooleansStoreTheirValue_data();
    void ruleBooleansStoreTheirValue();
    void matchBooleansNeedBooleans();
};

void TestConfigFlags::flagForms_data()
{
    QTest::addColumn<QString>("bare");
    QTest::addColumn<QString>("setTrue");
    QTest::addColumn<QString>("setFalse");
    QTest::addColumn<int>("index");
    for (std::size_t index = 0; index < std::size(kFlags); ++index) {
        const FlagCase &flag = kFlags[index];
        const QString name = QString::fromUtf8(flag.name);
        QTest::newRow(qPrintable(rowName(flag, "forms"))) << flagText(flag, name) << flagText(flag, name + QStringLiteral(" true"))
                                                          << flagText(flag, name + QStringLiteral(" false")) << static_cast<int>(index);
    }
}

void TestConfigFlags::flagForms()
{
    QFETCH(QString, bare);
    QFETCH(QString, setTrue);
    QFETCH(QString, setFalse);
    QFETCH(int, index);
    const FlagCase &flag = kFlags[index];
    const bool on = !flag.inverted;
    QCOMPARE(flag.read(parsed(bare)), on);
    QCOMPARE(flag.read(parsed(setTrue)), on);
    QCOMPARE(flag.read(parsed(setFalse)), !on);
}

void TestConfigFlags::flagsRejectOtherValues_data()
{
    QTest::addColumn<QString>("marked");
    QTest::addColumn<QString>("message");
    for (const FlagCase &flag : kFlags) {
        const QString name = QString::fromUtf8(flag.name);
        QTest::newRow(qPrintable(rowName(flag, "number")))
            << flagText(flag, name + QStringLiteral(" »1")) << QStringLiteral("expected a boolean");
        QTest::newRow(qPrintable(rowName(flag, "string")))
            << flagText(flag, name + QStringLiteral(" »\"true\"")) << QStringLiteral("expected a boolean");
        QTest::newRow(qPrintable(rowName(flag, "two")))
            << flagText(flag, name + QStringLiteral(" true »false")) << QStringLiteral("unexpected argument");
        QTest::newRow(qPrintable(rowName(flag, "property")))
            << flagText(flag, name + QStringLiteral(" »x=true")) << QStringLiteral("unexpected property `x`");
    }
}

void TestConfigFlags::flagsRejectOtherValues()
{
    QFETCH(QString, marked);
    QFETCH(QString, message);
    verifyFailure(marked, message);
}

void TestConfigFlags::touchpadRejectsTouchscreenFlags()
{
    verifyFailure(
        QStringLiteral("gestures { touchpad { »long-press-to-move; }; }"), QStringLiteral("unexpected node `long-press-to-move`"));
    verifyFailure(QStringLiteral("gestures { touchpad { »long-press-ms 500; }; }"), QStringLiteral("unexpected node `long-press-ms`"));
}

void TestConfigFlags::warpMouseTakesNoArguments()
{
    QCOMPARE(parsed(QString()).input.warpMouseToFocus, false);
    QCOMPARE(parsed(QStringLiteral("input { warp-mouse-to-focus; }")).input.warpMouseToFocus, true);
    verifyFailure(QStringLiteral("input { warp-mouse-to-focus »false; }"), QStringLiteral("no arguments expected for this node"));
    verifyFailure(QStringLiteral("input { warp-mouse-to-focus mode=»\"center\"; }"),
        QStringLiteral("expected one of `center-xy`, `center-xy-always`"));
}

void TestConfigFlags::ruleBooleansNeedAValue_data()
{
    QTest::addColumn<QString>("name");
    for (const QString &name : ruleBooleans()) {
        QTest::newRow(qPrintable(name)) << name;
    }
}

void TestConfigFlags::ruleBooleansNeedAValue()
{
    QFETCH(QString, name);
    verifyFailure(
        QStringLiteral("window-rule {\n    »%1\n}\n").arg(name), QStringLiteral("additional argument `%1` is required").arg(name));
    verifyFailure(QStringLiteral("window-rule {\n    %1 »1\n}\n").arg(name), QStringLiteral("expected a boolean"));
    verifyFailure(QStringLiteral("window-rule {\n    %1 true »true\n}\n").arg(name), QStringLiteral("unexpected argument"));
}

void TestConfigFlags::ruleBooleansStoreTheirValue_data()
{
    ruleBooleansNeedAValue_data();
}

void TestConfigFlags::ruleBooleansStoreTheirValue()
{
    QFETCH(QString, name);
    const QList<std::optional<bool> WindowRule::*> fields {&WindowRule::openMaximized, &WindowRule::openMaximizedToEdges,
        &WindowRule::openFullscreen, &WindowRule::openFloating, &WindowRule::openFocused, &WindowRule::manage, &WindowRule::clipToGeometry,
        &WindowRule::forceResizable, &WindowRule::floatChildWindows};
    const auto field = fields.at(ruleBooleans().indexOf(name));
    QCOMPARE(parsed(QStringLiteral("window-rule { }")).windowRules.first().*field, std::nullopt);
    QCOMPARE(parsed(QStringLiteral("window-rule { %1 true; }").arg(name)).windowRules.first().*field, std::optional(true));
    QCOMPARE(parsed(QStringLiteral("window-rule { %1 false; }").arg(name)).windowRules.first().*field, std::optional(false));
}

void TestConfigFlags::matchBooleansNeedBooleans()
{
    const QStringList names {QStringLiteral("is-active"), QStringLiteral("is-focused"), QStringLiteral("is-active-in-column"),
        QStringLiteral("is-floating"), QStringLiteral("is-urgent"), QStringLiteral("at-startup")};
    for (const QString &name : names) {
        verifyFailure(QStringLiteral("window-rule { match %1=»\"true\"; }").arg(name), QStringLiteral("expected a boolean"));
        verifyFailure(QStringLiteral("window-rule { exclude %1=»1; }").arg(name), QStringLiteral("expected a boolean"));
    }
}

QTEST_MAIN(TestConfigFlags)
#include "test_config_flags.moc"
