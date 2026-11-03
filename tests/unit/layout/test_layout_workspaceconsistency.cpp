#include "striphelpers.h"

using namespace LayoutTest;

class TestLayoutWorkspaceConsistency : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void emptyWorkspaceIsConsistent()
    {
        MonitorKit kit;
        QCOMPARE(kit.monitor.workspaces().front().checkConsistency(), QString());
    }

    void reportsTilingProblems()
    {
        MonitorKit kit;
        kit.add(0);
        kit.monitor.workspaces().front().scrolling().columns().front().activeTileIndex = 4;
        QVERIFY(kit.monitor.workspaces().front().checkConsistency().contains(QStringLiteral("active tile index out of range")));
    }

    void reportsFloatingProblems()
    {
        MonitorKit kit;
        Layout::Workspace &workspace = kit.monitor.workspaces().front();
        workspace.floating().tiles().push_back(kit.makeTile());
        QVERIFY(workspace.checkConsistency().contains(QStringLiteral("tiles and data must have the same length")));
    }
};

QTEST_GUILESS_MAIN(TestLayoutWorkspaceConsistency)
#include "test_layout_workspaceconsistency.moc"
