#include "input/inputmotion.h"

#include <QTest>

using namespace Konveyor;

class TestInputMotion : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void normalWheelIsAboutHalfSpeed()
    {
        InputMotion motion;
        double at = 0.0;
        for (int notch = 0; notch < 25; ++notch, at += 0.2) {
            motion.scroll(15.0, at);
        }
        const float speed = motion.read(at - 0.1).scrollSpeed;
        QVERIFY2(speed > 0.4F && speed < 0.6F, qPrintable(QString::number(speed)));
    }

    void fasterScrollReadsHigherAndStaysBelowOne()
    {
        InputMotion slow;
        InputMotion fast;
        for (int notch = 0; notch < 50; ++notch) {
            slow.scroll(15.0, notch * 0.2);
            fast.scroll(15.0, notch * 0.02);
        }
        const float slowSpeed = slow.read(49 * 0.2).scrollSpeed;
        const float fastSpeed = fast.read(49 * 0.02).scrollSpeed;
        QVERIFY(fastSpeed > slowSpeed);
        QVERIFY(fastSpeed < 1.0F);
    }

    void directionFollowsTheSignAndFlips()
    {
        InputMotion motion;
        motion.scroll(15.0, 0.0);
        QVERIFY(motion.read(0.01).scrollDirection > 0.9F);
        motion.scroll(-15.0, 2.0);
        QVERIFY(motion.read(2.01).scrollDirection < -0.9F);
    }

    void everythingDecaysToSettled()
    {
        InputMotion motion;
        motion.scroll(15.0, 0.0);
        motion.pointer(100.0, 0.0);
        motion.key(0.0);
        const InputReadings early = motion.read(0.01);
        QVERIFY(!early.settled);
        QVERIFY(motion.read(0.5).scrollSpeed < early.scrollSpeed);
        QVERIFY(motion.read(0.5).pointerSpeed < early.pointerSpeed);
        QVERIFY(motion.read(0.5).typingRate < early.typingRate);
        QVERIFY(motion.read(60.0).settled);
    }

    void decayDependsOnElapsedTimeNotOnHowOftenItIsRead()
    {
        InputMotion once;
        InputMotion often;
        once.key(0.0);
        often.key(0.0);
        for (int tick = 1; tick < 100; ++tick) {
            often.read(tick * 0.01);
        }
        QCOMPARE(once.read(1.0).typingRate, often.read(1.0).typingRate);
    }

    void highResolutionDeltasAddUpToSteps()
    {
        InputMotion motion;
        int steps = 0;
        for (int tick = 0; tick < 150; ++tick) {
            steps += motion.scroll(1.0, tick * 0.01);
        }
        QCOMPARE(steps, 10);
    }

    void reversingDropsThePartialStep()
    {
        InputMotion motion;
        QCOMPARE(motion.scroll(14.0, 0.0), 0);
        QCOMPARE(motion.scroll(-1.0, 0.1), 0);
        QCOMPARE(motion.scroll(-14.0, 0.2), 1);
    }

    void aWholeNotchIsOneStepAnyDirection()
    {
        InputMotion motion;
        QCOMPARE(motion.scroll(15.0, 0.0), 1);
        QCOMPARE(motion.scroll(-15.0, 0.1), 1);
        QCOMPARE(motion.scroll(45.0, 0.2), 3);
    }

    void distanceGrowsWhileScrollingAndFades()
    {
        InputMotion motion;
        for (int notch = 0; notch < 10; ++notch) {
            motion.scroll(15.0, notch * 0.01);
        }
        const float distance = motion.read(0.1).scrollDistance;
        QVERIFY2(distance > 0.4F && distance < 0.6F, qPrintable(QString::number(distance)));
        QVERIFY(motion.read(5.0).scrollDistance < distance / 2.0F);
    }

    void typingAtFiveKeysASecondIsAboutHalf()
    {
        InputMotion motion;
        double at = 0.0;
        for (int key = 0; key < 50; ++key, at += 0.2) {
            motion.key(at);
        }
        const float rate = motion.read(at - 0.1).typingRate;
        QVERIFY2(rate > 0.4F && rate < 0.6F, qPrintable(QString::number(rate)));
    }
};

QTEST_MAIN(TestInputMotion)
#include "test_input_motion.moc"
