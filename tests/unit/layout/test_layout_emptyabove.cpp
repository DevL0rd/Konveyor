#include "actionhelpers.h"

using namespace LayoutTest;

namespace
{

const QString Main = QStringLiteral("DP-1");

Config::Config emptyAboveConfig()
{
    Config::Config config = instantConfig();
    config.layout.emptyWorkspaceAboveFirst = true;
    return config;
}

struct EmptyAbove
{
    Fixture fixture {emptyAboveConfig()};
    Layout::WindowId upper = fixture.add(QStringLiteral("upper"));
    Layout::WindowId lower = addLower();

    Layout::WindowId addLower()
    {
        act(fixture, QStringLiteral("focus-workspace-down"));
        const Layout::WindowId id = fixture.add(QStringLiteral("lower"));
        act(fixture, QStringLiteral("focus-workspace-up"));
        return id;
    }

    void expectLayout(int upperIndex, int lowerIndex, int count)
    {
        QCOMPARE(placeOf(fixture, upper).second, upperIndex);
        QCOMPARE(placeOf(fixture, lower).second, lowerIndex);
        QCOMPARE(workspacesOn(fixture, Main).size(), count);
        VERIFY_INVARIANTS(fixture);
    }
};

}

class TestLayoutEmptyAbove : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void windowsStartBelowTheEmptyFirstWorkspace()
    {
        EmptyAbove f;
        f.expectLayout(2, 3, 4);
        QCOMPARE(activeWorkspaceOn(f.fixture, Main), 2);
    }

    void focusWorkspaceUpReachesTheEmptyFirstWorkspace()
    {
        EmptyAbove f;
        QVERIFY(act(f.fixture, QStringLiteral("focus-workspace-up")).ok);
        QCOMPARE(activeWorkspaceOn(f.fixture, Main), 1);
        QCOMPARE(f.fixture.focused(), std::optional<Layout::WindowId>());
        QVERIFY(act(f.fixture, QStringLiteral("focus-workspace-up")).ok);
        QCOMPARE(activeWorkspaceOn(f.fixture, Main), 1);
        act(f.fixture, QStringLiteral("focus-workspace-down"));
        f.expectLayout(2, 3, 4);
    }

    void windowMovedIntoTheEmptyFirstWorkspaceGetsANewOneAbove()
    {
        EmptyAbove f;
        QVERIFY(act(f.fixture, QStringLiteral("move-window-to-workspace-up")).ok);
        f.expectLayout(2, 3, 4);
        QCOMPARE(activeWorkspaceOn(f.fixture, Main), 2);
        COMPARE_FOCUS(f.fixture, f.upper);
    }

    void columnMovedIntoTheEmptyFirstWorkspaceWithoutFocusGetsANewOneAbove()
    {
        EmptyAbove f;
        act(f.fixture, QStringLiteral("focus-workspace-down"));
        QVERIFY(act(f.fixture, QStringLiteral("move-column-to-workspace"), {QStringLiteral("1")},
            {{QStringLiteral("focus"), QStringLiteral("false")}})
                .ok);
        QCOMPARE(placeOf(f.fixture, f.lower).second, 2);
        QCOMPARE(placeOf(f.fixture, f.upper).second, 3);
        QCOMPARE(activeWorkspaceOn(f.fixture, Main), 4);
        QCOMPARE(workspacesOn(f.fixture, Main).size(), 5);
        act(f.fixture, QStringLiteral("focus-workspace-up"));
        QCOMPARE(workspacesOn(f.fixture, Main).size(), 4);
        VERIFY_INVARIANTS(f.fixture);
    }

    void moveWorkspaceUpToTheTopKeepsTheEmptyFirstWorkspace()
    {
        EmptyAbove f;
        act(f.fixture, QStringLiteral("focus-workspace-down"));
        QVERIFY(act(f.fixture, QStringLiteral("move-workspace-up")).ok);
        f.expectLayout(3, 2, 4);
        QVERIFY(act(f.fixture, QStringLiteral("move-workspace-up")).ok);
        f.expectLayout(3, 2, 4);
        QCOMPARE(activeWorkspaceOn(f.fixture, Main), 2);
    }

    void moveWorkspaceToIndexOneLandsBelowTheEmptyFirstWorkspace()
    {
        EmptyAbove f;
        act(f.fixture, QStringLiteral("focus-workspace-down"));
        QVERIFY(act(f.fixture, QStringLiteral("move-workspace-to-index"), {QStringLiteral("1")}).ok);
        f.expectLayout(3, 2, 4);
        COMPARE_FOCUS(f.fixture, f.lower);
    }

    void closingEveryWindowCollapsesToOneEmptyWorkspaceOnceTheEmptyActiveOneIsLeft()
    {
        EmptyAbove f;
        f.fixture.remove(f.upper);
        f.fixture.remove(f.lower);
        f.fixture.advance(1);
        QCOMPARE(workspacesOn(f.fixture, Main).size(), 3);
        QCOMPARE(activeWorkspaceOn(f.fixture, Main), 2);
        act(f.fixture, QStringLiteral("focus-workspace-up"));
        QCOMPARE(workspacesOn(f.fixture, Main).size(), 1);
        QCOMPARE(activeWorkspaceOn(f.fixture, Main), 1);
        VERIFY_INVARIANTS(f.fixture);
    }

    void movingWorkspacesBetweenOutputsKeepsEmptyWorkspacesAtBothEnds()
    {
        EmptyAbove f;
        const QString other = QStringLiteral("DP-2");
        addOutputAt(f.fixture, other, QRectF(1920, 0, 1920, 1080));
        f.fixture.engine().focusOutput(Main);
        QVERIFY(act(f.fixture, QStringLiteral("move-workspace-to-monitor-right")).ok);
        QCOMPARE(placeOf(f.fixture, f.upper), std::pair(other, 2));
        QCOMPARE(workspacesOn(f.fixture, other).size(), 3);
        QCOMPARE(placeOf(f.fixture, f.lower), std::pair(Main, 2));
        QCOMPARE(workspacesOn(f.fixture, Main).size(), 3);
        QVERIFY(act(f.fixture, QStringLiteral("move-window-to-monitor-left")).ok);
        QCOMPARE(f.fixture.state(f.upper).output, Main);
        QCOMPARE(focusedOutput(f.fixture), Main);
        VERIFY_INVARIANTS(f.fixture);
    }

    void togglingTheSettingLiveKeepsFocusAndWindows()
    {
        EmptyAbove f;
        Config::Config config = emptyAboveConfig();
        config.layout.emptyWorkspaceAboveFirst = false;
        f.fixture.setConfig(config);
        f.fixture.advance(1);
        f.expectLayout(1, 2, 3);
        COMPARE_FOCUS(f.fixture, f.upper);
        f.fixture.setConfig(emptyAboveConfig());
        f.fixture.advance(1);
        f.expectLayout(2, 3, 4);
        COMPARE_FOCUS(f.fixture, f.upper);
    }

    void togglingTheSettingLiveWhileOnTheEmptyFirstWorkspace()
    {
        EmptyAbove f;
        act(f.fixture, QStringLiteral("focus-workspace-up"));
        Config::Config config = emptyAboveConfig();
        config.layout.emptyWorkspaceAboveFirst = false;
        f.fixture.setConfig(config);
        f.fixture.advance(1);
        act(f.fixture, QStringLiteral("focus-workspace-down"));
        f.fixture.advance(1);
        f.expectLayout(1, 2, 3);
    }
};

QTEST_GUILESS_MAIN(TestLayoutEmptyAbove)
#include "test_layout_emptyabove.moc"
