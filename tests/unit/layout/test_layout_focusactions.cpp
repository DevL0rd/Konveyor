#include "actionhelpers.h"

using namespace LayoutTest;

namespace
{

struct StackBesideColumn
{
    Fixture fixture;
    WindowIds stack = stackOf(fixture, 2);
    Layout::WindowId single = fixture.add(QStringLiteral("single"));
};

void performEach(Fixture &fixture, const QString &name, const WindowIds &expected)
{
    for (const Layout::WindowId id : expected) {
        QVERIFY(fixture.perform(name).ok);
        COMPARE_FOCUS(fixture, id);
    }
}

}

class TestLayoutFocusActions : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void focusColumnRightOrFirstWrapsFromTheLastColumn()
    {
        Fixture fixture;
        const WindowIds ids = columnsOf(fixture, 3);
        performEach(fixture, QStringLiteral("focus-column-right-or-first"), {ids[0], ids[1], ids[2], ids[0]});
        VERIFY_INVARIANTS(fixture);
    }

    void focusColumnLeftOrLastWrapsFromTheFirstColumn()
    {
        Fixture fixture;
        const WindowIds ids = columnsOf(fixture, 3);
        fixture.perform(QStringLiteral("focus-column-first"));
        performEach(fixture, QStringLiteral("focus-column-left-or-last"), {ids[2], ids[1], ids[0], ids[2]});
        VERIFY_INVARIANTS(fixture);
    }

    void wrappingColumnFocusKeepsASingleColumnFocused()
    {
        Fixture fixture;
        const auto id = fixture.add();
        performEach(fixture, QStringLiteral("focus-column-right-or-first"), {id});
        performEach(fixture, QStringLiteral("focus-column-left-or-last"), {id});
    }

    void focusActionsOnAnEmptyWorkspaceFocusNothing()
    {
        Fixture fixture;
        for (const QString &name :
            {QStringLiteral("focus-column-right-or-first"), QStringLiteral("focus-column-left-or-last"), QStringLiteral("focus-window-top"),
                QStringLiteral("focus-window-down-or-top"), QStringLiteral("focus-window-up-or-bottom"),
                QStringLiteral("focus-window-down-or-column-right"), QStringLiteral("focus-window-up-or-column-left")}) {
            QVERIFY2(fixture.perform(name).ok, qPrintable(name));
            QCOMPARE(fixture.focused(), std::optional<Layout::WindowId>());
        }
        QVERIFY(fixture.perform(QStringLiteral("focus-window-in-column"), {QStringLiteral("2")}).ok);
        QVERIFY(fixture.perform(QStringLiteral("focus-column"), {QStringLiteral("2")}).ok);
        QCOMPARE(fixture.focused(), std::optional<Layout::WindowId>());
        VERIFY_INVARIANTS(fixture);
    }

    void focusWindowUpAndDownWalkTheStackAndStopAtTheEnds()
    {
        Fixture fixture;
        const WindowIds ids = stackOf(fixture, 3);
        COMPARE_FOCUS(fixture, ids[2]);
        performEach(fixture, QStringLiteral("focus-window-up"), {ids[1], ids[0], ids[0]});
        performEach(fixture, QStringLiteral("focus-window-down"), {ids[1], ids[2], ids[2]});
        QCOMPARE(columns(fixture), Columns {ids});
    }

    void focusWindowUpAndDownStayInASingleWindowColumn()
    {
        Fixture fixture;
        const WindowIds ids = columnsOf(fixture, 2);
        performEach(fixture, QStringLiteral("focus-window-up"), {ids[1]});
        performEach(fixture, QStringLiteral("focus-window-down"), {ids[1]});
    }

    void focusWindowTopAndBottomJumpToTheEndsOfTheStack()
    {
        Fixture fixture;
        const WindowIds ids = stackOf(fixture, 3);
        performEach(fixture, QStringLiteral("focus-window-top"), {ids[0], ids[0]});
        performEach(fixture, QStringLiteral("focus-window-bottom"), {ids[2], ids[2]});
    }

    void focusWindowDownOrTopWrapsToTheTop()
    {
        Fixture fixture;
        const WindowIds ids = stackOf(fixture, 3);
        performEach(fixture, QStringLiteral("focus-window-down-or-top"), {ids[0], ids[1], ids[2], ids[0]});
    }

    void focusWindowUpOrBottomWrapsToTheBottom()
    {
        Fixture fixture;
        const WindowIds ids = stackOf(fixture, 3);
        fixture.perform(QStringLiteral("focus-window-top"));
        performEach(fixture, QStringLiteral("focus-window-up-or-bottom"), {ids[2], ids[1], ids[0], ids[2]});
    }

    void focusWindowDownOrColumnRightLeavesTheStackFromTheBottom()
    {
        StackBesideColumn f;
        f.fixture.perform(QStringLiteral("focus-column-left"));
        f.fixture.perform(QStringLiteral("focus-window-top"));
        performEach(f.fixture, QStringLiteral("focus-window-down-or-column-right"), {f.stack[1], f.single, f.single});
    }

    void focusWindowUpOrColumnLeftEntersTheStackAtItsActiveWindow()
    {
        StackBesideColumn f;
        performEach(f.fixture, QStringLiteral("focus-window-up-or-column-left"), {f.stack[1], f.stack[0], f.stack[0]});
    }

    void focusWindowDownOrColumnLeftMovesLeftFromTheBottom()
    {
        StackBesideColumn f;
        performEach(f.fixture, QStringLiteral("focus-window-down-or-column-left"), {f.stack[1], f.stack[1]});
        f.fixture.perform(QStringLiteral("focus-window-top"));
        performEach(f.fixture, QStringLiteral("focus-window-down-or-column-left"), {f.stack[1]});
    }

    void focusWindowUpOrColumnRightMovesRightFromTheTop()
    {
        StackBesideColumn f;
        f.fixture.perform(QStringLiteral("focus-column-left"));
        performEach(f.fixture, QStringLiteral("focus-window-up-or-column-right"), {f.stack[0], f.single, f.single});
    }

    void focusWindowInColumnIsOneBasedAndClampsToTheStack()
    {
        Fixture fixture;
        const WindowIds ids = stackOf(fixture, 3);
        const QList<std::pair<QString, Layout::WindowId>> steps {{QStringLiteral("1"), ids[0]}, {QStringLiteral("3"), ids[2]},
            {QStringLiteral("2"), ids[1]}, {QStringLiteral("0"), ids[0]}, {QStringLiteral("99"), ids[2]}};
        for (const auto &[index, expected] : steps) {
            QVERIFY(fixture.perform(QStringLiteral("focus-window-in-column"), {index}).ok);
            COMPARE_FOCUS(fixture, expected);
        }
        QVERIFY(!fixture.perform(QStringLiteral("focus-window-in-column"), {QStringLiteral("x")}).ok);
        QCOMPARE(columns(fixture), Columns {ids});
    }

    void focusColumnClampsOutOfRangeIndices()
    {
        Fixture fixture;
        const WindowIds ids = columnsOf(fixture, 3);
        QVERIFY(fixture.perform(QStringLiteral("focus-column"), {QStringLiteral("0")}).ok);
        COMPARE_FOCUS(fixture, ids[0]);
        QVERIFY(fixture.perform(QStringLiteral("focus-column"), {QStringLiteral("99")}).ok);
        COMPARE_FOCUS(fixture, ids[2]);
    }

    void verticalFocusFromAFloatingWindowGoesBackToTheStack()
    {
        Fixture fixture;
        const WindowIds ids = stackOf(fixture, 2);
        const auto floating = addFloating(fixture);
        COMPARE_FOCUS(fixture, floating);
        QVERIFY(fixture.perform(QStringLiteral("focus-window-top")).ok);
        COMPARE_FOCUS(fixture, ids[0]);
        fixture.perform(QStringLiteral("focus-floating"));
        QVERIFY(fixture.perform(QStringLiteral("focus-window-in-column"), {QStringLiteral("2")}).ok);
        COMPARE_FOCUS(fixture, ids[1]);
        QVERIFY(fixture.state(floating).isFloating);
    }

    void focusWindowOrWorkspaceDownWalksTheStackThenSwitchesWorkspace()
    {
        Fixture fixture;
        const WindowIds ids = stackOf(fixture, 2);
        fixture.perform(QStringLiteral("focus-workspace-down"));
        const auto below = fixture.add();
        fixture.perform(QStringLiteral("focus-workspace-up"));
        fixture.perform(QStringLiteral("focus-window-top"));
        performEach(fixture, QStringLiteral("focus-window-or-workspace-down"), {ids[1], below});
        QCOMPARE(activeWorkspaceOn(fixture, QStringLiteral("DP-1")), 2);
        QVERIFY(fixture.perform(QStringLiteral("focus-window-or-workspace-down")).ok);
        QCOMPARE(activeWorkspaceOn(fixture, QStringLiteral("DP-1")), 3);
        QCOMPARE(fixture.focused(), std::optional<Layout::WindowId>());
        QVERIFY(fixture.perform(QStringLiteral("focus-window-or-workspace-down")).ok);
        QCOMPARE(activeWorkspaceOn(fixture, QStringLiteral("DP-1")), 3);
        VERIFY_INVARIANTS(fixture);
    }

    void focusWindowOrWorkspaceUpWalksTheStackThenSwitchesWorkspace()
    {
        Fixture fixture;
        const auto above = fixture.add();
        fixture.perform(QStringLiteral("focus-workspace-down"));
        const WindowIds ids = stackOf(fixture, 2);
        performEach(fixture, QStringLiteral("focus-window-or-workspace-up"), {ids[0], above, above});
        QCOMPARE(activeWorkspaceOn(fixture, QStringLiteral("DP-1")), 1);
        VERIFY_INVARIANTS(fixture);
    }
};

QTEST_GUILESS_MAIN(TestLayoutFocusActions)
#include "test_layout_focusactions.moc"
