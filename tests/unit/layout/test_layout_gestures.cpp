#include "helpers.h"

#include "layout/common/geometry.h"

#include <cmath>

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

struct AppliedWindow
{
    QRectF current;
    std::optional<QSizeF> requested;
};

QRectF appliedFrame(const Layout::WindowState &state)
{
    return QRectF(state.renderFrame.isEmpty() ? state.targetFrame.topLeft() : state.renderFrame.topLeft(), state.targetFrame.size());
}

int applyLikeKWin(Fixture &fixture, QHash<Layout::WindowId, AppliedWindow> &windows, double scale)
{
    int resizes = 0;
    for (const Layout::WindowState &state : fixture.engine().windowStates()) {
        const QRectF frame = appliedFrame(state);
        AppliedWindow &window = windows[state.id];
        const Layout::GeometryUpdate update = Layout::geometryUpdateFor(window.current, window.requested, frame);
        window.requested = frame.size();
        if (update == Layout::GeometryUpdate::Move) {
            window.current.moveTopLeft(frame.topLeft());
        } else if (update == Layout::GeometryUpdate::MoveResize) {
            const auto shortByOnePixel = [scale](double logical) { return std::floor(std::round(logical) * scale) / scale; };
            window.current = QRectF(frame.topLeft(), QSizeF(shortByOnePixel(frame.width()), shortByOnePixel(frame.height())));
            ++resizes;
        }
    }
    return resizes;
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

    void scrollingTheRowNeverResizesStackedWindows_data()
    {
        QTest::addColumn<int>("rows");
        QTest::newRow("one row") << 1;
        QTest::newRow("two rows") << 2;
        QTest::newRow("three rows") << 3;
    }

    void scrollingTheRowNeverResizesStackedWindows()
    {
        QFETCH(int, rows);
        const double scale = 1.25;
        const QRectF geometry(0, 0, 1478.4, 2368);
        Config::Config config = instantConfig();
        config.layout.defaultColumnWidth = Config::Proportion {1.0};
        Fixture fixture(config, geometry);
        Layout::OutputInfo output = makeOutput(QStringLiteral("DP-1"), geometry, scale);
        output.workArea = QRectF(0, 42.4, 1478.4, 2325.6);
        fixture.engine().updateOutput(output);
        fixture.settle();

        fixture.add(QStringLiteral("left"));
        QList<Layout::WindowId> column;
        for (int row = 0; row < rows; ++row) {
            column.append(fixture.add(QStringLiteral("stacked")));
            if (row > 0) {
                QVERIFY(fixture.perform(QStringLiteral("consume-or-expel-window-left")).ok);
            }
        }
        fixture.add(QStringLiteral("right"));
        fixture.engine().activateWindow(column.first());
        fixture.settle();
        for (const Layout::WindowId id : column) {
            QCOMPARE(fixture.state(id).columnIndex, fixture.state(column.first()).columnIndex);
        }

        QHash<Layout::WindowId, QSizeF> sizes;
        for (const Layout::WindowId id : column) {
            sizes.insert(id, fixture.frame(id).size());
        }
        QHash<Layout::WindowId, AppliedWindow> windows;
        applyLikeKWin(fixture, windows, scale);

        fixture.engine().beginSwipe(QStringLiteral("DP-1"), false);
        int resizes = 0;
        for (int frame = 1; frame <= 120; ++frame) {
            fixture.engine().updateSwipe(frame <= 60 ? 7.3 : -7.3, frame * 16, false);
            fixture.settle();
            for (const Layout::WindowId id : column) {
                QCOMPARE(fixture.frame(id).size(), sizes.value(id));
            }
            resizes += applyLikeKWin(fixture, windows, scale);
        }
        QCOMPARE(resizes, 0);
        fixture.engine().endSwipe(false);
        fixture.settle();
        VERIFY_INVARIANTS(fixture);
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
