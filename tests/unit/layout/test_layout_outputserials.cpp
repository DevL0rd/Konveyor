#include "actionhelpers.h"

using namespace LayoutTest;

namespace
{

bool isOnPixelGrid(double value, double scale)
{
    return std::abs(value * scale - std::round(value * scale)) < 0.001;
}

Layout::WindowId windowLeftByAnUnpluggedOutput(Fixture &fixture, const Layout::OutputInfo &output)
{
    fixture.addOutput(output);
    fixture.engine().focusOutput(output.name);
    const auto id = fixture.add(QStringLiteral("a"));
    fixture.removeOutput(output.name);
    return id;
}

}

class TestLayoutOutputSerials : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void windowsStayOnTheirOutputsWithNegativeOriginsAndScales()
    {
        for (const QList<Layout::OutputInfo> &outputs : {mixedOutputs(), scaledOutputs()}) {
            Fixture fixture;
            fixture.removeOutput(QStringLiteral("DP-1"));
            for (const Layout::OutputInfo &output : outputs) {
                fixture.addOutput(output);
            }
            for (const Layout::OutputInfo &output : outputs) {
                fixture.engine().focusOutput(output.name);
                const auto id = fixture.add(output.name);
                const QRectF frame = fixture.frame(id);
                QCOMPARE(fixture.state(id).output, output.name);
                QVERIFY2(output.workArea.contains(frame), qPrintable(output.name));
                QVERIFY(isOnPixelGrid(frame.x(), output.scale) && isOnPixelGrid(frame.width(), output.scale));
            }
        }
    }

    void replugWithTheSameSerialReclaimsWorkspaces()
    {
        Fixture fixture;
        const Layout::OutputInfo lg = mixedOutputs().at(2);
        const auto id = windowLeftByAnUnpluggedOutput(fixture, lg);
        QCOMPARE(fixture.state(id).output, QStringLiteral("DP-1"));
        Layout::OutputInfo moved = lg;
        moved.name = QStringLiteral("DP-5");
        fixture.addOutput(moved);
        QCOMPARE(fixture.state(id).output, QStringLiteral("DP-5"));
    }

    void sameNameWithANewSerialIsAnotherMonitor()
    {
        Fixture fixture;
        const Layout::OutputInfo lg = mixedOutputs().at(2);
        const auto id = windowLeftByAnUnpluggedOutput(fixture, lg);
        fixture.addOutput(withSerial(lg, QStringLiteral("Samsung 4444")));
        QCOMPARE(fixture.state(id).output, QStringLiteral("DP-1"));
        fixture.removeOutput(lg.name);
        fixture.addOutput(lg);
        QCOMPARE(fixture.state(id).output, lg.name);
    }

    void identicalMonitorKeepsTheEmptyWorkspaceOfTheOtherOne()
    {
        Fixture fixture;
        fixture.removeOutput(QStringLiteral("DP-1"));
        fixture.addOutput(makeOutput(QStringLiteral("DP-1"), QRectF(0, 0, 1920, 1080), 1.0, QStringLiteral("Acme 0")));
        const Layout::OutputInfo lg = mixedOutputs().at(2);
        const auto id = windowLeftByAnUnpluggedOutput(fixture, lg);
        QCOMPARE(fixture.state(id).output, QStringLiteral("DP-1"));
        fixture.addOutput(makeOutput(QStringLiteral("DP-2"), QRectF(1920, 0, 1920, 1080), 1.0, QStringLiteral("Acme 0")));
        const QList<Layout::WorkspaceState> workspaces = workspacesOn(fixture, QStringLiteral("DP-1"));
        QVERIFY(!workspaces.isEmpty());
        QCOMPARE(workspaces.last().activeWindow, std::optional<Layout::WindowId>());
        QCOMPARE(fixture.state(id).output, QStringLiteral("DP-1"));
        VERIFY_INVARIANTS(fixture);
    }
    void fixedSizeWindowWithABorderKeepsItsHeightAtAFractionalScale()
    {
        Config::Config config = instantConfig();
        config.layout.border.enabled = true;
        config.layout.border.width = 9.5;
        Fixture fixture(config);
        fixture.removeOutput(QStringLiteral("DP-1"));
        fixture.addOutput(makeOutput(QStringLiteral("DP-1"), QRectF(0, 0, 1280, 720), 1.25));
        Layout::WindowProperties properties = makeWindow(QStringLiteral("fixed"), QStringLiteral("fixed"), QSizeF(573, 110));
        properties.isResizable = false;
        const auto id = fixture.addWith(properties);
        QVERIFY2(fixture.frame(id).height() >= 110.0 - 1.0 / 1.25, qPrintable(QString::number(fixture.frame(id).height())));
        VERIFY_INVARIANTS(fixture);
    }
};

QTEST_GUILESS_MAIN(TestLayoutOutputSerials)
#include "test_layout_outputserials.moc"
