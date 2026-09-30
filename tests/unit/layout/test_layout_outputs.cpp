#include "actionhelpers.h"

#include <cmath>

using namespace LayoutTest;

namespace
{

const QString Laptop = QStringLiteral("DP-1");
const QString Dock = QStringLiteral("HDMI-A-1");

struct UpperAndLower
{
    Fixture fixture;
    Layout::WindowId upper = fixture.add(QStringLiteral("upper"));
    Layout::WindowId lower = addLower();

    Layout::WindowId addLower()
    {
        act(fixture, QStringLiteral("focus-workspace-down"));
        return fixture.add(QStringLiteral("lower"));
    }

    void unplug(const QString &name)
    {
        fixture.engine().removeOutput(name);
        fixture.advance(1);
    }
};

bool alignedToPixels(double value, double scale)
{
    const double pixels = value * scale;
    return std::abs(pixels - std::round(pixels)) < 1e-6;
}

bool frameAlignedToPixels(const QRectF &frame, double scale)
{
    return alignedToPixels(frame.x(), scale) && alignedToPixels(frame.y(), scale) && alignedToPixels(frame.width(), scale)
        && alignedToPixels(frame.height(), scale);
}

}

class TestLayoutOutputs : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void removingEveryOutputKeepsWorkspacesForTheNextOutput()
    {
        UpperAndLower f;
        f.unplug(Laptop);
        QCOMPARE(f.fixture.engine().workspaceStates().size(), 0);
        QCOMPARE(f.fixture.engine().focusedOutput(), std::optional<QString>());
        QVERIFY(f.fixture.engine().hasWindow(f.upper));
        VERIFY_INVARIANTS(f.fixture);

        addOutputAt(f.fixture, Dock, QRectF(0, 0, 2560, 1440));
        QCOMPARE(placeOf(f.fixture, f.upper), std::pair(Dock, 1));
        QCOMPARE(placeOf(f.fixture, f.lower), std::pair(Dock, 2));
        QCOMPARE(workspacesOn(f.fixture, Dock).size(), 3);
        QVERIFY(QRectF(0, 0, 2560, 1440).contains(f.fixture.frame(f.upper)));
        VERIFY_INVARIANTS(f.fixture);
    }

    void actionsWithoutAnyOutputSucceedAndKeepEveryWindow()
    {
        UpperAndLower f;
        f.unplug(Laptop);
        for (const QString &name :
            {QStringLiteral("focus-column-left"), QStringLiteral("focus-workspace-down"), QStringLiteral("move-column-to-workspace-down"),
                QStringLiteral("move-window-to-monitor-next"), QStringLiteral("move-workspace-to-monitor-left"),
                QStringLiteral("focus-monitor-next"), QStringLiteral("move-column-to-last"),
                QStringLiteral("focus-window-or-workspace-down"), QStringLiteral("move-window-down-or-to-workspace-down")}) {
            QVERIFY2(act(f.fixture, name).ok, qPrintable(name));
        }
        QVERIFY(f.fixture.engine().hasWindow(f.upper));
        QVERIFY(f.fixture.engine().hasWindow(f.lower));
        VERIFY_INVARIANTS(f.fixture);
        addOutputAt(f.fixture, Dock, QRectF(0, 0, 2560, 1440));
        QCOMPARE(f.fixture.state(f.upper).output, Dock);
        QCOMPARE(f.fixture.state(f.lower).output, Dock);
        VERIFY_INVARIANTS(f.fixture);
    }

    void windowOpenedWithoutAnyOutputAppearsOnTheNextOutput()
    {
        UpperAndLower f;
        f.unplug(Laptop);
        const auto late = f.fixture.add(QStringLiteral("late"));
        QVERIFY(f.fixture.engine().hasWindow(late));
        addOutputAt(f.fixture, Dock, QRectF(0, 0, 2560, 1440));
        QCOMPARE(f.fixture.state(late).output, Dock);
        QVERIFY(QRectF(0, 0, 2560, 1440).intersects(f.fixture.frame(late)));
        VERIFY_INVARIANTS(f.fixture);
    }

    void replugKeepsTheActiveWorkspaceOfTheOutput()
    {
        UpperAndLower f;
        addOutputAt(f.fixture, Dock, QRectF(1920, 0, 1920, 1080));
        f.unplug(Laptop);
        QCOMPARE(placeOf(f.fixture, f.lower), std::pair(Dock, 2));
        addOutputAt(f.fixture, Laptop, QRectF(0, 0, 1920, 1080));
        QCOMPARE(placeOf(f.fixture, f.upper), std::pair(Laptop, 1));
        QCOMPARE(placeOf(f.fixture, f.lower), std::pair(Laptop, 2));
        QCOMPARE(activeWorkspaceOn(f.fixture, Laptop), 2);
        QCOMPARE(workspacesOn(f.fixture, Dock).size(), 1);
        VERIFY_INVARIANTS(f.fixture);
    }

    void scaleChangeKeepsTheLayoutAlignedToPhysicalPixels()
    {
        Fixture fixture;
        const WindowIds ids = stackOf(fixture, 2);
        const auto right = fixture.add();
        for (const double scale : {1.25, 1.5, 2.0}) {
            fixture.engine().updateOutput(makeOutput(Laptop, QRectF(0, 0, 1920, 1080), scale));
            fixture.advance(1);
            for (const Layout::WindowId id : {ids[0], ids[1], right}) {
                QVERIFY2(frameAlignedToPixels(fixture.frame(id), scale), qPrintable(QString::number(scale)));
            }
            QCOMPARE(columns(fixture), (Columns {ids, {right}}));
            COMPARE_FOCUS(fixture, right);
            VERIFY_INVARIANTS(fixture);
        }
    }

    void updateOutputForAnUnknownOutputAddsIt()
    {
        Fixture fixture;
        fixture.engine().updateOutput(makeOutput(Dock, QRectF(1920, 0, 1920, 1080)));
        QCOMPARE(workspacesOn(fixture, Dock).size(), 1);
        VERIFY_INVARIANTS(fixture);
    }

    void outputsWithNegativeOriginsPlaceWindowsInGlobalCoordinates()
    {
        const QRectF geometry(-1920, -300, 1920, 1080);
        Fixture fixture(instantConfig(), geometry);
        addOutputAt(fixture, Dock, QRectF(0, 0, 2560, 1440));
        fixture.engine().focusOutput(Laptop);
        const WindowIds ids = columnsOf(fixture, 2);
        for (const Layout::WindowId id : ids) {
            QVERIFY(geometry.contains(fixture.frame(id)));
            QCOMPARE(fixture.engine().windowAt(fixture.frame(id).center()), std::optional(id));
        }
        QVERIFY(act(fixture, QStringLiteral("move-window-to-monitor-right")).ok);
        QVERIFY(QRectF(0, 0, 2560, 1440).contains(fixture.frame(ids[1])));
        QVERIFY(act(fixture, QStringLiteral("focus-monitor-left")).ok);
        QCOMPARE(focusedOutput(fixture), Laptop);
        VERIFY_INVARIANTS(fixture);
    }

    void floatingWindowStaysReachableWhenTheOutputShrinks()
    {
        Fixture fixture;
        fixture.add();
        const auto floating = addFloating(fixture);
        act(fixture, QStringLiteral("move-floating-window"), {}, {{QStringLiteral("x"), QStringLiteral("1700")}});
        const QRectF small(0, 0, 1280, 720);
        fixture.engine().updateOutput(makeOutput(Laptop, small));
        fixture.advance(1);
        const QRectF visible = small.intersected(fixture.frame(floating));
        QVERIFY(visible.width() >= 75 && visible.height() >= 75);
        fixture.engine().updateOutput(makeOutput(Laptop, QRectF(0, 0, 1920, 1080)));
        fixture.advance(1);
        QCOMPARE(fixture.frame(floating).x(), 1700.0);
        VERIFY_INVARIANTS(fixture);
    }

    void floatingWindowMovesToTheRemainingOutputWhenItsOutputDisappears()
    {
        Fixture fixture;
        const QRectF dock(1920, 0, 1280, 720);
        addOutputAt(fixture, Dock, dock);
        fixture.engine().focusOutput(Dock);
        const auto floating = addFloating(fixture);
        fixture.engine().removeOutput(Dock);
        fixture.advance(1);
        QVERIFY(fixture.state(floating).isFloating);
        QCOMPARE(fixture.state(floating).output, Laptop);
        act(fixture, QStringLiteral("focus-window"), {}, {{QStringLiteral("id"), QString::number(floating)}});
        QVERIFY(QRectF(0, 0, 1920, 1080).intersects(fixture.frame(floating)));
        fixture.engine().addOutput(makeOutput(Dock, dock));
        fixture.advance(1);
        QCOMPARE(fixture.state(floating).output, Dock);
        QVERIFY(dock.contains(fixture.frame(floating)));
        VERIFY_INVARIANTS(fixture);
    }

    void outputLayoutCanTurnOnTheEmptyWorkspaceAboveFirst()
    {
        Fixture fixture;
        const auto id = fixture.add(QStringLiteral("a"));
        Config::Config config = instantConfig();
        Config::Layout layout = config.layout;
        layout.emptyWorkspaceAboveFirst = true;
        config.outputs.append(Config::OutputConfig {QStringLiteral("DP-1"), layout, std::nullopt, std::nullopt});
        fixture.setConfig(config);
        QCOMPARE(workspacesOn(fixture, QStringLiteral("DP-1")).first().activeWindow, std::optional<Layout::WindowId>());
        QCOMPARE(fixture.state(id).workspaceIndex, 2);
        fixture.setConfig(instantConfig());
        QCOMPARE(fixture.state(id).workspaceIndex, 1);
        VERIFY_INVARIANTS(fixture);
    }
};

QTEST_GUILESS_MAIN(TestLayoutOutputs)
#include "test_layout_outputs.moc"
