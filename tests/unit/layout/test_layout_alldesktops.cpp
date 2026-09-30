#include "actionhelpers.h"

using namespace LayoutTest;

namespace
{

Layout::WindowProperties pinnedWindow(const QString &appId)
{
    Layout::WindowProperties properties = makeWindow(appId, appId);
    properties.onAllDesktops = true;
    return properties;
}

void setOnAllDesktops(Fixture &fixture, Layout::WindowId id, const QString &appId, bool onAll)
{
    Layout::WindowProperties properties = makeWindow(appId, appId);
    properties.onAllDesktops = onAll;
    fixture.engine().updateWindowProperties(id, properties);
    fixture.settle();
}

bool showsOnActiveWorkspace(Fixture &fixture, Layout::WindowId id)
{
    const Layout::WindowState state = fixture.state(id);
    return state.onAllDesktops && state.onActiveWorkspace && state.workspaceIndex == activeWorkspaceOn(fixture, state.output);
}

}

class TestLayoutAllDesktops : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void pinnedWindowStaysTiledAndFollowsWorkspaceSwitches()
    {
        Fixture fixture;
        const auto pinned = fixture.add(QStringLiteral("player"));
        setOnAllDesktops(fixture, pinned, QStringLiteral("player"), true);
        QVERIFY(!fixture.state(pinned).isFloating);
        for (const QString &action : {QStringLiteral("focus-workspace-down"), QStringLiteral("focus-workspace-down"),
                 QStringLiteral("focus-workspace-up"), QStringLiteral("focus-workspace-up")}) {
            QVERIFY(fixture.perform(action).ok);
            QVERIFY2(showsOnActiveWorkspace(fixture, pinned), qPrintable(action));
            QVERIFY(!fixture.state(pinned).isFloating);
            VERIFY_INVARIANTS(fixture);
        }
    }

    void pinnedWindowKeepsItsColumnPosition()
    {
        Fixture fixture;
        const auto a = fixture.add(QStringLiteral("a"));
        QVERIFY(fixture.perform(QStringLiteral("focus-workspace-down")).ok);
        const auto b = fixture.add(QStringLiteral("b"));
        const auto c = fixture.add(QStringLiteral("c"));
        const auto d = fixture.add(QStringLiteral("d"));
        QVERIFY(fixture.perform(QStringLiteral("focus-workspace-up")).ok);
        const auto pinned = fixture.add(QStringLiteral("player"));
        setOnAllDesktops(fixture, pinned, QStringLiteral("player"), true);
        QCOMPARE(fixture.state(pinned).columnIndex, 1);
        QVERIFY(fixture.perform(QStringLiteral("focus-workspace-down")).ok);
        QCOMPARE(fixture.state(pinned).workspace, fixture.state(b).workspace);
        QCOMPARE(fixture.state(b).columnIndex, 0);
        QCOMPARE(fixture.state(pinned).columnIndex, 1);
        QCOMPARE(fixture.state(c).columnIndex, 2);
        QCOMPARE(fixture.focused(), std::optional(d));
        QVERIFY(fixture.perform(QStringLiteral("focus-workspace-up")).ok);
        QCOMPARE(fixture.state(pinned).workspace, fixture.state(a).workspace);
        QCOMPARE(fixture.state(pinned).columnIndex, 1);
        VERIFY_INVARIANTS(fixture);
    }

    void pinnedWindowOpenedPinnedFollowsToo()
    {
        Fixture fixture;
        const auto pinned = fixture.addWith(pinnedWindow(QStringLiteral("player")));
        QVERIFY(fixture.state(pinned).onAllDesktops);
        QVERIFY(fixture.perform(QStringLiteral("focus-workspace-down")).ok);
        QVERIFY(showsOnActiveWorkspace(fixture, pinned));
        VERIFY_INVARIANTS(fixture);
    }

    void floatingPinnedWindowFollowsAndStaysFloating()
    {
        Fixture fixture;
        fixture.add(QStringLiteral("a"));
        const auto pinned = fixture.add(QStringLiteral("player"));
        QVERIFY(fixture.perform(QStringLiteral("toggle-window-floating")).ok);
        setOnAllDesktops(fixture, pinned, QStringLiteral("player"), true);
        const QRectF before = fixture.frame(pinned);
        QVERIFY(fixture.perform(QStringLiteral("focus-workspace-down")).ok);
        QVERIFY(showsOnActiveWorkspace(fixture, pinned));
        QVERIFY(fixture.state(pinned).isFloating);
        QCOMPARE(fixture.frame(pinned), before);
        VERIFY_INVARIANTS(fixture);
    }

    void unpinnedWindowStaysOnItsWorkspace()
    {
        Fixture fixture;
        const auto pinned = fixture.add(QStringLiteral("player"));
        setOnAllDesktops(fixture, pinned, QStringLiteral("player"), true);
        QVERIFY(fixture.perform(QStringLiteral("focus-workspace-down")).ok);
        setOnAllDesktops(fixture, pinned, QStringLiteral("player"), false);
        const Layout::WorkspaceId stays = fixture.state(pinned).workspace;
        QVERIFY(!fixture.state(pinned).onAllDesktops);
        QVERIFY(fixture.perform(QStringLiteral("focus-workspace-up")).ok);
        QCOMPARE(fixture.state(pinned).workspace, stays);
        QVERIFY(!fixture.state(pinned).onActiveWorkspace);
        VERIFY_INVARIANTS(fixture);
    }

    void goingDownAgainAndAgainDoesNotPileUpWorkspaces()
    {
        Fixture fixture;
        const auto pinned = fixture.add(QStringLiteral("player"));
        setOnAllDesktops(fixture, pinned, QStringLiteral("player"), true);
        const auto count = [&fixture] { return fixture.engine().workspaceStates().size(); };
        QVERIFY(fixture.perform(QStringLiteral("focus-workspace-down")).ok);
        fixture.advance(1);
        const auto settled = count();
        for (int step = 0; step < 5; ++step) {
            QVERIFY(fixture.perform(QStringLiteral("focus-workspace-down")).ok);
            fixture.advance(1);
            QCOMPARE(count(), settled);
            QVERIFY(showsOnActiveWorkspace(fixture, pinned));
        }
        VERIFY_INVARIANTS(fixture);
    }

    void pinnedWindowFollowsAnimatedSwitchesAndSurvivesTheirEnd()
    {
        Fixture fixture(linearAnimationConfig());
        fixture.add(QStringLiteral("a"));
        const auto pinned = fixture.add(QStringLiteral("player"));
        setOnAllDesktops(fixture, pinned, QStringLiteral("player"), true);
        QVERIFY(fixture.perform(QStringLiteral("focus-workspace-down")).ok);
        QVERIFY(showsOnActiveWorkspace(fixture, pinned));
        fixture.advanceInSteps(400);
        QVERIFY(fixture.perform(QStringLiteral("focus-workspace-up")).ok);
        fixture.advanceInSteps(100);
        QVERIFY(fixture.perform(QStringLiteral("focus-workspace-down")).ok);
        fixture.advanceInSteps(400);
        QVERIFY(showsOnActiveWorkspace(fixture, pinned));
        VERIFY_INVARIANTS(fixture);
    }

    void pinnedWindowStaysOnItsOwnMonitor()
    {
        Fixture fixture;
        fixture.addOutput(QStringLiteral("DP-2"), QRectF(1920, 0, 1920, 1080));
        const auto pinned = addOn(fixture, QStringLiteral("DP-1"), QStringLiteral("player"));
        setOnAllDesktops(fixture, pinned, QStringLiteral("player"), true);
        const Layout::WorkspaceId home = fixture.state(pinned).workspace;
        fixture.engine().focusOutput(QStringLiteral("DP-2"));
        QVERIFY(fixture.perform(QStringLiteral("focus-workspace-down")).ok);
        QCOMPARE(fixture.state(pinned).workspace, home);
        QCOMPARE(fixture.state(pinned).output, QStringLiteral("DP-1"));
        QVERIFY(fixture.perform(QStringLiteral("move-window-to-monitor"), {QStringLiteral("DP-2")}, idProperty(pinned)).ok);
        QCOMPARE(fixture.state(pinned).output, QStringLiteral("DP-2"));
        QVERIFY(showsOnActiveWorkspace(fixture, pinned));
        fixture.engine().focusOutput(QStringLiteral("DP-2"));
        QVERIFY(fixture.perform(QStringLiteral("focus-workspace-up")).ok);
        QVERIFY(showsOnActiveWorkspace(fixture, pinned));
        VERIFY_INVARIANTS(fixture);
    }

    void pinnedWindowMovedToAnotherWorkspaceComesBack()
    {
        Fixture fixture;
        fixture.add(QStringLiteral("a"));
        const auto pinned = fixture.add(QStringLiteral("player"));
        setOnAllDesktops(fixture, pinned, QStringLiteral("player"), true);
        QVERIFY(
            fixture.perform(QStringLiteral("move-window-to-workspace-down"), {}, {{QStringLiteral("focus"), QStringLiteral("false")}}).ok);
        QVERIFY(showsOnActiveWorkspace(fixture, pinned));
        VERIFY_INVARIANTS(fixture);
    }
};

QTEST_GUILESS_MAIN(TestLayoutAllDesktops)
#include "test_layout_alldesktops.moc"
