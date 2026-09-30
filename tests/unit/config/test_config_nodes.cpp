#include "configtesthelpers.h"

#include <QTest>

using namespace Konveyor::Config;
using namespace Konveyor::Config::Testing;

namespace
{

EasingParams easingOf(const AnimationParams &params)
{
    return std::get<EasingParams>(params.kind);
}

}

class TestConfigNodes : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void warpMouseModes();
    void modKeySpellings_data();
    void modKeySpellings();
    void rejectsUnknownModKey();
    void windowMovementSlot();
    void curveOnlyOnSpringSlot();
    void durationOnlyKeepsSlotCurve();
    void offSlotKeepsDefaultKind();
    void curveKeywords_data();
    void curveKeywords();
    void curveRejectsExtraArguments();
    void workspaceSwitchEdge();
    void edgeScrollTriggerNamesDiffer();
    void touchKeywords();
    void titlebarDrag();
    void tabIndicatorPositions();
    void tabIndicatorMergesFieldByField();
    void tabIndicatorLengthNeedsItsProperty();
    void themeColorKeywords_data();
    void themeColorKeywords();
    void themeColorsAreOnlyForDecorations();
    void gradientsForInactiveAndInsertHint();
};

void TestConfigNodes::warpMouseModes()
{
    QCOMPARE(parsed(QString()).input.warpMouseMode, WarpMouseMode::Separate);
    QCOMPARE(parsed(QStringLiteral("input { warp-mouse-to-focus; }")).input.warpMouseMode, WarpMouseMode::Separate);
    const Input centered = parsed(QStringLiteral("input { warp-mouse-to-focus mode=\"center-xy\"; }")).input;
    QCOMPARE(centered.warpMouseToFocus, true);
    QCOMPARE(centered.warpMouseMode, WarpMouseMode::CenterXY);
    QCOMPARE(parsed(QStringLiteral("input { warp-mouse-to-focus mode=\"center-xy-always\"; }")).input.warpMouseMode,
        WarpMouseMode::CenterXYAlways);
    verifyFailure(QStringLiteral("input { warp-mouse-to-focus »x=1; }"), QStringLiteral("unexpected property `x`"));
}

void TestConfigNodes::modKeySpellings_data()
{
    QTest::addColumn<QString>("spelling");
    QTest::addColumn<QString>("canonical");

    QTest::newRow("ctrl") << QStringLiteral("ctrl") << QStringLiteral("Ctrl");
    QTest::newRow("Control") << QStringLiteral("Control") << QStringLiteral("Ctrl");
    QTest::newRow("shift") << QStringLiteral("SHIFT") << QStringLiteral("Shift");
    QTest::newRow("alt") << QStringLiteral("alt") << QStringLiteral("Alt");
    QTest::newRow("super") << QStringLiteral("Super") << QStringLiteral("Super");
    QTest::newRow("win") << QStringLiteral("Win") << QStringLiteral("Super");
    QTest::newRow("iso level 3") << QStringLiteral("ISO_Level3_Shift") << QStringLiteral("ISO_Level3_Shift");
    QTest::newRow("mod5") << QStringLiteral("Mod5") << QStringLiteral("ISO_Level3_Shift");
    QTest::newRow("iso level 5") << QStringLiteral("iso_level5_shift") << QStringLiteral("ISO_Level5_Shift");
    QTest::newRow("mod3") << QStringLiteral("mod3") << QStringLiteral("ISO_Level5_Shift");
}

void TestConfigNodes::modKeySpellings()
{
    QFETCH(QString, spelling);
    QFETCH(QString, canonical);
    const Config config = parsed(QStringLiteral("input { mod-key \"%1\"; }\nbinds { Mod+Q { close-window; }; }").arg(spelling));
    QCOMPARE(config.input.modKey, canonical);
    QCOMPARE(config.binds.first().resolvedModifiers.testFlag(BindModifier::Mod), false);
    QCOMPARE(config.binds.first().resolvedModifiers,
        parsed(QStringLiteral("binds { %1+Q { close-window; }; }").arg(spelling)).binds.first().keyModifiers);
}

void TestConfigNodes::rejectsUnknownModKey()
{
    verifyFailure(QStringLiteral("input { »mod-key \"Hyper\"; }"), QStringLiteral("invalid Mod key: Hyper"));
    verifyFailure(QStringLiteral("input { mod-key »4; }"), QStringLiteral("expected a string"));
    verifyFailure(QStringLiteral("input { »mod-key; }"), QStringLiteral("additional argument `mod-key` is required"));
}

void TestConfigNodes::windowMovementSlot()
{
    QCOMPARE(std::get<SpringParams>(parsed(QString()).animations.windowMovement.kind), (SpringParams {1.0, 800, 0.0001}));
    const Animations animations = parsed(QStringLiteral(R"(
        animations {
            window-movement { spring damping-ratio=0.5 stiffness=300 epsilon=0.01; }
        }
    )"))
                                      .animations;
    QCOMPARE(std::get<SpringParams>(animations.windowMovement.kind), (SpringParams {0.5, 300, 0.01}));
    QCOMPARE(animations.windowResize, defaultConfig().animations.windowResize);
}

void TestConfigNodes::curveOnlyOnSpringSlot()
{
    const Animations animations = parsed(QStringLiteral("animations { workspace-switch { curve \"linear\"; }; }")).animations;
    const EasingParams easing = easingOf(animations.workspaceSwitch);
    QCOMPARE(easing.curve, EasingCurve::Linear);
    QCOMPARE(easing.durationMs, 250.0);
    QCOMPARE(animations.workspaceSwitch.enabled, true);
}

void TestConfigNodes::durationOnlyKeepsSlotCurve()
{
    const Animations animations = parsed(QStringLiteral(R"(
        animations {
            window-open { duration-ms 400; }
            horizontal-view-movement { duration-ms 90; }
        }
    )"))
                                      .animations;
    QCOMPARE(easingOf(animations.windowOpen).curve, EasingCurve::EaseOutExpo);
    QCOMPARE(easingOf(animations.windowOpen).durationMs, 400.0);
    QCOMPARE(easingOf(animations.horizontalViewMovement).curve, EasingCurve::EaseOutCubic);
    QCOMPARE(easingOf(animations.horizontalViewMovement).durationMs, 90.0);
}

void TestConfigNodes::offSlotKeepsDefaultKind()
{
    const Animations animations = parsed(QStringLiteral("animations { window-resize { off; }; window-open { off false; }; }")).animations;
    QCOMPARE(animations.windowResize.enabled, false);
    QCOMPARE(animations.windowResize.kind, defaultConfig().animations.windowResize.kind);
    QCOMPARE(animations.windowOpen, defaultConfig().animations.windowOpen);
    verifyFailure(
        QStringLiteral("animations { window-open { off; »off; }; }"), QStringLiteral("duplicate node `off`, single node expected"));
    verifyFailure(QStringLiteral("animations { window-open »1 {}; }"), QStringLiteral("no arguments expected for this node"));
}

void TestConfigNodes::curveKeywords_data()
{
    QTest::addColumn<QString>("name");
    QTest::addColumn<EasingCurve>("curve");

    QTest::newRow("linear") << QStringLiteral("linear") << EasingCurve::Linear;
    QTest::newRow("ease-out-quad") << QStringLiteral("ease-out-quad") << EasingCurve::EaseOutQuad;
    QTest::newRow("ease-out-cubic") << QStringLiteral("ease-out-cubic") << EasingCurve::EaseOutCubic;
    QTest::newRow("ease-out-expo") << QStringLiteral("ease-out-expo") << EasingCurve::EaseOutExpo;
}

void TestConfigNodes::curveKeywords()
{
    QFETCH(QString, name);
    QFETCH(EasingCurve, curve);
    QCOMPARE(
        easingOf(parsed(QStringLiteral("animations { window-open { curve \"%1\"; }; }").arg(name)).animations.windowOpen).curve, curve);
}

void TestConfigNodes::curveRejectsExtraArguments()
{
    verifyFailure(QStringLiteral("animations { window-open { curve \"linear\" »0.5; }; }"), QStringLiteral("unexpected argument"));
    verifyFailure(QStringLiteral("animations { window-open { »curve \"cubic-bezier\" 0 0 1 1 1; }; }"),
        QStringLiteral("cubic-bezier requires x1, y1, x2 and y2 control point coordinates"));
    verifyFailure(QStringLiteral("animations { window-open { curve »1; }; }"), QStringLiteral("expected a string"));
    verifyFailure(QStringLiteral("animations { window-open { curve \"linear\" »a=1; }; }"), QStringLiteral("unexpected property `a`"));
    verifyLoads(QStringLiteral("animations { window-open { curve \"cubic-bezier\" 0 -2 1 3; }; }"));
}

void TestConfigNodes::workspaceSwitchEdge()
{
    const DndEdgeScroll defaults = parsed(QString()).gestures.dndEdgeWorkspaceSwitch;
    QCOMPARE(defaults, (DndEdgeScroll {50, 100, 1500}));
    const DndEdgeScroll edge
        = parsed(QStringLiteral("gestures { dnd-edge-workspace-switch { delay-ms 250; }; }")).gestures.dndEdgeWorkspaceSwitch;
    QCOMPARE(edge, (DndEdgeScroll {50, 250, 1500}));
    QCOMPARE(parsed(QStringLiteral("gestures { dnd-edge-view-scroll { trigger-width 0; }; }")).gestures.dndEdgeViewScroll.triggerSize, 0.0);
}

void TestConfigNodes::edgeScrollTriggerNamesDiffer()
{
    verifyFailure(
        QStringLiteral("gestures { dnd-edge-workspace-switch { »trigger-width 5; }; }"), QStringLiteral("unexpected node `trigger-width`"));
    verifyFailure(
        QStringLiteral("gestures { dnd-edge-view-scroll { »trigger-height 5; }; }"), QStringLiteral("unexpected node `trigger-height`"));
    verifyFailure(QStringLiteral("gestures { dnd-edge-view-scroll »1 {}; }"), QStringLiteral("no arguments expected for this node"));
}

void TestConfigNodes::touchKeywords()
{
    const MultiTouch touch = parsed(QStringLiteral(R"(
        gestures {
            touchscreen {
                horizontal-swipe "off"
                vertical-swipe "switch-workspace"
                pinch "toggle-overview"
                window-horizontal-swipe "consume-or-expel"
                window-vertical-swipe "move-window"
            }
        }
    )"))
                                 .gestures.touchscreen;
    QCOMPARE(touch.horizontalSwipe, HorizontalSwipe::Off);
    QCOMPARE(touch.verticalSwipe, VerticalSwipe::SwitchWorkspace);
    QCOMPARE(touch.pinch, PinchAction::ToggleOverview);
    QCOMPARE(touch.windowHorizontalSwipe, WindowHorizontalSwipe::ConsumeOrExpel);
    QCOMPARE(touch.windowVerticalSwipe, WindowVerticalSwipe::MoveWindow);
    verifyFailure(QStringLiteral("gestures { touchpad { vertical-swipe »\"scroll-view\"; }; }"),
        QStringLiteral("expected one of `switch-workspace`, `off`"));
    verifyFailure(QStringLiteral("gestures { touchpad { five-finger-tap »\"close\"; }; }"),
        QStringLiteral("expected one of `cycle-width`, `kontrol-panel`, `toggle-overview`, `off`"));
    verifyFailure(QStringLiteral("gestures { touchpad { off; »off; }; }"), QStringLiteral("duplicate node `off`, single node expected"));
}

void TestConfigNodes::titlebarDrag()
{
    QCOMPARE(parsed(QString()).gestures.titlebarDrag, TitlebarDrag::ScrollView);
    QCOMPARE(parsed(QStringLiteral("gestures { titlebar-drag \"scroll-view\"; }")).gestures.titlebarDrag, TitlebarDrag::ScrollView);
    verifyFailure(QStringLiteral("gestures { titlebar-drag »true; }"), QStringLiteral("expected a string"));
}

void TestConfigNodes::tabIndicatorPositions()
{
    const QList<std::pair<QString, TabIndicatorPosition>> positions {{QStringLiteral("left"), TabIndicatorPosition::Left},
        {QStringLiteral("right"), TabIndicatorPosition::Right}, {QStringLiteral("top"), TabIndicatorPosition::Top},
        {QStringLiteral("bottom"), TabIndicatorPosition::Bottom}};
    for (const auto &[name, position] : positions) {
        QCOMPARE(parsed(QStringLiteral("layout { tab-indicator { position \"%1\"; }; }").arg(name)).layout.tabIndicator.position, position);
    }
    verifyFailure(QStringLiteral("layout { tab-indicator { position »\"center\"; }; }"),
        QStringLiteral("expected one of `left`, `right`, `top`, `bottom`"));
}

void TestConfigNodes::tabIndicatorMergesFieldByField()
{
    const Config config = parsed(QStringLiteral(R"(
        layout {
            tab-indicator {
                width 9
                gap -3
                position "top"
                length total-proportion=0.8
                gaps-between-tabs 2
                corner-radius 4
                hide-when-single-tab
                active-color "#ff0000"
                inactive-color "#00ff00"
            }
        }
        output "DP-1" {
            layout {
                tab-indicator {
                    off
                    corner-radius 1
                    place-within-column
                    active-gradient from="#000000" to="#ffffff"
                    urgent-color "hover"
                }
            }
        }
    )"));
    const TabIndicator &global = config.layout.tabIndicator;
    const TabIndicator &scoped = config.outputs.first().layout->tabIndicator;
    QCOMPARE(global.enabled, true);
    QCOMPARE(scoped.enabled, false);
    QCOMPARE(scoped.width, 9.0);
    QCOMPARE(scoped.gap, -3.0);
    QCOMPARE(scoped.position, TabIndicatorPosition::Top);
    QCOMPARE(scoped.lengthTotalProportion, 0.8);
    QCOMPARE(scoped.gapsBetweenTabs, 2.0);
    QCOMPARE(scoped.cornerRadius, 1.0);
    QCOMPARE(scoped.hideWhenSingleTab, true);
    QCOMPARE(scoped.placeWithinColumn, true);
    QCOMPARE(global.placeWithinColumn, false);
    QCOMPARE(scoped.active->color, QColor(255, 0, 0));
    QVERIFY(scoped.active->gradient.has_value());
    QVERIFY(!global.active->gradient.has_value());
    QCOMPARE(scoped.inactive->color, QColor(0, 255, 0));
    QCOMPARE(scoped.urgent->source, ColorSource::SystemHover);
    QCOMPARE(global.urgent, std::nullopt);
}

void TestConfigNodes::tabIndicatorLengthNeedsItsProperty()
{
    verifyFailure(QStringLiteral("layout { tab-indicator { »length; }; }"), QStringLiteral("property `total-proportion` is required"));
    verifyFailure(QStringLiteral("layout { tab-indicator { length »0.5; }; }"), QStringLiteral("no arguments expected for this node"));
    verifyFailure(QStringLiteral("layout { tab-indicator { length »total=0.5; }; }"), QStringLiteral("unexpected property `total`"));
    QCOMPARE(parsed(QString()).layout.tabIndicator.lengthTotalProportion, 0.5);
}

void TestConfigNodes::themeColorKeywords_data()
{
    QTest::addColumn<QString>("keyword");
    QTest::addColumn<ColorSource>("source");

    QTest::newRow("accent") << QStringLiteral("accent") << ColorSource::SystemAccent;
    QTest::newRow("focus") << QStringLiteral("focus") << ColorSource::SystemFocus;
    QTest::newRow("hover") << QStringLiteral("Hover") << ColorSource::SystemHover;
    QTest::newRow("window") << QStringLiteral("window") << ColorSource::SystemWindow;
    QTest::newRow("window-text") << QStringLiteral("WINDOW-TEXT") << ColorSource::SystemWindowText;
    QTest::newRow("inactive-text") << QStringLiteral(" inactive-text ") << ColorSource::SystemInactiveText;
}

void TestConfigNodes::themeColorKeywords()
{
    QFETCH(QString, keyword);
    QFETCH(ColorSource, source);
    const Config config = parsed(QStringLiteral(R"(
        layout {
            focus-ring { inactive-color "%1"; }
            border { urgent-color "%1"; }
            tab-indicator { active-color "%1"; }
            insert-hint { color "%1"; }
        }
        window-rule { focus-ring { active-color "%1"; }; }
    )")
            .arg(keyword));
    QCOMPARE(config.layout.focusRing.inactive.source, source);
    QCOMPARE(config.layout.border.urgent.source, source);
    QCOMPARE(config.layout.tabIndicator.active->source, source);
    QCOMPARE(config.layout.insertHint.paint.source, source);
    QCOMPARE(config.windowRules.first().focusRing.active->source, source);
}

void TestConfigNodes::themeColorsAreOnlyForDecorations()
{
    verifyFailure(QStringLiteral("layout { »background-color \"window\"; }"),
        QStringLiteral("`accent` is only supported for focus ring, border, tab indicator and insert hint colors"));
    verifyFailure(QStringLiteral("layout { focus-ring { active-gradient from=»\"accent\" to=\"red\"; }; }"),
        QStringLiteral("gradients need explicit colors, theme colors like `accent` are not supported"));
    verifyFailure(QStringLiteral("layout { border { active-gradient from=\"red\" to=»\"focus\"; }; }"),
        QStringLiteral("gradients need explicit colors, theme colors like `accent` are not supported"));
}

void TestConfigNodes::gradientsForInactiveAndInsertHint()
{
    const Config config = parsed(QStringLiteral(R"(
        layout {
            border { inactive-gradient from="#000000" to="#ffffff" angle=90; }
            insert-hint { gradient from="#ff0000" to="#0000ff" relative-to="workspace-view"; }
        }
    )"));
    QCOMPARE(config.layout.border.inactive.gradient->to, QColor(255, 255, 255));
    QCOMPARE(config.layout.border.inactive.gradient->angle, 90.0);
    QCOMPARE(config.layout.insertHint.paint.gradient->from, QColor(255, 0, 0));
    QCOMPARE(config.layout.insertHint.paint.gradient->relativeTo, GradientRelativeTo::WorkspaceView);
}

QTEST_MAIN(TestConfigNodes)
#include "test_config_nodes.moc"
