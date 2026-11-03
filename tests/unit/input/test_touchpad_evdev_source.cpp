#include "input/touchpadevdevsource.h"

#include <QCoreApplication>
#include <QDir>
#include <QTemporaryDir>
#include <QTest>

#include <cstdarg>
#include <cstring>
#include <fcntl.h>
#include <linux/input.h>
#include <sys/stat.h>
#include <unistd.h>

namespace
{

struct FakeAxes
{
    int resolutionX = 0;
    int resolutionY = 0;
    bool failing = false;
};

FakeAxes fakeAxes;

}

extern "C" int ioctl(int, unsigned long request, ...)
{
    va_list arguments;
    va_start(arguments, request);
    void *argument = va_arg(arguments, void *);
    va_end(arguments);
    if (fakeAxes.failing) {
        return -1;
    }
    input_absinfo info {};
    if (request == EVIOCGABS(ABS_MT_POSITION_X)) {
        info.resolution = fakeAxes.resolutionX;
    } else if (request == EVIOCGABS(ABS_MT_POSITION_Y)) {
        info.resolution = fakeAxes.resolutionY;
    } else {
        return -1;
    }
    std::memcpy(argument, &info, sizeof(info));
    return 0;
}

using Konveyor::TouchpadContactHandlers;
using Konveyor::TouchpadEvdevSource;

class TestTouchpadEvdevSource : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir m_dir;
    QString m_node;
    int m_writer = -1;
    QStringList m_events;
    TouchpadContactHandlers m_handlers {
        [this](
            qint32 slot, const QPointF &mm, qint64) { m_events.append(QStringLiteral("down %1 %2,%3").arg(slot).arg(mm.x()).arg(mm.y())); },
        [this](qint32 slot, const QPointF &mm) { m_events.append(QStringLiteral("motion %1 %2,%3").arg(slot).arg(mm.x()).arg(mm.y())); },
        [this](qint32 slot, qint64) { m_events.append(QStringLiteral("up %1").arg(slot)); },
        [this] { m_events.append(QStringLiteral("press")); },
        [this] { m_events.append(QStringLiteral("reset")); },
    };

    static input_event event(quint16 type, quint16 code, qint32 value)
    {
        input_event result {};
        result.type = type;
        result.code = code;
        result.value = value;
        return result;
    }

    void send(const QList<input_event> &events) const
    {
        for (const input_event &item : events) {
            QCOMPARE(::write(m_writer, &item, sizeof(item)), ssize_t(sizeof(item)));
        }
    }

    static int openDescriptors() { return QDir(QStringLiteral("/proc/self/fd")).entryList(QDir::NoDotAndDotDot).size(); }

private Q_SLOTS:
    void init()
    {
        m_events.clear();
        fakeAxes = {10, 20, false};
        m_node = m_dir.filePath(QStringLiteral("event0"));
        QFile::remove(m_node);
        QVERIFY(mkfifo(m_node.toLocal8Bit().constData(), 0600) == 0);
    }

    void cleanup()
    {
        if (m_writer >= 0) {
            close(m_writer);
            m_writer = -1;
        }
    }

    void reportsAMissingNode()
    {
        QTest::ignoreMessage(QtWarningMsg, QRegularExpression(QStringLiteral("could not be opened")));
        QVERIFY(!TouchpadEvdevSource::open(m_dir.filePath(QStringLiteral("missing")), QStringLiteral("pad"), m_handlers, 0));
    }

    void rejectsADeviceWithoutResolution()
    {
        fakeAxes.resolutionY = 0;
        QTest::ignoreMessage(QtWarningMsg, QRegularExpression(QStringLiteral("no multitouch resolution")));
        const int before = openDescriptors();
        QVERIFY(!TouchpadEvdevSource::open(m_node, QStringLiteral("pad"), m_handlers, 0));
        QCOMPARE(openDescriptors(), before);
    }

    void rejectsADeviceWhoseResolutionCannotBeRead()
    {
        fakeAxes.failing = true;
        QTest::ignoreMessage(QtWarningMsg, QRegularExpression(QStringLiteral("no multitouch resolution")));
        QVERIFY(!TouchpadEvdevSource::open(m_node, QStringLiteral("pad"), m_handlers, 0));
    }

    void decodesEventsFromTheNodeAndReleasesTheDescriptor()
    {
        const int before = openDescriptors();
        {
            const std::unique_ptr<TouchpadEvdevSource> source = TouchpadEvdevSource::open(m_node, QStringLiteral("pad"), m_handlers, 64);
            QVERIFY(source);
            QCOMPARE(openDescriptors(), before + 1);
            m_writer = ::open(QFile::encodeName(m_node).constData(), O_WRONLY | O_NONBLOCK);
            QVERIFY(m_writer >= 0);
            send({event(EV_ABS, ABS_MT_SLOT, 0), event(EV_ABS, ABS_MT_TRACKING_ID, 3), event(EV_ABS, ABS_MT_POSITION_X, 100),
                event(EV_ABS, ABS_MT_POSITION_Y, 400), event(EV_SYN, SYN_REPORT, 0)});
            QTRY_COMPARE(m_events, QStringList {QStringLiteral("down 64 10,20")});
            send({event(EV_ABS, ABS_MT_POSITION_X, 150), event(EV_SYN, SYN_REPORT, 0)});
            QTRY_COMPARE(m_events.size(), 2);
            QCOMPARE(m_events.last(), QStringLiteral("motion 64 15,20"));
        }
        QCOMPARE(openDescriptors(), before + 1);
    }
};

QTEST_MAIN(TestTouchpadEvdevSource)
#include "test_touchpad_evdev_source.moc"
