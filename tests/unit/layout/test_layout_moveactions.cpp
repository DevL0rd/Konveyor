#include "actionhelpers.h"

using namespace LayoutTest;

namespace
{

struct ColumnAndStack
{
    Fixture fixture;
    Layout::WindowId single = fixture.add(QStringLiteral("single"));
    WindowIds stack = stackBeside();

    WindowIds stackBeside()
    {
        const Layout::WindowId top = fixture.add(QStringLiteral("top"));
        const Layout::WindowId bottom = fixture.add(QStringLiteral("bottom"));
        fixture.perform(QStringLiteral("consume-or-expel-window-left"));
        return {top, bottom};
    }
};

void performAndCompare(Fixture &fixture, const QString &name, const Columns &expected)
{
    QVERIFY2(fixture.perform(name).ok, qPrintable(name));
    QCOMPARE(columns(fixture), expected);
    VERIFY_INVARIANTS(fixture);
}

}

class TestLayoutMoveActions : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void moveColumnToLastPutsTheFocusedColumnAtTheEnd()
    {
        Fixture fixture;
        const WindowIds ids = columnsOf(fixture, 3);
        fixture.perform(QStringLiteral("focus-column-first"));
        performAndCompare(fixture, QStringLiteral("move-column-to-last"), {{ids[1]}, {ids[2]}, {ids[0]}});
        COMPARE_FOCUS(fixture, ids[0]);
        performAndCompare(fixture, QStringLiteral("move-column-to-last"), {{ids[1]}, {ids[2]}, {ids[0]}});
    }

    void moveColumnToFirstPutsTheFocusedColumnAtTheStart()
    {
        Fixture fixture;
        const WindowIds ids = columnsOf(fixture, 3);
        performAndCompare(fixture, QStringLiteral("move-column-to-first"), {{ids[2]}, {ids[0]}, {ids[1]}});
        COMPARE_FOCUS(fixture, ids[2]);
        performAndCompare(fixture, QStringLiteral("move-column-to-first"), {{ids[2]}, {ids[0]}, {ids[1]}});
    }

    void moveColumnToLastCarriesTheWholeStack()
    {
        ColumnAndStack f;
        f.fixture.perform(QStringLiteral("focus-column-first"));
        performAndCompare(f.fixture, QStringLiteral("move-column-to-last"), {f.stack, {f.single}});
        f.fixture.perform(QStringLiteral("focus-column-first"));
        COMPARE_FOCUS(f.fixture, f.stack[1]);
    }

    void moveColumnToIndexClampsOutOfRangeIndices()
    {
        Fixture fixture;
        const WindowIds ids = columnsOf(fixture, 3);
        fixture.perform(QStringLiteral("focus-column-first"));
        QVERIFY(fixture.perform(QStringLiteral("move-column-to-index"), {QStringLiteral("99")}).ok);
        QCOMPARE(columns(fixture), (Columns {{ids[1]}, {ids[2]}, {ids[0]}}));
        QVERIFY(fixture.perform(QStringLiteral("move-column-to-index"), {QStringLiteral("0")}).ok);
        QCOMPARE(columns(fixture), (Columns {{ids[0]}, {ids[1]}, {ids[2]}}));
        QVERIFY(!fixture.perform(QStringLiteral("move-column-to-index"), {QStringLiteral("-1")}).ok);
        COMPARE_FOCUS(fixture, ids[0]);
    }

    void moveColumnToIndexWithAnIdMovesThatColumnAndKeepsFocusAndView()
    {
        Fixture fixture;
        const WindowIds ids = columnsOf(fixture, 4);
        fixture.perform(QStringLiteral("focus-column"), {QStringLiteral("2")});
        const QRectF focusedFrame = fixture.engine().windowState(ids[1])->targetFrame;
        const auto moveTo = [&fixture](Layout::WindowId id, const QString &index) {
            return fixture.perform(QStringLiteral("move-column-to-index"), {index}, {{QStringLiteral("id"), QString::number(id)}}).ok;
        };
        QVERIFY(moveTo(ids[0], QStringLiteral("4")));
        QCOMPARE(columns(fixture), (Columns {{ids[1]}, {ids[2]}, {ids[3]}, {ids[0]}}));
        COMPARE_FOCUS(fixture, ids[1]);
        QCOMPARE(fixture.engine().windowState(ids[1])->targetFrame, focusedFrame);
        QVERIFY(moveTo(ids[3], QStringLiteral("1")));
        QCOMPARE(columns(fixture), (Columns {{ids[3]}, {ids[1]}, {ids[2]}, {ids[0]}}));
        COMPARE_FOCUS(fixture, ids[1]);
        QVERIFY(moveTo(ids[2], QStringLiteral("3")));
        QCOMPARE(columns(fixture), (Columns {{ids[3]}, {ids[1]}, {ids[2]}, {ids[0]}}));
        QVERIFY(moveTo(ids[1], QStringLiteral("4")));
        QCOMPARE(columns(fixture), (Columns {{ids[3]}, {ids[2]}, {ids[0]}, {ids[1]}}));
        COMPARE_FOCUS(fixture, ids[1]);
        QVERIFY(moveTo(999, QStringLiteral("1")));
        QCOMPARE(columns(fixture), (Columns {{ids[3]}, {ids[2]}, {ids[0]}, {ids[1]}}));
        VERIFY_INVARIANTS(fixture);
    }

    void workspaceStatesListTheColumnsInOrder()
    {
        ColumnAndStack f;
        const Layout::WorkspaceState state = workspacesOn(f.fixture, QStringLiteral("DP-1")).first();
        QCOMPARE(state.columns, (QList<QList<Layout::WindowId>> {{f.single}, f.stack}));
        QCOMPARE(state.groupAppWindows, Config::GroupAppWindows::Beside);
        QVERIFY(workspacesOn(f.fixture, QStringLiteral("DP-1")).last().columns.isEmpty());
    }

    void columnMovesOnASingleColumnChangeNothing()
    {
        Fixture fixture;
        const auto id = fixture.add();
        for (const QString &name : {QStringLiteral("move-column-left"), QStringLiteral("move-column-right"),
                 QStringLiteral("move-column-to-first"), QStringLiteral("move-column-to-last"), QStringLiteral("move-window-up"),
                 QStringLiteral("move-window-down"), QStringLiteral("swap-window-left"), QStringLiteral("swap-window-right"),
                 QStringLiteral("consume-window-into-column"), QStringLiteral("expel-window-from-column")}) {
            performAndCompare(fixture, name, {{id}});
        }
        COMPARE_FOCUS(fixture, id);
    }

    void columnMovesOnAnEmptyWorkspaceSucceed()
    {
        Fixture fixture;
        for (const QString &name :
            {QStringLiteral("move-column-to-last"), QStringLiteral("move-window-up"), QStringLiteral("consume-window-into-column"),
                QStringLiteral("expel-window-from-column"), QStringLiteral("swap-window-right")}) {
            performAndCompare(fixture, name, {});
        }
    }

    void moveWindowUpAndDownReorderTheStackAndKeepFocus()
    {
        Fixture fixture;
        const WindowIds ids = stackOf(fixture, 3);
        performAndCompare(fixture, QStringLiteral("move-window-up"), {{ids[0], ids[2], ids[1]}});
        performAndCompare(fixture, QStringLiteral("move-window-up"), {{ids[2], ids[0], ids[1]}});
        performAndCompare(fixture, QStringLiteral("move-window-up"), {{ids[2], ids[0], ids[1]}});
        COMPARE_FOCUS(fixture, ids[2]);
        performAndCompare(fixture, QStringLiteral("move-window-down"), {{ids[0], ids[2], ids[1]}});
        performAndCompare(fixture, QStringLiteral("move-window-down"), {{ids[0], ids[1], ids[2]}});
        performAndCompare(fixture, QStringLiteral("move-window-down"), {{ids[0], ids[1], ids[2]}});
        COMPARE_FOCUS(fixture, ids[2]);
    }

    void movedWindowsSwapTheirFrames()
    {
        Fixture fixture;
        const WindowIds ids = stackOf(fixture, 2);
        const QRectF top = fixture.frame(ids[0]);
        const QRectF bottom = fixture.frame(ids[1]);
        fixture.perform(QStringLiteral("move-window-up"));
        QCOMPARE(fixture.frame(ids[1]).topLeft(), top.topLeft());
        QCOMPARE(fixture.frame(ids[0]).bottomLeft(), bottom.bottomLeft());
    }

    void consumeWindowIntoColumnPullsTheNextColumnBelow()
    {
        Fixture fixture;
        const WindowIds ids = columnsOf(fixture, 3);
        fixture.perform(QStringLiteral("focus-column-first"));
        performAndCompare(fixture, QStringLiteral("consume-window-into-column"), {{ids[0], ids[1]}, {ids[2]}});
        COMPARE_FOCUS(fixture, ids[0]);
        performAndCompare(fixture, QStringLiteral("consume-window-into-column"), {{ids[0], ids[1], ids[2]}});
        COMPARE_FOCUS(fixture, ids[0]);
    }

    void consumeWindowIntoColumnTakesOnlyTheTopOfAStack()
    {
        ColumnAndStack f;
        f.fixture.perform(QStringLiteral("focus-column-first"));
        performAndCompare(f.fixture, QStringLiteral("consume-window-into-column"), {{f.single, f.stack[0]}, {f.stack[1]}});
    }

    void consumeWindowIntoColumnDoesNothingOnTheLastColumn()
    {
        Fixture fixture;
        const WindowIds ids = columnsOf(fixture, 2);
        performAndCompare(fixture, QStringLiteral("consume-window-into-column"), {{ids[0]}, {ids[1]}});
    }

    void expelWindowFromColumnMovesTheBottomWindowIntoANewColumnOnTheRight()
    {
        Fixture fixture;
        const WindowIds ids = stackOf(fixture, 3);
        const auto right = fixture.add(QStringLiteral("right"));
        fixture.perform(QStringLiteral("focus-column-first"));
        fixture.perform(QStringLiteral("focus-window-top"));
        performAndCompare(fixture, QStringLiteral("expel-window-from-column"), {{ids[0], ids[1]}, {ids[2]}, {right}});
        COMPARE_FOCUS(fixture, ids[0]);
        performAndCompare(fixture, QStringLiteral("expel-window-from-column"), {{ids[0]}, {ids[1]}, {ids[2]}, {right}});
    }

    void swapWindowRightExchangesStackedWindowsAndFollowsTheWindow()
    {
        ColumnAndStack f;
        f.fixture.perform(QStringLiteral("focus-column-first"));
        f.fixture.perform(QStringLiteral("consume-window-into-column"));
        QCOMPARE(columns(f.fixture), (Columns {{f.single, f.stack[0]}, {f.stack[1]}}));
        f.fixture.perform(QStringLiteral("focus-window-bottom"));
        performAndCompare(f.fixture, QStringLiteral("swap-window-right"), {{f.single, f.stack[1]}, {f.stack[0]}});
        COMPARE_FOCUS(f.fixture, f.stack[0]);
    }

    void swapWindowRightFromASingleWindowColumnLeavesTheOtherWindowBehind()
    {
        ColumnAndStack f;
        f.fixture.perform(QStringLiteral("focus-column-first"));
        performAndCompare(f.fixture, QStringLiteral("swap-window-right"), {{f.stack[1]}, {f.stack[0], f.single}});
        COMPARE_FOCUS(f.fixture, f.single);
    }

    void swapWindowLeftIntoASingleWindowColumn()
    {
        ColumnAndStack f;
        performAndCompare(f.fixture, QStringLiteral("swap-window-left"), {{f.stack[1]}, {f.stack[0], f.single}});
        COMPARE_FOCUS(f.fixture, f.stack[1]);
    }

    void swapWindowBetweenSingleWindowColumnsMovesTheColumn()
    {
        Fixture fixture;
        const WindowIds ids = columnsOf(fixture, 3);
        performAndCompare(fixture, QStringLiteral("swap-window-left"), {{ids[0]}, {ids[2]}, {ids[1]}});
        performAndCompare(fixture, QStringLiteral("swap-window-right"), {{ids[0]}, {ids[1]}, {ids[2]}});
        performAndCompare(fixture, QStringLiteral("swap-window-right"), {{ids[0]}, {ids[1]}, {ids[2]}});
        COMPARE_FOCUS(fixture, ids[2]);
    }

    void tilingMovesLeaveAFocusedFloatingWindowAlone()
    {
        Fixture fixture;
        const WindowIds ids = columnsOf(fixture, 2);
        const auto floating = addFloating(fixture);
        const QRectF frame = fixture.frame(floating);
        for (const QString &name :
            {QStringLiteral("move-column-to-first"), QStringLiteral("move-column-to-last"), QStringLiteral("consume-window-into-column"),
                QStringLiteral("expel-window-from-column"), QStringLiteral("swap-window-left"), QStringLiteral("move-window-up")}) {
            performAndCompare(fixture, name, {{ids[0]}, {ids[1]}});
        }
        COMPARE_FOCUS(fixture, floating);
        QCOMPARE(fixture.frame(floating), frame);
    }

    void moveWindowDownOrToWorkspaceDownLeavesTheStackAtTheBottom()
    {
        Fixture fixture;
        const WindowIds ids = stackOf(fixture, 2);
        fixture.perform(QStringLiteral("focus-window-top"));
        performAndCompare(fixture, QStringLiteral("move-window-down-or-to-workspace-down"), {{ids[1], ids[0]}});
        QVERIFY(fixture.perform(QStringLiteral("move-window-down-or-to-workspace-down")).ok);
        QCOMPARE(placeOf(fixture, ids[0]), std::pair(QStringLiteral("DP-1"), 2));
        QCOMPARE(placeOf(fixture, ids[1]), std::pair(QStringLiteral("DP-1"), 1));
        QCOMPARE(activeWorkspaceOn(fixture, QStringLiteral("DP-1")), 2);
        COMPARE_FOCUS(fixture, ids[0]);
        QCOMPARE(workspacesOn(fixture, QStringLiteral("DP-1")).size(), 3);
        VERIFY_INVARIANTS(fixture);
    }

    void moveWindowUpOrToWorkspaceUpLeavesTheStackAtTheTop()
    {
        Fixture fixture;
        const auto above = fixture.add();
        fixture.perform(QStringLiteral("focus-workspace-down"));
        const WindowIds ids = stackOf(fixture, 2);
        QVERIFY(fixture.perform(QStringLiteral("move-window-up-or-to-workspace-up")).ok);
        QCOMPARE(columnsOn(fixture, QStringLiteral("DP-1"), 2), (Columns {{ids[1], ids[0]}}));
        QVERIFY(fixture.perform(QStringLiteral("move-window-up-or-to-workspace-up")).ok);
        QCOMPARE(placeOf(fixture, ids[1]), std::pair(QStringLiteral("DP-1"), 1));
        QCOMPARE(columnsOn(fixture, QStringLiteral("DP-1"), 1).size(), 2);
        QVERIFY(columnsOn(fixture, QStringLiteral("DP-1"), 1).contains(WindowIds {above}));
        COMPARE_FOCUS(fixture, ids[1]);
        QVERIFY(fixture.perform(QStringLiteral("move-window-up-or-to-workspace-up")).ok);
        QCOMPARE(placeOf(fixture, ids[1]), std::pair(QStringLiteral("DP-1"), 1));
        VERIFY_INVARIANTS(fixture);
    }
};

QTEST_GUILESS_MAIN(TestLayoutMoveActions)
#include "test_layout_moveactions.moc"
