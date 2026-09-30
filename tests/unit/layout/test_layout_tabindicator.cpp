#include "tabhelpers.h"

using namespace LayoutTest;

namespace
{

const QRectF Screen(0, 0, 1920, 1080);

}

class TestLayoutTabIndicator : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void theShownTabIsStackedAboveTheOtherTabs()
    {
        Fixture fixture(tabbedConfig(Config::TabIndicatorPosition::Left, 4, 5));
        const QList<Layout::WindowId> tabs = addTabs(fixture, 3);
        QCOMPARE(topmostTab(fixture, tabs), 2);
        fixture.perform(QStringLiteral("focus-window-up"));
        QCOMPARE(topmostTab(fixture, tabs), 1);
        fixture.perform(QStringLiteral("focus-window-top"));
        QCOMPARE(topmostTab(fixture, tabs), 0);
        QVERIFY(fixture.state(tabs[0]).visible);
        fixture.perform(QStringLiteral("focus-window-bottom"));
        QCOMPARE(topmostTab(fixture, tabs), 2);
    }

    void onlyTheShownTabOfAFullscreenColumnStacksAsFullscreen()
    {
        Fixture fixture(tabbedConfig(Config::TabIndicatorPosition::Left, 4, 5));
        const QList<Layout::WindowId> tabs = addTabs(fixture, 3);
        const auto other = addPlainColumn(fixture, QStringLiteral("other"));
        fixture.perform(QStringLiteral("focus-column-left"));
        fixture.perform(QStringLiteral("fullscreen-window"));
        QCOMPARE(topmostTab(fixture, tabs), 2);
        QVERIFY(fixture.state(tabs[2]).stackingIndex > fixture.state(other).stackingIndex);
        QVERIFY(fixture.state(tabs[0]).stackingIndex < fixture.state(other).stackingIndex);
        fixture.perform(QStringLiteral("focus-window-up"));
        QCOMPARE(topmostTab(fixture, tabs), 1);
        QVERIFY(fixture.state(tabs[1]).stackingIndex > fixture.state(other).stackingIndex);
        QVERIFY(fixture.state(tabs[2]).stackingIndex < fixture.state(other).stackingIndex);
    }

    void clickingATabRaisesIt()
    {
        Fixture fixture(tabbedConfig(Config::TabIndicatorPosition::Left, 4, 5));
        const QList<Layout::WindowId> tabs = addTabs(fixture, 3);
        fixture.advance(1);
        const QList<QRectF> rects = fixture.state(tabs[2]).tabBar.tabRects;
        const auto clicked = fixture.engine().windowAt(rects[0].center());
        QCOMPARE(clicked, std::optional(tabs[0]));
        fixture.engine().activateWindow(*clicked);
        fixture.settle();
        QCOMPARE(topmostTab(fixture, tabs), 0);
    }

    void hitsTellTabsFromWindows()
    {
        Fixture fixture(tabbedConfig(Config::TabIndicatorPosition::Left, 4, -2));
        const QList<Layout::WindowId> tabs = addTabs(fixture, 2);
        fixture.advance(1);
        const QList<QRectF> rects = fixture.state(tabs[1]).tabBar.tabRects;
        QCOMPARE(fixture.engine().hitAt(rects[0].center()), std::optional(Layout::WindowHit {tabs[0], true}));
        QCOMPARE(fixture.engine().hitAt(rects[1].center()), std::optional(Layout::WindowHit {tabs[1], true}));
        QVERIFY(fixture.frame(tabs[1]).contains(rects[1].center()));
        QCOMPARE(fixture.engine().hitAt(QPointF(400, 500)), std::optional(Layout::WindowHit {tabs[1], false}));
        QCOMPARE(fixture.engine().hitAt(QPointF(400, 100)), std::optional(Layout::WindowHit {tabs[1], false}));
        QCOMPARE(fixture.engine().hitAt(QPointF(1900, 1070)), std::nullopt);
    }

    void aFloatingWindowOverATabTakesTheHit()
    {
        Fixture fixture(tabbedConfig(Config::TabIndicatorPosition::Left, 4, 5));
        const QList<Layout::WindowId> tabs = addTabs(fixture, 2);
        fixture.advance(1);
        const QPointF onTab = fixture.state(tabs[1]).tabBar.tabRects[0].center();
        const auto floating = fixture.add(QStringLiteral("floating"), QSizeF(300, 300));
        fixture.perform(QStringLiteral("toggle-window-floating"));
        fixture.engine().setFloatingFrame(floating, QRectF(onTab.x() - 150, onTab.y() - 150, 300, 300));
        fixture.settle();
        QVERIFY(fixture.frame(floating).contains(onTab));
        QCOMPARE(fixture.engine().hitAt(onTab), std::optional(Layout::WindowHit {floating, false}));
    }

    void outsideTabsKeepTheirDistanceFromEverything_data()
    {
        QTest::addColumn<Config::TabIndicatorPosition>("position");
        QTest::addColumn<double>("width");
        QTest::addColumn<double>("gap");
        const QList<std::pair<const char *, Config::TabIndicatorPosition>> positions {{"left", Config::TabIndicatorPosition::Left},
            {"right", Config::TabIndicatorPosition::Right}, {"top", Config::TabIndicatorPosition::Top},
            {"bottom", Config::TabIndicatorPosition::Bottom}};
        for (const auto &[name, position] : positions) {
            for (const double width : {1.0, 4.0, 12.0, 32.0, 100.0}) {
                for (const double gap : {0.0, 5.0, 20.0}) {
                    QTest::addRow("%s width %g gap %g", name, width, gap) << position << width << gap;
                }
            }
        }
    }

    void outsideTabsKeepTheirDistanceFromEverything()
    {
        QFETCH(Config::TabIndicatorPosition, position);
        QFETCH(double, width);
        QFETCH(double, gap);
        Fixture fixture(tabbedConfig(position, width, gap));
        const auto before = fixture.add(QStringLiteral("before"));
        fixture.perform(QStringLiteral("toggle-column-tabbed-display"));
        const QList<Layout::WindowId> tabs = addTabs(fixture, 2);
        const auto after = fixture.add(QStringLiteral("after"));
        fixture.perform(QStringLiteral("toggle-column-tabbed-display"));
        fixture.perform(QStringLiteral("focus-column-left"));
        fixture.advance(1);
        const Layout::TabBarState bar = fixture.state(tabs[1]).tabBar;
        QVERIFY(bar.visible);
        const QRectF window = fixture.frame(tabs[1]);
        for (const QRectF &rect : bar.tabRects) {
            QVERIFY2(Screen.contains(rect), qPrintable(QStringLiteral("tab %1 leaves the output").arg(QDebug::toString(rect))));
            QCOMPARE(distance(rect, window), gap);
            QVERIFY(!rect.intersects(window));
            for (const Layout::WindowId other : {before, after}) {
                const QRectF otherFrame = fixture.frame(other);
                if (beside(rect, otherFrame, position)) {
                    QVERIFY2(distance(rect, otherFrame) >= gap,
                        qPrintable(QStringLiteral("tab %1 is closer than %2 to %3")
                                .arg(QDebug::toString(rect))
                                .arg(gap)
                                .arg(QDebug::toString(otherFrame))));
                }
            }
        }
        VERIFY_INVARIANTS(fixture);
    }

    void outsideTabsThatFitInTheGapMoveNothing()
    {
        Fixture fixture(tabbedConfig(Config::TabIndicatorPosition::Left, 4, 5));
        const QList<Layout::WindowId> tabs = addTabs(fixture, 2);
        QCOMPARE(fixture.frame(tabs[1]), QRectF(16, 16, 936, 1048));
    }

    void thickOutsideTabsMoveTheWindowAndItsNeighbours_data()
    {
        QTest::addColumn<Config::TabIndicatorPosition>("position");
        QTest::addColumn<QRectF>("frame");
        QTest::addColumn<QPointF>("nextOrigin");
        QTest::newRow("left") << Config::TabIndicatorPosition::Left << QRectF(42, 16, 910, 1048) << QPointF(968, 16);
        QTest::newRow("right") << Config::TabIndicatorPosition::Right << QRectF(16, 16, 910, 1048) << QPointF(968, 16);
        QTest::newRow("top") << Config::TabIndicatorPosition::Top << QRectF(16, 42, 936, 1022) << QPointF(968, 16);
        QTest::newRow("bottom") << Config::TabIndicatorPosition::Bottom << QRectF(16, 16, 936, 1022) << QPointF(968, 16);
    }

    void thickOutsideTabsMoveTheWindowAndItsNeighbours()
    {
        QFETCH(Config::TabIndicatorPosition, position);
        QFETCH(QRectF, frame);
        QFETCH(QPointF, nextOrigin);
        Fixture fixture(tabbedConfig(position, 32, 5));
        const QList<Layout::WindowId> tabs = addTabs(fixture, 2);
        QCOMPARE(fixture.frame(tabs[0]), frame);
        QCOMPARE(fixture.frame(tabs[1]), frame);
        fixture.perform(QStringLiteral("toggle-column-tabbed-display"));
        QCOMPARE(fixture.frame(tabs[0]).height() + fixture.frame(tabs[1]).height(), 1048.0 - 16.0);
        fixture.perform(QStringLiteral("toggle-column-tabbed-display"));
        const auto next = fixture.add(QStringLiteral("next"));
        fixture.perform(QStringLiteral("toggle-column-tabbed-display"));
        QCOMPARE(fixture.frame(next).topLeft(), nextOrigin);
        VERIFY_INVARIANTS(fixture);
    }

    void thicknessChangesMoveTheWindowLive()
    {
        Config::Config config = tabbedConfig(Config::TabIndicatorPosition::Left, 4, 5);
        Fixture fixture(config);
        const QList<Layout::WindowId> tabs = addTabs(fixture, 2);
        QCOMPARE(fixture.frame(tabs[1]), QRectF(16, 16, 936, 1048));
        config.layout.tabIndicator.width = 32;
        fixture.setConfig(config);
        QCOMPARE(fixture.frame(tabs[1]), QRectF(42, 16, 910, 1048));
        config.layout.gaps = 50;
        fixture.setConfig(config);
        QCOMPARE(fixture.frame(tabs[1]), QRectF(50, 50, 885, 980));
        config.layout.tabIndicator.enabled = false;
        fixture.setConfig(config);
        QCOMPARE(fixture.frame(tabs[1]), QRectF(50, 50, 885, 980));
        config.layout.gaps = 16;
        config.layout.tabIndicator.enabled = true;
        config.layout.tabIndicator.hideWhenSingleTab = true;
        fixture.setConfig(config);
        QCOMPARE(fixture.frame(tabs[1]), QRectF(42, 16, 910, 1048));
        fixture.remove(tabs[0]);
        QCOMPARE(fixture.frame(tabs[1]), QRectF(16, 16, 936, 1048));
    }

    void roundnessRoundsEveryTab_data()
    {
        QTest::addColumn<double>("radius");
        QTest::addColumn<double>("expected");
        QTest::newRow("square") << 0.0 << 0.0;
        QTest::newRow("small") << 2.0 << 2.0;
        QTest::newRow("half the thickness") << 5.0 << 5.0;
        QTest::newRow("more than the thickness allows") << 40.0 << 5.0;
    }

    void roundnessRoundsEveryTab()
    {
        QFETCH(double, radius);
        QFETCH(double, expected);
        for (const auto position : {Config::TabIndicatorPosition::Left, Config::TabIndicatorPosition::Right,
                 Config::TabIndicatorPosition::Top, Config::TabIndicatorPosition::Bottom}) {
            Config::Config config = tabbedConfig(position, 10, 5);
            config.layout.tabIndicator.cornerRadius = radius;
            Fixture fixture(config);
            const QList<Layout::WindowId> tabs = addTabs(fixture, 3);
            fixture.advance(1);
            const Layout::TabBarState bar = fixture.state(tabs[2]).tabBar;
            QCOMPARE(bar.tabRadii.size(), bar.tabRects.size());
            for (const Config::CornerRadius &corners : bar.tabRadii) {
                QCOMPARE(corners, (Config::CornerRadius {expected, expected, expected, expected}));
            }
        }
    }

    void roundnessFollowsTheShorterSideOfTinyTabs()
    {
        Config::Config config = tabbedConfig(Config::TabIndicatorPosition::Top, 40, 5);
        config.layout.tabIndicator.cornerRadius = 30;
        config.layout.tabIndicator.lengthTotalProportion = 0.01;
        Fixture fixture(config);
        const QList<Layout::WindowId> tabs = addTabs(fixture, 3);
        fixture.advance(1);
        const Layout::TabBarState bar = fixture.state(tabs[2]).tabBar;
        for (qsizetype i = 0; i < bar.tabRects.size(); ++i) {
            const double half = std::min(bar.tabRects[i].width(), bar.tabRects[i].height()) / 2.0;
            QCOMPARE(bar.tabRadii[i].topLeft, half);
            QVERIFY(half < 20.0);
        }
    }
};

QTEST_GUILESS_MAIN(TestLayoutTabIndicator)
#include "test_layout_tabindicator.moc"
