#include "helpers.h"

#include "layout/gestures/gesturerouter.h"

using namespace LayoutTest;

namespace
{

const QString Output = QStringLiteral("DP-1");

Config::Config fixedColumns(Config::CenterFocusedColumn centering = Config::CenterFocusedColumn::Never)
{
    Config::Config config = instantConfig();
    config.layout.defaultColumnWidth = Config::Fixed {600};
    config.layout.centerFocusedColumn = centering;
    return config;
}

QList<Layout::WindowId> addRow(Fixture &fixture, int count)
{
    QList<Layout::WindowId> ids;
    for (int i = 0; i < count; ++i) {
        ids.append(fixture.add(QStringLiteral("column")));
    }
    return ids;
}

void resize(Fixture &fixture, Layout::WindowId id, Layout::ResizeEdge edge, QPointF delta)
{
    QVERIFY(fixture.engine().beginResize(id, static_cast<quint8>(edge)));
    fixture.engine().updateResize(delta);
    fixture.settle();
    fixture.engine().endResize();
    fixture.settle();
}

void touchpadSwipe(Layout::GestureRouter &router, int fingers, QPointF delta)
{
    router.touchpadSwipeBegin(fingers, Output);
    router.touchpadSwipeUpdate(delta / 20.0, 10);
    router.touchpadSwipeUpdate(delta, 20);
    router.touchpadSwipeEnd();
}

}

class TestLayoutGestureEdges : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void contactsResetCancelsAPendingTap()
    {
        Fixture fixture;
        Layout::GestureRouter router(fixture.engine());
        router.setConfig(instantConfig().gestures);
        const auto id = fixture.add();
        for (qint32 slot = 0; slot < 3; ++slot) {
            router.touchpadContactDown(slot, QPointF(20.0 + slot * 15.0, 30.0), 0);
        }
        router.touchpadContactsReset();
        QVERIFY(router.takesTouchpadTapButton());
        for (qint32 slot = 0; slot < 3; ++slot) {
            QVERIFY(!router.touchpadContactUp(slot, 50));
        }
        fixture.settle();
        QCOMPARE(fixture.frame(id).width(), 936.0);
        for (qint32 slot = 0; slot < 3; ++slot) {
            router.touchpadContactDown(slot, QPointF(20.0 + slot * 15.0, 30.0), 100);
        }
        bool tapped = false;
        for (qint32 slot = 0; slot < 3; ++slot) {
            tapped = router.touchpadContactUp(slot, 150) || tapped;
        }
        fixture.settle();
        QVERIFY(tapped);
        QCOMPARE(fixture.frame(id).width(), 1253.0);
    }

    void endSwipeWithAnUnknownWindowSnapsNormally()
    {
        Fixture fixture(fixedColumns());
        const QList<Layout::WindowId> row = addRow(fixture, 4);
        fixture.perform(QStringLiteral("focus-column-first"));
        fixture.engine().beginSwipe(Output, false);
        fixture.engine().updateSwipe(900.0, 10, false);
        fixture.engine().updateSwipe(900.0, 20, false);
        fixture.engine().endSwipe(false, Layout::WindowId(999));
        fixture.settle();
        QVERIFY(fixture.focused() != std::optional(row[0]));
        VERIFY_INVARIANTS(fixture);
    }

    void endSwipeKeepsTheDraggedWindowCentered()
    {
        Fixture fixture(fixedColumns(Config::CenterFocusedColumn::Always));
        const QList<Layout::WindowId> row = addRow(fixture, 4);
        fixture.engine().activateWindow(row[1]);
        fixture.engine().beginSwipe(Output, false);
        fixture.engine().updateSwipe(700.0, 10, false);
        fixture.engine().updateSwipe(700.0, 20, false);
        fixture.engine().endSwipe(false, row[1]);
        fixture.settle();
        QCOMPARE(fixture.focused(), std::optional(row[1]));
        QCOMPARE(fixture.frame(row[1]).x(), 660.0);
        VERIFY_INVARIANTS(fixture);
    }

    void resizingTheTopEdgeOfTheFirstRowDoesNothing()
    {
        Fixture fixture;
        const auto [top, bottom] = addStackedPair(fixture);
        resize(fixture, top, Layout::ResizeEdge::Top, QPointF(0, -100));
        QCOMPARE(fixture.frame(top).height(), 516.0);
        QCOMPARE(fixture.frame(bottom).height(), 516.0);
        VERIFY_INVARIANTS(fixture);
    }

    void resizingTheTopEdgeOfALowerRowTakesFromTheRowAbove()
    {
        Fixture fixture;
        const auto [top, bottom] = addStackedPair(fixture);
        resize(fixture, bottom, Layout::ResizeEdge::Top, QPointF(0, -100));
        QCOMPARE(fixture.frame(bottom).height(), 616.0);
        QCOMPARE(fixture.frame(top).height(), 416.0);
        QCOMPARE(fixture.frame(bottom).y(), 448.0);
        VERIFY_INVARIANTS(fixture);
    }

    void resizingACornerChangesBothSizes()
    {
        Fixture fixture;
        const auto [top, bottom] = addStackedPair(fixture);
        const quint8 corner = static_cast<quint8>(Layout::ResizeEdge::Bottom) | static_cast<quint8>(Layout::ResizeEdge::Right);
        QVERIFY(fixture.engine().beginResize(top, corner));
        fixture.engine().updateResize(QPointF(64, 84));
        fixture.settle();
        fixture.engine().endResize();
        fixture.settle();
        QCOMPARE(fixture.frame(top).size(), QSizeF(1000, 600));
        QCOMPARE(fixture.frame(bottom).size(), QSizeF(1000, 432));
        VERIFY_INVARIANTS(fixture);
    }

    void resizingWhileAlwaysCenteredGrowsBothSides()
    {
        Fixture fixture(fixedColumns(Config::CenterFocusedColumn::Always));
        const QList<Layout::WindowId> row = addRow(fixture, 2);
        resize(fixture, row[1], Layout::ResizeEdge::Right, QPointF(50, 0));
        QCOMPARE(fixture.frame(row[1]).width(), 700.0);
        QCOMPARE(fixture.frame(row[1]).x(), 610.0);
        resize(fixture, row[1], Layout::ResizeEdge::Left, QPointF(50, 0));
        QCOMPARE(fixture.frame(row[1]).width(), 600.0);
        QCOMPARE(fixture.frame(row[1]).x(), 660.0);
        VERIFY_INVARIANTS(fixture);
    }

    void expandedColumnsRefuseInteractiveResize()
    {
        Fixture fixture;
        const auto id = fixture.add();
        fixture.perform(QStringLiteral("fullscreen-window"));
        QVERIFY(!fixture.engine().beginResize(id, static_cast<quint8>(Layout::ResizeEdge::Right)));
        fixture.perform(QStringLiteral("fullscreen-window"));
        fixture.perform(QStringLiteral("maximize-window-to-edges"));
        QVERIFY(!fixture.engine().beginResize(id, static_cast<quint8>(Layout::ResizeEdge::Right)));
        fixture.engine().updateResize(QPointF(100, 0));
        fixture.settle();
        QCOMPARE(fixture.frame(id), QRectF(0, 0, 1920, 1080));
    }

    void floatingFrameDuringADragDoesNotMoveTheDraggedWindow()
    {
        Fixture fixture;
        fixture.add(QStringLiteral("tiled"));
        const auto floating = fixture.add(QStringLiteral("app"), QSizeF(400, 300));
        fixture.perform(QStringLiteral("toggle-window-floating"));
        fixture.advance(1);
        const QPointF grab = fixture.frame(floating).center();
        QVERIFY(fixture.engine().beginWindowDrag(floating, grab));
        fixture.engine().updateWindowDrag(grab - QPointF(300, 100), Output);
        fixture.engine().setFloatingFrame(floating, QRectF(0, 0, 400, 300));
        fixture.engine().updateWindowDrag(grab - QPointF(310, 110), Output);
        fixture.engine().endWindowDrag();
        fixture.settle();
        QVERIFY(fixture.state(floating).isFloating);
        QCOMPARE(fixture.frame(floating).center(), grab - QPointF(310, 110));
        fixture.engine().setFloatingFrame(floating, QRectF(50, 60, 400, 300));
        fixture.settle();
        QCOMPARE(fixture.frame(floating).topLeft(), QPointF(50, 60));
        VERIFY_INVARIANTS(fixture);
    }

    void rowSwipeWinsWhenFingerCountsMatch()
    {
        Config::Config config = fixedColumns();
        config.gestures.touchpad.swipeFingers = 4;
        config.gestures.touchpad.windowSwipeFingers = 4;
        Fixture fixture(config);
        Layout::GestureRouter router(fixture.engine());
        router.setConfig(config.gestures);
        const auto [top, bottom] = addStackedPair(fixture);
        fixture.add(QStringLiteral("right"));
        fixture.add(QStringLiteral("far"));
        fixture.engine().activateWindow(bottom);
        touchpadSwipe(router, 4, QPointF(-1500, 0));
        fixture.settle();
        QCOMPARE(fixture.state(bottom).columnIndex, fixture.state(top).columnIndex);
        QVERIFY(fixture.focused() != std::optional(bottom));
        VERIFY_INVARIANTS(fixture);
    }

    void windowSwipeTakesMatchingFingersOnceRowSwipesAreOff()
    {
        Config::Config config = fixedColumns();
        config.gestures.touchpad.swipeFingers = 4;
        config.gestures.touchpad.windowSwipeFingers = 4;
        config.gestures.touchpad.horizontalSwipe = Config::HorizontalSwipe::Off;
        config.gestures.touchpad.verticalSwipe = Config::VerticalSwipe::Off;
        Fixture fixture(config);
        Layout::GestureRouter router(fixture.engine());
        router.setConfig(config.gestures);
        const auto [top, bottom] = addStackedPair(fixture);
        touchpadSwipe(router, 4, QPointF(300, 0));
        fixture.settle();
        QVERIFY(fixture.state(bottom).columnIndex != fixture.state(top).columnIndex);
        VERIFY_INVARIANTS(fixture);
    }
};

QTEST_GUILESS_MAIN(TestLayoutGestureEdges)
#include "test_layout_gestureedges.moc"
