#include "actionhelpers.h"

using namespace LayoutTest;

namespace
{

Config::Config wrappedPinConfig(bool wrap, std::optional<Config::ColumnPosition> position, std::optional<int> openAt)
{
    Config::Config config = instantConfig();
    config.layout.wrapColumns = wrap;
    config.layout.centerFocusedColumn = Config::CenterFocusedColumn::OnOverflow;
    config.layout.defaultColumnWidth = Config::Proportion {0.9};
    Config::WindowRule rule = ruleFor(QStringLiteral("pinned"));
    rule.columnPosition = position;
    rule.openAtColumn = openAt;
    config.windowRules.append(rule);
    return config;
}

WindowIds order(Fixture &fixture)
{
    WindowIds ids;
    for (const WindowIds &column : columns(fixture)) {
        ids.append(column);
    }
    return ids;
}

struct ThreeColumns
{
    Layout::WindowId pinned;
    Layout::WindowId second;
    Layout::WindowId third;
};

ThreeColumns addThree(Fixture &fixture)
{
    ThreeColumns row;
    row.pinned = fixture.add(QStringLiteral("pinned"));
    row.second = fixture.add(QStringLiteral("a"));
    row.third = fixture.add(QStringLiteral("b"));
    return row;
}

}

class TestLayoutWrapPins : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void aStartPinnedColumnIsMovedPastWhenTheStripWraps()
    {
        Fixture fixture(wrappedPinConfig(true, Config::ColumnPosition::Start, std::nullopt));
        const ThreeColumns row = addThree(fixture);
        QVERIFY(fixture.perform(QStringLiteral("move-column-to-first")).ok);
        QCOMPARE(order(fixture), (WindowIds {row.third, row.pinned, row.second}));
        VERIFY_INVARIANTS(fixture);
    }

    void aStartPinnedColumnStaysFirstWithoutWrapping()
    {
        Fixture fixture(wrappedPinConfig(false, Config::ColumnPosition::Start, std::nullopt));
        const ThreeColumns row = addThree(fixture);
        QVERIFY(fixture.perform(QStringLiteral("move-column-to-first")).ok);
        QCOMPARE(order(fixture), (WindowIds {row.pinned, row.third, row.second}));
        VERIFY_INVARIANTS(fixture);
    }

    void anEndPinnedColumnIsMovedPastWhenTheStripWraps()
    {
        Fixture fixture(wrappedPinConfig(true, Config::ColumnPosition::End, std::nullopt));
        const Layout::WindowId pinned = fixture.add(QStringLiteral("pinned"));
        const Layout::WindowId second = fixture.add(QStringLiteral("a"));
        const Layout::WindowId third = fixture.add(QStringLiteral("b"));
        fixture.engine().activateWindow(pinned);
        fixture.settle();
        QVERIFY(fixture.perform(QStringLiteral("move-column-to-last")).ok);
        QCOMPARE(order(fixture), (WindowIds {second, third, pinned}));
        VERIFY_INVARIANTS(fixture);
    }

    void aPinnedColumnKeepsItsPlaceWhenOthersShuffleAroundIt()
    {
        Fixture fixture(wrappedPinConfig(true, Config::ColumnPosition::Start, std::nullopt));
        const ThreeColumns row = addThree(fixture);
        QVERIFY(fixture.perform(QStringLiteral("move-column-left")).ok);
        QVERIFY(fixture.perform(QStringLiteral("move-column-left")).ok);
        QCOMPARE(order(fixture), (WindowIds {row.third, row.pinned, row.second}));
        QVERIFY(fixture.perform(QStringLiteral("move-column-right")).ok);
        QCOMPARE(order(fixture), (WindowIds {row.pinned, row.third, row.second}));
        QVERIFY(fixture.perform(QStringLiteral("focus-column-left")).ok);
        QCOMPARE(order(fixture), (WindowIds {row.pinned, row.third, row.second}));
        VERIFY_INVARIANTS(fixture);
    }

    void openAtColumnIsIgnoredWhenTheStripWraps_data()
    {
        QTest::addColumn<bool>("wrap");
        QTest::addColumn<int>("expected");
        QTest::newRow("wrapping") << true << 3;
        QTest::newRow("not wrapping") << false << 0;
    }

    void openAtColumnIsIgnoredWhenTheStripWraps()
    {
        QFETCH(bool, wrap);
        QFETCH(int, expected);
        Fixture fixture(wrappedPinConfig(wrap, std::nullopt, 1));
        columnsOf(fixture, 3);
        const Layout::WindowId opened = fixture.add(QStringLiteral("pinned"));
        const Columns placed = columns(fixture);
        QCOMPARE(placed.size(), 4);
        QCOMPARE(placed.at(expected), WindowIds {opened});
        VERIFY_INVARIANTS(fixture);
    }

    void aPinnedColumnThatOpensAtTheFirstColumnOpensBesideTheFocusedOneWhenWrapping()
    {
        Fixture fixture(wrappedPinConfig(true, Config::ColumnPosition::Start, 1));
        const Layout::WindowId first = fixture.add(QStringLiteral("a"));
        const Layout::WindowId second = fixture.add(QStringLiteral("b"));
        const Layout::WindowId pinned = fixture.add(QStringLiteral("pinned"));
        QCOMPARE(order(fixture), (WindowIds {first, second, pinned}));
        VERIFY_INVARIANTS(fixture);
    }
    void aPinnedColumnJoinsTheReflowWhenStackingIsSwitchedOnWhileWrapping_data()
    {
        QTest::addColumn<bool>("wrap");
        QTest::addColumn<int>("columnsAfter");
        QTest::newRow("wrapping") << true << 2;
        QTest::newRow("not wrapping") << false << 3;
    }

    void aPinnedColumnJoinsTheReflowWhenStackingIsSwitchedOnWhileWrapping()
    {
        QFETCH(bool, wrap);
        QFETCH(int, columnsAfter);
        Config::Config config = wrappedPinConfig(wrap, Config::ColumnPosition::Start, std::nullopt);
        Fixture fixture(config);
        const Layout::WindowId pinned = fixture.add(QStringLiteral("pinned"));
        fixture.add(QStringLiteral("a"));
        fixture.add(QStringLiteral("b"));
        fixture.add(QStringLiteral("c"));
        QCOMPARE(columns(fixture).size(), 4);
        config.layout.newWindowPlacement = Config::NewWindowPlacement::Stack;
        config.layout.maxRowsPerColumn = 2;
        config.layout.groupAppWindows = Config::GroupAppWindows::Off;
        fixture.setConfig(config);
        fixture.settle();
        const Columns placed = columns(fixture);
        QCOMPARE(placed.size(), columnsAfter);
        QCOMPARE(placed.front().contains(pinned) && placed.front().size() > 1, wrap);
        VERIFY_INVARIANTS(fixture);
    }
};

QTEST_GUILESS_MAIN(TestLayoutWrapPins)
#include "test_layout_wrappins.moc"
