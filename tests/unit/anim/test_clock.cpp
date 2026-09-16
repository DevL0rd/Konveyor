#include "anim/clock.h"

#include <QTest>

using namespace std::chrono_literals;
using Konveyor::Anim::Clock;
using Konveyor::Anim::Duration;

class TestClock : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void frozenClock()
    {
        Clock clock = Clock::frozenAt(Duration::zero());
        QCOMPARE(clock.now(), Duration::zero());
        clock.setRawNow(100ms);
        QCOMPARE(clock.now(), Duration(100ms));
        clock.setRawNow(200ms);
        QCOMPARE(clock.now(), Duration(200ms));
    }

    void rateChange()
    {
        Clock clock = Clock::frozenAt(Duration::zero());
        clock.setRate(0.5);

        clock.setRawNow(100ms);
        QCOMPARE(clock.rawNow(), Duration(100ms));
        QCOMPARE(clock.now(), Duration(50ms));

        clock.setRawNow(200ms);
        QCOMPARE(clock.rawNow(), Duration(200ms));
        QCOMPARE(clock.now(), Duration(100ms));

        clock.setRawNow(150ms);
        QCOMPARE(clock.rawNow(), Duration(150ms));
        QCOMPARE(clock.now(), Duration(75ms));

        clock.setRate(2.0);
        clock.setRawNow(250ms);
        QCOMPARE(clock.rawNow(), Duration(250ms));
        QCOMPARE(clock.now(), Duration(275ms));
    }

    void rewindSaturatesAtZero()
    {
        Clock clock = Clock::frozenAt(100ms);
        clock.setRate(2.0);
        clock.setRawNow(Duration::zero());
        QCOMPARE(clock.now(), Duration::zero());
    }

    void rateIsClamped()
    {
        Clock clock = Clock::frozenAt(Duration::zero());
        clock.setRate(-1.0);
        QCOMPARE(clock.rate(), 0.0);
        clock.setRate(5000.0);
        QCOMPARE(clock.rate(), 1000.0);
    }

    void zeroRateFreezesTime()
    {
        Clock clock = Clock::frozenAt(10ms);
        clock.setRate(0.0);
        clock.setRawNow(500ms);
        QCOMPARE(clock.now(), Duration(10ms));
    }

    void timeSourceIsFetchedLazilyUntilCleared()
    {
        int calls = 0;
        Duration sourceTime = 5ms;
        Clock clock([&] {
            ++calls;
            return sourceTime;
        });
        QCOMPARE(calls, 1);
        sourceTime = 9ms;
        QCOMPARE(clock.now(), Duration(5ms));
        QCOMPARE(calls, 1);
        clock.clear();
        QCOMPARE(clock.now(), Duration(9ms));
        QCOMPARE(calls, 2);
    }

    void monotonicClockAdvances()
    {
        Clock clock;
        const Duration first = clock.now();
        QCOMPARE(clock.now(), first);
        QTest::qSleep(2);
        clock.clear();
        QVERIFY(clock.now() > first);
    }

    void copiesShareState()
    {
        Clock clock = Clock::frozenAt(Duration::zero());
        Clock copy = clock;
        copy.setRawNow(42ms);
        copy.setSkipAnimations(true);
        QCOMPARE(clock.now(), Duration(42ms));
        QVERIFY(clock.skipsAnimations());
        QVERIFY(clock == copy);
        QVERIFY(!(clock == Clock::frozenAt(Duration::zero())));
    }

    void applyConfigSlowdown_data()
    {
        QTest::addColumn<double>("slowdown");
        QTest::addColumn<double>("rate");
        QTest::newRow("normal") << 1.0 << 1.0;
        QTest::newRow("slower") << 2.0 << 0.5;
        QTest::newRow("faster") << 0.5 << 2.0;
        QTest::newRow("zero") << 0.0 << 1000.0;
        QTest::newRow("negative") << -3.0 << 1000.0;
    }

    void applyConfigSlowdown()
    {
        QFETCH(double, slowdown);
        QFETCH(double, rate);
        Konveyor::Config::Animations config;
        config.slowdown = slowdown;
        Clock clock = Clock::frozenAt(Duration::zero());
        clock.applyConfig(config);
        QCOMPARE(clock.rate(), rate);
        QVERIFY(!clock.skipsAnimations());
    }

    void applyConfigDisabledCompletesInstantly()
    {
        Konveyor::Config::Animations config;
        config.enabled = false;
        Clock clock = Clock::frozenAt(Duration::zero());
        clock.applyConfig(config);
        QVERIFY(clock.skipsAnimations());
        config.enabled = true;
        clock.applyConfig(config);
        QVERIFY(!clock.skipsAnimations());
    }
};

QTEST_GUILESS_MAIN(TestClock)
#include "test_clock.moc"
