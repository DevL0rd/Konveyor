#include "configtesthelpers.h"

#include <QTest>

using namespace Konveyor::Config;
using namespace Konveyor::Config::Testing;

class TestConfigDefaults : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void emptyConfigEqualsDefaults();
    void defaultLayoutValues();
    void defaultAppearanceValues();
    void defaultAnimationValues();
    void defaultGestureAndInputValues();
    void mergedLayoutAppliesPart();
    void mergedLayoutUnsetsWithFalseFlag();
    void mergedLayoutRestoresEmptyPresets();
    void keysymLookup();
    void loadErrorFormatting();
};

void TestConfigDefaults::emptyConfigEqualsDefaults()
{
    QVERIFY(parsed(QString()) == defaultConfig());
    QVERIFY(parsed(QStringLiteral("\n// only a comment\n")) == defaultConfig());
}

void TestConfigDefaults::defaultLayoutValues()
{
    const Layout layout = defaultConfig().layout;
    QCOMPARE(layout.gaps, 16.0);
    QCOMPARE(layout.centerFocusedColumn, CenterFocusedColumn::Never);
    QCOMPARE(layout.alwaysCenterSingleColumn, false);
    QCOMPARE(layout.emptyWorkspaceAboveFirst, false);
    QCOMPARE(layout.defaultColumnDisplay, ColumnDisplay::Normal);
    QCOMPARE(layout.presetColumnWidths.size(), 4);
    QCOMPARE(proportionOf(layout.presetColumnWidths.at(0)), 0.25);
    QCOMPARE(proportionOf(layout.presetColumnWidths.at(1)), 0.5);
    QCOMPARE(proportionOf(layout.presetColumnWidths.at(2)), 0.75);
    QCOMPARE(proportionOf(layout.presetColumnWidths.at(3)), 1.0);
    QCOMPARE(layout.presetWindowHeights.size(), 4);
    QCOMPARE(proportionOf(layout.presetWindowHeights.at(1)), 1.0 / 3.0);
    QCOMPARE(proportionOf(layout.presetWindowHeights.at(3)), 2.0 / 3.0);
    QVERIFY(layout.defaultColumnWidth.has_value());
    QCOMPARE(proportionOf(*layout.defaultColumnWidth), 0.25);
    QCOMPARE(layout.rememberWindowSizes, false);
    QCOMPARE(layout.rememberWindowPositions, false);
    QCOMPARE(layout.groupAppWindows, GroupAppWindows::Beside);
    QCOMPARE(layout.maxRowsPerColumn, 3);
    QCOMPARE(layout.newWindowPlacement, NewWindowPlacement::Column);
    QCOMPARE(layout.floatChildWindows, false);
    QCOMPARE(layout.struts, Struts {});
    QCOMPARE(layout.backgroundColor, QColor(0x40, 0x40, 0x40));
}

void TestConfigDefaults::defaultAppearanceValues()
{
    const Layout layout = defaultConfig().layout;
    QCOMPARE(layout.focusRing.enabled, true);
    QCOMPARE(layout.focusRing.width, 4.0);
    QCOMPARE(layout.focusRing.active.color, QColor(127, 200, 255));
    QCOMPARE(layout.focusRing.inactive.color, QColor(80, 80, 80));
    QCOMPARE(layout.focusRing.urgent.color, QColor(155, 0, 0));
    QCOMPARE(layout.border.enabled, false);
    QCOMPARE(layout.border.width, 4.0);
    QCOMPARE(layout.border.active.color, QColor(255, 200, 127));
    QCOMPARE(layout.tabIndicator.enabled, true);
    QCOMPARE(layout.tabIndicator.gap, 5.0);
    QCOMPARE(layout.tabIndicator.width, 4.0);
    QCOMPARE(layout.tabIndicator.lengthTotalProportion, 0.5);
    QCOMPARE(layout.tabIndicator.position, TabIndicatorPosition::Left);
    QCOMPARE(layout.insertHint.enabled, true);
    QCOMPARE(layout.insertHint.paint.color, QColor(127, 200, 255, 128));
}

void TestConfigDefaults::defaultAnimationValues()
{
    const Animations animations = defaultConfig().animations;
    QCOMPARE(animations.enabled, true);
    QCOMPARE(animations.slowdown, 1.0);
    QCOMPARE(std::get<SpringParams>(animations.workspaceSwitch.kind), (SpringParams {1.0, 1000, 0.0001}));
    QCOMPARE(std::get<SpringParams>(animations.horizontalViewMovement.kind), (SpringParams {1.0, 800, 0.0001}));
    QCOMPARE(std::get<SpringParams>(animations.windowMovement.kind), (SpringParams {1.0, 800, 0.0001}));
    QCOMPARE(std::get<SpringParams>(animations.windowResize.kind), (SpringParams {1.0, 800, 0.0001}));
    const auto open = std::get<EasingParams>(animations.windowOpen.kind);
    QCOMPARE(open.durationMs, 150.0);
    QCOMPARE(open.curve, EasingCurve::EaseOutExpo);
}

void TestConfigDefaults::defaultGestureAndInputValues()
{
    const Config config = defaultConfig();
    QCOMPARE(config.gestures.dndEdgeViewScroll.triggerSize, 30.0);
    QCOMPARE(config.gestures.dndEdgeViewScroll.delayMs, 100.0);
    QCOMPARE(config.gestures.dndEdgeViewScroll.maxSpeed, 1500.0);
    QCOMPARE(config.gestures.dndEdgeWorkspaceSwitch.triggerSize, 50.0);
    QCOMPARE(config.gestures.hotCorners.enabled, true);
    QCOMPARE(config.gestures.hotCorners.topLeft, true);
    QCOMPARE(config.gestures.hotCorners.topRight, false);
    QCOMPARE(config.gestures.titlebarDrag, TitlebarDrag::ScrollView);
    QCOMPARE(config.gestures.resizeTiledWindows, false);
    QCOMPARE(config.input.focusFollowsMouse, false);
    QCOMPARE(config.input.warpMouseToFocus, false);
    QCOMPARE(config.input.workspaceAutoBackAndForth, false);
    QCOMPARE(config.input.modKey, QStringLiteral("Super"));
    QCOMPARE(config.configNotificationDisableFailed, false);
}

void TestConfigDefaults::mergedLayoutAppliesPart()
{
    LayoutPart part;
    part.gaps = 8;
    part.centerFocusedColumn = CenterFocusedColumn::OnOverflow;
    part.struts = Struts {1, 2, 3, 4};
    BorderRule border;
    border.enabled = true;
    border.width = 9;
    part.border = border;

    const Layout merged = mergedLayout(defaultConfig().layout, part);
    QCOMPARE(merged.gaps, 8.0);
    QCOMPARE(merged.centerFocusedColumn, CenterFocusedColumn::OnOverflow);
    QCOMPARE(merged.struts, (Struts {1, 2, 3, 4}));
    QCOMPARE(merged.border.enabled, true);
    QCOMPARE(merged.border.width, 9.0);
    QCOMPARE(merged.focusRing.width, 4.0);
}

void TestConfigDefaults::mergedLayoutUnsetsWithFalseFlag()
{
    Layout base = defaultConfig().layout;
    base.alwaysCenterSingleColumn = true;
    base.emptyWorkspaceAboveFirst = true;

    LayoutPart part;
    part.alwaysCenterSingleColumn = false;
    part.emptyWorkspaceAboveFirst = false;

    const Layout merged = mergedLayout(base, part);
    QCOMPARE(merged.alwaysCenterSingleColumn, false);
    QCOMPARE(merged.emptyWorkspaceAboveFirst, false);
}

void TestConfigDefaults::mergedLayoutRestoresEmptyPresets()
{
    LayoutPart part;
    part.presetColumnWidths = QList<PresetSize> {};
    part.presetWindowHeights = QList<PresetSize> {};

    const Layout merged = mergedLayout(defaultConfig().layout, part);
    QCOMPARE(merged.presetColumnWidths.size(), 4);
    QCOMPARE(merged.presetWindowHeights.size(), 4);
}

void TestConfigDefaults::keysymLookup()
{
    QCOMPARE(keysymFromName(QStringLiteral("a")), keysymFromName(QStringLiteral("A")));
    QVERIFY(keysymFromName(QStringLiteral("Escape")) != 0);
    QVERIFY(keysymFromName(QStringLiteral("Page_Down")) != 0);
    QVERIFY(keysymFromName(QStringLiteral("XF86AudioRaiseVolume")) != 0);
    QCOMPARE(keysymFromName(QStringLiteral("NotAKeyAtAll")), 0u);
}

void TestConfigDefaults::loadErrorFormatting()
{
    const LoadError error = mustFail(QStringLiteral("layout {\n    gaps \"wide\"\n}\n"));
    QCOMPARE(error.location.file, QStringLiteral("config.kdl"));
    QCOMPARE(error.location.line, 2);
    QCOMPARE(error.sourceLine, QStringLiteral("    gaps \"wide\""));
    const QString text = error.toString();
    QVERIFY(text.startsWith(QStringLiteral("config.kdl:2:")));
    QVERIFY(text.contains(error.message));
    QVERIFY(text.contains(QStringLiteral("    gaps \"wide\"")));
}

QTEST_MAIN(TestConfigDefaults)
#include "test_config_defaults.moc"
