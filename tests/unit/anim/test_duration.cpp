#include "anim/duration.h"

#include <QTest>

#include <cmath>
#include <limits>

using namespace std::chrono_literals;
using Konveyor::Anim::Duration;
using Konveyor::Anim::durationFromSeconds;
using Konveyor::Anim::saturatingAdd;
using Konveyor::Anim::saturatingSub;
using Konveyor::Anim::toSeconds;

class TestDuration : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void toSecondsKeepsSubSecondPrecision()
    {
        QCOMPARE(toSeconds(1500ms), 1.5);
        QCOMPARE(toSeconds(Duration::zero()), 0.0);
    }

    void durationFromSecondsRoundsToNanoseconds() { QCOMPARE(durationFromSeconds(0.25), Duration(250'000'000)); }

    void durationFromSecondsClampsNonPositiveAndNanToZero()
    {
        QCOMPARE(durationFromSeconds(0.0), Duration::zero());
        QCOMPARE(durationFromSeconds(-3.0), Duration::zero());
        QCOMPARE(durationFromSeconds(std::numeric_limits<double>::quiet_NaN()), Duration::zero());
    }

    void durationFromSecondsSaturatesAtTheLargestDuration()
    {
        QCOMPARE(durationFromSeconds(1e30), Duration::max());
        QCOMPARE(durationFromSeconds(std::numeric_limits<double>::infinity()), Duration::max());
    }

    void additionSaturates()
    {
        QCOMPARE(saturatingAdd(1s, 2s), 3s);
        QCOMPARE(saturatingAdd(Duration::max(), 1ns), Duration::max());
        QCOMPARE(saturatingAdd(1ns, Duration::max()), Duration::max());
    }

    void subtractionStopsAtZero()
    {
        QCOMPARE(saturatingSub(3s, 1s), 2s);
        QCOMPARE(saturatingSub(1s, 3s), Duration::zero());
        QCOMPARE(saturatingSub(1s, 1s), Duration::zero());
    }
};

QTEST_GUILESS_MAIN(TestDuration)
#include "test_duration.moc"
