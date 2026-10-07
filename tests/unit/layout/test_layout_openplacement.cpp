#include "actionhelpers.h"

using namespace LayoutTest;

class TestLayoutOpenPlacement : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void openOnWorkspaceByIndex()
    {
        Config::Config config = instantConfig();
        Config::WindowRule rule = ruleFor(QStringLiteral("chat"));
        rule.openOnWorkspaceIndex = 2;
        config.windowRules.append(rule);
        Fixture fixture(config);
        fixture.add(QStringLiteral("editor"));
        const auto id = fixture.add(QStringLiteral("chat"));
        QCOMPARE(fixture.state(id).workspaceIndex, 2);
        QCOMPARE(activeWorkspaceOn(fixture, QStringLiteral("DP-1")), 2);
        VERIFY_INVARIANTS(fixture);
    }

    void openOnWorkspaceByIndexStopsAtTheLastWorkspace()
    {
        Config::Config config = instantConfig();
        Config::WindowRule rule = ruleFor(QStringLiteral("chat"));
        rule.openOnWorkspaceIndex = 9;
        config.windowRules.append(rule);
        Fixture fixture(config);
        fixture.add(QStringLiteral("editor"));
        const auto id = fixture.add(QStringLiteral("chat"));
        QCOMPARE(fixture.state(id).workspaceIndex, 2);
        VERIFY_INVARIANTS(fixture);
    }

    void openAtColumn_data()
    {
        QTest::addColumn<int>("column");
        QTest::addColumn<int>("expected");
        QTest::newRow("first") << 1 << 0;
        QTest::newRow("second") << 2 << 1;
        QTest::newRow("past the end") << 9 << 3;
    }

    void openAtColumn()
    {
        QFETCH(int, column);
        QFETCH(int, expected);
        Config::Config config = instantConfig();
        Config::WindowRule rule = ruleFor(QStringLiteral("player"));
        rule.openAtColumn = column;
        config.windowRules.append(rule);
        Fixture fixture(config);
        columnsOf(fixture, 3);
        fixture.perform(QStringLiteral("focus-column"), {QStringLiteral("2")});
        const auto player = fixture.add(QStringLiteral("player"));
        const Columns placed = columns(fixture);
        QCOMPARE(placed.size(), 4);
        QCOMPARE(placed.at(expected), WindowIds {player});
        VERIFY_INVARIANTS(fixture);
    }

    void openAtColumnLeavesFloatingWindowsAlone()
    {
        Config::Config config = instantConfig();
        Config::WindowRule rule = ruleFor(QStringLiteral("player"));
        rule.openAtColumn = 1;
        rule.openFloating = true;
        config.windowRules.append(rule);
        Fixture fixture(config);
        columnsOf(fixture, 2);
        const auto player = fixture.add(QStringLiteral("player"));
        QVERIFY(fixture.state(player).isFloating);
        QCOMPARE(columns(fixture).size(), 2);
        VERIFY_INVARIANTS(fixture);
    }

    void outputMatchesTheMonitorTheWindowOpensOn()
    {
        Config::Config config = instantConfig();
        Config::WindowRule rule = ruleFor(QStringLiteral("term"));
        rule.matches.first().output = QRegularExpression(QStringLiteral("^DP-2$"));
        rule.defaultColumnWidth = std::optional<Config::PresetSize>(Config::Fixed {500});
        config.windowRules.append(rule);
        Fixture fixture(config);
        fixture.addOutput(QStringLiteral("DP-2"), QRectF(1920, 0, 1920, 1080));
        const auto there = addOn(fixture, QStringLiteral("DP-2"), QStringLiteral("term"));
        const auto here = addOn(fixture, QStringLiteral("DP-1"), QStringLiteral("term"));
        QCOMPARE(fixture.frame(there).width(), 500.0);
        QVERIFY(fixture.frame(here).width() != 500.0);
        VERIFY_INVARIANTS(fixture);
    }

    void openOnAllWorkspacesIsReported()
    {
        Config::Config config = instantConfig();
        Config::WindowRule rule = ruleFor(QStringLiteral("player"));
        rule.openOnAllWorkspaces = true;
        config.windowRules.append(rule);
        Fixture fixture(config);
        const auto player = fixture.add(QStringLiteral("player"));
        const auto other = fixture.add(QStringLiteral("other"));
        QVERIFY(fixture.state(player).wantsAllDesktops);
        QVERIFY(!fixture.state(other).wantsAllDesktops);
    }
};

QTEST_GUILESS_MAIN(TestLayoutOpenPlacement)

#include "test_layout_openplacement.moc"
