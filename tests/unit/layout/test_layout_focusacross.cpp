#include "actionhelpers.h"

using namespace LayoutTest;

namespace
{

const QString Main = QStringLiteral("DP-1");
const QString Other = QStringLiteral("DP-2");

Layout::ActionResult focusById(Fixture &fixture, Layout::WindowId id)
{
    return act(fixture, QStringLiteral("focus-window"), {}, {{QStringLiteral("id"), QString::number(id)}});
}

struct SpreadWindows
{
    Fixture fixture;
    Layout::WindowId first = fixture.add(QStringLiteral("first"));
    Layout::WindowId below = addBelow();
    Layout::WindowId beside = addBeside();

    Layout::WindowId addBelow()
    {
        act(fixture, QStringLiteral("focus-workspace-down"));
        return fixture.add(QStringLiteral("below"));
    }

    Layout::WindowId addBeside()
    {
        addOutputAt(fixture, Other, QRectF(1920, 0, 1920, 1080));
        return addOn(fixture, Other, QStringLiteral("beside"));
    }
};

WindowIds tabbedStack(Fixture &fixture)
{
    const WindowIds ids = stackOf(fixture, 3);
    act(fixture, QStringLiteral("toggle-column-tabbed-display"));
    return ids;
}

}

class TestLayoutFocusAcross : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void focusWindowByIdSwitchesWorkspaceAndOutput()
    {
        SpreadWindows f;
        QVERIFY(focusById(f.fixture, f.first).ok);
        QCOMPARE(focusedOutput(f.fixture), Main);
        QCOMPARE(activeWorkspaceOn(f.fixture, Main), 1);
        COMPARE_FOCUS(f.fixture, f.first);
        QVERIFY(focusById(f.fixture, f.beside).ok);
        QCOMPARE(focusedOutput(f.fixture), Other);
        QVERIFY(focusById(f.fixture, f.below).ok);
        QCOMPARE(activeWorkspaceOn(f.fixture, Main), 2);
        COMPARE_FOCUS(f.fixture, f.below);
    }

    void focusWindowPreviousCrossesWorkspacesAndOutputs()
    {
        SpreadWindows f;
        focusById(f.fixture, f.first);
        QVERIFY(act(f.fixture, QStringLiteral("focus-window-previous")).ok);
        COMPARE_FOCUS(f.fixture, f.beside);
        QCOMPARE(focusedOutput(f.fixture), Other);
        QVERIFY(act(f.fixture, QStringLiteral("focus-window-previous")).ok);
        COMPARE_FOCUS(f.fixture, f.first);
        QCOMPARE(activeWorkspaceOn(f.fixture, Main), 1);
    }

    void focusWindowPreviousSkipsClosedWindows()
    {
        SpreadWindows f;
        focusById(f.fixture, f.first);
        f.fixture.remove(f.beside);
        f.fixture.advance(1);
        QVERIFY(act(f.fixture, QStringLiteral("focus-window-previous")).ok);
        COMPARE_FOCUS(f.fixture, f.below);
        QCOMPARE(activeWorkspaceOn(f.fixture, Main), 2);
    }

    void focusWindowPreviousWithASingleWindowKeepsIt()
    {
        Fixture fixture;
        const auto id = fixture.add();
        QVERIFY(act(fixture, QStringLiteral("focus-window-previous")).ok);
        COMPARE_FOCUS(fixture, id);
    }

    void focusWindowDownInATabbedColumnSwitchesTheVisibleTab()
    {
        Fixture fixture;
        const WindowIds ids = tabbedStack(fixture);
        QVERIFY(act(fixture, QStringLiteral("focus-window-top")).ok);
        COMPARE_FOCUS(fixture, ids[0]);
        QVERIFY(fixture.state(ids[0]).visible);
        QVERIFY(!fixture.state(ids[2]).visible);
        QVERIFY(act(fixture, QStringLiteral("focus-window-down")).ok);
        COMPARE_FOCUS(fixture, ids[1]);
        QVERIFY(fixture.state(ids[1]).visible);
        QVERIFY(!fixture.state(ids[0]).visible);
        QVERIFY(act(fixture, QStringLiteral("focus-window-in-column"), {QStringLiteral("3")}).ok);
        QVERIFY(fixture.state(ids[2]).visible);
    }

    void moveWindowUpInATabbedColumnReordersTheTabs()
    {
        Fixture fixture;
        const WindowIds ids = tabbedStack(fixture);
        QVERIFY(act(fixture, QStringLiteral("move-window-up")).ok);
        QCOMPARE(columns(fixture), (Columns {{ids[0], ids[2], ids[1]}}));
        COMPARE_FOCUS(fixture, ids[2]);
        QVERIFY(fixture.state(ids[2]).visible);
        QCOMPARE(fixture.state(ids[2]).tabBar.tabRects.size(), 3);
    }

    void movingATabbedColumnToAnotherWorkspaceKeepsItTabbed()
    {
        Fixture fixture;
        const WindowIds ids = tabbedStack(fixture);
        QVERIFY(act(fixture, QStringLiteral("move-column-to-workspace-down")).ok);
        QCOMPARE(columnsOn(fixture, Main, 1), Columns {ids});
        QVERIFY(fixture.state(ids[2]).visible);
        QVERIFY(!fixture.state(ids[0]).visible);
        QVERIFY(fixture.state(ids[2]).tabBar.visible);
    }

    void movingOneWindowOutOfAStackToAnotherWorkspaceLeavesTheRest()
    {
        Fixture fixture;
        const WindowIds ids = stackOf(fixture, 3);
        act(fixture, QStringLiteral("focus-window-in-column"), {QStringLiteral("2")});
        QVERIFY(act(fixture, QStringLiteral("move-window-to-workspace-down")).ok);
        QCOMPARE(columnsOn(fixture, Main, 1), (Columns {{ids[0], ids[2]}}));
        QCOMPARE(columnsOn(fixture, Main, 2), Columns {{ids[1]}});
        COMPARE_FOCUS(fixture, ids[1]);
        act(fixture, QStringLiteral("focus-workspace-up"));
        COMPARE_FOCUS(fixture, ids[2]);
    }

    void fullscreenWindowStaysFullscreenAcrossWorkspaceMoves()
    {
        Fixture fixture;
        fixture.add();
        const auto id = fixture.add();
        act(fixture, QStringLiteral("fullscreen-window"));
        QVERIFY(act(fixture, QStringLiteral("move-window-to-workspace-down")).ok);
        QCOMPARE(fixture.state(id).sizingMode, Layout::WindowMode::Fullscreen);
        QCOMPARE(fixture.frame(id), QRectF(0, 0, 1920, 1080));
        QVERIFY(act(fixture, QStringLiteral("move-column-to-workspace-up")).ok);
        QCOMPARE(fixture.state(id).sizingMode, Layout::WindowMode::Fullscreen);
        QCOMPARE(fixture.frame(id), QRectF(0, 0, 1920, 1080));
        VERIFY_INVARIANTS(fixture);
    }

    void focusMovesAwayFromAFullscreenWindowAndBack()
    {
        Fixture fixture;
        const auto left = fixture.add();
        const auto id = fixture.add();
        act(fixture, QStringLiteral("fullscreen-window"));
        QVERIFY(act(fixture, QStringLiteral("focus-column-left")).ok);
        COMPARE_FOCUS(fixture, left);
        QVERIFY(act(fixture, QStringLiteral("focus-column-right-or-first")).ok);
        COMPARE_FOCUS(fixture, id);
        QCOMPARE(fixture.frame(id), QRectF(0, 0, 1920, 1080));
        QVERIFY(act(fixture, QStringLiteral("move-column-to-first")).ok);
        QCOMPARE(columns(fixture), (Columns {{id}, {left}}));
        QCOMPARE(fixture.frame(id), QRectF(0, 0, 1920, 1080));
        VERIFY_INVARIANTS(fixture);
    }
};

QTEST_GUILESS_MAIN(TestLayoutFocusAcross)
#include "test_layout_focusacross.moc"
