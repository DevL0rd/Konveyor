#include "actionhelpers.h"

using namespace LayoutTest;

namespace
{

const QString Acme = QStringLiteral("Acme 27Q 0");
const QString Plain = QStringLiteral("DP-1");

Layout::OutputInfo acme(const QString &name, double x, const QString &serial = Acme)
{
    return makeOutput(name, QRectF(x, 0, 1920, 1080), 1.0, serial);
}

struct TwinOutputs
{
    Fixture fixture;
    Layout::WindowId left = 0;
    Layout::WindowId right = 0;

    explicit TwinOutputs(const QString &serial = Acme)
    {
        fixture.addOutput(acme(QStringLiteral("DP-2"), 1920, serial));
        left = addOn(fixture, QStringLiteral("DP-2"), QStringLiteral("left"));
        fixture.addOutput(acme(QStringLiteral("DP-3"), 3840, serial));
        right = addOn(fixture, QStringLiteral("DP-3"), QStringLiteral("right"));
    }

    QString outputOf(Layout::WindowId id) { return fixture.state(id).output; }
    bool eachAtHome() { return outputOf(left) == QLatin1String("DP-2") && outputOf(right) == QLatin1String("DP-3"); }
};

}

class TestLayoutIdenticalOutputs : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void secondIdenticalMonitorLeavesTheFirstOnesWorkspacesAlone_data()
    {
        QTest::addColumn<QString>("serial");
        QTest::newRow("same make, model and serial") << Acme;
        QTest::newRow("same make and model, no serial") << QStringLiteral("Acme 27Q");
    }

    void secondIdenticalMonitorLeavesTheFirstOnesWorkspacesAlone()
    {
        QFETCH(QString, serial);
        TwinOutputs twins(serial);
        QVERIFY(twins.eachAtHome());
        VERIFY_INVARIANTS(twins.fixture);
    }

    void secondIdenticalMonitorOnTheFirstOutputLeavesItsWorkspacesAlone()
    {
        Fixture fixture;
        fixture.removeOutput(Plain);
        fixture.addOutput(acme(Plain, 0));
        const auto first = fixture.add(QStringLiteral("first"));
        QVERIFY(fixture.perform(QStringLiteral("focus-workspace-down")).ok);
        const auto lower = fixture.add(QStringLiteral("lower"));
        fixture.addOutput(acme(QStringLiteral("DP-2"), 1920));
        QCOMPARE(fixture.state(first).output, Plain);
        QCOMPARE(fixture.state(lower).output, Plain);
        QCOMPARE(workspacesOn(fixture, QStringLiteral("DP-2")).size(), 1);
        VERIFY_INVARIANTS(fixture);
    }

    void eachIdenticalMonitorGetsItsOwnWorkspacesBack()
    {
        TwinOutputs twins;
        twins.fixture.removeOutput(QStringLiteral("DP-3"));
        QCOMPARE(twins.outputOf(twins.right), Plain);
        twins.fixture.addOutput(acme(QStringLiteral("DP-3"), 3840));
        QVERIFY(twins.eachAtHome());
        twins.fixture.removeOutput(QStringLiteral("DP-2"));
        twins.fixture.addOutput(acme(QStringLiteral("DP-2"), 1920));
        QVERIFY(twins.eachAtHome());
        VERIFY_INVARIANTS(twins.fixture);
    }

    void identicalMonitorsComingBackInTheOtherOrderSortTheirWorkspacesOut()
    {
        TwinOutputs twins;
        twins.fixture.removeOutput(QStringLiteral("DP-2"));
        twins.fixture.removeOutput(QStringLiteral("DP-3"));
        QCOMPARE(twins.outputOf(twins.left), Plain);
        QCOMPARE(twins.outputOf(twins.right), Plain);
        twins.fixture.addOutput(acme(QStringLiteral("DP-3"), 3840));
        QCOMPARE(twins.outputOf(twins.left), QStringLiteral("DP-3"));
        QCOMPARE(twins.outputOf(twins.right), QStringLiteral("DP-3"));
        twins.fixture.addOutput(acme(QStringLiteral("DP-2"), 1920));
        QVERIFY(twins.eachAtHome());
        VERIFY_INVARIANTS(twins.fixture);
    }

    void identicalMonitorsSurviveLosingEveryOutput()
    {
        TwinOutputs twins;
        for (const QString &name : {Plain, QStringLiteral("DP-2"), QStringLiteral("DP-3")}) {
            twins.fixture.removeOutput(name);
        }
        twins.fixture.addOutput(acme(QStringLiteral("DP-3"), 3840));
        twins.fixture.addOutput(acme(QStringLiteral("DP-2"), 1920));
        twins.fixture.addOutput(makeOutput(Plain, QRectF(0, 0, 1920, 1080)));
        QVERIFY(twins.eachAtHome());
        VERIFY_INVARIANTS(twins.fixture);
    }

    void identicalMonitorUnderANewConnectorTakesTheWorkspacesLeftWaiting()
    {
        TwinOutputs twins;
        twins.fixture.removeOutput(QStringLiteral("DP-3"));
        twins.fixture.addOutput(acme(QStringLiteral("DP-5"), 3840));
        QCOMPARE(twins.outputOf(twins.left), QStringLiteral("DP-2"));
        QCOMPARE(twins.outputOf(twins.right), QStringLiteral("DP-5"));
        twins.fixture.removeOutput(QStringLiteral("DP-2"));
        QCOMPARE(twins.outputOf(twins.left), Plain);
        VERIFY_INVARIANTS(twins.fixture);
    }

    void workspaceMovedBetweenIdenticalMonitorsKeepsItsHome()
    {
        TwinOutputs twins;
        QVERIFY(twins.fixture.perform(QStringLiteral("focus-window"), {}, idProperty(twins.right)).ok);
        QVERIFY(twins.fixture.perform(QStringLiteral("move-workspace-to-monitor"), {QStringLiteral("DP-2")}).ok);
        QCOMPARE(twins.outputOf(twins.right), QStringLiteral("DP-2"));
        twins.fixture.removeOutput(QStringLiteral("DP-3"));
        twins.fixture.addOutput(acme(QStringLiteral("DP-3"), 3840));
        QVERIFY(twins.eachAtHome());
        VERIFY_INVARIANTS(twins.fixture);
    }

    void namedWorkspaceOnAnIdenticalMonitorByConnectorFindsTheRightOne()
    {
        Config::Config config = instantConfig();
        config.workspaces = {namedWorkspace(QStringLiteral("chat"), QStringLiteral("DP-3"))};
        Fixture fixture(config);
        fixture.addOutput(acme(QStringLiteral("DP-2"), 1920));
        fixture.addOutput(acme(QStringLiteral("DP-3"), 3840));
        QVERIFY(namesOn(fixture, QStringLiteral("DP-3")).contains(QStringLiteral("chat")));
        fixture.removeOutput(QStringLiteral("DP-3"));
        fixture.removeOutput(QStringLiteral("DP-2"));
        fixture.addOutput(acme(QStringLiteral("DP-2"), 1920));
        fixture.addOutput(acme(QStringLiteral("DP-3"), 3840));
        QVERIFY(namesOn(fixture, QStringLiteral("DP-3")).contains(QStringLiteral("chat")));
        QVERIFY(!namesOn(fixture, QStringLiteral("DP-2")).contains(QStringLiteral("chat")));
        VERIFY_INVARIANTS(fixture);
    }
};

QTEST_GUILESS_MAIN(TestLayoutIdenticalOutputs)
#include "test_layout_identicaloutputs.moc"
