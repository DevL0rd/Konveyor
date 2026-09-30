#include "tabhelpers.h"

using namespace LayoutTest;

namespace
{

Config::Config thickTabs()
{
    return tabbedConfig(Config::TabIndicatorPosition::Left, 32, 5);
}

}

class TestLayoutTabInteractions : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void closingTheShownTabShowsTheNextOne()
    {
        Fixture fixture(thickTabs());
        QList<Layout::WindowId> tabs = addTabs(fixture, 4);
        fixture.perform(QStringLiteral("focus-window-up"));
        verifyShownTab(fixture, tabs, 2);
        fixture.remove(tabs.takeAt(2));
        verifyShownTab(fixture, tabs, 2);
        fixture.remove(tabs.takeAt(0));
        verifyShownTab(fixture, tabs, 1);
        QCOMPARE(fixture.state(tabs[1]).tabBar.tabRects.size(), 2);
        fixture.remove(tabs.takeAt(1));
        verifyShownTab(fixture, tabs, 0);
        QCOMPARE(fixture.frame(tabs[0]), QRectF(42, 16, 910, 1048));
        VERIFY_INVARIANTS(fixture);
    }

    void closingTheLastButOneTabWithHideWhenSingleGivesTheRoomBack()
    {
        Config::Config config = thickTabs();
        config.layout.tabIndicator.hideWhenSingleTab = true;
        Fixture fixture(config);
        const QList<Layout::WindowId> tabs = addTabs(fixture, 2);
        QCOMPARE(fixture.frame(tabs[0]), QRectF(42, 16, 910, 1048));
        fixture.remove(tabs[1]);
        QCOMPARE(fixture.frame(tabs[0]), QRectF(16, 16, 936, 1048));
        QVERIFY(!fixture.state(tabs[0]).tabBar.visible);
    }

    void expellingATabShowsItBesideTheColumn()
    {
        Fixture fixture(thickTabs());
        const QList<Layout::WindowId> tabs = addTabs(fixture, 3);
        fixture.perform(QStringLiteral("focus-window-top"));
        fixture.perform(QStringLiteral("consume-or-expel-window-right"));
        QCOMPARE(fixture.state(tabs[0]).columnIndex, 1);
        QVERIFY(fixture.state(tabs[0]).visible);
        verifyShownTab(fixture, {tabs[1], tabs[2]}, 0);
        const QRectF column = fixture.frame(tabs[1]);
        const QRectF expelled = fixture.frame(tabs[0]);
        QVERIFY(!expelled.intersects(column));
        for (const QRectF &rect : fixture.state(tabs[1]).tabBar.tabRects) {
            QVERIFY(!rect.intersects(expelled));
        }
        VERIFY_INVARIANTS(fixture);
    }

    void consumingIntoTheTabsFromBothSides()
    {
        Fixture fixture(thickTabs());
        const auto left = addPlainColumn(fixture, QStringLiteral("left"));
        const QList<Layout::WindowId> tabs = addTabs(fixture, 2);
        const auto right = addPlainColumn(fixture, QStringLiteral("right"));
        fixture.perform(QStringLiteral("consume-or-expel-window-left"));
        QCOMPARE(fixture.state(right).columnIndex, 1);
        verifyShownTab(fixture, {tabs[0], tabs[1], right}, 2);
        fixture.perform(QStringLiteral("focus-column-left"));
        fixture.perform(QStringLiteral("consume-or-expel-window-right"));
        QCOMPARE(fixture.state(left).columnIndex, 0);
        QCOMPARE(fixture.frame(left), fixture.frame(tabs[0]));
        QCOMPARE(fixture.state(left).tileIndex, 3);
        QCOMPARE(fixture.focused(), std::optional(left));
        verifyShownTab(fixture, {tabs[0], tabs[1], right, left}, 3);
        VERIFY_INVARIANTS(fixture);
    }

    void movingATabReordersTheTabsAndKeepsItShown()
    {
        Fixture fixture(thickTabs());
        const QList<Layout::WindowId> tabs = addTabs(fixture, 3);
        fixture.perform(QStringLiteral("move-window-up"));
        QCOMPARE(fixture.state(tabs[2]).tileIndex, 1);
        verifyShownTab(fixture, {tabs[0], tabs[2], tabs[1]}, 1);
        fixture.perform(QStringLiteral("move-window-up"));
        verifyShownTab(fixture, {tabs[2], tabs[0], tabs[1]}, 0);
        fixture.perform(QStringLiteral("move-window-down"));
        verifyShownTab(fixture, {tabs[0], tabs[2], tabs[1]}, 1);
        VERIFY_INVARIANTS(fixture);
    }

    void movingTheTabbedColumnKeepsTheTabsAndTheirRoom()
    {
        Fixture fixture(thickTabs());
        const auto plain = addPlainColumn(fixture, QStringLiteral("plain"));
        const QList<Layout::WindowId> tabs = addTabs(fixture, 2);
        fixture.perform(QStringLiteral("focus-window-up"));
        fixture.perform(QStringLiteral("move-column-left"));
        QCOMPARE(fixture.state(tabs[0]).columnIndex, 0);
        verifyShownTab(fixture, tabs, 0);
        QCOMPARE(fixture.frame(tabs[0]).left(), 42.0);
        QVERIFY(fixture.frame(plain).left() - fixture.frame(tabs[0]).right() >= 16.0);
        fixture.perform(QStringLiteral("move-column-to-workspace-down"));
        QCOMPARE(fixture.state(tabs[0]).workspace, fixture.state(tabs[1]).workspace);
        QVERIFY(fixture.state(tabs[0]).workspace != fixture.state(plain).workspace);
        verifyShownTab(fixture, tabs, 0);
        QCOMPARE(fixture.frame(tabs[0]), QRectF(42, 16, 910, 1048));
        VERIFY_INVARIANTS(fixture);
    }

    void movingATabToAnotherWorkspaceLeavesTheRestShown()
    {
        Fixture fixture(thickTabs());
        const QList<Layout::WindowId> tabs = addTabs(fixture, 3);
        fixture.perform(QStringLiteral("move-window-to-workspace-down"));
        QVERIFY(fixture.state(tabs[2]).workspace != fixture.state(tabs[0]).workspace);
        QCOMPARE(fixture.focused(), std::optional(tabs[2]));
        QVERIFY(fixture.state(tabs[2]).visible);
        fixture.perform(QStringLiteral("focus-workspace-up"));
        verifyShownTab(fixture, {tabs[0], tabs[1]}, 1);
        QCOMPARE(fixture.state(tabs[1]).tabBar.tabRects.size(), 2);
        VERIFY_INVARIANTS(fixture);
    }

    void turningTabsOffStacksTheWindowsAndGivesTheRoomBack()
    {
        Fixture fixture(thickTabs());
        const QList<Layout::WindowId> tabs = addTabs(fixture, 2);
        fixture.perform(QStringLiteral("focus-window-up"));
        fixture.perform(QStringLiteral("toggle-column-tabbed-display"));
        QVERIFY(fixture.state(tabs[0]).visible);
        QVERIFY(fixture.state(tabs[1]).visible);
        QVERIFY(!fixture.state(tabs[0]).tabBar.visible);
        QCOMPARE(fixture.frame(tabs[0]).left(), 16.0);
        QVERIFY(!fixture.frame(tabs[0]).intersects(fixture.frame(tabs[1])));
        QVERIFY(fixture.state(tabs[0]).stackingIndex < fixture.state(tabs[1]).stackingIndex);
        fixture.perform(QStringLiteral("set-column-display"), {QStringLiteral("tabbed")});
        verifyShownTab(fixture, tabs, 0);
        QCOMPARE(fixture.frame(tabs[0]).left(), 42.0);
        VERIFY_INVARIANTS(fixture);
    }

    void urgentAndFocusedTabsGetTheirPaint()
    {
        Config::Config config = thickTabs();
        config.layout.tabIndicator.active = Config::Paint {Config::ColorSource::Explicit, QColor(Qt::magenta), std::nullopt};
        config.layout.tabIndicator.inactive = Config::Paint {Config::ColorSource::Explicit, QColor(Qt::cyan), std::nullopt};
        config.layout.tabIndicator.urgent = Config::Paint {Config::ColorSource::Explicit, QColor(Qt::red), std::nullopt};
        Fixture fixture(config);
        const QList<Layout::WindowId> tabs = addTabs(fixture, 3);
        fixture.engine().setWindowUrgent(tabs[0], true);
        const auto colors = [&] {
            QList<QColor> list;
            for (const Layout::ResolvedPaint &paint : fixture.state(fixture.focused().value()).tabBar.tabPaints) {
                list.append(paint.color);
            }
            return list;
        };
        QCOMPARE(colors(), (QList<QColor> {Qt::red, Qt::cyan, Qt::magenta}));
        fixture.perform(QStringLiteral("focus-window-up"));
        QCOMPARE(colors(), (QList<QColor> {Qt::red, Qt::magenta, Qt::cyan}));
        const auto other = addPlainColumn(fixture, QStringLiteral("other"));
        QCOMPARE(fixture.focused(), std::optional(other));
        QCOMPARE(fixture.state(tabs[1]).tabBar.tabPaints[1].color, QColor(Qt::cyan));
    }

    void tabsFallBackToTheFocusRingPaintAndKeepGradients()
    {
        Config::Config config = thickTabs();
        Config::Gradient gradient;
        gradient.from = Qt::red;
        gradient.to = Qt::blue;
        config.layout.focusRing.active = Config::Paint {Config::ColorSource::SystemAccent, QColor(), std::nullopt};
        config.layout.focusRing.inactive = Config::Paint {Config::ColorSource::Explicit, QColor(Qt::gray), std::nullopt};
        config.layout.tabIndicator.active = Config::Paint {Config::ColorSource::Explicit, QColor(), gradient};
        Fixture fixture(config);
        const QList<Layout::WindowId> tabs = addTabs(fixture, 2);
        const QList<Layout::ResolvedPaint> paints = fixture.state(tabs[1]).tabBar.tabPaints;
        QCOMPARE(paints[0].color, QColor(Qt::gray));
        QCOMPARE(paints[1].gradient, std::optional(gradient));
        config.layout.tabIndicator.active.reset();
        fixture.setConfig(config);
        QCOMPARE(fixture.state(tabs[1]).tabBar.tabPaints[1].source, Config::ColorSource::SystemAccent);
    }

    void lengthAndSpaceBetweenTabsShapeTheBar()
    {
        Config::Config config = tabbedConfig(Config::TabIndicatorPosition::Top, 6, 5);
        config.layout.tabIndicator.lengthTotalProportion = 0.8;
        config.layout.tabIndicator.gapsBetweenTabs = 10;
        Fixture fixture(config);
        const QList<Layout::WindowId> tabs = addTabs(fixture, 3);
        fixture.advance(1);
        const QList<QRectF> rects = fixture.state(tabs[2]).tabBar.tabRects;
        const QRectF window = fixture.frame(tabs[2]);
        QCOMPARE(rects.size(), 3);
        QCOMPARE(rects[1].left() - rects[0].right(), 10.0);
        QCOMPARE(rects[2].left() - rects[1].right(), 10.0);
        QVERIFY(std::abs(rects[2].right() - rects[0].left() - window.width() * 0.8) <= 1.0);
        QVERIFY(std::abs((rects[0].left() - window.left()) - (window.right() - rects[2].right())) <= 1.0);
        for (const QRectF &rect : rects) {
            QCOMPARE(rect.height(), 6.0);
            QCOMPARE(window.top() - rect.bottom(), 5.0);
        }
    }

    void disabledTabsReserveNothingAndCannotBeHit()
    {
        Config::Config config = thickTabs();
        config.layout.tabIndicator.enabled = false;
        Fixture fixture(config);
        const QList<Layout::WindowId> tabs = addTabs(fixture, 2);
        QCOMPARE(fixture.frame(tabs[1]), QRectF(16, 16, 936, 1048));
        QVERIFY(!fixture.state(tabs[1]).tabBar.visible);
        QCOMPARE(fixture.engine().hitAt(QPointF(8, 540)), std::nullopt);
        verifyShownTab(fixture, tabs, 1);
    }

    void fullscreenAndMaximizedTabsIgnoreTheRoom()
    {
        Fixture fixture(thickTabs());
        const QList<Layout::WindowId> tabs = addTabs(fixture, 2);
        fixture.perform(QStringLiteral("maximize-column"));
        QCOMPARE(fixture.frame(tabs[1]), QRectF(42, 16, 1862, 1048));
        fixture.perform(QStringLiteral("maximize-column"));
        fixture.perform(QStringLiteral("fullscreen-window"));
        QCOMPARE(fixture.frame(tabs[1]), QRectF(0, 0, 1920, 1080));
        verifyShownTab(fixture, tabs, 1);
        fixture.perform(QStringLiteral("focus-window-up"));
        verifyShownTab(fixture, tabs, 0);
        fixture.perform(QStringLiteral("focus-window-down"));
        fixture.perform(QStringLiteral("fullscreen-window"));
        QCOMPARE(fixture.frame(tabs[1]), QRectF(42, 16, 910, 1048));
        VERIFY_INVARIANTS(fixture);
    }

    void widthPresetsCountTheRoomForTheTabs()
    {
        Fixture fixture(thickTabs());
        const QList<Layout::WindowId> tabs = addTabs(fixture, 2);
        fixture.perform(QStringLiteral("switch-preset-column-width"));
        QCOMPARE(fixture.frame(tabs[1]), QRectF(42, 16, 1227, 1048));
        fixture.perform(QStringLiteral("set-window-width"), {QStringLiteral("500")});
        QCOMPARE(fixture.frame(tabs[1]), QRectF(42, 16, 500, 1048));
        VERIFY_INVARIANTS(fixture);
    }

    void scaledOutputsKeepTabsOnWholePixels()
    {
        Fixture fixture(tabbedConfig(Config::TabIndicatorPosition::Right, 7.3, 4.6));
        fixture.addOutput(makeOutput(QStringLiteral("DP-2"), QRectF(1920, 0, 1536, 864), 1.25));
        fixture.engine().focusOutput(QStringLiteral("DP-2"));
        const QList<Layout::WindowId> tabs = addTabs(fixture, 3);
        fixture.advance(1);
        for (const QRectF &rect : fixture.state(tabs[2]).tabBar.tabRects) {
            for (const double edge : {rect.left(), rect.top(), rect.right(), rect.bottom()}) {
                QCOMPARE(std::round(edge * 1.25), edge * 1.25);
            }
            QVERIFY(QRectF(1920, 0, 1536, 864).contains(rect));
        }
        VERIFY_INVARIANTS(fixture);
    }
};

QTEST_GUILESS_MAIN(TestLayoutTabInteractions)
#include "test_layout_tabinteractions.moc"
