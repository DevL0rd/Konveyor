#include "helpers.h"

using namespace LayoutTest;

Q_DECLARE_METATYPE(Config::TabIndicatorPosition)

namespace
{

Config::Config tabbedConfig()
{
    Config::Config config = instantConfig();
    config.layout.defaultColumnDisplay = Config::ColumnDisplay::Tabbed;
    return config;
}

QList<Layout::WindowId> addTabs(Fixture &fixture, int count)
{
    QList<Layout::WindowId> ids {fixture.add(QStringLiteral("tab"))};
    for (int i = 1; i < count; ++i) {
        ids.append(fixture.add(QStringLiteral("tab")));
        fixture.perform(QStringLiteral("consume-or-expel-window-left"));
    }
    return ids;
}

void verifySameFrame(Fixture &fixture, const QList<Layout::WindowId> &ids, QRectF frame)
{
    for (const Layout::WindowId id : ids) {
        QCOMPARE(fixture.frame(id), frame);
        QCOMPARE(fixture.state(id).columnIndex, 0);
    }
}

}

class TestLayoutTabbed : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void tabsShareOneFrameAndOnlyTheActiveOneShows()
    {
        Fixture fixture(tabbedConfig());
        const QList<Layout::WindowId> tabs = addTabs(fixture, 3);
        verifySameFrame(fixture, tabs, QRectF(16, 16, 936, 1048));
        QCOMPARE(fixture.focused(), std::optional(tabs[2]));
        QVERIFY(fixture.state(tabs[2]).visible);
        QVERIFY(!fixture.state(tabs[0]).visible);
        QVERIFY(!fixture.state(tabs[1]).visible);
        fixture.perform(QStringLiteral("focus-window-up"));
        QVERIFY(fixture.state(tabs[1]).visible);
        QVERIFY(!fixture.state(tabs[2]).visible);
        VERIFY_INVARIANTS(fixture);
    }

    void tabBarSitsBesideTheColumn()
    {
        Fixture fixture(tabbedConfig());
        const QList<Layout::WindowId> tabs = addTabs(fixture, 2);
        fixture.advance(1);
        const Layout::TabBarState bar = fixture.state(tabs[1]).tabBar;
        QVERIFY(bar.visible);
        QCOMPARE(bar.tabRects, (QList<QRectF> {QRectF(7, 278, 4, 262), QRectF(7, 540, 4, 262)}));
        QCOMPARE(fixture.state(tabs[0]).tabBar.visible, false);
    }

    void tabIndicatorPlacementReservesSpaceInsideTheColumn_data()
    {
        QTest::addColumn<Config::TabIndicatorPosition>("position");
        QTest::addColumn<QRectF>("frame");
        QTest::addColumn<QPointF>("nextOrigin");
        QTest::newRow("left") << Config::TabIndicatorPosition::Left << QRectF(25, 16, 927, 1048) << QPointF(977, 16);
        QTest::newRow("right") << Config::TabIndicatorPosition::Right << QRectF(16, 16, 927, 1048) << QPointF(968, 16);
        QTest::newRow("top") << Config::TabIndicatorPosition::Top << QRectF(16, 25, 936, 1039) << QPointF(968, 25);
        QTest::newRow("bottom") << Config::TabIndicatorPosition::Bottom << QRectF(16, 16, 936, 1039) << QPointF(968, 16);
    }

    void tabIndicatorPlacementReservesSpaceInsideTheColumn()
    {
        QFETCH(Config::TabIndicatorPosition, position);
        QFETCH(QRectF, frame);
        QFETCH(QPointF, nextOrigin);
        Config::Config config = tabbedConfig();
        config.layout.tabIndicator.placeWithinColumn = true;
        config.layout.tabIndicator.position = position;
        Fixture fixture(config);
        const QList<Layout::WindowId> tabs = addTabs(fixture, 2);
        verifySameFrame(fixture, tabs, frame);
        const auto next = fixture.add(QStringLiteral("next"));
        QCOMPARE(fixture.frame(next).topLeft(), nextOrigin);
        fixture.perform(QStringLiteral("focus-column-left"));
        fixture.advance(1);
        const Layout::TabBarState bar = fixture.state(tabs[1]).tabBar;
        QVERIFY(bar.visible);
        for (const QRectF &rect : bar.tabRects) {
            QVERIFY(QRectF(16, 16, 936, 1048).contains(rect));
            QVERIFY(!rect.intersects(frame));
        }
        VERIFY_INVARIANTS(fixture);
    }

    void placementChangesLive()
    {
        Config::Config config = tabbedConfig();
        Fixture fixture(config);
        const QList<Layout::WindowId> tabs = addTabs(fixture, 2);
        config.layout.tabIndicator.placeWithinColumn = true;
        fixture.setConfig(config);
        verifySameFrame(fixture, tabs, QRectF(25, 16, 927, 1048));
        config.layout.tabIndicator.gap = 10;
        config.layout.tabIndicator.width = 10;
        fixture.setConfig(config);
        verifySameFrame(fixture, tabs, QRectF(36, 16, 916, 1048));
        config.layout.tabIndicator.placeWithinColumn = false;
        fixture.setConfig(config);
        verifySameFrame(fixture, tabs, QRectF(16, 16, 936, 1048));
    }

    void hideWhenSingleTabReservesNothingForOneTab()
    {
        Config::Config config = tabbedConfig();
        config.layout.tabIndicator.placeWithinColumn = true;
        config.layout.tabIndicator.hideWhenSingleTab = true;
        Fixture fixture(config);
        const auto only = fixture.add(QStringLiteral("tab"));
        QCOMPARE(fixture.frame(only), QRectF(16, 16, 936, 1048));
        QVERIFY(!fixture.state(only).tabBar.visible);
        const auto second = fixture.add(QStringLiteral("tab"));
        fixture.perform(QStringLiteral("consume-or-expel-window-left"));
        verifySameFrame(fixture, {only, second}, QRectF(25, 16, 927, 1048));
        QVERIFY(fixture.state(second).tabBar.visible);
        fixture.remove(second);
        QCOMPARE(fixture.frame(only), QRectF(16, 16, 936, 1048));
        QVERIFY(!fixture.state(only).tabBar.visible);
    }

    void disabledTabIndicatorReservesNothing()
    {
        Config::Config config = tabbedConfig();
        config.layout.tabIndicator.placeWithinColumn = true;
        config.layout.tabIndicator.enabled = false;
        Fixture fixture(config);
        const QList<Layout::WindowId> tabs = addTabs(fixture, 2);
        verifySameFrame(fixture, tabs, QRectF(16, 16, 936, 1048));
        QVERIFY(!fixture.state(tabs[1]).tabBar.visible);
    }

    void heightPresetsApplyToEveryTab()
    {
        Fixture fixture(tabbedConfig());
        const QList<Layout::WindowId> tabs = addTabs(fixture, 2);
        fixture.perform(QStringLiteral("switch-preset-window-height"));
        verifySameFrame(fixture, tabs, QRectF(16, 16, 936, 250));
        fixture.perform(QStringLiteral("switch-preset-window-height"));
        verifySameFrame(fixture, tabs, QRectF(16, 16, 936, 339));
        fixture.perform(QStringLiteral("focus-window-up"));
        fixture.perform(QStringLiteral("set-window-height"), {QStringLiteral("600")});
        verifySameFrame(fixture, tabs, QRectF(16, 16, 936, 600));
        fixture.perform(QStringLiteral("reset-window-height"));
        verifySameFrame(fixture, tabs, QRectF(16, 16, 936, 1048));
        VERIFY_INVARIANTS(fixture);
    }

    void widthChangesApplyToEveryTab()
    {
        Fixture fixture(tabbedConfig());
        const QList<Layout::WindowId> tabs = addTabs(fixture, 2);
        fixture.perform(QStringLiteral("switch-preset-column-width"));
        verifySameFrame(fixture, tabs, QRectF(16, 16, 1253, 1048));
        fixture.perform(QStringLiteral("set-window-width"), {QStringLiteral("500")}, {{QStringLiteral("id"), QString::number(tabs[0])}});
        verifySameFrame(fixture, tabs, QRectF(16, 16, 500, 1048));
    }

    void stackedHeightsBecomeOneTabHeightAndComeBack()
    {
        Fixture fixture;
        const auto [top, bottom] = addStackedPair(fixture);
        fixture.perform(QStringLiteral("set-window-height"), {QStringLiteral("300")});
        fixture.perform(QStringLiteral("toggle-column-tabbed-display"));
        verifySameFrame(fixture, {top, bottom}, QRectF(16, 16, 936, 300));
        fixture.perform(QStringLiteral("toggle-column-tabbed-display"));
        QCOMPARE(fixture.frame(bottom).height(), 300.0);
        QCOMPARE(fixture.frame(top).height(), 732.0);
        VERIFY_INVARIANTS(fixture);
    }

    void consumeIntoATabbedColumnAddsAHiddenTab()
    {
        Fixture fixture(tabbedConfig());
        const auto first = fixture.add(QStringLiteral("a"));
        const auto second = fixture.add(QStringLiteral("b"));
        fixture.perform(QStringLiteral("focus-column-left"));
        fixture.perform(QStringLiteral("consume-window-into-column"));
        verifySameFrame(fixture, {first, second}, QRectF(16, 16, 936, 1048));
        QCOMPARE(fixture.state(second).tileIndex, 1);
        QCOMPARE(fixture.focused(), std::optional(first));
        QVERIFY(!fixture.state(second).visible);
        VERIFY_INVARIANTS(fixture);
    }

    void expelLeavesTheRestTabbed()
    {
        Fixture fixture(tabbedConfig());
        const QList<Layout::WindowId> tabs = addTabs(fixture, 3);
        fixture.perform(QStringLiteral("focus-window-top"));
        fixture.perform(QStringLiteral("expel-window-from-column"));
        QCOMPARE(fixture.state(tabs[2]).columnIndex, 1);
        QCOMPARE(fixture.focused(), std::optional(tabs[0]));
        QCOMPARE(fixture.frame(tabs[0]), fixture.frame(tabs[1]));
        QVERIFY(fixture.state(tabs[0]).visible);
        QVERIFY(!fixture.state(tabs[1]).visible);
        QVERIFY(fixture.state(tabs[2]).visible);
        fixture.perform(QStringLiteral("focus-column-right"));
        fixture.perform(QStringLiteral("consume-or-expel-window-left"));
        verifySameFrame(fixture, tabs, fixture.frame(tabs[0]));
        QCOMPARE(fixture.state(tabs[2]).tileIndex, 2);
        VERIFY_INVARIANTS(fixture);
    }

    void removingTheActiveTabShowsTheNextOne()
    {
        Fixture fixture(tabbedConfig());
        const QList<Layout::WindowId> tabs = addTabs(fixture, 3);
        fixture.perform(QStringLiteral("focus-window-up"));
        QCOMPARE(fixture.focused(), std::optional(tabs[1]));
        fixture.remove(tabs[1]);
        QCOMPARE(fixture.focused(), std::optional(tabs[2]));
        QVERIFY(fixture.state(tabs[2]).visible);
        QVERIFY(!fixture.state(tabs[0]).visible);
        fixture.remove(tabs[2]);
        QCOMPARE(fixture.focused(), std::optional(tabs[0]));
        QVERIFY(fixture.state(tabs[0]).visible);
        QCOMPARE(fixture.frame(tabs[0]), QRectF(16, 16, 936, 1048));
        VERIFY_INVARIANTS(fixture);
    }

    void fullscreenKeepsTheTabsTogether()
    {
        Fixture fixture(tabbedConfig());
        const QList<Layout::WindowId> tabs = addTabs(fixture, 2);
        fixture.perform(QStringLiteral("fullscreen-window"));
        QCOMPARE(fixture.state(tabs[1]).columnIndex, 0);
        QCOMPARE(fixture.frame(tabs[1]), QRectF(0, 0, 1920, 1080));
        QCOMPARE(fixture.state(tabs[1]).sizingMode, Layout::WindowMode::Fullscreen);
        QVERIFY(!fixture.state(tabs[1]).tabBar.visible);
        fixture.perform(QStringLiteral("fullscreen-window"));
        verifySameFrame(fixture, tabs, QRectF(16, 16, 936, 1048));
        QVERIFY(fixture.state(tabs[1]).tabBar.visible);
        VERIFY_INVARIANTS(fixture);
    }

    void fullscreenTabBarNeverReachesTheNeighbouringOutput()
    {
        Fixture fixture(tabbedConfig());
        fixture.addOutput(QStringLiteral("DP-2"), QRectF(1920, 0, 1920, 1080));
        fixture.engine().focusOutput(QStringLiteral("DP-2"));
        const QList<Layout::WindowId> tabs = addTabs(fixture, 2);
        QCOMPARE(fixture.state(tabs[1]).output, QStringLiteral("DP-2"));
        fixture.perform(QStringLiteral("fullscreen-window"));
        fixture.advance(1);
        QCOMPARE(fixture.frame(tabs[1]), QRectF(1920, 0, 1920, 1080));
        for (const QRectF &rect : fixture.state(tabs[1]).tabBar.tabRects) {
            QVERIFY(QRectF(1920, 0, 1920, 1080).contains(rect));
        }
        QVERIFY(!fixture.state(tabs[1]).tabBar.visible);
    }

    void maximizeToEdgesKeepsTheTabsTogether()
    {
        Fixture fixture(tabbedConfig());
        const QList<Layout::WindowId> tabs = addTabs(fixture, 2);
        fixture.perform(QStringLiteral("maximize-window-to-edges"));
        verifySameFrame(fixture, tabs, QRectF(0, 0, 1920, 1080));
        QVERIFY(!fixture.state(tabs[1]).tabBar.visible);
        fixture.perform(QStringLiteral("maximize-window-to-edges"));
        verifySameFrame(fixture, tabs, QRectF(16, 16, 936, 1048));
        VERIFY_INVARIANTS(fixture);
    }

    void maximizeColumnFillsTheWidthForEveryTab()
    {
        Fixture fixture(tabbedConfig());
        const QList<Layout::WindowId> tabs = addTabs(fixture, 2);
        fixture.perform(QStringLiteral("maximize-column"));
        verifySameFrame(fixture, tabs, QRectF(16, 16, 1888, 1048));
    }

    void dropOntoATabbedColumnAddsATab()
    {
        Fixture fixture(tabbedConfig());
        const QList<Layout::WindowId> tabs = addTabs(fixture, 2);
        const auto dragged = fixture.add(QStringLiteral("dragged"));
        QVERIFY(fixture.engine().beginWindowDrag(dragged, fixture.frame(dragged).center()));
        fixture.engine().updateWindowDrag(QPointF(480, 900), QStringLiteral("DP-1"));
        fixture.engine().endWindowDrag();
        fixture.settle();
        verifySameFrame(fixture, {tabs[0], tabs[1], dragged}, QRectF(16, 16, 936, 1048));
        QCOMPARE(fixture.focused(), std::optional(dragged));
        QVERIFY(fixture.state(dragged).visible);
        VERIFY_INVARIANTS(fixture);
    }
};

QTEST_GUILESS_MAIN(TestLayoutTabbed)
#include "test_layout_tabbed.moc"
