#include "helpers.h"

using namespace LayoutTest;

namespace
{

const QString Left = QStringLiteral("DP-1");
const QString Middle = QStringLiteral("DP-2");
const QString Right = QStringLiteral("DP-3");

QRectF outputGeometry(int slot)
{
    return {1920.0 * slot, 0, 1920, 1080};
}

Config::Config twoNamedOnMiddle()
{
    Config::Config config = instantConfig();
    config.workspaces = {namedWorkspace(QStringLiteral("chat"), Middle), namedWorkspace(QStringLiteral("mail"), Middle)};
    return config;
}

struct TwoOutputs
{
    Fixture fixture {twoNamedOnMiddle()};
    Layout::WindowId window = 0;

    TwoOutputs()
    {
        fixture.addOutput(Middle, outputGeometry(1));
        fixture.engine().focusOutput(Left);
        window = fixture.add(QStringLiteral("a"));
    }

    bool isActive(const QString &name) const { return fixture.workspaceNamed(name).isActive; }
};

struct ThreeOutputs
{
    Fixture fixture;
    Layout::WindowId window = fixture.add(QStringLiteral("a"));

    ThreeOutputs()
    {
        fixture.addOutput(Middle, outputGeometry(1));
        fixture.addOutput(Right, outputGeometry(2));
        fixture.engine().focusOutput(Left);
    }

    void replug(const QString &name, int slot)
    {
        fixture.removeOutput(name);
        fixture.addOutput(name, outputGeometry(slot));
    }
};

}

class TestLayoutMonitorTargets : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void moveWindowToNamedWorkspaceOnAnotherMonitorLandsInThatWorkspace()
    {
        TwoOutputs t;
        QVERIFY(t.isActive(QStringLiteral("chat")));
        QVERIFY(t.fixture.perform(QStringLiteral("move-window-to-workspace"), {QStringLiteral("mail")}).ok);
        QCOMPARE(t.fixture.state(t.window).output, Middle);
        QCOMPARE(t.fixture.state(t.window).workspace, t.fixture.workspaceNamed(QStringLiteral("mail")).id);
        QVERIFY(t.isActive(QStringLiteral("mail")));
        QCOMPARE(t.fixture.focused(), std::optional(t.window));
        VERIFY_INVARIANTS(t.fixture);
    }

    void moveWindowToWorkspaceOnAnotherMonitorWithoutFocusKeepsTheMonitor()
    {
        TwoOutputs t;
        const auto moved = t.fixture.add(QStringLiteral("b"));
        QVERIFY(t.fixture
                .perform(QStringLiteral("move-window-to-workspace"), {QStringLiteral("mail")},
                    {{QStringLiteral("focus"), QStringLiteral("false")}})
                .ok);
        QCOMPARE(t.fixture.state(moved).output, Middle);
        QCOMPARE(t.fixture.state(moved).workspace, t.fixture.workspaceNamed(QStringLiteral("mail")).id);
        QCOMPARE(t.fixture.engine().focusedOutput(), std::optional(Left));
        QCOMPARE(t.fixture.focused(), std::optional(t.window));
        QVERIFY(t.isActive(QStringLiteral("chat")));
        VERIFY_INVARIANTS(t.fixture);
    }

    void moveColumnToNamedWorkspaceOnAnotherMonitorLandsInThatWorkspace()
    {
        TwoOutputs t;
        QVERIFY(t.fixture.perform(QStringLiteral("move-column-to-workspace"), {QStringLiteral("mail")}).ok);
        QCOMPARE(t.fixture.state(t.window).output, Middle);
        QCOMPARE(t.fixture.state(t.window).workspace, t.fixture.workspaceNamed(QStringLiteral("mail")).id);
        QVERIFY(t.isActive(QStringLiteral("mail")));
        QCOMPARE(t.fixture.engine().focusedOutput(), std::optional(Middle));
        VERIFY_INVARIANTS(t.fixture);
    }

    void moveColumnToWorkspaceOnAnotherMonitorWithoutFocusStaysHere()
    {
        TwoOutputs t;
        QVERIFY(t.fixture
                .perform(QStringLiteral("move-column-to-workspace"), {QStringLiteral("mail")},
                    {{QStringLiteral("focus"), QStringLiteral("false")}})
                .ok);
        QCOMPARE(t.fixture.state(t.window).workspace, t.fixture.workspaceNamed(QStringLiteral("mail")).id);
        QCOMPARE(t.fixture.engine().focusedOutput(), std::optional(Left));
        QVERIFY(t.isActive(QStringLiteral("chat")));
        VERIFY_INVARIANTS(t.fixture);
    }

    void moveWindowToWorkspaceIndexWithIdUsesTheWindowsMonitor()
    {
        TwoOutputs t;
        t.fixture.engine().focusOutput(Middle);
        const auto other = t.fixture.add(QStringLiteral("b"));
        QCOMPARE(t.fixture.state(other).output, Middle);
        t.fixture.engine().focusOutput(Left);
        QVERIFY(t.fixture.perform(QStringLiteral("move-window-to-workspace"), {QStringLiteral("2")}, idProperty(other)).ok);
        QCOMPARE(t.fixture.state(other).output, Middle);
        QCOMPARE(t.fixture.state(other).workspace, t.fixture.workspaceNamed(QStringLiteral("mail")).id);
        QCOMPARE(t.fixture.state(t.window).output, Left);
        VERIFY_INVARIANTS(t.fixture);
    }

    void moveWindowToUnknownWorkspaceReportsAnError()
    {
        TwoOutputs t;
        QVERIFY(!t.fixture.perform(QStringLiteral("move-window-to-workspace"), {QStringLiteral("nowhere")}).ok);
        QVERIFY(!t.fixture.perform(QStringLiteral("move-column-to-workspace"), {QStringLiteral("nowhere")}).ok);
        QVERIFY(!t.fixture.perform(QStringLiteral("move-window-to-workspace")).ok);
        QVERIFY(!t.fixture.perform(QStringLiteral("move-window-to-workspace"), {QStringLiteral("1")}, idProperty(99)).ok);
        QCOMPARE(t.fixture.state(t.window).output, Left);
        VERIFY_INVARIANTS(t.fixture);
    }

    void workspaceMovedToAnotherMonitorReturnsThereAfterThatMonitorIsReplugged()
    {
        ThreeOutputs t;
        QVERIFY(t.fixture.perform(QStringLiteral("move-workspace-to-monitor"), {Right}).ok);
        QCOMPARE(t.fixture.state(t.window).output, Right);
        t.replug(Right, 2);
        QCOMPARE(t.fixture.state(t.window).output, Right);
        VERIFY_INVARIANTS(t.fixture);
    }

    void workspaceMovedToAnotherMonitorStaysWhenItsFirstMonitorIsReplugged()
    {
        ThreeOutputs t;
        QVERIFY(t.fixture.perform(QStringLiteral("move-workspace-to-monitor"), {Right}).ok);
        t.replug(Left, 0);
        QCOMPARE(t.fixture.state(t.window).output, Right);
        VERIFY_INVARIANTS(t.fixture);
    }

    void workspaceOfAnUnpluggedMonitorReturnsHomeFromTheFirstMonitor()
    {
        ThreeOutputs t;
        t.fixture.engine().focusOutput(Right);
        const auto onRight = t.fixture.add(QStringLiteral("b"));
        t.fixture.removeOutput(Right);
        QCOMPARE(t.fixture.state(onRight).output, Left);
        t.fixture.addOutput(Right, outputGeometry(2));
        QCOMPARE(t.fixture.state(onRight).output, Right);
        QCOMPARE(t.fixture.state(t.window).output, Left);
        VERIFY_INVARIANTS(t.fixture);
    }

    void workspacesFollowTheFirstMonitorWhenItIsUnpluggedAndComeHome()
    {
        ThreeOutputs t;
        t.fixture.engine().focusOutput(Middle);
        const auto onMiddle = t.fixture.add(QStringLiteral("b"));
        t.fixture.removeOutput(Middle);
        t.fixture.removeOutput(Left);
        QCOMPARE(t.fixture.state(onMiddle).output, Right);
        QCOMPARE(t.fixture.state(t.window).output, Right);
        t.fixture.addOutput(Left, outputGeometry(0));
        t.fixture.addOutput(Middle, outputGeometry(1));
        QCOMPARE(t.fixture.state(t.window).output, Left);
        QCOMPARE(t.fixture.state(onMiddle).output, Middle);
        VERIFY_INVARIANTS(t.fixture);
    }

    void unpluggingEveryMonitorKeepsWindowsAndBringsThemBack()
    {
        ThreeOutputs t;
        t.fixture.removeOutput(Left);
        t.fixture.removeOutput(Middle);
        t.fixture.removeOutput(Right);
        QVERIFY(t.fixture.engine().hasWindow(t.window));
        VERIFY_INVARIANTS(t.fixture);
        t.fixture.addOutput(Middle, outputGeometry(1));
        QCOMPARE(t.fixture.state(t.window).output, Middle);
        t.fixture.addOutput(Left, outputGeometry(0));
        QCOMPARE(t.fixture.state(t.window).output, Left);
        VERIFY_INVARIANTS(t.fixture);
    }

    void movedWindowTakesItsWorkspaceHomeWithIt()
    {
        ThreeOutputs t;
        QVERIFY(t.fixture.perform(QStringLiteral("move-window-to-monitor"), {Right}).ok);
        QCOMPARE(t.fixture.state(t.window).output, Right);
        t.replug(Right, 2);
        QCOMPARE(t.fixture.state(t.window).output, Right);
        VERIFY_INVARIANTS(t.fixture);
    }
};

QTEST_GUILESS_MAIN(TestLayoutMonitorTargets)
#include "test_layout_monitortargets.moc"
