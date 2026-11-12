#include "helpers.h"

using namespace LayoutTest;

namespace
{

constexpr double Gaps = 16.0;

Config::Config wrapConfig(Config::Config config, bool wrap, Config::CenterFocusedColumn center, double width)
{
    config.layout.wrapColumns = wrap;
    config.layout.centerFocusedColumn = center;
    config.layout.defaultColumnWidth = Config::Proportion {width};
    return config;
}

Config::Config centeredRow(bool wrap)
{
    return wrapConfig(instantConfig(), wrap, Config::CenterFocusedColumn::Always, 0.9);
}

QList<Layout::WindowId> addColumns(Fixture &fixture, int count)
{
    QList<Layout::WindowId> ids(count);
    for (Layout::WindowId &id : ids) {
        id = fixture.add(QStringLiteral("wrapped"));
    }
    return ids;
}

bool near(double actual, double expected)
{
    return std::abs(actual - expected) <= 1.0;
}

bool centered(Fixture &fixture, Layout::WindowId id)
{
    const QRectF frame = fixture.frame(id);
    return std::abs(frame.center().x() - 960.0) < 1.0;
}

void swipeBy(Fixture &fixture, double delta)
{
    fixture.advance(1000);
    fixture.engine().beginSwipe(QStringLiteral("DP-1"), false);
    fixture.engine().updateSwipe(delta, 10, false);
    fixture.engine().endSwipe(false);
    fixture.settle();
}

QList<Layout::WindowId> activateFirstBesideFullscreenLast(Fixture &fixture)
{
    const QList<Layout::WindowId> row = addColumns(fixture, 4);
    fixture.engine().setWindowFullscreen(row[3], true);
    fixture.settle();
    fixture.engine().activateWindow(row[0]);
    fixture.settle();
    return row;
}

struct ThreeColumnRow
{
    Layout::WindowId left;
    Layout::WindowId wide;
    Layout::WindowId right;
};

ThreeColumnRow addThreeColumnRow(Fixture &fixture)
{
    ThreeColumnRow row;
    row.left = fixture.add(QStringLiteral("left"));
    row.wide = fixture.add(QStringLiteral("wide"));
    row.right = fixture.add(QStringLiteral("right"));
    fixture.engine().activateWindow(row.left);
    fixture.settle();
    fixture.perform(QStringLiteral("set-column-width"), {QStringLiteral("1260")});
    fixture.engine().activateWindow(row.wide);
    fixture.settle();
    fixture.perform(QStringLiteral("set-column-width"), {QStringLiteral("5088")});
    fixture.engine().activateWindow(row.right);
    fixture.settle();
    fixture.perform(QStringLiteral("set-column-width"), {QStringLiteral("1260")});
    return row;
}

Config::Config wideConfig()
{
    return wrapConfig(instantConfig(), true, Config::CenterFocusedColumn::OnOverflow, 0.25);
}

bool onScreen(Fixture &fixture, Layout::WindowId id)
{
    const QRectF frame = fixture.frame(id);
    return frame.right() > 0.0 && frame.left() < 5120.0;
}

}

class TestLayoutColumnWrap : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void theFirstColumnFollowsTheLastOne()
    {
        Fixture fixture(centeredRow(true));
        const QList<Layout::WindowId> row = addColumns(fixture, 4);
        QVERIFY(centered(fixture, row[3]));
        QVERIFY(near(fixture.frame(row[0]).left(), fixture.frame(row[3]).right() + Gaps));
        QVERIFY(near(fixture.frame(row[2]).right(), fixture.frame(row[3]).left() - Gaps));
        fixture.perform(QStringLiteral("focus-column-first"));
        QVERIFY(centered(fixture, row[0]));
        QVERIFY(near(fixture.frame(row[3]).right(), fixture.frame(row[0]).left() - Gaps));
        QVERIFY(near(fixture.frame(row[1]).left(), fixture.frame(row[0]).right() + Gaps));
        VERIFY_INVARIANTS(fixture);
    }

    void aWideColumnStillWrapsAroundInBothDirections()
    {
        Fixture fixture(wideConfig(), QRectF(0, 0, 5120, 1440));
        const ThreeColumnRow row = addThreeColumnRow(fixture);
        const std::array<Layout::WindowId, 3> order {row.left, row.wide, row.right};
        std::size_t at = 2;
        for (const char *direction : {"focus-column-right", "focus-column-left"}) {
            const int step = QLatin1String(direction) == QLatin1String("focus-column-right") ? 1 : 2;
            for (int move = 0; move < 7; ++move) {
                QVERIFY(fixture.perform(QString::fromLatin1(direction)).ok);
                at = (at + static_cast<std::size_t>(step)) % 3;
                QCOMPARE(fixture.focused(), std::optional(order[at]));
                QVERIFY2(onScreen(fixture, order[at]), "the focused column is on screen");
                QVERIFY2(fixture.frame(order[at]).left() >= 0.0 && fixture.frame(order[at]).right() <= 5120.0,
                    "the focused column is fully shown");
            }
        }
        VERIFY_INVARIANTS(fixture);
    }

    void aWideColumnSlidesForwardInsteadOfBackToTheStart_data()
    {
        QTest::addColumn<bool>("forward");
        QTest::newRow("focus right") << true;
        QTest::newRow("focus left") << false;
    }

    void aWideColumnSlidesForwardInsteadOfBackToTheStart()
    {
        QFETCH(bool, forward);
        Fixture fixture(
            wrapConfig(linearAnimationConfig(200), true, Config::CenterFocusedColumn::OnOverflow, 0.25), QRectF(0, 0, 5120, 1440));
        const ThreeColumnRow row = addThreeColumnRow(fixture);
        fixture.advanceInSteps(1000);
        const std::array<Layout::WindowId, 3> ids {row.left, row.wide, row.right};
        const double period = fixture.frame(row.left).width() * 2.0 + fixture.frame(row.wide).width() + 3.0 * Gaps;
        for (int move = 0; move < 6; ++move) {
            std::array<double, 3> previous {};
            for (std::size_t i = 0; i < ids.size(); ++i) {
                previous[i] = fixture.state(ids[i]).renderFrame.x();
            }
            QVERIFY(fixture.perform(QString::fromLatin1(forward ? "focus-column-right" : "focus-column-left")).ok);
            for (int step = 0; step < 16; ++step) {
                fixture.advance(16);
                for (std::size_t i = 0; i < ids.size(); ++i) {
                    const double moved = fixture.state(ids[i]).renderFrame.x() - previous[i];
                    const double delta = moved - std::round(moved / period) * period;
                    QVERIFY2(forward ? delta <= 0.5 : delta >= -0.5, "every column keeps sliding the same way");
                    previous[i] += moved;
                }
            }
            fixture.advanceInSteps(400);
            QVERIFY(onScreen(fixture, *fixture.focused()));
        }
        VERIFY_INVARIANTS(fixture);
    }

    void withoutTheOptionTheRowEndsAtTheLastColumn()
    {
        Fixture fixture(centeredRow(false));
        const QList<Layout::WindowId> row = addColumns(fixture, 4);
        QVERIFY(centered(fixture, row[3]));
        QVERIFY(fixture.frame(row[0]).right() < fixture.frame(row[3]).left());
        QVERIFY(!fixture.perform(QStringLiteral("focus-column-right")).ok || fixture.focused() == row[3]);
        QCOMPARE(fixture.focused(), std::optional(row[3]));
        VERIFY_INVARIANTS(fixture);
    }

    void focusKeysGoAroundBothEnds()
    {
        Fixture fixture(centeredRow(true));
        const QList<Layout::WindowId> row = addColumns(fixture, 4);
        QVERIFY(fixture.perform(QStringLiteral("focus-column-right")).ok);
        QCOMPARE(fixture.focused(), std::optional(row[0]));
        QVERIFY(centered(fixture, row[0]));
        QVERIFY(fixture.perform(QStringLiteral("focus-column-left")).ok);
        QCOMPARE(fixture.focused(), std::optional(row[3]));
        QVERIFY(centered(fixture, row[3]));
        QVERIFY(fixture.perform(QStringLiteral("focus-column-right")).ok);
        QVERIFY(fixture.perform(QStringLiteral("focus-window-down-or-column-right")).ok);
        QCOMPARE(fixture.focused(), std::optional(row[1]));
        VERIFY_INVARIANTS(fixture);
    }

    void crossingTheSeamSlidesOneColumnForward()
    {
        Fixture fixture(wrapConfig(linearAnimationConfig(200), true, Config::CenterFocusedColumn::Always, 0.9));
        const QList<Layout::WindowId> row = addColumns(fixture, 4);
        fixture.advanceInSteps(1000);
        const double start = fixture.state(row[0]).renderFrame.x();
        const double end = fixture.frame(row[3]).x();
        QVERIFY(fixture.perform(QStringLiteral("focus-column-right")).ok);
        double previous = start;
        for (int step = 0; step < 15; ++step) {
            fixture.advance(16);
            const double x = fixture.state(row[0]).renderFrame.x();
            QVERIFY2(x <= previous + 0.5, "the first column only ever slides left");
            QVERIFY2(x >= end - 0.5, "the slide never passes the centered position");
            previous = x;
        }
        fixture.advanceInSteps(400);
        QCOMPARE(fixture.state(row[0]).renderFrame.x(), end);
        QVERIFY(near(fixture.state(row[3]).renderFrame.right(), end - Gaps));
        VERIFY_INVARIANTS(fixture);
    }

    void aRowThatFitsDoesNotWrap()
    {
        Fixture fixture(wrapConfig(instantConfig(), true, Config::CenterFocusedColumn::Never, 0.3));
        const QList<Layout::WindowId> row = addColumns(fixture, 3);
        QCOMPARE(fixture.frame(row[0]).x(), Gaps);
        fixture.perform(QStringLiteral("focus-column-right"));
        QCOMPARE(fixture.focused(), std::optional(row[2]));
        QCOMPARE(fixture.frame(row[0]).x(), Gaps);
        VERIFY_INVARIANTS(fixture);
    }

    void revealModeShowsTheFirstColumnAtTheRightEdge()
    {
        Fixture fixture(wrapConfig(instantConfig(), true, Config::CenterFocusedColumn::Never, 0.6));
        const QList<Layout::WindowId> row = addColumns(fixture, 3);
        QCOMPARE(fixture.frame(row[2]).right(), 1920.0 - Gaps);
        QVERIFY(fixture.perform(QStringLiteral("focus-column-right")).ok);
        QCOMPARE(fixture.focused(), std::optional(row[0]));
        QCOMPARE(fixture.frame(row[0]).right(), 1920.0 - Gaps);
        QVERIFY(near(fixture.frame(row[2]).right(), fixture.frame(row[0]).left() - Gaps));
        VERIFY_INVARIANTS(fixture);
    }

    void swipingPastTheLastColumnLandsOnTheFirst()
    {
        Fixture fixture(centeredRow(true));
        const QList<Layout::WindowId> row = addColumns(fixture, 4);
        swipeBy(fixture, fixture.frame(row[3]).width() + Gaps);
        QCOMPARE(fixture.focused(), std::optional(row[0]));
        QVERIFY(centered(fixture, row[0]));
        swipeBy(fixture, -2.0 * (fixture.frame(row[0]).width() + Gaps));
        QCOMPARE(fixture.focused(), std::optional(row[2]));
        QVERIFY(centered(fixture, row[2]));
        VERIFY_INVARIANTS(fixture);
    }

    void columnOrMonitorFocusStillCrossesToTheNextMonitor()
    {
        Fixture fixture(centeredRow(true));
        const QList<Layout::WindowId> row = addColumns(fixture, 4);
        fixture.addOutput(QStringLiteral("DP-2"), QRectF(1920, 0, 1920, 1080));
        fixture.engine().activateWindow(row[3]);
        fixture.settle();
        QVERIFY(fixture.perform(QStringLiteral("focus-column-or-monitor-right")).ok);
        QCOMPARE(fixture.engine().focusedOutput(), std::optional(QStringLiteral("DP-2")));
        VERIFY_INVARIANTS(fixture);
    }

    void columnOrMonitorFocusWrapsWhenThereIsNoMonitorThatWay()
    {
        Fixture fixture(centeredRow(true));
        const QList<Layout::WindowId> row = addColumns(fixture, 4);
        QVERIFY(fixture.perform(QStringLiteral("focus-column-or-monitor-right")).ok);
        QCOMPARE(fixture.focused(), std::optional(row[0]));
        QVERIFY(fixture.perform(QStringLiteral("focus-column-or-monitor-left")).ok);
        QCOMPARE(fixture.focused(), std::optional(row[3]));
        QVERIFY(centered(fixture, row[3]));
        VERIFY_INVARIANTS(fixture);
    }

    void columnOrMonitorFocusStopsAtTheEndsWithoutTheOption()
    {
        Fixture fixture(centeredRow(false));
        const QList<Layout::WindowId> row = addColumns(fixture, 4);
        QVERIFY(fixture.perform(QStringLiteral("focus-column-or-monitor-right")).ok);
        QCOMPARE(fixture.focused(), std::optional(row[3]));
        fixture.perform(QStringLiteral("focus-column-first"));
        QVERIFY(fixture.perform(QStringLiteral("focus-column-or-monitor-left")).ok);
        QCOMPARE(fixture.focused(), std::optional(row[0]));
        VERIFY_INVARIANTS(fixture);
    }

    void aFullscreenColumnAcrossTheSeamIsOverlaidFromTheRight()
    {
        Fixture fixture(centeredRow(true));
        const QList<Layout::WindowId> row = activateFirstBesideFullscreenLast(fixture);
        QVERIFY(fixture.frame(row[0]).center().x() > 960.0);
        QVERIFY(near(fixture.frame(row[0]).right(), 1920.0 - Gaps));
        VERIFY_INVARIANTS(fixture);
    }

    void aLoneFullscreenColumnStaysOnTheSideItIsOn()
    {
        Fixture fixture(centeredRow(true));
        const QList<Layout::WindowId> row = addColumns(fixture, 2);
        fixture.engine().setWindowFullscreen(row[1], true);
        fixture.settle();
        fixture.engine().activateWindow(row[0]);
        fixture.settle();
        QVERIFY(fixture.frame(row[0]).center().x() < 960.0);
        QVERIFY(near(fixture.frame(row[0]).left(), Gaps));
        fixture.engine().activateWindow(row[1]);
        fixture.settle();
        fixture.engine().activateWindow(row[0]);
        fixture.settle();
        QVERIFY(fixture.frame(row[0]).center().x() < 960.0);
        VERIFY_INVARIANTS(fixture);
    }

    void aFullscreenColumnAcrossTheSeamIsOverlaidFromTheLeftWithoutTheOption()
    {
        Fixture fixture(centeredRow(false));
        const QList<Layout::WindowId> row = activateFirstBesideFullscreenLast(fixture);
        QVERIFY(fixture.frame(row[0]).center().x() < 960.0);
        VERIFY_INVARIANTS(fixture);
    }

    void movingTheLastColumnToTheFirstSlidesTheShortWay()
    {
        Fixture fixture(wrapConfig(linearAnimationConfig(200), true, Config::CenterFocusedColumn::Always, 0.9));
        const QList<Layout::WindowId> row = addColumns(fixture, 4);
        fixture.advanceInSteps(1000);
        const double moved = fixture.state(row[3]).renderFrame.x();
        const double next = fixture.state(row[0]).renderFrame.x();
        QVERIFY(fixture.perform(QStringLiteral("move-column-to-first")).ok);
        for (int step = 0; step < 15; ++step) {
            fixture.advance(16);
            QVERIFY2(near(fixture.state(row[3]).renderFrame.x(), moved), "the moved column stays at the centered position");
            QVERIFY2(near(fixture.state(row[0]).renderFrame.x(), next), "the column that followed it stays put");
        }
        fixture.advanceInSteps(400);
        QVERIFY(near(fixture.state(row[3]).renderFrame.x(), moved));
        VERIFY_INVARIANTS(fixture);
    }

    void eachOutputFollowsItsOwnSetting()
    {
        Config::Config config = centeredRow(false);
        Config::OutputConfig wrapped;
        wrapped.name = QStringLiteral("DP-2");
        Config::LayoutPart part;
        part.wrapColumns = true;
        wrapped.layout = part;
        config.outputs.append(wrapped);
        Fixture fixture(config);
        const QList<Layout::WindowId> plain = addColumns(fixture, 3);
        fixture.addOutput(QStringLiteral("DP-2"), QRectF(1920, 0, 1920, 1080));
        QVERIFY(fixture.perform(QStringLiteral("focus-monitor-right")).ok);
        const QList<Layout::WindowId> wrappedRow = addColumns(fixture, 3);
        QCOMPARE(fixture.state(wrappedRow[0]).output, QStringLiteral("DP-2"));
        QVERIFY(fixture.perform(QStringLiteral("focus-column-right")).ok);
        QCOMPARE(fixture.focused(), std::optional(wrappedRow[0]));
        QVERIFY(fixture.perform(QStringLiteral("focus-monitor-left")).ok);
        fixture.engine().activateWindow(plain[2]);
        fixture.settle();
        fixture.perform(QStringLiteral("focus-column-right"));
        QCOMPARE(fixture.focused(), std::optional(plain[2]));
        VERIFY_INVARIANTS(fixture);
    }
};

QTEST_GUILESS_MAIN(TestLayoutColumnWrap)
#include "test_layout_columnwrap.moc"
