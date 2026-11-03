#include "helpers.h"

using namespace LayoutTest;

namespace
{

Config::Config rememberingSizes()
{
    Config::Config config = instantConfig();
    config.layout.rememberWindowSizes = true;
    return config;
}

Layout::WindowProperties floatingWindow(const QString &appId, QSizeF size)
{
    Layout::WindowProperties properties = makeWindow(appId, appId, size);
    properties.isDialog = true;
    return properties;
}

Layout::WindowId addOnNextWorkspace(Fixture &fixture, const QString &appId)
{
    fixture.perform(QStringLiteral("focus-workspace-down"));
    return fixture.add(appId);
}

void focusWorkspaceUp(Fixture &fixture, int times)
{
    for (int i = 0; i < times; ++i) {
        fixture.perform(QStringLiteral("focus-workspace-up"));
    }
}

Config::Config fixedWorkspaces(int count)
{
    Config::Config config = instantConfig();
    config.layout.fixedWorkspaceCount = count;
    return config;
}

}

class TestLayoutEventRaces : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void aFullscreenFloatingWindowIsNotRemembered()
    {
        Fixture fixture(rememberingSizes());
        const auto id = fixture.addWith(floatingWindow(QStringLiteral("player"), QSizeF(400, 300)));
        QVERIFY(fixture.state(id).isFloating);
        fixture.engine().setWindowFullscreen(id, true);
        fixture.settle();
        fixture.remove(id);
        QCOMPARE(fixture.engine().windowMemory().value(QStringLiteral("player")).floatingSize, std::optional(QSize(400, 300)));
        VERIFY_INVARIANTS(fixture);
    }

    void aFloatingWindowWithoutASizeIsNotRemembered()
    {
        Fixture fixture(rememberingSizes());
        const auto id = fixture.addWith(floatingWindow(QStringLiteral("empty"), QSizeF(0, 0)));
        fixture.remove(id);
        QVERIFY(!fixture.engine().windowMemory().value(QStringLiteral("empty")).floatingSize);
        VERIFY_INVARIANTS(fixture);
    }

    void aWindowChangingWhileNoOutputExistsStaysUnlocked()
    {
        Fixture fixture;
        fixture.engine().removeOutput(QStringLiteral("DP-1"));
        Layout::WindowProperties properties = makeWindow(QStringLiteral("splash"), QStringLiteral("splash"));
        const auto id = fixture.addWith(properties);
        properties.isResizable = false;
        properties.minSize = QSizeF(1920, 1080);
        properties.maxSize = QSizeF(1920, 1080);
        fixture.engine().updateWindowProperties(id, properties);
        fixture.engine().setWindowFullscreen(id, false);
        fixture.addOutput(QStringLiteral("DP-1"), QRectF(0, 0, 1920, 1080));
        QVERIFY(fixture.engine().windowState(id).has_value());
        VERIFY_INVARIANTS(fixture);
    }

    void movingTheColumnOfAnEmptyWorkspaceDoesNothing()
    {
        Fixture fixture;
        const auto id = fixture.add(QStringLiteral("a"));
        fixture.perform(QStringLiteral("focus-workspace-down"));
        const auto before = fixture.focused();
        fixture.perform(QStringLiteral("move-column-to-workspace-up"));
        QCOMPARE(fixture.focused(), before);
        QVERIFY(fixture.engine().hasWindow(id));
        VERIFY_INVARIANTS(fixture);
    }

    void movingAWorkspaceAboveTheActiveOneKeepsTheActiveWorkspace()
    {
        Fixture fixture;
        const auto first = fixture.add(QStringLiteral("a"));
        addOnNextWorkspace(fixture, QStringLiteral("b"));
        const auto third = addOnNextWorkspace(fixture, QStringLiteral("c"));
        const auto fourth = addOnNextWorkspace(fixture, QStringLiteral("d"));
        QCOMPARE(fixture.focused(), std::optional(fourth));
        fixture.perform(
            QStringLiteral("move-workspace-to-index"), {QStringLiteral("2")}, {{QStringLiteral("reference"), QStringLiteral("1")}});
        QCOMPARE(fixture.focused(), std::optional(fourth));
        QVERIFY(fixture.engine().hasWindow(first));
        QVERIFY(fixture.engine().hasWindow(third));
        VERIFY_INVARIANTS(fixture);
    }

    void movingAWorkspaceBelowTheActiveOneKeepsTheActiveWorkspace()
    {
        Fixture fixture;
        const auto first = fixture.add(QStringLiteral("a"));
        const auto second = addOnNextWorkspace(fixture, QStringLiteral("b"));
        const auto third = addOnNextWorkspace(fixture, QStringLiteral("c"));
        focusWorkspaceUp(fixture, 2);
        QCOMPARE(fixture.focused(), std::optional(first));
        fixture.perform(
            QStringLiteral("move-workspace-to-index"), {QStringLiteral("3")}, {{QStringLiteral("reference"), QStringLiteral("2")}});
        QCOMPARE(fixture.focused(), std::optional(first));
        QVERIFY(fixture.engine().hasWindow(second));
        QVERIFY(fixture.engine().hasWindow(third));
        VERIFY_INVARIANTS(fixture);
    }

    void theDropHintSurvivesAWorkspaceClosingUnderTheDrag()
    {
        Fixture fixture;
        const auto dragged = fixture.add(QStringLiteral("dragged"));
        const auto closing = addOnNextWorkspace(fixture, QStringLiteral("closing"));
        const auto kept = addOnNextWorkspace(fixture, QStringLiteral("kept"));
        focusWorkspaceUp(fixture, 2);
        QVERIFY(startMove(fixture, dragged, QPointF(960, 300)));
        fixture.perform(QStringLiteral("focus-workspace-down"));
        QVERIFY(fixture.engine().outputStates().constFirst().dropHint.has_value());
        fixture.perform(QStringLiteral("focus-workspace-down"));
        fixture.remove(closing);
        fixture.advance(1000);
        QVERIFY(fixture.engine().outputStates().constFirst().dropHint.has_value());
        QCOMPARE(fixture.engine().workspaceStates().size(), 2);
        fixture.endDragAndSettle();
        QVERIFY(fixture.engine().hasWindow(dragged));
        QVERIFY(fixture.engine().hasWindow(kept));
        VERIFY_INVARIANTS(fixture);
    }

    void aHomeChangedWhileNoOutputExistsMovesNothing()
    {
        Config::Config config = instantConfig();
        config.workspaces = {namedWorkspace(QStringLiteral("chat"))};
        Fixture fixture(config);
        fixture.engine().removeOutput(QStringLiteral("DP-1"));
        config.workspaces = {namedWorkspace(QStringLiteral("chat"), QStringLiteral("DP-2"))};
        fixture.setConfig(config);
        fixture.addOutput(QStringLiteral("DP-2"), QRectF(1920, 0, 1920, 1080));
        bool onSecond = false;
        for (const Layout::WorkspaceState &state : fixture.engine().workspaceStates()) {
            onSecond = onSecond || (state.name == QLatin1String("chat") && state.output == QLatin1String("DP-2"));
        }
        QVERIFY(onSecond);
        VERIFY_INVARIANTS(fixture);
    }

    void aHomeChangedForAWorkspaceThatLostItsNameMovesNothing()
    {
        Config::Config config = instantConfig();
        config.workspaces = {namedWorkspace(QStringLiteral("chat"))};
        Fixture fixture(config);
        fixture.addOutput(QStringLiteral("DP-2"), QRectF(1920, 0, 1920, 1080));
        QVERIFY(fixture.perform(QStringLiteral("unset-workspace-name"), {}, {{QStringLiteral("workspace"), QStringLiteral("chat")}}).ok);
        config.workspaces = {namedWorkspace(QStringLiteral("chat"), QStringLiteral("DP-2"))};
        fixture.setConfig(config);
        QVERIFY(!fixture.workspaceNamed(QStringLiteral("chat")).output.isEmpty());
        VERIFY_INVARIANTS(fixture);
    }

    void gatheringEveryWindowWhileOneIsBeingDraggedKeepsTheDraggedWindow()
    {
        Fixture fixture;
        const auto dragged = fixture.add(QStringLiteral("dragged"));
        fixture.addOutput(QStringLiteral("DP-2"), QRectF(1920, 0, 1920, 1080));
        fixture.engine().focusOutput(QStringLiteral("DP-2"));
        const auto other = fixture.add(QStringLiteral("other"));
        fixture.engine().focusOutput(QStringLiteral("DP-1"));
        fixture.engine().activateWindow(dragged);
        QVERIFY(startMove(fixture, dragged, QPointF(900, 500)));
        QVERIFY(fixture.perform(QStringLiteral("move-all-windows-to-monitor"), {QStringLiteral("DP-2")}).ok);
        QCOMPARE(fixture.engine().movingWindow(), std::optional(dragged));
        fixture.endDragAndSettle();
        QVERIFY(fixture.engine().hasWindow(dragged));
        QVERIFY(fixture.engine().hasWindow(other));
        VERIFY_INVARIANTS(fixture);
    }

    void droppingAFloatingWindowThatNeverHadASizeKeepsItFloating()
    {
        Fixture fixture;
        const auto id = fixture.addWith(floatingWindow(QStringLiteral("empty"), QSizeF(0, 0)));
        QVERIFY(fixture.state(id).isFloating);
        QVERIFY(startMove(fixture, id, QPointF(700, 400)));
        fixture.endDragAndSettle();
        QVERIFY(fixture.state(id).isFloating);
        VERIFY_INVARIANTS(fixture);
    }

    void cancellingADragAfterTheFixedWorkspacesShrankReturnsToTheLastOne()
    {
        Fixture fixture(fixedWorkspaces(4));
        const auto dragged = fixture.add(QStringLiteral("dragged"));
        fixture.perform(QStringLiteral("focus-workspace-down"));
        fixture.perform(QStringLiteral("focus-workspace-down"));
        fixture.perform(QStringLiteral("focus-workspace-down"));
        const auto anchor = fixture.add(QStringLiteral("anchor"));
        fixture.perform(QStringLiteral("move-window-to-workspace-up"));
        fixture.perform(QStringLiteral("focus-workspace-up"));
        QVERIFY(startMove(fixture, anchor, QPointF(960, 300)));
        fixture.setConfig(fixedWorkspaces(2));
        fixture.engine().cancelWindowDrag();
        fixture.settle();
        QVERIFY(fixture.engine().hasWindow(anchor));
        QVERIFY(fixture.engine().hasWindow(dragged));
        QCOMPARE(fixture.engine().workspaceStates().size(), 2);
        VERIFY_INVARIANTS(fixture);
    }
};

QTEST_GUILESS_MAIN(TestLayoutEventRaces)

#include "test_layout_eventraces.moc"
