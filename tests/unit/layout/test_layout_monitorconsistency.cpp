#include "striphelpers.h"

using namespace LayoutTest;

class TestLayoutMonitorConsistency : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void freshMonitorIsConsistent()
    {
        MonitorKit kit;
        QCOMPARE(kit.monitor.checkConsistency(), QString());
        kit.add(0);
        QCOMPARE(kit.monitor.checkConsistency(), QString());
    }

    void rejectsMonitorWithoutWorkspaces()
    {
        MonitorKit kit;
        kit.monitor.workspaces().clear();
        QVERIFY(kit.monitor.checkConsistency().contains(QStringLiteral("must have at least one workspace")));
    }

    void rejectsActiveWorkspaceOutOfRange()
    {
        MonitorKit kit;
        kit.add(0);
        kit.monitor.switchWorkspace(1);
        QCOMPARE(kit.monitor.activeWorkspaceIndex(), std::size_t(1));
        kit.monitor.workspaces().pop_back();
        kit.monitor.workspaces().pop_back();
        kit.monitor.workspaces().emplace_back(kit.area, kit.clock, kit.options, std::nullopt);
        QVERIFY(kit.monitor.checkConsistency().contains(QStringLiteral("active workspace index out of range")));
    }

    void rejectsOccupiedLastWorkspace()
    {
        MonitorKit kit;
        kit.add(0);
        std::swap(kit.monitor.workspaces().at(0), kit.monitor.workspaces().at(1));
        QVERIFY(kit.monitor.checkConsistency().contains(QStringLiteral("the last workspace must be empty")));
    }

    void rejectsOccupiedFirstWorkspaceWhenAnEmptyOneIsKeptAbove()
    {
        Config::Config config = instantConfig();
        config.layout.emptyWorkspaceAboveFirst = true;
        MonitorKit kit(config);
        kit.add(0);
        kit.monitor.switchWorkspace(1);
        kit.add(1);
        QCOMPARE(kit.monitor.checkConsistency(), QString());
        std::swap(kit.monitor.workspaces().at(0), kit.monitor.workspaces().at(1));
        QVERIFY(kit.monitor.checkConsistency().contains(QStringLiteral("the first workspace must be empty")));
    }

    void reportsWorkspaceProblems()
    {
        MonitorKit kit;
        kit.add(0);
        kit.monitor.workspaces().front().scrolling().columns().front().tiles.clear();
        QVERIFY(kit.monitor.checkConsistency().contains(QStringLiteral("columns must not be empty")));
    }

    void fixedWorkspacesMayEndOccupied()
    {
        Config::Config config = instantConfig();
        config.layout.fixedWorkspaceCount = 2;
        MonitorKit kit(config);
        kit.add(1);
        QCOMPARE(kit.monitor.checkConsistency(), QString());
    }
};

QTEST_GUILESS_MAIN(TestLayoutMonitorConsistency)
#include "test_layout_monitorconsistency.moc"
