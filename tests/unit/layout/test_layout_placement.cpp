#include "helpers.h"

using namespace LayoutTest;

namespace
{

Config::Config stackConfig(int maxRows)
{
    Config::Config config = instantConfig();
    config.layout.groupAppWindows = Config::GroupAppWindows::Off;
    config.layout.newWindowPlacement = Config::NewWindowPlacement::Stack;
    config.layout.maxRowsPerColumn = maxRows;
    return config;
}

Config::Config portraitConfig()
{
    Config::Config config = instantConfig();
    Config::MonitorProfile profile;
    profile.name = QStringLiteral("portrait");
    Config::MonitorMatch match;
    match.aspectRatioBelow = 1.0;
    profile.matches.append(match);
    Config::Layout layout = config.layout;
    layout.defaultColumnWidth = Config::Proportion {1.0};
    layout.newWindowPlacement = Config::NewWindowPlacement::Stack;
    layout.maxRowsPerColumn = 2;
    profile.layout = layout;
    config.monitorProfiles.append(profile);
    return config;
}

void rotate(Fixture &fixture, bool portrait)
{
    const QRectF geometry = portrait ? QRectF(0, 0, 1080, 1920) : QRectF(0, 0, 1920, 1080);
    fixture.engine().updateOutput(makeOutput(QStringLiteral("DP-1"), geometry));
    fixture.settle();
}

}

class TestLayoutPlacement : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void stackFillsTheFocusedColumnThenOpensANewOne()
    {
        Fixture fixture(stackConfig(2));
        const auto browser = fixture.add(QStringLiteral("browser"));
        const auto editor = fixture.add(QStringLiteral("editor"));
        const auto chat = fixture.add(QStringLiteral("chat"));
        QCOMPARE(fixture.state(browser).columnIndex, 0);
        QCOMPARE(fixture.state(editor).columnIndex, 0);
        QCOMPARE(fixture.state(chat).columnIndex, 1);
        QCOMPARE(fixture.focused(), std::optional(chat));
        VERIFY_INVARIANTS(fixture);
    }

    void stackedRowsShareTheHeightEvenly()
    {
        Fixture fixture(stackConfig(2));
        const auto top = fixture.add(QStringLiteral("browser"));
        const auto bottom = fixture.add(QStringLiteral("editor"));
        QCOMPARE(fixture.state(top).columnIndex, fixture.state(bottom).columnIndex);
        QVERIFY(qAbs(fixture.frame(top).height() - fixture.frame(bottom).height()) <= 1.0);
    }

    void columnPlacementKeepsOneWindowPerColumn()
    {
        Config::Config config = stackConfig(2);
        config.layout.newWindowPlacement = Config::NewWindowPlacement::Column;
        Fixture fixture(config);
        const auto first = fixture.add(QStringLiteral("browser"));
        const auto second = fixture.add(QStringLiteral("editor"));
        QVERIFY(fixture.state(first).columnIndex != fixture.state(second).columnIndex);
    }

    void portraitProfileStacksTwoRowsOfFullWidthColumns()
    {
        Fixture fixture(portraitConfig(), QRectF(0, 0, 1080, 1920));
        const auto first = fixture.add(QStringLiteral("browser"));
        const auto second = fixture.add(QStringLiteral("editor"));
        const auto third = fixture.add(QStringLiteral("chat"));
        QCOMPARE(fixture.state(first).columnIndex, 0);
        QCOMPARE(fixture.state(second).columnIndex, 0);
        QCOMPARE(fixture.state(third).columnIndex, 1);
        QVERIFY(fixture.frame(first).width() > 1080 * 0.9);
        QVERIFY(fixture.frame(third).width() > 1080 * 0.9);
        VERIFY_INVARIANTS(fixture);
    }

    void portraitProfileLeavesLandscapeMonitorsAlone()
    {
        Fixture fixture(portraitConfig(), QRectF(0, 0, 1920, 1080));
        const auto first = fixture.add(QStringLiteral("browser"));
        const auto second = fixture.add(QStringLiteral("editor"));
        QCOMPARE(fixture.state(first).columnIndex, 0);
        QCOMPARE(fixture.state(second).columnIndex, 1);
    }

    void windowRulesOverrideThePlacement()
    {
        Config::Config config = stackConfig(2);
        Config::WindowRule loner = ruleFor(QStringLiteral("player"));
        loner.newWindowPlacement = Config::NewWindowPlacement::Column;
        config.windowRules.append(loner);
        Fixture fixture(config);
        const auto browser = fixture.add(QStringLiteral("browser"));
        const auto player = fixture.add(QStringLiteral("player"));
        QVERIFY(fixture.state(browser).columnIndex != fixture.state(player).columnIndex);
    }

    void stackPlacementFillsTheAppsColumnWhenGroupingBeside()
    {
        Config::Config config = stackConfig(2);
        config.layout.groupAppWindows = Config::GroupAppWindows::Beside;
        Config::WindowRule browserRule = ruleFor(QStringLiteral("browser"));
        browserRule.newWindowPlacement = Config::NewWindowPlacement::Column;
        config.windowRules.append(browserRule);
        Fixture fixture(config);
        const auto term = fixture.add(QStringLiteral("term"));
        const auto browser = fixture.add(QStringLiteral("browser"));
        fixture.engine().activateWindow(browser);
        fixture.settle();
        QVERIFY(fixture.state(browser).columnIndex != fixture.state(term).columnIndex);
        const auto secondTerm = fixture.add(QStringLiteral("term"));
        QCOMPARE(fixture.state(secondTerm).columnIndex, fixture.state(term).columnIndex);
        VERIFY_INVARIANTS(fixture);
    }

    void rotatingToPortraitStacksExistingWindows()
    {
        Fixture fixture(portraitConfig());
        std::vector<Layout::WindowId> ids;
        for (int idx = 0; idx < 5; ++idx) {
            ids.push_back(fixture.add(QStringLiteral("app%1").arg(idx)));
        }
        fixture.engine().activateWindow(ids[2]);
        fixture.settle();
        rotate(fixture, true);
        const std::vector<int> expected {0, 0, 1, 1, 2};
        for (std::size_t idx = 0; idx < ids.size(); ++idx) {
            QCOMPARE(fixture.state(ids[idx]).columnIndex, expected[idx]);
            QVERIFY(fixture.frame(ids[idx]).width() > 1080 * 0.9);
        }
        QCOMPARE(fixture.focused(), std::optional(ids[2]));
        VERIFY_INVARIANTS(fixture);
    }

    void rotatingBackUnstacksAndRestoresWidths()
    {
        Fixture fixture(portraitConfig());
        const auto first = fixture.add(QStringLiteral("browser"));
        const auto second = fixture.add(QStringLiteral("editor"));
        const double landscapeWidth = fixture.frame(first).width();
        rotate(fixture, true);
        QCOMPARE(fixture.state(second).columnIndex, 0);
        rotate(fixture, false);
        QCOMPARE(fixture.state(first).columnIndex, 0);
        QCOMPARE(fixture.state(second).columnIndex, 1);
        QCOMPARE(fixture.frame(first).width(), landscapeWidth);
        QCOMPARE(fixture.frame(second).width(), landscapeWidth);
        QCOMPARE(fixture.focused(), std::optional(second));
        VERIFY_INVARIANTS(fixture);
    }

    void portraitStacksUnstackWhenRotatedBack()
    {
        Fixture fixture(portraitConfig(), QRectF(0, 0, 1080, 1920));
        const auto first = fixture.add(QStringLiteral("browser"));
        const auto second = fixture.add(QStringLiteral("editor"));
        QCOMPARE(fixture.state(second).columnIndex, 0);
        rotate(fixture, false);
        QVERIFY(fixture.state(first).columnIndex != fixture.state(second).columnIndex);
        QVERIFY(fixture.frame(second).width() < 1920 * 0.5);
        VERIFY_INVARIANTS(fixture);
    }

    void addingThePortraitProfileRestacksARotatedMonitor()
    {
        Fixture fixture(instantConfig(), QRectF(0, 0, 1080, 1920));
        const auto first = fixture.add(QStringLiteral("browser"));
        const auto second = fixture.add(QStringLiteral("editor"));
        QVERIFY(fixture.state(first).columnIndex != fixture.state(second).columnIndex);
        fixture.setConfig(portraitConfig());
        fixture.settle();
        QCOMPARE(fixture.state(second).columnIndex, fixture.state(first).columnIndex);
        QVERIFY(fixture.frame(first).width() > 1080 * 0.9);
        QCOMPARE(fixture.focused(), std::optional(second));
        VERIFY_INVARIANTS(fixture);
    }

    void manualStacksSurviveRotation()
    {
        Fixture fixture(portraitConfig());
        const auto first = fixture.add(QStringLiteral("browser"));
        const auto second = fixture.add(QStringLiteral("editor"));
        const auto third = fixture.add(QStringLiteral("chat"));
        QVERIFY(fixture.perform(QStringLiteral("consume-or-expel-window-left")).ok);
        rotate(fixture, true);
        rotate(fixture, false);
        QCOMPARE(fixture.state(first).columnIndex, 0);
        QCOMPARE(fixture.state(second).columnIndex, 1);
        QCOMPARE(fixture.state(third).columnIndex, 1);
        VERIFY_INVARIANTS(fixture);
    }

    void resizedColumnsKeepTheirWidthWhenRotated()
    {
        Config::Config config = portraitConfig();
        config.layout.presetColumnWidths = {Config::PresetSize(Config::Fixed {700})};
        config.monitorProfiles[0].layout->presetColumnWidths = config.layout.presetColumnWidths;
        Config::WindowRule editorRule = ruleFor(QStringLiteral("editor"));
        editorRule.newWindowPlacement = Config::NewWindowPlacement::Column;
        config.windowRules.append(editorRule);
        Fixture fixture(config);
        const auto resized = fixture.add(QStringLiteral("browser"));
        fixture.perform(QStringLiteral("switch-preset-column-width"));
        const auto untouched = fixture.add(QStringLiteral("editor"));
        rotate(fixture, true);
        QCOMPARE(fixture.frame(resized).width(), 700.0);
        QVERIFY(fixture.frame(untouched).width() > 1080 * 0.9);
    }
};

QTEST_GUILESS_MAIN(TestLayoutPlacement)
#include "test_layout_placement.moc"
