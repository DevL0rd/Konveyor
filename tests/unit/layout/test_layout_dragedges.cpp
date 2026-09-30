#include "helpers.h"

using namespace LayoutTest;

Q_DECLARE_METATYPE(Konveyor::Config::DndEdgeScroll)

namespace
{

const QString Primary = QStringLiteral("DP-1");
const QString Secondary = QStringLiteral("DP-2");

struct Row : WideRow
{
    using WideRow::WideRow;

    bool hasDropHint()
    {
        const QList<Layout::OutputState> outputs = fixture.engine().outputStates();
        return std::ranges::any_of(outputs, [](const Layout::OutputState &output) { return output.dropHint.has_value(); });
    }

    bool firstIsOnScreen() { return fixture.frame(first).left() >= 0.0; }

    void holdDataDragAt(QPointF pointer, int moves)
    {
        for (int move = 0; move < moves; ++move) {
            fixture.passTime(100);
            fixture.engine().dataDragEdgeScroll(output, pointer, fixture.elapsed());
        }
        fixture.settle();
    }

    void holdWindowDragAt(QPointF pointer, int moves)
    {
        for (int move = 0; move < moves; ++move) {
            fixture.passTime(100);
            fixture.engine().updateWindowDrag(pointer + QPointF(0, move % 2), Primary);
        }
        fixture.settle();
    }
};

bool onScreen(const QRectF &frame, const QRectF &output)
{
    return output.contains(frame.center());
}

}

class TestLayoutDragEdges : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void closingTheDraggedWindowStopsEdgeScrollAndDropHint()
    {
        Row row(Primary, animatedWideColumns());
        QVERIFY(startMove(row.fixture, row.last, QPointF(10, 500)));
        QVERIFY(row.hasDropHint());
        row.fixture.remove(row.last);
        QVERIFY(!row.hasDropHint());
        QVERIFY(row.fixture.perform(QStringLiteral("focus-column-last")).ok);
        QVERIFY(row.fixture.perform(QStringLiteral("set-column-width"), {QStringLiteral("1800")}).ok);
        QVERIFY2(row.fixture.frame(row.third).right() <= 1920.0, "the row stayed in edge scroll after the dragged window closed");
        row.fixture.engine().endWindowDrag();
        QVERIFY(!row.fixture.engine().hasWindow(row.last));
        VERIFY_INVARIANTS(row.fixture);
    }

    void closingTheDraggedWindowBeforeItDetachesKeepsTheRowWorking()
    {
        Row row(Primary, animatedWideColumns());
        QVERIFY(startMove(row.fixture, row.last, row.fixture.frame(row.last).center() + QPointF(10, 0)));
        row.fixture.remove(row.last);
        QVERIFY(row.fixture.perform(QStringLiteral("focus-column-first")).ok);
        QVERIFY(row.firstIsOnScreen());
        row.fixture.engine().endWindowDrag();
        QCOMPARE(row.fixture.focused(), std::optional(row.first));
        VERIFY_INVARIANTS(row.fixture);
    }

    void closingAnotherWindowDuringADragKeepsTheDrag()
    {
        Row row(Primary, animatedWideColumns());
        QVERIFY(startMove(row.fixture, row.last, QPointF(10, 500)));
        row.fixture.remove(row.second);
        QVERIFY(row.hasDropHint());
        row.fixture.engine().endWindowDrag();
        row.fixture.settle();
        QVERIFY(row.fixture.engine().hasWindow(row.last));
        QVERIFY(!row.hasDropHint());
        VERIFY_INVARIANTS(row.fixture);
    }

    void windowDragAtTheEdgeScrollsTheRow()
    {
        Row row(Primary, animatedWideColumns());
        QVERIFY(startMove(row.fixture, row.last, QPointF(960, 500)));
        const double before = row.fixture.frame(row.first).left();
        row.holdWindowDragAt(QPointF(5, 500), 4);
        QVERIFY2(row.fixture.frame(row.first).left() > before, "dragging a window over the left edge did not scroll the row");
        row.fixture.engine().endWindowDrag();
        row.fixture.settle();
        QCOMPARE(row.fixture.focused(), std::optional(row.last));
        const QRectF frame = row.fixture.frame(row.last);
        QVERIFY2(frame.left() >= 0.0 && frame.right() <= 1920.0, "the dropped window is not in view");
        VERIFY_INVARIANTS(row.fixture);
    }

    void windowDragHeldStillAtTheEdgeKeepsScrolling()
    {
        Row row;
        QVERIFY(startMove(row.fixture, row.last, QPointF(960, 500)));
        row.fixture.passTime(100);
        row.fixture.engine().updateWindowDrag(QPointF(5, 500), Primary);
        const double before = row.fixture.frame(row.first).left();
        QVERIFY(row.fixture.engine().isAnimating());
        for (int frame = 0; frame < 10; ++frame) {
            row.fixture.advance(50);
        }
        QVERIFY2(row.fixture.frame(row.first).left() > before, "holding the pointer still at the edge stopped the scroll");
        row.fixture.engine().endWindowDrag();
        row.fixture.advance(1000);
        QVERIFY(!row.fixture.engine().isAnimating());
        VERIFY_INVARIANTS(row.fixture);
    }

    void dataDragHeldStillAtTheEdgeKeepsScrolling()
    {
        Row row;
        const double before = row.fixture.frame(row.first).left();
        row.fixture.engine().beginDataDrag();
        row.fixture.engine().dataDragEdgeScroll(Primary, QPointF(5, 500), row.fixture.elapsed());
        for (int frame = 0; frame < 10; ++frame) {
            row.fixture.advance(50);
        }
        QVERIFY2(row.fixture.frame(row.first).left() > before, "holding a file still at the edge stopped the scroll");
        row.fixture.engine().endDataDrag();
        row.fixture.advance(1000);
        QVERIFY(!row.fixture.engine().isAnimating());
        VERIFY_INVARIANTS(row.fixture);
    }

    void floatingWindowDragAtTheEdgeDoesNotScrollTheRow()
    {
        Row row(Primary, animatedWideColumns());
        QVERIFY(row.fixture.perform(QStringLiteral("toggle-window-floating")).ok);
        row.fixture.advanceInSteps(1000);
        QVERIFY(startMove(row.fixture, row.last, QPointF(960, 500)));
        const double before = row.fixture.frame(row.first).left();
        row.holdWindowDragAt(QPointF(5, 500), 4);
        QCOMPARE(row.fixture.frame(row.first).left(), before);
        row.fixture.engine().endWindowDrag();
        QVERIFY(row.fixture.state(row.last).isFloating);
        VERIFY_INVARIANTS(row.fixture);
    }

    void dataDragEdgeScrollMovesTheRowAndSettlesAfterwards()
    {
        Row row(Primary, animatedWideColumns());
        const double before = row.fixture.frame(row.first).left();
        row.fixture.engine().beginDataDrag();
        row.holdDataDragAt(QPointF(5, 500), 4);
        QVERIFY2(row.fixture.frame(row.first).left() > before, "dragging over the left edge did not scroll the row");
        row.fixture.engine().endDataDrag();
        row.fixture.advanceInSteps(1000);
        QVERIFY(row.fixture.perform(QStringLiteral("focus-column-first")).ok);
        row.fixture.advanceInSteps(1000);
        QVERIFY(row.firstIsOnScreen());
        QCOMPARE(row.fixture.focused(), std::optional(row.first));
        VERIFY_INVARIANTS(row.fixture);
    }

    void dataDragEdgeScrollUsesTheOutputsOwnPosition()
    {
        Row row(Secondary, animatedWideColumns());
        QCOMPARE(row.fixture.state(row.first).output, Secondary);
        const double before = row.fixture.frame(row.first).left();
        row.fixture.engine().beginDataDrag();
        row.holdDataDragAt(QPointF(1925, 500), 4);
        QVERIFY2(row.fixture.frame(row.first).left() > before, "dragging over the left edge of the second output did not scroll left");
        row.fixture.engine().endDataDrag();
        VERIFY_INVARIANTS(row.fixture);
    }

    void dataDragEdgeScrollWaitsForTheDelay()
    {
        Row row(Primary, animatedWideColumns());
        const double before = row.fixture.frame(row.first).left();
        row.fixture.engine().beginDataDrag();
        row.fixture.passTime(10);
        row.fixture.engine().dataDragEdgeScroll(Primary, QPointF(5, 500), row.fixture.elapsed());
        row.fixture.passTime(50);
        row.fixture.engine().dataDragEdgeScroll(Primary, QPointF(5, 500), row.fixture.elapsed());
        row.fixture.settle();
        QCOMPARE(row.fixture.frame(row.first).left(), before);
        row.fixture.engine().endDataDrag();
    }

    void rowEdgeScrollFollowsTheTriggerWidthAndDelay_data()
    {
        QTest::addColumn<double>("triggerWidth");
        QTest::addColumn<double>("delayMs");
        QTest::addColumn<double>("pointerX");
        QTest::addColumn<bool>("scrolls");
        QTest::newRow("pointer past the default trigger width") << 30.0 << 100.0 << 100.0 << false;
        QTest::newRow("pointer inside a wider trigger width") << 200.0 << 100.0 << 100.0 << true;
        QTest::newRow("trigger width of zero") << 0.0 << 100.0 << 0.0 << false;
        QTest::newRow("delay longer than the hold") << 30.0 << 1000.0 << 5.0 << false;
        QTest::newRow("no delay") << 30.0 << 0.0 << 5.0 << true;
    }

    void rowEdgeScrollFollowsTheTriggerWidthAndDelay()
    {
        QFETCH(double, triggerWidth);
        QFETCH(double, delayMs);
        QFETCH(double, pointerX);
        QFETCH(bool, scrolls);
        Config::Config config = animatedWideColumns();
        config.gestures.dndEdgeViewScroll = Config::DndEdgeScroll {triggerWidth, delayMs, 1500};
        Row row(Primary, config);
        const double before = row.fixture.frame(row.first).left();
        row.fixture.engine().beginDataDrag();
        row.holdDataDragAt(QPointF(pointerX, 500), 4);
        QCOMPARE(row.fixture.frame(row.first).left() > before, scrolls);
        row.fixture.engine().endDataDrag();
        VERIFY_INVARIANTS(row.fixture);
    }

    void rowEdgeScrollSpeedFollowsMaxSpeed()
    {
        const auto scrolledBy = [](double maxSpeed) {
            Config::Config config = animatedWideColumns();
            config.gestures.dndEdgeViewScroll.maxSpeed = maxSpeed;
            Row row(Primary, config);
            const double before = row.fixture.frame(row.first).left();
            row.fixture.engine().beginDataDrag();
            row.holdDataDragAt(QPointF(0, 500), 4);
            const double distance = row.fixture.frame(row.first).left() - before;
            row.fixture.engine().endDataDrag();
            return distance;
        };
        const double slow = scrolledBy(1000);
        QVERIFY(slow > 0.0);
        QCOMPARE(scrolledBy(2000), 2.0 * slow);
    }

    void holdingADragAtTheBottomEdgeOfTheOverviewSwitchesWorkspace_data()
    {
        QTest::addColumn<bool>("overview");
        QTest::addColumn<Config::DndEdgeScroll>("settings");
        QTest::addColumn<double>("pointerY");
        QTest::addColumn<int>("workspace");
        const Config::DndEdgeScroll defaults {50, 100, 1500};
        QTest::newRow("at the bottom edge") << true << defaults << 1075.0 << 2;
        QTest::newRow("above the trigger height") << true << defaults << 900.0 << 1;
        QTest::newRow("inside a taller trigger height") << true << Config::DndEdgeScroll {250, 100, 1500} << 900.0 << 2;
        QTest::newRow("trigger height of zero") << true << Config::DndEdgeScroll {0, 100, 1500} << 1080.0 << 1;
        QTest::newRow("delay longer than the hold") << true << Config::DndEdgeScroll {50, 5000, 1500} << 1075.0 << 1;
        QTest::newRow("max speed too low to get there") << true << Config::DndEdgeScroll {50, 100, 300} << 1075.0 << 1;
        QTest::newRow("overview closed") << false << defaults << 1075.0 << 1;
    }

    void holdingADragAtTheBottomEdgeOfTheOverviewSwitchesWorkspace()
    {
        QFETCH(bool, overview);
        QFETCH(Config::DndEdgeScroll, settings);
        QFETCH(double, pointerY);
        QFETCH(int, workspace);
        Config::Config config = animatedWideColumns();
        config.gestures.dndEdgeWorkspaceSwitch = settings;
        Row row(Primary, config);
        row.fixture.engine().setOverviewOpen(overview);
        row.fixture.engine().beginDataDrag();
        row.holdDataDragAt(QPointF(960, pointerY), 16);
        row.fixture.engine().endDataDrag();
        row.fixture.advanceInSteps(1000);
        int active = 0;
        for (const Layout::WorkspaceState &state : row.fixture.engine().workspaceStates()) {
            active = state.isActive ? state.index : active;
        }
        QCOMPARE(active, workspace);
        VERIFY_INVARIANTS(row.fixture);
    }

    void dataDragEdgeScrollOnAnUnknownOutputDoesNothing()
    {
        Row row(Primary, animatedWideColumns());
        const QRectF before = row.fixture.frame(row.first);
        row.fixture.engine().beginDataDrag();
        for (int move = 0; move < 4; ++move) {
            row.fixture.passTime(100);
            row.fixture.engine().dataDragEdgeScroll(QStringLiteral("HDMI-9"), QPointF(5, 500), row.fixture.elapsed());
        }
        QCOMPARE(row.fixture.frame(row.first), before);
        row.fixture.engine().endDataDrag();
        VERIFY_INVARIANTS(row.fixture);
    }

    void dataDragInTheMiddleOfTheOutputDoesNotScroll()
    {
        Row row(Primary, animatedWideColumns());
        const QRectF before = row.fixture.frame(row.first);
        row.fixture.engine().beginDataDrag();
        row.holdDataDragAt(QPointF(960, 500), 4);
        QCOMPARE(row.fixture.frame(row.first), before);
        row.fixture.engine().endDataDrag();
        VERIFY_INVARIANTS(row.fixture);
    }

    void droppingOnAnOutputThatWasUnpluggedLandsOnScreen_data()
    {
        QTest::addColumn<bool>("floating");
        QTest::newRow("tiled") << false;
        QTest::newRow("floating") << true;
    }

    void droppingOnAnOutputThatWasUnpluggedLandsOnScreen()
    {
        QFETCH(bool, floating);
        Fixture fixture;
        const QRectF secondary(1920, 0, 2560, 1440);
        fixture.addOutput(Secondary, secondary);
        fixture.engine().focusOutput(Secondary);
        const auto id = fixture.add(QStringLiteral("a"));
        if (floating) {
            QVERIFY(fixture.perform(QStringLiteral("toggle-window-floating")).ok);
        }
        QCOMPARE(fixture.state(id).output, Secondary);
        QVERIFY(startMove(fixture, id, QPointF(1920 + 2400, 1300), Secondary));
        fixture.engine().updateWindowDrag(QPointF(1920 + 2450, 1350), Secondary);
        fixture.removeOutput(Secondary);
        fixture.engine().endWindowDrag();
        fixture.settle();
        QCOMPARE(fixture.state(id).output, Primary);
        QCOMPARE(fixture.state(id).isFloating, floating);
        QVERIFY2(onScreen(fixture.frame(id), QRectF(0, 0, 1920, 1080)), "the dropped window is off screen");
        VERIFY_INVARIANTS(fixture);
    }

    void draggingOnAnOutputThatWasUnpluggedFollowsThePointerOnTheRemainingOutput()
    {
        Fixture fixture;
        fixture.addOutput(Secondary, QRectF(1920, 0, 1920, 1080));
        fixture.engine().focusOutput(Secondary);
        const auto id = fixture.add(QStringLiteral("a"));
        QVERIFY(fixture.perform(QStringLiteral("toggle-window-floating")).ok);
        QVERIFY(startMove(fixture, id, QPointF(2400, 500), Secondary));
        fixture.removeOutput(Secondary);
        fixture.engine().updateWindowDrag(QPointF(700, 400), Primary);
        fixture.engine().endWindowDrag();
        fixture.settle();
        QCOMPARE(fixture.state(id).output, Primary);
        QVERIFY(fixture.frame(id).contains(QPointF(700, 400)));
        VERIFY_INVARIANTS(fixture);
    }

    void unpluggingEveryOutputDuringADragKeepsTheWindow()
    {
        Fixture fixture;
        fixture.add(QStringLiteral("a"));
        const auto id = fixture.add(QStringLiteral("b"));
        QVERIFY(startMove(fixture, id, QPointF(10, 500)));
        fixture.removeOutput(Primary);
        fixture.engine().updateWindowDrag(QPointF(20, 500), Primary);
        fixture.engine().endWindowDrag();
        QVERIFY(fixture.engine().hasWindow(id));
        VERIFY_INVARIANTS(fixture);
        fixture.addOutput(Primary, QRectF(0, 0, 1920, 1080));
        QCOMPARE(fixture.state(id).output, Primary);
        VERIFY_INVARIANTS(fixture);
    }

    void removingAnotherOutputDuringADragKeepsTheDrag()
    {
        Fixture fixture;
        fixture.addOutput(Secondary, QRectF(1920, 0, 1920, 1080));
        fixture.engine().focusOutput(Primary);
        const auto first = fixture.add(QStringLiteral("a"));
        const auto second = fixture.add(QStringLiteral("b"));
        QVERIFY(startMove(fixture, second, QPointF(10, 500)));
        fixture.removeOutput(Secondary);
        fixture.engine().endWindowDrag();
        fixture.settle();
        QCOMPARE(fixture.state(second).output, Primary);
        QCOMPARE(fixture.state(second).columnIndex, 0);
        QCOMPARE(fixture.state(first).columnIndex, 1);
        VERIFY_INVARIANTS(fixture);
    }
};

QTEST_GUILESS_MAIN(TestLayoutDragEdges)
#include "test_layout_dragedges.moc"
