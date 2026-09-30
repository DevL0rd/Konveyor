#include "helpers.h"

using namespace LayoutTest;

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
