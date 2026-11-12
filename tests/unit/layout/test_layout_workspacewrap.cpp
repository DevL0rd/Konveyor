#include "actionhelpers.h"

using namespace LayoutTest;

namespace
{

const QString Main = QStringLiteral("DP-1");

Config::Config wrapping(int fixedCount, bool wrap = true)
{
    Config::Config config = instantConfig();
    config.layout.fixedWorkspaceCount = fixedCount;
    config.layout.wrapWorkspaces = wrap;
    return config;
}

}

class TestLayoutWorkspaceWrap : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void focusWrapsAroundBothEndsOfAFixedSet()
    {
        Fixture fixture(wrapping(3));
        for (const int index : {2, 3, 1, 2}) {
            QVERIFY(act(fixture, QStringLiteral("focus-workspace-down")).ok);
            QCOMPARE(activeWorkspaceOn(fixture, Main), index);
        }
        QVERIFY(act(fixture, QStringLiteral("focus-workspace"), {QStringLiteral("1")}).ok);
        for (const int index : {3, 2, 1, 3}) {
            QVERIFY(act(fixture, QStringLiteral("focus-workspace-up")).ok);
            QCOMPARE(activeWorkspaceOn(fixture, Main), index);
        }
        VERIFY_INVARIANTS(fixture);
    }

    void focusStopsAtTheEndsWhenWrappingIsOff()
    {
        Fixture fixture(wrapping(3, false));
        QVERIFY(act(fixture, QStringLiteral("focus-workspace-up")).ok);
        QCOMPARE(activeWorkspaceOn(fixture, Main), 1);
        QVERIFY(act(fixture, QStringLiteral("focus-workspace"), {QStringLiteral("3")}).ok);
        QVERIFY(act(fixture, QStringLiteral("focus-workspace-down")).ok);
        QCOMPARE(activeWorkspaceOn(fixture, Main), 3);
    }

    void dynamicWorkspacesNeverWrap()
    {
        Config::Config config = instantConfig();
        config.layout.wrapWorkspaces = true;
        Fixture fixture(config);
        fixture.add();
        QVERIFY(act(fixture, QStringLiteral("focus-workspace-down")).ok);
        QCOMPARE(activeWorkspaceOn(fixture, Main), 2);
        QVERIFY(act(fixture, QStringLiteral("focus-workspace-down")).ok);
        QCOMPARE(activeWorkspaceOn(fixture, Main), 2);
        QVERIFY(act(fixture, QStringLiteral("focus-workspace"), {QStringLiteral("1")}).ok);
        QVERIFY(act(fixture, QStringLiteral("focus-workspace-up")).ok);
        QCOMPARE(activeWorkspaceOn(fixture, Main), 1);
    }

    void aSingleFixedWorkspaceStaysPut()
    {
        Fixture fixture(wrapping(1));
        QVERIFY(act(fixture, QStringLiteral("focus-workspace-down")).ok);
        QCOMPARE(activeWorkspaceOn(fixture, Main), 1);
        QVERIFY(act(fixture, QStringLiteral("focus-workspace-up")).ok);
        QCOMPARE(activeWorkspaceOn(fixture, Main), 1);
    }

    void movingAWindowWrapsAndFollowsFocus()
    {
        Fixture fixture(wrapping(3));
        const auto id = fixture.add();
        QVERIFY(act(fixture, QStringLiteral("move-window-to-workspace-up")).ok);
        QCOMPARE(fixture.state(id).workspaceIndex, 3);
        QCOMPARE(activeWorkspaceOn(fixture, Main), 3);
        QCOMPARE(fixture.focused(), id);
        QVERIFY(act(fixture, QStringLiteral("move-window-to-workspace-down")).ok);
        QCOMPARE(fixture.state(id).workspaceIndex, 1);
        QCOMPARE(activeWorkspaceOn(fixture, Main), 1);
        VERIFY_INVARIANTS(fixture);
    }

    void movingAColumnWraps()
    {
        Fixture fixture(wrapping(3));
        const auto id = fixture.add();
        QVERIFY(act(fixture, QStringLiteral("move-column-to-workspace-up")).ok);
        QCOMPARE(fixture.state(id).workspaceIndex, 3);
        QVERIFY(act(fixture, QStringLiteral("move-column-to-workspace-down")).ok);
        QCOMPARE(fixture.state(id).workspaceIndex, 1);
        VERIFY_INVARIANTS(fixture);
    }

    void focusingPastTheLastWindowWrapsToTheFirstWorkspace()
    {
        Fixture fixture(wrapping(2));
        fixture.add();
        QVERIFY(act(fixture, QStringLiteral("focus-window-or-workspace-down")).ok);
        QCOMPARE(activeWorkspaceOn(fixture, Main), 2);
        QVERIFY(act(fixture, QStringLiteral("focus-window-or-workspace-down")).ok);
        QCOMPARE(activeWorkspaceOn(fixture, Main), 1);
    }
};

QTEST_GUILESS_MAIN(TestLayoutWorkspaceWrap)
#include "test_layout_workspacewrap.moc"
