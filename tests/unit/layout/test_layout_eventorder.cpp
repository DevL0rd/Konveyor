#include "helpers.h"

using namespace LayoutTest;

namespace
{

std::optional<QRectF> dropHint(Fixture &fixture)
{
    return fixture.engine().outputStates().constFirst().dropHint;
}

}

class TestLayoutEventOrder : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void aWindowClosingRightAfterLeavingTheDropTargetKeepsTheDragGoing()
    {
        Fixture fixture;
        const Layout::WindowId dragged = fixture.add(QStringLiteral("dragged"));
        fixture.perform(QStringLiteral("focus-workspace-down"));
        const Layout::WindowId kept = fixture.add(QStringLiteral("kept"));
        const Layout::WindowId closing = fixture.add(QStringLiteral("closing"));
        fixture.perform(QStringLiteral("focus-workspace-up"));

        QVERIFY(startMove(fixture, dragged, QPointF(960, 300)));
        QVERIFY(dropHint(fixture));
        fixture.perform(QStringLiteral("focus-workspace-down"));
        fixture.remove(closing);
        QCOMPARE(fixture.engine().movingWindow(), std::optional(dragged));
        QCOMPARE(fixture.engine().outputStates().size(), 1);

        fixture.advance(1000);
        QCOMPARE(fixture.engine().movingWindow(), std::optional(dragged));

        fixture.engine().endWindowDrag();
        fixture.settle();
        QVERIFY(fixture.engine().hasWindow(dragged));
        QVERIFY(fixture.engine().hasWindow(kept));
        VERIFY_INVARIANTS(fixture);
    }

    void aWindowClosingWhileItsDragHasNotDetachedEndsTheDragWithIt()
    {
        Fixture fixture;
        const Layout::WindowId closing = fixture.add(QStringLiteral("closing"));
        const Layout::WindowId kept = fixture.add(QStringLiteral("kept"));
        QVERIFY(fixture.engine().beginWindowDrag(closing, fixture.frame(closing).center()));
        fixture.remove(closing);
        fixture.engine().updateWindowDrag(QPointF(900, 500), QStringLiteral("DP-1"));
        fixture.engine().endWindowDrag();
        fixture.engine().cancelWindowDrag();
        QCOMPARE(fixture.engine().movingWindow(), std::nullopt);
        QVERIFY(fixture.engine().hasWindow(kept));
        VERIFY_INVARIANTS(fixture);
    }

    void removingTheOnlyOutputDuringADragKeepsTheWindowForTheNextOutput()
    {
        Fixture fixture;
        const Layout::WindowId dragged = fixture.add(QStringLiteral("dragged"));
        QVERIFY(startMove(fixture, dragged, QPointF(900, 500)));
        fixture.engine().removeOutput(QStringLiteral("DP-1"));
        QCOMPARE(fixture.engine().movingWindow(), std::nullopt);
        QVERIFY(fixture.engine().hasWindow(dragged));
        fixture.engine().cancelWindowDrag();
        fixture.addOutput(QStringLiteral("DP-1"), QRectF(0, 0, 1920, 1080));
        QVERIFY(fixture.engine().windowState(dragged).has_value());
        VERIFY_INVARIANTS(fixture);
    }

    void aWindowOpeningWithNoOutputsWaitsForOne()
    {
        Fixture fixture;
        fixture.engine().removeOutput(QStringLiteral("DP-1"));
        const Layout::WindowId opened = fixture.add(QStringLiteral("opened"));
        QVERIFY(fixture.engine().hasWindow(opened));
        QVERIFY(!fixture.engine().beginWindowDrag(opened, QPointF(10, 10)));
        fixture.engine().perform(action(QStringLiteral("move-window-to-monitor-next")));
        fixture.engine().perform(action(QStringLiteral("move-column-to-monitor-next")));
        fixture.engine().perform(action(QStringLiteral("move-workspace-to-monitor-next")));
        fixture.addOutput(QStringLiteral("DP-1"), QRectF(0, 0, 1920, 1080));
        QVERIFY(fixture.engine().windowState(opened).has_value());
        VERIFY_INVARIANTS(fixture);
    }
};

QTEST_GUILESS_MAIN(TestLayoutEventOrder)

#include "test_layout_eventorder.moc"
