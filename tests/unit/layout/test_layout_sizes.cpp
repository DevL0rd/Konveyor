#include "helpers.h"

using namespace LayoutTest;

namespace
{

Config::Config borderedFixedPresetConfig()
{
    Config::Config config = instantConfig();
    config.layout.border.enabled = true;
    config.layout.border.width = 4;
    config.layout.presetColumnWidths = {Config::PresetSize(Config::Fixed {500}), Config::PresetSize(Config::Proportion {0.5})};
    return config;
}

void verifyHeights(Fixture &fixture, Layout::WindowId top, Layout::WindowId bottom, double topHeight, double bottomHeight)
{
    QCOMPARE(fixture.frame(top).height(), topHeight);
    QCOMPARE(fixture.frame(bottom).height(), bottomHeight);
    QCOMPARE(fixture.frame(bottom).y(), fixture.frame(top).bottom() + 16.0);
}

}

class TestLayoutSizes : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void switchPresetWindowWidthCyclesForwardAndBack()
    {
        Fixture fixture;
        const auto a = fixture.add(QStringLiteral("a"));
        const auto b = fixture.add(QStringLiteral("b"));
        QCOMPARE(fixture.state(b).widthPresetIndex, std::optional(1));
        fixture.perform(QStringLiteral("switch-preset-window-width"));
        QCOMPARE(fixture.frame(b).width(), 1253.0);
        QCOMPARE(fixture.state(b).widthPresetIndex, std::optional(2));
        fixture.perform(QStringLiteral("switch-preset-window-width"));
        QCOMPARE(fixture.frame(b).width(), 618.0);
        QCOMPARE(fixture.state(b).widthPresetIndex, std::optional(0));
        fixture.perform(QStringLiteral("switch-preset-window-width-back"));
        QCOMPARE(fixture.frame(b).width(), 1253.0);
        QCOMPARE(fixture.frame(a).width(), 936.0);
        VERIFY_INVARIANTS(fixture);
    }

    void switchPresetWindowWidthTargetsTheGivenWindow()
    {
        Fixture fixture;
        const auto a = fixture.add(QStringLiteral("a"));
        const auto b = fixture.add(QStringLiteral("b"));
        fixture.perform(QStringLiteral("switch-preset-window-width"), {}, {{QStringLiteral("id"), QString::number(a)}});
        QCOMPARE(fixture.frame(a).width(), 1253.0);
        QCOMPARE(fixture.frame(b).width(), 936.0);
        QCOMPARE(fixture.focused(), std::optional(b));
        VERIFY_INVARIANTS(fixture);
    }

    void switchPresetWindowHeightCyclesThroughThePresets()
    {
        Fixture fixture;
        const auto [top, bottom] = addStackedPair(fixture);
        verifyHeights(fixture, top, bottom, 516.0, 516.0);
        fixture.perform(QStringLiteral("switch-preset-window-height"));
        verifyHeights(fixture, top, bottom, 339.0, 693.0);
        fixture.perform(QStringLiteral("switch-preset-window-height"));
        verifyHeights(fixture, top, bottom, 782.0, 250.0);
        fixture.perform(QStringLiteral("switch-preset-window-height-back"));
        verifyHeights(fixture, top, bottom, 339.0, 693.0);
        VERIFY_INVARIANTS(fixture);
    }

    void switchPresetWindowHeightBackPicksTheNextSmallerPreset()
    {
        Fixture fixture;
        const auto [top, bottom] = addStackedPair(fixture);
        fixture.perform(QStringLiteral("switch-preset-window-height-back"));
        verifyHeights(fixture, top, bottom, 693.0, 339.0);
        fixture.perform(QStringLiteral("switch-preset-window-height-back"));
        verifyHeights(fixture, top, bottom, 782.0, 250.0);
        fixture.perform(QStringLiteral("switch-preset-window-height-back"));
        verifyHeights(fixture, top, bottom, 339.0, 693.0);
    }

    void heightPresetOnALoneWindowLeavesItShorterUntilReset()
    {
        Fixture fixture;
        const auto a = fixture.add(QStringLiteral("a"));
        QCOMPARE(fixture.frame(a).height(), 1048.0);
        fixture.perform(QStringLiteral("switch-preset-window-height"));
        QCOMPARE(fixture.frame(a), QRectF(16, 16, 936, 250));
        fixture.perform(QStringLiteral("reset-window-height"));
        QCOMPARE(fixture.frame(a), QRectF(16, 16, 936, 1048));
        VERIFY_INVARIANTS(fixture);
    }

    void presetWindowHeightsMixFixedAndProportion()
    {
        Config::Config config = instantConfig();
        config.layout.presetWindowHeights = {Config::PresetSize(Config::Fixed {400}), Config::PresetSize(Config::Proportion {0.75})};
        Fixture fixture(config);
        const auto [top, bottom] = addStackedPair(fixture);
        fixture.perform(QStringLiteral("switch-preset-window-height"));
        verifyHeights(fixture, top, bottom, 250.0, 782.0);
        fixture.perform(QStringLiteral("switch-preset-window-height"));
        verifyHeights(fixture, top, bottom, 632.0, 400.0);
    }

    void heightPresetsIncludeTheBorder()
    {
        Config::Config config = instantConfig();
        config.layout.border.enabled = true;
        config.layout.border.width = 4;
        config.layout.presetWindowHeights = {Config::PresetSize(Config::Fixed {400}), Config::PresetSize(Config::Proportion {0.5})};
        Fixture fixture(config);
        const auto [top, bottom] = addStackedPair(fixture);
        fixture.perform(QStringLiteral("switch-preset-window-height"));
        QCOMPARE(fixture.frame(bottom).height(), 400.0);
        QCOMPARE(fixture.frame(top).height(), 1032.0 - 408.0 - 8.0);
        fixture.perform(QStringLiteral("switch-preset-window-height"));
        QCOMPARE(fixture.frame(bottom).height(), 516.0 - 8.0);
        QCOMPARE(fixture.frame(top).height(), 516.0 - 8.0);
    }

    void setWindowHeightTakesProportionsFixedAndAdjustments()
    {
        Fixture fixture;
        const auto [top, bottom] = addStackedPair(fixture);
        fixture.perform(QStringLiteral("set-window-height"), {QStringLiteral("25%")});
        verifyHeights(fixture, top, bottom, 782.0, 250.0);
        fixture.perform(QStringLiteral("set-window-height"), {QStringLiteral("+100")});
        verifyHeights(fixture, top, bottom, 682.0, 350.0);
        fixture.perform(QStringLiteral("set-window-height"), {QStringLiteral("-10%")});
        verifyHeights(fixture, top, bottom, 788.0, 244.0);
        fixture.perform(QStringLiteral("set-window-height"), {QStringLiteral("5000")});
        verifyHeights(fixture, top, bottom, 1.0, 1031.0);
        VERIFY_INVARIANTS(fixture);
    }

    void setWindowHeightTargetsTheGivenWindow()
    {
        Fixture fixture;
        const auto [top, bottom] = addStackedPair(fixture);
        fixture.perform(QStringLiteral("set-window-height"), {QStringLiteral("300")}, {{QStringLiteral("id"), QString::number(top)}});
        verifyHeights(fixture, top, bottom, 300.0, 732.0);
        QCOMPARE(fixture.focused(), std::optional(bottom));
    }

    void setWindowWidthOnATiledWindowSetsItsColumn()
    {
        Fixture fixture;
        const auto a = fixture.add(QStringLiteral("a"));
        const auto b = fixture.add(QStringLiteral("b"));
        fixture.perform(QStringLiteral("set-window-width"), {QStringLiteral("700")});
        QCOMPARE(fixture.frame(b).width(), 700.0);
        QCOMPARE(fixture.state(b).widthPresetIndex, std::nullopt);
        fixture.perform(QStringLiteral("set-window-width"), {QStringLiteral("+10%")});
        QCOMPARE(fixture.frame(b).width(), 890.0);
        fixture.perform(QStringLiteral("set-window-width"), {QStringLiteral("-50")}, {{QStringLiteral("id"), QString::number(a)}});
        QCOMPARE(fixture.frame(a).width(), 886.0);
        QCOMPARE(fixture.frame(b).width(), 890.0);
        VERIFY_INVARIANTS(fixture);
    }

    void setWindowWidthResizesTheWholeStackedColumn()
    {
        Fixture fixture;
        const auto [top, bottom] = addStackedPair(fixture);
        fixture.perform(QStringLiteral("set-window-width"), {QStringLiteral("400")});
        QCOMPARE(fixture.frame(top).width(), 400.0);
        QCOMPARE(fixture.frame(bottom).width(), 400.0);
    }

    void livePresetChangeResetsTheWidthIndex()
    {
        Fixture fixture;
        const auto a = fixture.add(QStringLiteral("a"));
        fixture.perform(QStringLiteral("switch-preset-column-width"));
        QCOMPARE(fixture.frame(a).width(), 1253.0);
        Config::Config config = instantConfig();
        config.layout.presetColumnWidths = {Config::Proportion {0.25}, Config::Proportion {0.75}};
        fixture.setConfig(config);
        QCOMPARE(fixture.state(a).widthPresetIndex, std::nullopt);
        QCOMPARE(fixture.state(a).widthPresetCount, 2);
        QCOMPARE(fixture.frame(a).width(), 1253.0);
        fixture.perform(QStringLiteral("switch-preset-column-width"));
        QCOMPARE(fixture.frame(a).width(), 1412.0);
        QCOMPARE(fixture.state(a).widthPresetIndex, std::optional(1));
    }

    void livePresetChangeKeepsHeightsAndCyclesTheNewPresets()
    {
        Fixture fixture;
        const auto [top, bottom] = addStackedPair(fixture);
        fixture.perform(QStringLiteral("switch-preset-window-height"));
        verifyHeights(fixture, top, bottom, 339.0, 693.0);
        Config::Config config = instantConfig();
        config.layout.presetWindowHeights = {Config::Proportion {0.5}, Config::Proportion {0.75}};
        fixture.setConfig(config);
        verifyHeights(fixture, top, bottom, 339.0, 693.0);
        fixture.perform(QStringLiteral("switch-preset-window-height"));
        verifyHeights(fixture, top, bottom, 250.0, 782.0);
    }

    void emptyPresetListsFallBackToTheDefaults()
    {
        Config::Config config = instantConfig();
        config.layout.presetColumnWidths.clear();
        Fixture fixture(config);
        const auto a = fixture.add(QStringLiteral("a"));
        QCOMPARE(fixture.state(a).widthPresetCount, 4);
        QCOMPARE(fixture.state(a).widthPresetIndex, std::optional(1));
        fixture.perform(QStringLiteral("switch-preset-column-width"));
        QCOMPARE(fixture.frame(a).width(), 1412.0);
        fixture.perform(QStringLiteral("switch-preset-column-width"));
        QCOMPARE(fixture.frame(a).width(), 1888.0);
        fixture.perform(QStringLiteral("switch-preset-column-width"));
        QCOMPARE(fixture.frame(a).width(), 460.0);
    }

    void fixedPresetsAreWindowSizesAndProportionsAreTileSizes()
    {
        Config::Config config = borderedFixedPresetConfig();
        Fixture fixture(config);
        const auto a = fixture.add(QStringLiteral("a"));
        QCOMPARE(fixture.frame(a), QRectF(20, 20, 928, 1040));
        QCOMPARE(fixture.state(a).widthPresetIndex, std::optional(1));
        fixture.perform(QStringLiteral("switch-preset-column-width"));
        QCOMPARE(fixture.frame(a).width(), 500.0);
        fixture.perform(QStringLiteral("switch-preset-column-width"));
        QCOMPARE(fixture.frame(a).width(), 928.0);
    }

    void fixedDefaultWidthWithABorderKnowsItsPreset()
    {
        Config::Config config = borderedFixedPresetConfig();
        config.layout.defaultColumnWidth = Config::Fixed {500};
        Fixture fixture(config);
        const auto a = fixture.add(QStringLiteral("a"));
        QCOMPARE(fixture.frame(a).width(), 500.0);
        QCOMPARE(fixture.state(a).widthPresetIndex, std::optional(0));
        fixture.perform(QStringLiteral("switch-preset-column-width"));
        QCOMPARE(fixture.frame(a).width(), 928.0);
    }

    void focusRingWidthNeverChangesTheGeometry()
    {
        Config::Config config = instantConfig();
        config.layout.focusRing.enabled = true;
        config.layout.focusRing.width = 20;
        Fixture fixture(config);
        const auto a = fixture.add(QStringLiteral("a"));
        QCOMPARE(fixture.frame(a), QRectF(16, 16, 936, 1048));
        QVERIFY(fixture.state(a).focusRing.enabled);
        QCOMPARE(fixture.state(a).focusRing.width, 20.0);
    }

    void borderWidthShrinksTheWindowInsideItsTile()
    {
        Config::Config config = instantConfig();
        config.layout.border.enabled = true;
        config.layout.border.width = 10;
        Fixture fixture(config);
        const auto a = fixture.add(QStringLiteral("a"));
        const auto b = fixture.add(QStringLiteral("b"));
        QCOMPARE(fixture.frame(a), QRectF(26, 26, 916, 1028));
        QCOMPARE(fixture.frame(b).x(), 16.0 + 936.0 + 16.0 + 10.0);
        config.layout.border.width = 2;
        fixture.setConfig(config);
        QCOMPARE(fixture.frame(a), QRectF(18, 18, 932, 1044));
        config.layout.border.enabled = false;
        fixture.setConfig(config);
        QCOMPARE(fixture.frame(a), QRectF(16, 16, 936, 1048));
        VERIFY_INVARIANTS(fixture);
    }

    void gapsChangeLive()
    {
        Fixture fixture;
        const auto a = fixture.add(QStringLiteral("a"));
        const auto b = fixture.add(QStringLiteral("b"));
        Config::Config config = instantConfig();
        config.layout.gaps = 0;
        fixture.setConfig(config);
        QCOMPARE(fixture.frame(a), QRectF(0, 0, 960, 1080));
        QCOMPARE(fixture.frame(b), QRectF(960, 0, 960, 1080));
        config.layout.gaps = 40;
        fixture.setConfig(config);
        QCOMPARE(fixture.frame(a), QRectF(40, 40, 900, 1000));
        QCOMPARE(fixture.frame(b).x(), 980.0);
        VERIFY_INVARIANTS(fixture);
    }
};

QTEST_GUILESS_MAIN(TestLayoutSizes)
#include "test_layout_sizes.moc"
