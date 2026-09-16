#include "helpers.h"

using namespace LayoutTest;

namespace
{

int activeWorkspaceIndex(Layout::Engine &engine)
{
    for (const Layout::WorkspaceState &state : engine.workspaceStates()) {
        if (state.isActive) {
            return state.index;
        }
    }
    return 0;
}

double firstColumnX(Fixture &fixture)
{
    for (const Layout::WindowState &state : fixture.engine().windowStates()) {
        if (state.columnIndex == 0) {
            return state.targetFrame.x();
        }
    }
    return 0.0;
}

bool startMove(Fixture &fixture, Layout::WindowId id, QPointF to)
{
    const QPointF start = fixture.frame(id).center();
    if (!fixture.engine().beginWindowDrag(id, start)) {
        return false;
    }
    fixture.engine().updateWindowDrag(to, QStringLiteral("DP-1"));
    return true;
}

Config::Config wideColumns()
{
    Config::Config config = instantConfig();
    config.layout.defaultColumnWidth = Config::PresetSize(Config::Fixed {900});
    return config;
}

}

class TestLayoutGestures : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void viewOffsetGestureScrollsOneToOne()
    {
        Fixture fixture;
        fixture.add();
        const auto middle = fixture.add();
        fixture.add();
        fixture.engine().activateWindow(middle);
        fixture.engine().beginSwipe(QStringLiteral("DP-1"), false);
        fixture.settle();
        const double start = fixture.frame(middle).x();
        fixture.engine().updateSwipe(100.0, 16, false);
        fixture.settle();
        const double first = fixture.frame(middle).x();
        fixture.engine().updateSwipe(100.0, 32, false);
        fixture.settle();
        const double second = fixture.frame(middle).x();
        QVERIFY2(first < start, "a positive gesture delta scrolls the row to the left");
        QCOMPARE(second - first, -100.0);
        fixture.engine().endSwipe(false);
        fixture.settle();
    }

    void viewOffsetGestureSnapsToColumn()
    {
        Fixture fixture(wideColumns());
        fixture.add(QStringLiteral("a"));
        fixture.add(QStringLiteral("b"));
        fixture.add(QStringLiteral("c"));
        fixture.perform(QStringLiteral("focus-column-first"));
        const auto first = fixture.focused();

        fixture.engine().beginSwipe(QStringLiteral("DP-1"), true);
        fixture.engine().updateSwipe(1200.0, 10, true);
        fixture.engine().endSwipe(true);
        fixture.settle();
        QVERIFY(fixture.focused() != first);
        VERIFY_INVARIANTS(fixture);
    }

    void titlebarDragKeepsTheDraggedWindowActive()
    {
        Fixture fixture(wideColumns());
        const auto dragged = fixture.add(QStringLiteral("a"));
        fixture.add(QStringLiteral("b"));
        fixture.add(QStringLiteral("c"));
        fixture.engine().activateWindow(dragged);
        fixture.settle();

        fixture.engine().beginSwipe(QStringLiteral("DP-1"), false);
        fixture.engine().updateSwipe(400.0, 10, false);
        fixture.engine().updateSwipe(400.0, 20, false);
        fixture.engine().endSwipe(false, dragged);
        fixture.settle();
        QCOMPARE(fixture.focused(), std::optional(dragged));
        QVERIFY(fixture.frame(dragged).left() >= 0.0 && fixture.frame(dragged).right() <= 1920.0);
        VERIFY_INVARIANTS(fixture);
    }

    void viewOffsetGestureIgnoresWrongDevice()
    {
        Fixture fixture(wideColumns());
        fixture.add(QStringLiteral("a"));
        fixture.add(QStringLiteral("b"));
        const auto before = fixture.focused();
        fixture.engine().beginSwipe(QStringLiteral("DP-1"), true);
        fixture.engine().updateSwipe(-2000.0, 10, false);
        fixture.engine().endSwipe(false);
        fixture.settle();
        QCOMPARE(fixture.focused(), before);
        VERIFY_INVARIANTS(fixture);
    }

    void viewOffsetGestureBackToFirstColumn()
    {
        Fixture fixture(wideColumns());
        fixture.add(QStringLiteral("a"));
        fixture.add(QStringLiteral("b"));
        fixture.add(QStringLiteral("c"));
        fixture.engine().beginSwipe(QStringLiteral("DP-1"), true);
        fixture.engine().updateSwipe(-5000.0, 10, true);
        fixture.engine().endSwipe(true);
        fixture.settle();
        QVERIFY(fixture.state(*fixture.focused()).columnIndex < 2);
        QCOMPARE(firstColumnX(fixture), 16.0);
        VERIFY_INVARIANTS(fixture);
    }

    void workspaceSwitchGestureMovesOneWorkspace()
    {
        Fixture fixture;
        fixture.add();
        QCOMPARE(activeWorkspaceIndex(fixture.engine()), 1);
        fixture.engine().beginWorkspaceSwipe(QStringLiteral("DP-1"), true);
        fixture.engine().updateWorkspaceSwipe(300.0, 10, true);
        fixture.engine().endWorkspaceSwipe(true);
        fixture.advance(1);
        QCOMPARE(activeWorkspaceIndex(fixture.engine()), 2);
        VERIFY_INVARIANTS(fixture);
    }

    void workspaceSwitchGestureIsClampedToOneWorkspace()
    {
        Fixture fixture;
        fixture.add();
        fixture.perform(QStringLiteral("focus-workspace-down"));
        fixture.add();
        fixture.perform(QStringLiteral("focus-workspace-down"));
        fixture.advance(1);
        QCOMPARE(activeWorkspaceIndex(fixture.engine()), 3);
        fixture.engine().beginWorkspaceSwipe(QStringLiteral("DP-1"), true);
        fixture.engine().updateWorkspaceSwipe(-5000.0, 10, true);
        fixture.engine().endWorkspaceSwipe(true);
        fixture.advance(1);
        QCOMPARE(activeWorkspaceIndex(fixture.engine()), 2);
        VERIFY_INVARIANTS(fixture);
    }

    void workspaceSwitchGestureSmallSwipeSnapsBack()
    {
        Fixture fixture;
        fixture.add();
        fixture.engine().beginWorkspaceSwipe(QStringLiteral("DP-1"), true);
        fixture.engine().updateWorkspaceSwipe(60.0, 10, true);
        fixture.engine().endWorkspaceSwipe(true);
        fixture.advance(1);
        QCOMPARE(activeWorkspaceIndex(fixture.engine()), 1);
        VERIFY_INVARIANTS(fixture);
    }

    void interactiveResizeChangesWidth()
    {
        Fixture fixture;
        const auto id = fixture.add();
        const double before = fixture.frame(id).width();
        QVERIFY(fixture.engine().beginResize(id, static_cast<quint8>(Layout::ResizeEdge::Right)));
        fixture.engine().updateResize(QPointF(100, 0));
        fixture.settle();
        QCOMPARE(fixture.frame(id).width(), before + 100.0);
        fixture.engine().endResize();
        fixture.settle();
        VERIFY_INVARIANTS(fixture);
    }

    void interactiveResizeByLeftEdge()
    {
        Fixture fixture;
        const auto id = fixture.add();
        const double before = fixture.frame(id).width();
        QVERIFY(fixture.engine().beginResize(id, static_cast<quint8>(Layout::ResizeEdge::Left)));
        fixture.engine().updateResize(QPointF(-80, 0));
        fixture.settle();
        QCOMPARE(fixture.frame(id).width(), before + 80.0);
        fixture.engine().endResize();
        fixture.settle();
        VERIFY_INVARIANTS(fixture);
    }

    void interactiveResizeOfFloatingWindowChangesHeight()
    {
        Fixture fixture;
        const auto id = fixture.add(QStringLiteral("app"), QSizeF(400, 300));
        fixture.perform(QStringLiteral("toggle-window-floating"));
        const double before = fixture.frame(id).height();
        QVERIFY(fixture.engine().beginResize(id, static_cast<quint8>(Layout::ResizeEdge::Bottom)));
        fixture.engine().updateResize(QPointF(0, 60));
        fixture.settle();
        QCOMPARE(fixture.frame(id).height(), before + 60.0);
        fixture.engine().endResize();
        VERIFY_INVARIANTS(fixture);
    }

    void interactiveMoveKeepsWindowUntilThresholdIsCrossed()
    {
        Fixture fixture;
        const auto id = fixture.add(QStringLiteral("a"));
        fixture.add(QStringLiteral("b"));
        QVERIFY(startMove(fixture, id, fixture.frame(id).center() + QPointF(10, 0)));
        QCOMPARE(fixture.state(id).columnIndex, 0);
        fixture.engine().endWindowDrag();
        fixture.settle();
        QCOMPARE(fixture.state(id).columnIndex, 0);
        VERIFY_INVARIANTS(fixture);
    }

    void interactiveMoveDropsWindowIntoNewColumn()
    {
        Fixture fixture;
        const auto first = fixture.add(QStringLiteral("a"));
        const auto second = fixture.add(QStringLiteral("b"));
        QVERIFY(startMove(fixture, second, QPointF(20, 500)));
        QVERIFY(fixture.engine().outputStates().first().dropHint.has_value());
        fixture.engine().endWindowDrag();
        fixture.settle();
        QCOMPARE(fixture.state(second).columnIndex, 0);
        QCOMPARE(fixture.state(first).columnIndex, 1);
        QVERIFY(!fixture.engine().outputStates().first().dropHint.has_value());
        VERIFY_INVARIANTS(fixture);
    }

    void interactiveMoveCanToggleFloating()
    {
        Fixture fixture;
        const auto id = fixture.add(QStringLiteral("a"));
        fixture.add(QStringLiteral("b"));
        QVERIFY(startMove(fixture, id, fixture.frame(id).center() + QPointF(600, 300)));
        fixture.engine().toggleWindowDragFloating();
        fixture.engine().endWindowDrag();
        fixture.settle();
        QVERIFY(fixture.state(id).isFloating);
        VERIFY_INVARIANTS(fixture);
    }

    void interactiveMoveBetweenOutputs()
    {
        Fixture fixture;
        fixture.engine().addOutput(makeOutput(QStringLiteral("DP-2"), QRectF(1920, 0, 1280, 720)));
        const auto id = fixture.add(QStringLiteral("a"));
        QVERIFY(startMove(fixture, id, fixture.frame(id).center() + QPointF(600, 300)));
        fixture.engine().updateWindowDrag(QPointF(2400, 300), QStringLiteral("DP-2"));
        fixture.engine().endWindowDrag();
        fixture.settle();
        QCOMPARE(fixture.state(id).output, QStringLiteral("DP-2"));
        VERIFY_INVARIANTS(fixture);
    }

    void dndEdgeScrollDoesNotBreakInvariants()
    {
        Fixture fixture(wideColumns());
        fixture.add(QStringLiteral("a"));
        fixture.add(QStringLiteral("b"));
        const auto id = fixture.add(QStringLiteral("c"));
        QVERIFY(startMove(fixture, id, QPointF(10, 500)));
        fixture.engine().beginDataDrag();
        fixture.engine().dataDragEdgeScroll(QStringLiteral("DP-1"), QPointF(5, 500), 16);
        fixture.advance(200);
        fixture.engine().dataDragEdgeScroll(QStringLiteral("DP-1"), QPointF(5, 500), 216);
        fixture.advance(200);
        fixture.engine().endDataDrag();
        fixture.engine().endWindowDrag();
        fixture.settle();
        VERIFY_INVARIANTS(fixture);
    }
};

QTEST_GUILESS_MAIN(TestLayoutGestures)
#include "test_layout_gestures.moc"
