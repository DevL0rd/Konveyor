#include "helpers.h"

using namespace LayoutTest;

namespace
{

const QString Primary = QStringLiteral("DP-1");
const QString Secondary = QStringLiteral("DP-2");

constexpr auto RightEdge = static_cast<quint8>(Layout::ResizeEdge::Right);

bool widenFollows(WideRow &row, Layout::WindowId id)
{
    row.fixture.engine().activateWindow(id);
    row.fixture.perform(QStringLiteral("set-column-width"), {QStringLiteral("1800")});
    row.fixture.advanceInSteps(1000);
    return row.inView(id);
}

Layout::WindowId addOnSecondary(WideRow &row)
{
    row.fixture.addOutput(Secondary, QRectF(1920, 0, 1920, 1080));
    row.fixture.engine().focusOutput(Secondary);
    return row.fixture.add(QStringLiteral("e"));
}

void swipe(Fixture &fixture, const QString &output, double delta)
{
    fixture.engine().beginSwipe(output, true);
    fixture.engine().updateSwipe(delta, 10, true);
}

}

class TestLayoutResizeSwipe : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void secondResizeOnAnotherMonitorEndsTheFirst()
    {
        WideRow row(Primary, animatedWideColumns());
        const auto other = addOnSecondary(row);
        QVERIFY(row.fixture.engine().beginResize(row.last, RightEdge));
        QVERIFY(row.fixture.engine().beginResize(other, RightEdge));
        row.fixture.engine().endResize();
        QVERIFY2(row.fixture.engine().beginResize(row.last, RightEdge), "the first strip is still resizing");
        row.fixture.engine().endResize();
        QVERIFY(widenFollows(row, row.last));
        VERIFY_INVARIANTS(row.fixture);
    }

    void secondResizeOnTheSameRowEndsTheFirst()
    {
        WideRow row(Primary, animatedWideColumns());
        QVERIFY(row.fixture.engine().beginResize(row.last, RightEdge));
        QVERIFY(row.fixture.engine().beginResize(row.third, RightEdge));
        row.fixture.engine().updateResize(QPointF(100, 0));
        row.fixture.engine().endResize();
        row.fixture.settle();
        QCOMPARE(row.fixture.frame(row.third).width(), 1000.0);
        QCOMPARE(row.fixture.frame(row.last).width(), 900.0);
        QVERIFY(row.fixture.engine().beginResize(row.last, RightEdge));
        row.fixture.engine().endResize();
        VERIFY_INVARIANTS(row.fixture);
    }

    void closingTheResizedWindowEndsTheResize()
    {
        WideRow row(Primary, animatedWideColumns());
        QVERIFY(row.fixture.engine().beginResize(row.last, RightEdge));
        row.fixture.engine().updateResize(QPointF(100, 0));
        row.fixture.remove(row.last);
        row.fixture.engine().updateResize(QPointF(200, 0));
        row.fixture.engine().endResize();
        QCOMPARE(row.fixture.frame(row.third).width(), 900.0);
        QVERIFY(row.fixture.engine().beginResize(row.third, RightEdge));
        row.fixture.engine().endResize();
        QVERIFY(widenFollows(row, row.third));
        VERIFY_INVARIANTS(row.fixture);
    }

    void resizeFromTheTopEdgeOfTheTopWindowLeavesTheHeightAlone()
    {
        Fixture fixture;
        const auto top = fixture.add(QStringLiteral("a"));
        fixture.add(QStringLiteral("b"));
        QVERIFY(fixture.perform(QStringLiteral("consume-or-expel-window-left")).ok);
        const double before = fixture.frame(top).height();
        QVERIFY(fixture.engine().beginResize(top, static_cast<quint8>(Layout::ResizeEdge::Top)));
        fixture.engine().updateResize(QPointF(0, -100));
        fixture.engine().endResize();
        fixture.settle();
        QCOMPARE(fixture.frame(top).height(), before);
        VERIFY_INVARIANTS(fixture);
    }

    void resizeWithCenteringAlwaysOnGrowsBothSides()
    {
        Config::Config config = instantConfig();
        config.layout.centerFocusedColumn = Config::CenterFocusedColumn::Always;
        Fixture fixture(config);
        fixture.add(QStringLiteral("a"));
        const auto id = fixture.add(QStringLiteral("b"));
        const double before = fixture.frame(id).width();
        const double center = fixture.frame(id).center().x();
        QVERIFY(fixture.engine().beginResize(id, RightEdge));
        fixture.engine().updateResize(QPointF(50, 0));
        fixture.engine().endResize();
        fixture.settle();
        QCOMPARE(fixture.frame(id).width(), before + 100.0);
        QCOMPARE(fixture.frame(id).center().x(), center);
        VERIFY_INVARIANTS(fixture);
    }

    void resizeWithoutABeginDoesNothing()
    {
        WideRow row(Primary, animatedWideColumns());
        const QRectF before = row.fixture.frame(row.last);
        row.fixture.engine().updateResize(QPointF(100, 0));
        row.fixture.engine().endResize();
        QCOMPARE(row.fixture.frame(row.last), before);
        QVERIFY(!row.fixture.engine().beginResize(99, RightEdge));
        VERIFY_INVARIANTS(row.fixture);
    }

    void resizeOfAFullscreenWindowIsRefused()
    {
        WideRow row(Primary, animatedWideColumns());
        QVERIFY(row.fixture.perform(QStringLiteral("fullscreen-window")).ok);
        QVERIFY(!row.fixture.engine().beginResize(row.last, RightEdge));
        QVERIFY(row.fixture.perform(QStringLiteral("fullscreen-window")).ok);
        QVERIFY(row.fixture.engine().beginResize(row.last, RightEdge));
        row.fixture.engine().endResize();
    }

    void resizeFromTheTopEdgeChangesTheStackedHeights()
    {
        Fixture fixture;
        const auto top = fixture.add(QStringLiteral("a"));
        const auto bottom = fixture.add(QStringLiteral("b"));
        QVERIFY(fixture.perform(QStringLiteral("consume-or-expel-window-left")).ok);
        QCOMPARE(fixture.state(bottom).columnIndex, fixture.state(top).columnIndex);
        const double before = fixture.frame(bottom).height();
        const double bottomEdge = fixture.frame(bottom).bottom();
        QVERIFY(fixture.engine().beginResize(bottom, static_cast<quint8>(Layout::ResizeEdge::Top)));
        fixture.engine().updateResize(QPointF(0, -100));
        fixture.engine().endResize();
        fixture.settle();
        QCOMPARE(fixture.frame(bottom).height(), before + 100.0);
        QCOMPARE(fixture.frame(bottom).bottom(), bottomEdge);
        QCOMPARE(fixture.frame(top).height(), before - 100.0);
        VERIFY_INVARIANTS(fixture);
    }

    void swipeThatOutlivesItsWorkspaceLeavesTheRowWorking()
    {
        WideRow row(Primary, animatedWideColumns());
        QVERIFY(row.fixture.perform(QStringLiteral("move-window-to-workspace-down")).ok);
        const auto elsewhere = row.last;
        QVERIFY(row.fixture.perform(QStringLiteral("focus-workspace-up")).ok);
        row.fixture.advanceInSteps(1000);
        swipe(row.fixture, Primary, 300.0);
        row.fixture.engine().activateWindow(elsewhere);
        row.fixture.engine().updateSwipe(300.0, 20, true);
        row.fixture.engine().endSwipe(true);
        row.fixture.advanceInSteps(1000);
        QCOMPARE(row.fixture.focused(), std::optional(elsewhere));
        QVERIFY(row.fixture.perform(QStringLiteral("focus-workspace-up")).ok);
        row.fixture.advanceInSteps(1000);
        QVERIFY2(widenFollows(row, row.third), "the first workspace is still swiping");
        VERIFY_INVARIANTS(row.fixture);
    }

    void swipeOnAnUnpluggedOutputEndsOnTheOutputItMovedTo()
    {
        WideRow row(Secondary, animatedWideColumns());
        swipe(row.fixture, Secondary, 300.0);
        row.fixture.removeOutput(Secondary);
        row.fixture.engine().updateSwipe(300.0, 20, true);
        row.fixture.engine().endSwipe(true);
        row.fixture.advanceInSteps(1000);
        row.output = Primary;
        QCOMPARE(row.fixture.state(row.first).output, Primary);
        QVERIFY(row.fixture.perform(QStringLiteral("focus-workspace"), {QStringLiteral("2")}).ok);
        row.fixture.advanceInSteps(1000);
        QVERIFY2(widenFollows(row, row.third), "the moved workspace is still swiping");
        VERIFY_INVARIANTS(row.fixture);
    }

    void swipeOnAnUnknownOutputDoesNothing()
    {
        WideRow row(Primary, animatedWideColumns());
        const QRectF before = row.fixture.frame(row.first);
        swipe(row.fixture, QStringLiteral("HDMI-9"), 800.0);
        row.fixture.engine().endSwipe(true);
        row.fixture.advanceInSteps(1000);
        QCOMPARE(row.fixture.frame(row.first), before);
        QCOMPARE(row.fixture.focused(), std::optional(row.last));
    }

    void swipeWithoutABeginDoesNothing()
    {
        WideRow row(Primary, animatedWideColumns());
        const QRectF before = row.fixture.frame(row.first);
        row.fixture.engine().updateSwipe(800.0, 10, true);
        row.fixture.engine().endSwipe(true);
        row.fixture.advanceInSteps(1000);
        QCOMPARE(row.fixture.frame(row.first), before);
    }

    void closingAWindowMidSwipeKeepsTheSwipe()
    {
        WideRow row(Primary, animatedWideColumns());
        swipe(row.fixture, Primary, -900.0);
        row.fixture.remove(row.first);
        row.fixture.engine().updateSwipe(-900.0, 20, true);
        row.fixture.engine().endSwipe(true);
        row.fixture.advanceInSteps(1000);
        QVERIFY(row.fixture.focused().has_value());
        QVERIFY(row.inView(*row.fixture.focused()));
        VERIFY_INVARIANTS(row.fixture);
    }

    void swipeEndingWithKeepActiveKeepsThatWindowFocused()
    {
        WideRow row(Primary, animatedWideColumns());
        swipe(row.fixture, Primary, -3000.0);
        row.fixture.engine().endSwipe(true, row.last);
        row.fixture.advanceInSteps(1000);
        QCOMPARE(row.fixture.focused(), std::optional(row.last));
        QVERIFY(row.inView(row.last));
        VERIFY_INVARIANTS(row.fixture);
    }

    void swipeOnOneOutputLeavesTheOtherAlone()
    {
        WideRow row(Primary, animatedWideColumns());
        const auto other = addOnSecondary(row);
        const QRectF before = row.fixture.frame(other);
        swipe(row.fixture, Primary, -3000.0);
        row.fixture.engine().endSwipe(true);
        row.fixture.advanceInSteps(1000);
        QCOMPARE(row.fixture.frame(other), before);
        QVERIFY(row.fixture.frame(row.first).left() >= 0.0);
        VERIFY_INVARIANTS(row.fixture);
    }
    void resizeEndedByAnotherResizeLetsTheFirstRowSettle()
    {
        Config::Config config = linearAnimationConfig();
        Fixture fixture(config);
        const auto first = fixture.add(QStringLiteral("a"));
        const auto second = fixture.add(QStringLiteral("b"));
        fixture.perform(QStringLiteral("focus-column-left"));
        fixture.addOutput(Secondary, QRectF(1920, 0, 1920, 1080));
        fixture.engine().focusOutput(Secondary);
        const auto other = fixture.add(QStringLiteral("c"));
        fixture.advanceInSteps(400);
        config.layout.struts.left = 200;
        fixture.setConfig(config);
        fixture.advanceInSteps(48);
        QVERIFY(fixture.engine().beginResize(second, RightEdge));
        QVERIFY(fixture.engine().beginResize(other, RightEdge));
        fixture.settle();
        QCOMPARE(fixture.frame(first).left(), 200.0 + 16.0);
        fixture.engine().endResize();
        VERIFY_INVARIANTS(fixture);
    }
};

QTEST_GUILESS_MAIN(TestLayoutResizeSwipe)
#include "test_layout_resizeswipe.moc"
