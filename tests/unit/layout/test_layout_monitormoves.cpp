#include "actionhelpers.h"

using namespace LayoutTest;

namespace
{

const QString Home = QStringLiteral("DP-1");
const QString Side = QStringLiteral("DP-2");
const QRectF SideGeometry(1920, 300, 1280, 720);

void placeSide(Fixture &fixture)
{
    addOutputAt(fixture, Side, SideGeometry);
}

struct TwoOutputs
{
    Fixture fixture;
    Layout::WindowId resident = 0;
    WindowIds home;

    TwoOutputs()
    {
        placeSide(fixture);
        resident = addOn(fixture, Side, QStringLiteral("resident"));
        fixture.engine().focusOutput(Home);
        home = columnsOf(fixture, 2);
    }
};

void compareOnSide(Fixture &fixture, Layout::WindowId id)
{
    QCOMPARE(fixture.state(id).output, Side);
    QVERIFY(SideGeometry.intersects(fixture.frame(id)));
    QCOMPARE(focusedOutput(fixture), Side);
    VERIFY_INVARIANTS(fixture);
}

}

class TestLayoutMonitorMoves : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void moveWindowToMonitorRightCarriesFocusToTheOtherOutput()
    {
        Fixture fixture;
        placeSide(fixture);
        const WindowIds ids = columnsOf(fixture, 2);
        QVERIFY(act(fixture, QStringLiteral("move-window-to-monitor-right")).ok);
        compareOnSide(fixture, ids[1]);
        COMPARE_FOCUS(fixture, ids[1]);
        QCOMPARE(columnsOn(fixture, Home, 1), Columns {{ids[0]}});
        QVERIFY(act(fixture, QStringLiteral("move-window-to-monitor-left")).ok);
        QCOMPARE(columnsOn(fixture, Home, 1).size(), 2);
        COMPARE_FOCUS(fixture, ids[1]);
    }

    void moveWindowToMonitorTakesOneWindowOutOfAStack()
    {
        Fixture fixture;
        placeSide(fixture);
        const WindowIds ids = stackOf(fixture, 2);
        QVERIFY(act(fixture, QStringLiteral("move-window-to-monitor"), {Side}).ok);
        compareOnSide(fixture, ids[1]);
        QCOMPARE(columnsOn(fixture, Home, 1), Columns {{ids[0]}});
        QCOMPARE(columnsOn(fixture, Side, 1), Columns {{ids[1]}});
    }

    void moveWindowToMonitorKeepsAFloatingWindowAtTheSameRelativePosition()
    {
        Fixture fixture;
        placeSide(fixture);
        fixture.add();
        const auto floating = addFloating(fixture);
        const QRectF before = fixture.frame(floating);
        QVERIFY(act(fixture, QStringLiteral("move-window-to-monitor-next")).ok);
        QVERIFY(fixture.state(floating).isFloating);
        compareOnSide(fixture, floating);
        const QRectF after = fixture.frame(floating);
        QCOMPARE(after.size(), before.size());
        QCOMPARE(qRound((after.x() - SideGeometry.x()) / SideGeometry.width() * 100), qRound(before.x() / 1920.0 * 100));
    }

    void moveWindowToMonitorKeepsAFullscreenWindowCoveringTheNewOutput()
    {
        Fixture fixture;
        placeSide(fixture);
        const auto id = fixture.add();
        act(fixture, QStringLiteral("fullscreen-window"));
        QVERIFY(act(fixture, QStringLiteral("move-window-to-monitor-right")).ok);
        compareOnSide(fixture, id);
        QCOMPARE(fixture.state(id).sizingMode, Layout::WindowMode::Fullscreen);
        QVERIFY(SideGeometry.intersects(fixture.frame(id)));
    }

    void moveWindowToMonitorByIdMovesThatWindow()
    {
        TwoOutputs f;
        const QString id = QString::number(f.home[0]);
        QVERIFY(act(f.fixture, QStringLiteral("move-window-to-monitor"), {Side}, {{QStringLiteral("id"), id}}).ok);
        compareOnSide(f.fixture, f.home[0]);
        QCOMPARE(columnsOn(f.fixture, Home, 1), Columns {{f.home[1]}});
    }

    void moveActionsWithoutAMonitorInThatDirectionChangeNothing()
    {
        TwoOutputs f;
        for (const QString &name : {QStringLiteral("move-window-to-monitor-left"), QStringLiteral("move-column-to-monitor-up"),
                 QStringLiteral("move-workspace-to-monitor-down"), QStringLiteral("move-window-to-monitor"),
                 QStringLiteral("move-column-to-monitor"), QStringLiteral("move-workspace-to-monitor")}) {
            const QStringList arguments = name.endsWith(QStringLiteral("monitor")) ? QStringList {Home} : QStringList {};
            QVERIFY2(act(f.fixture, name, arguments).ok, qPrintable(name));
            QCOMPARE(columnsOn(f.fixture, Home, 1), (Columns {{f.home[0]}, {f.home[1]}}));
            QCOMPARE(focusedOutput(f.fixture), Home);
        }
        QVERIFY(!act(f.fixture, QStringLiteral("move-column-to-monitor"), {QStringLiteral("HDMI-9")}).ok);
        QVERIFY(!act(f.fixture, QStringLiteral("move-workspace-to-monitor"), {QStringLiteral("HDMI-9")}).ok);
        VERIFY_INVARIANTS(f.fixture);
    }

    void moveColumnToMonitorCarriesTheWholeStack()
    {
        Fixture fixture;
        placeSide(fixture);
        const WindowIds ids = stackOf(fixture, 2);
        const auto other = fixture.add();
        act(fixture, QStringLiteral("focus-column-first"));
        QVERIFY(act(fixture, QStringLiteral("move-column-to-monitor-right")).ok);
        QCOMPARE(columnsOn(fixture, Side, 1), Columns {ids});
        QCOMPARE(columnsOn(fixture, Home, 1), Columns {{other}});
        compareOnSide(fixture, ids[0]);
        COMPARE_FOCUS(fixture, ids[1]);
    }

    void moveColumnToMonitorLandsBesideTheTargetsActiveColumn()
    {
        TwoOutputs f;
        QVERIFY(act(f.fixture, QStringLiteral("move-column-to-monitor"), {Side}).ok);
        QCOMPARE(columnsOn(f.fixture, Side, 1), (Columns {{f.resident}, {f.home[1]}}));
        COMPARE_FOCUS(f.fixture, f.home[1]);
    }

    void moveColumnToMonitorMovesAFocusedFloatingWindow()
    {
        Fixture fixture;
        placeSide(fixture);
        const auto tiled = fixture.add();
        const auto floating = addFloating(fixture);
        QVERIFY(act(fixture, QStringLiteral("move-column-to-monitor-right")).ok);
        QVERIFY(fixture.state(floating).isFloating);
        compareOnSide(fixture, floating);
        QCOMPARE(fixture.state(tiled).output, Home);
        COMPARE_FOCUS(fixture, floating);
    }

    void moveColumnToMonitorOnAnEmptyWorkspaceOnlyMovesFocus()
    {
        TwoOutputs f;
        act(f.fixture, QStringLiteral("focus-workspace-down"));
        QVERIFY(act(f.fixture, QStringLiteral("move-column-to-monitor-right")).ok);
        QCOMPARE(columnsOn(f.fixture, Side, 1), Columns {{f.resident}});
        QCOMPARE(focusedOutput(f.fixture), Home);
        VERIFY_INVARIANTS(f.fixture);
    }

    void moveWorkspaceToMonitorTakesItsWindowsAndName()
    {
        TwoOutputs f;
        act(f.fixture, QStringLiteral("set-workspace-name"), {QStringLiteral("work")});
        QVERIFY(act(f.fixture, QStringLiteral("move-workspace-to-monitor-right")).ok);
        QCOMPARE(namesOn(f.fixture, Side), QStringList({QString(), QStringLiteral("work"), QString()}));
        QCOMPARE(namesOn(f.fixture, Home), QStringList({QString()}));
        QCOMPARE(activeWorkspaceOn(f.fixture, Side), 2);
        QCOMPARE(columnsOn(f.fixture, Side, 2), (Columns {{f.home[0]}, {f.home[1]}}));
        compareOnSide(f.fixture, f.home[1]);
        COMPARE_FOCUS(f.fixture, f.home[1]);
    }

    void moveWorkspaceToMonitorLeavesTheOtherWorkspacesBehind()
    {
        TwoOutputs f;
        act(f.fixture, QStringLiteral("focus-workspace-down"));
        const auto lower = f.fixture.add();
        QVERIFY(act(f.fixture, QStringLiteral("move-workspace-to-monitor"), {Side}).ok);
        QCOMPARE(placeOf(f.fixture, lower), std::pair(Side, 2));
        QCOMPARE(workspacesOn(f.fixture, Home).size(), 2);
        QCOMPARE(placeOf(f.fixture, f.home[0]), std::pair(Home, 1));
        QVERIFY(act(f.fixture, QStringLiteral("move-workspace-to-monitor-left")).ok);
        QCOMPARE(placeOf(f.fixture, lower), std::pair(Home, 2));
        QCOMPARE(workspacesOn(f.fixture, Side).size(), 2);
        QCOMPARE(focusedOutput(f.fixture), Home);
        VERIFY_INVARIANTS(f.fixture);
    }

    void moveColumnLeftOrToMonitorLeftCrossesFromTheFirstColumn()
    {
        TwoOutputs f;
        f.fixture.engine().focusOutput(Side);
        const auto second = f.fixture.add();
        QVERIFY(act(f.fixture, QStringLiteral("move-column-left-or-to-monitor-left")).ok);
        QCOMPARE(columnsOn(f.fixture, Side, 1), (Columns {{second}, {f.resident}}));
        QVERIFY(act(f.fixture, QStringLiteral("move-column-left-or-to-monitor-left")).ok);
        QCOMPARE(placeOf(f.fixture, second), std::pair(Home, 1));
        QCOMPARE(focusedOutput(f.fixture), Home);
        COMPARE_FOCUS(f.fixture, second);
        QVERIFY(act(f.fixture, QStringLiteral("move-column-left-or-to-monitor-left")).ok);
        QCOMPARE(placeOf(f.fixture, second), std::pair(Home, 1));
        VERIFY_INVARIANTS(f.fixture);
    }

    void moveColumnRightOrToMonitorRightCrossesFromTheLastColumn()
    {
        TwoOutputs f;
        act(f.fixture, QStringLiteral("focus-column-first"));
        QVERIFY(act(f.fixture, QStringLiteral("move-column-right-or-to-monitor-right")).ok);
        QCOMPARE(columnsOn(f.fixture, Home, 1), (Columns {{f.home[1]}, {f.home[0]}}));
        QVERIFY(act(f.fixture, QStringLiteral("move-column-right-or-to-monitor-right")).ok);
        compareOnSide(f.fixture, f.home[0]);
        QVERIFY(act(f.fixture, QStringLiteral("move-column-right-or-to-monitor-right")).ok);
        QCOMPARE(columnsOn(f.fixture, Side, 1), (Columns {{f.resident}, {f.home[0]}}));
        QCOMPARE(focusedOutput(f.fixture), Side);
    }
};

QTEST_GUILESS_MAIN(TestLayoutMonitorMoves)
#include "test_layout_monitormoves.moc"
