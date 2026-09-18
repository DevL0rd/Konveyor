#include "framerate.h"

#include <QTest>

using namespace ProcessMonitor;

class TestFrameRate : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void measuresRecentFrames()
    {
        FrameRateTracker tracker;
        constexpr qint64 interval = 1000000000 / 60;
        for (int frame = 0; frame <= 60; ++frame) {
            tracker.addFrame(frame * interval);
        }
        const auto rate = tracker.rateAt(60 * interval);
        QVERIFY(rate.has_value());
        QCOMPARE(rate->fps, 60);
        QVERIFY(qAbs(rate->frametime - 16.67) < 0.05);
        QCOMPARE(rate->fpsLow, 60);
    }

    void expiresInactiveWindow()
    {
        FrameRateTracker tracker;
        tracker.addFrame(0);
        tracker.addFrame(16666666);
        QVERIFY(!tracker.rateAt(2000000000).has_value());
    }

    void resetsAfterLongPause()
    {
        FrameRateTracker tracker;
        tracker.addFrame(0);
        tracker.addFrame(16666666);
        tracker.addFrame(3000000000);
        QVERIFY(!tracker.rateAt(3000000000).has_value());
    }

    void ignoresDuplicateDamage()
    {
        FrameRateTracker tracker;
        tracker.addFrame(0);
        tracker.addFrame(100000);
        tracker.addFrame(16666666);
        const auto rate = tracker.rateAt(16666666);
        QVERIFY(rate.has_value());
        QCOMPARE(rate->fps, 60);
    }
};

QTEST_GUILESS_MAIN(TestFrameRate)

#include "test_framerate.moc"
