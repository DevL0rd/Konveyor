#include "helpers.h"

using namespace LayoutTest;

namespace
{

bool isOnPixelGrid(double value, double scale)
{
    return std::abs(value * scale - std::round(value * scale)) < 0.001;
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
        fixture.addOutput(lg);
        fixture.engine().focusOutput(lg.name);
        const auto id = fixture.add(QStringLiteral("a"));
        fixture.removeOutput(lg.name);
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
        fixture.addOutput(lg);
        fixture.engine().focusOutput(lg.name);
        const auto id = fixture.add(QStringLiteral("a"));
        fixture.removeOutput(lg.name);
        fixture.addOutput(withSerial(lg, QStringLiteral("Samsung 4444")));
        QCOMPARE(fixture.state(id).output, QStringLiteral("DP-1"));
        fixture.removeOutput(lg.name);
        fixture.addOutput(lg);
        QCOMPARE(fixture.state(id).output, lg.name);
    }
};

QTEST_GUILESS_MAIN(TestLayoutOutputSerials)
#include "test_layout_outputserials.moc"
