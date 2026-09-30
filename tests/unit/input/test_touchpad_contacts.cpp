#include "input/touchpadcontactdecoder.h"

#include <QTest>

#include <linux/input.h>

using Konveyor::TouchpadContactDecoder;
using Konveyor::TouchpadContactHandlers;

class TestTouchpadContacts : public QObject
{
    Q_OBJECT

private:
    QStringList m_events;
    TouchpadContactHandlers m_handlers {
        [this](qint32 slot, const QPointF &mm, qint64 time) {
            m_events.append(QStringLiteral("down %1 %2,%3 @%4").arg(slot).arg(mm.x()).arg(mm.y()).arg(time));
        },
        [this](qint32 slot, const QPointF &mm) { m_events.append(QStringLiteral("motion %1 %2,%3").arg(slot).arg(mm.x()).arg(mm.y())); },
        [this](qint32 slot, qint64 time) { m_events.append(QStringLiteral("up %1 @%2").arg(slot).arg(time)); },
        [this] { m_events.append(QStringLiteral("press")); },
        [this] { m_events.append(QStringLiteral("reset")); },
    };

    static input_event event(quint16 type, quint16 code, qint32 value, qint64 milliseconds = 0)
    {
        input_event result {};
        result.input_event_sec = milliseconds / 1000;
        result.input_event_usec = (milliseconds % 1000) * 1000;
        result.type = type;
        result.code = code;
        result.value = value;
        return result;
    }

    static void feed(TouchpadContactDecoder &decoder, const QList<input_event> &events)
    {
        for (const input_event &event : events) {
            decoder.feed(event);
        }
    }

    static input_event report(qint64 milliseconds) { return event(EV_SYN, SYN_REPORT, 0, milliseconds); }

private Q_SLOTS:
    void init() { m_events.clear(); }

    void followsOneFingerInMillimeters()
    {
        TouchpadContactDecoder decoder(m_handlers, 0, QPointF(10.0, 20.0));
        feed(decoder,
            {event(EV_ABS, ABS_MT_SLOT, 0), event(EV_ABS, ABS_MT_TRACKING_ID, 7), event(EV_ABS, ABS_MT_POSITION_X, 100),
                event(EV_ABS, ABS_MT_POSITION_Y, 200), report(1500)});
        feed(decoder, {event(EV_ABS, ABS_MT_POSITION_X, 150), report(1510)});
        feed(decoder, {event(EV_ABS, ABS_MT_POSITION_Y, 400), report(1520)});
        feed(decoder, {report(1530)});
        feed(decoder, {event(EV_ABS, ABS_MT_TRACKING_ID, -1), report(1540)});
        QCOMPARE(m_events,
            (QStringList {QStringLiteral("down 0 10,10 @1500"), QStringLiteral("motion 0 15,10"), QStringLiteral("motion 0 15,20"),
                QStringLiteral("up 0 @1540")}));
    }

    void keepsSlotsApartAndOffsetsThem()
    {
        TouchpadContactDecoder decoder(m_handlers, 64, QPointF(1.0, 1.0));
        feed(decoder,
            {event(EV_ABS, ABS_MT_TRACKING_ID, 1), event(EV_ABS, ABS_MT_POSITION_X, 5), event(EV_ABS, ABS_MT_SLOT, 1),
                event(EV_ABS, ABS_MT_TRACKING_ID, 2), event(EV_ABS, ABS_MT_POSITION_X, 9), report(10)});
        feed(decoder, {event(EV_ABS, ABS_MT_TRACKING_ID, -1), report(20)});
        feed(decoder, {event(EV_ABS, ABS_MT_SLOT, 0), event(EV_ABS, ABS_MT_POSITION_Y, 3), report(30)});
        QCOMPARE(m_events.size(), 4);
        QVERIFY(m_events.contains(QStringLiteral("down 64 5,0 @10")));
        QVERIFY(m_events.contains(QStringLiteral("down 65 9,0 @10")));
        QCOMPARE(m_events.at(2), QStringLiteral("up 65 @20"));
        QCOMPARE(m_events.at(3), QStringLiteral("motion 64 5,3"));
    }

    void aTouchThatStartsAndEndsInOneFrameStillTaps()
    {
        TouchpadContactDecoder decoder(m_handlers, 0, QPointF(1.0, 1.0));
        feed(decoder,
            {event(EV_ABS, ABS_MT_TRACKING_ID, 3), event(EV_ABS, ABS_MT_POSITION_X, 4), event(EV_ABS, ABS_MT_TRACKING_ID, -1), report(40)});
        QCOMPARE(m_events, (QStringList {QStringLiteral("down 0 4,0 @40"), QStringLiteral("up 0 @40")}));
    }

    void ignoresMotionAndLiftsOfSlotsThatNeverStarted()
    {
        TouchpadContactDecoder decoder(m_handlers, 0, QPointF(1.0, 1.0));
        feed(decoder,
            {event(EV_ABS, ABS_MT_POSITION_X, 4), event(EV_ABS, ABS_MT_TRACKING_ID, -1), event(EV_ABS, ABS_MT_PRESSURE, 30),
                event(EV_KEY, BTN_TOUCH, 1), event(EV_SYN, SYN_CONFIG, 0), report(50)});
        QVERIFY(m_events.isEmpty());
    }

    void reportsPhysicalClicksOnly()
    {
        TouchpadContactDecoder decoder(m_handlers, 0, QPointF(1.0, 1.0));
        feed(decoder, {event(EV_KEY, BTN_LEFT, 1), event(EV_KEY, BTN_LEFT, 0), event(EV_KEY, BTN_RIGHT, 1), report(60)});
        QCOMPARE(m_events, QStringList {QStringLiteral("press")});
    }

    void startsOverAfterDroppedEvents()
    {
        TouchpadContactDecoder decoder(m_handlers, 0, QPointF(1.0, 1.0));
        feed(decoder, {event(EV_ABS, ABS_MT_TRACKING_ID, 1), event(EV_ABS, ABS_MT_POSITION_X, 2), report(70)});
        feed(decoder, {event(EV_SYN, SYN_DROPPED, 0), event(EV_ABS, ABS_MT_TRACKING_ID, -1), event(EV_KEY, BTN_LEFT, 1), report(80)});
        feed(decoder, {event(EV_ABS, ABS_MT_TRACKING_ID, 2), event(EV_ABS, ABS_MT_POSITION_X, 6), report(90)});
        QCOMPARE(m_events, (QStringList {QStringLiteral("down 0 2,0 @70"), QStringLiteral("reset"), QStringLiteral("down 0 6,0 @90")}));
    }
};

QTEST_GUILESS_MAIN(TestTouchpadContacts)

#include "test_touchpad_contacts.moc"
