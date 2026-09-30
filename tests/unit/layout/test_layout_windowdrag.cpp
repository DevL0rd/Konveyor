#include "helpers.h"

using namespace LayoutTest;

namespace
{

enum class Request
{
    Client,
    Action,
    WindowedAction,
};

void requestFullscreen(Fixture &fixture, Layout::WindowId id, Request request)
{
    if (request == Request::Client) {
        fixture.engine().setWindowFullscreen(id, true);
        return;
    }
    const QString name = request == Request::Action ? QStringLiteral("fullscreen-window") : QStringLiteral("toggle-windowed-fullscreen");
    QVERIFY(fixture.engine().perform({name, {}, {}}, id).ok);
}

void addRequestRows()
{
    QTest::addColumn<Request>("request");
    QTest::newRow("client") << Request::Client;
    QTest::newRow("action") << Request::Action;
    QTest::newRow("windowed action") << Request::WindowedAction;
}

bool isFullscreenLike(const Layout::WindowState &state)
{
    return state.requestedSizingMode == Layout::WindowMode::Fullscreen || state.isWindowedFullscreen;
}

}

class TestLayoutWindowDrag : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void interactiveMoveWithinOffsetOutputKeepsTheWorkspace_data()
    {
        QTest::addColumn<QRectF>("geometry");
        QTest::newRow("beside, top aligned") << QRectF(1920, 0, 1024, 600);
        QTest::newRow("beside, bottom aligned") << QRectF(1920, 480, 1024, 600);
        QTest::newRow("below") << QRectF(0, 1080, 1024, 600);
    }

    void interactiveMoveWithinOffsetOutputKeepsTheWorkspace()
    {
        QFETCH(QRectF, geometry);
        Fixture fixture;
        fixture.engine().addOutput(makeOutput(QStringLiteral("DP-2"), geometry));
        fixture.perform(QStringLiteral("focus-monitor"), {QStringLiteral("DP-2")});
        const auto id = fixture.add(QStringLiteral("a"));
        fixture.add(QStringLiteral("b"));
        QCOMPARE(fixture.state(id).output, QStringLiteral("DP-2"));
        const Layout::WorkspaceId workspace = fixture.state(id).workspace;
        const int column = fixture.state(id).columnIndex;
        const QPointF start = fixture.frame(id).center();
        QVERIFY(fixture.engine().beginWindowDrag(id, start));
        fixture.engine().updateWindowDrag(start + QPointF(0, 300), QStringLiteral("DP-2"));
        fixture.engine().updateWindowDrag(start + QPointF(10, 20), QStringLiteral("DP-2"));
        fixture.engine().endWindowDrag();
        fixture.settle();
        QCOMPARE(fixture.state(id).output, QStringLiteral("DP-2"));
        QCOMPARE(fixture.state(id).workspace, workspace);
        QCOMPARE(fixture.state(id).columnIndex, column);
        QVERIFY(fixture.state(id).onActiveWorkspace);
        VERIFY_INVARIANTS(fixture);
    }

    void aWindowDroppedOnTheOtherMonitorHasFocusThere()
    {
        Fixture fixture;
        fixture.addOutput(QStringLiteral("DP-2"), QRectF(1920, 0, 1920, 1080));
        fixture.engine().focusOutput(QStringLiteral("DP-2"));
        const auto there = fixture.add(QStringLiteral("there"));
        fixture.engine().focusOutput(QStringLiteral("DP-1"));
        fixture.add(QStringLiteral("a"));
        const auto moved = fixture.add(QStringLiteral("b"));
        QVERIFY(fixture.engine().beginWindowDrag(moved, QPointF(1800, 540)));
        fixture.engine().updateWindowDrag(QPointF(1900, 540), QStringLiteral("DP-1"));
        fixture.engine().updateWindowDrag(QPointF(2200, 540), QStringLiteral("DP-2"));
        fixture.engine().updateWindowDrag(QPointF(2300, 540), QStringLiteral("DP-2"));
        fixture.engine().endWindowDrag();
        fixture.settle();
        QCOMPARE(fixture.state(moved).output, QStringLiteral("DP-2"));
        QCOMPARE(fixture.focused(), std::optional(moved));
        QCOMPARE(fixture.engine().focusedOutput(), std::optional(QStringLiteral("DP-2")));
        QVERIFY(!fixture.state(there).isFocused);
        VERIFY_INVARIANTS(fixture);
    }

    void fullscreenMidDragDropsTheWindowWhereTheHintIs_data() { addRequestRows(); }

    void fullscreenMidDragDropsTheWindowWhereTheHintIs()
    {
        QFETCH(Request, request);
        Fixture fixture;
        fixture.add(QStringLiteral("a"));
        fixture.add(QStringLiteral("b"));
        const auto c = fixture.add(QStringLiteral("c"));
        QVERIFY(startMove(fixture, c, QPointF(2, 540)));
        QCOMPARE(fixture.engine().movingWindow(), std::optional(c));
        requestFullscreen(fixture, c, request);
        fixture.settle();
        QCOMPARE(fixture.engine().movingWindow(), std::nullopt);
        QVERIFY(!fixture.engine().outputStates().constFirst().dropHint);
        QVERIFY(isFullscreenLike(fixture.state(c)));
        QCOMPARE(fixture.state(c).columnIndex, 0);
        QCOMPARE(fixture.focused(), std::optional(c));
        fixture.engine().updateWindowDrag(QPointF(1900, 540), QStringLiteral("DP-1"));
        fixture.engine().endWindowDrag();
        fixture.settle();
        QVERIFY(isFullscreenLike(fixture.state(c)));
        QCOMPARE(fixture.state(c).columnIndex, 0);
        VERIFY_INVARIANTS(fixture);
    }

    void fullscreenBeforeTheWindowDetachesStaysFullscreen_data() { addRequestRows(); }

    void fullscreenBeforeTheWindowDetachesStaysFullscreen()
    {
        QFETCH(Request, request);
        Fixture fixture;
        fixture.add(QStringLiteral("a"));
        const auto b = fixture.add(QStringLiteral("b"));
        const QPointF start = fixture.frame(b).center();
        QVERIFY(fixture.engine().beginWindowDrag(b, start));
        fixture.engine().updateWindowDrag(start + QPointF(20, 0), QStringLiteral("DP-1"));
        requestFullscreen(fixture, b, request);
        fixture.engine().updateWindowDrag(start + QPointF(-900, 0), QStringLiteral("DP-1"));
        fixture.settle();
        QCOMPARE(fixture.engine().movingWindow(), std::nullopt);
        QVERIFY(isFullscreenLike(fixture.state(b)));
        QCOMPARE(fixture.state(b).columnIndex, 1);
        fixture.engine().endWindowDrag();
        VERIFY_INVARIANTS(fixture);
    }

    void leavingFullscreenAfterAFullscreenDropKeepsTheDropPlace()
    {
        Fixture fixture;
        fixture.add(QStringLiteral("a"));
        fixture.add(QStringLiteral("b"));
        const auto c = fixture.add(QStringLiteral("c"));
        QVERIFY(startMove(fixture, c, QPointF(2, 540)));
        fixture.engine().setWindowFullscreen(c, true);
        fixture.settle();
        fixture.engine().setWindowFullscreen(c, false);
        fixture.settle();
        QCOMPARE(fixture.state(c).requestedSizingMode, Layout::WindowMode::Normal);
        QCOMPARE(fixture.state(c).columnIndex, 0);
        VERIFY_INVARIANTS(fixture);
    }

    void fullscreenForAFloatingWindowMidDragDropsItFloating()
    {
        Fixture fixture;
        fixture.add(QStringLiteral("a"));
        const auto b = fixture.add(QStringLiteral("b"));
        QVERIFY(fixture.perform(QStringLiteral("toggle-window-floating")).ok);
        fixture.advance(1000);
        const QPointF start = fixture.frame(b).center();
        QVERIFY(fixture.engine().beginWindowDrag(b, start));
        fixture.engine().updateWindowDrag(start + QPointF(-200, 100), QStringLiteral("DP-1"));
        fixture.engine().setWindowFullscreen(b, true);
        fixture.settle();
        QCOMPARE(fixture.engine().movingWindow(), std::nullopt);
        QCOMPARE(fixture.state(b).requestedSizingMode, Layout::WindowMode::Fullscreen);
        fixture.engine().setWindowFullscreen(b, false);
        fixture.advance(1000);
        QVERIFY(fixture.state(b).isFloating);
        QCOMPARE(fixture.frame(b).center(), start + QPointF(-200, 100));
        VERIFY_INVARIANTS(fixture);
    }

    void anActionWithoutATargetActsOnTheDraggedWindow()
    {
        Fixture fixture;
        fixture.add(QStringLiteral("a"));
        const auto b = fixture.add(QStringLiteral("b"));
        const auto c = fixture.add(QStringLiteral("c"));
        QVERIFY(startMove(fixture, c, QPointF(2, 540)));
        QVERIFY(fixture.perform(QStringLiteral("fullscreen-window")).ok);
        QCOMPARE(fixture.engine().movingWindow(), std::nullopt);
        QCOMPARE(fixture.state(c).requestedSizingMode, Layout::WindowMode::Fullscreen);
        QCOMPARE(fixture.state(b).requestedSizingMode, Layout::WindowMode::Normal);
        QCOMPARE(fixture.state(c).columnIndex, 0);
        VERIFY_INVARIANTS(fixture);
    }

    void fullscreenForAnotherWindowKeepsTheDrag()
    {
        Fixture fixture;
        const auto a = fixture.add(QStringLiteral("a"));
        fixture.add(QStringLiteral("b"));
        const auto c = fixture.add(QStringLiteral("c"));
        QVERIFY(startMove(fixture, c, QPointF(2, 540)));
        fixture.engine().setWindowFullscreen(a, true);
        QCOMPARE(fixture.engine().movingWindow(), std::optional(c));
        fixture.engine().endWindowDrag();
        fixture.settle();
        QCOMPARE(fixture.state(c).requestedSizingMode, Layout::WindowMode::Normal);
        VERIFY_INVARIANTS(fixture);
    }
};

QTEST_GUILESS_MAIN(TestLayoutWindowDrag)
#include "test_layout_windowdrag.moc"
