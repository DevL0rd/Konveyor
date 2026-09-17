#include "input/touchpadcontactreader.h"

#include <core/inputdevice.h>
#include <input.h>

#include <QFileInfo>
#include <QSocketNotifier>

#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <linux/input.h>
#include <sys/ioctl.h>
#include <unistd.h>

namespace Konveyor
{

namespace
{

constexpr qint32 SlotsPerSource = 64;
constexpr qint64 MicrosecondsPerMillisecond = 1000;
constexpr qint64 MillisecondsPerSecond = 1000;

double resolutionOf(int fd, unsigned int axis)
{
    input_absinfo info {};
    if (ioctl(fd, EVIOCGABS(axis), &info) < 0 || info.resolution <= 0) {
        return 0.0;
    }
    return info.resolution;
}

}

TouchpadContactReader::TouchpadContactReader(TouchpadContactHandlers handlers)
    : m_handlers(std::move(handlers))
{
    for (KWin::InputDevice *device : KWin::input()->devices()) {
        add(device);
    }
    connect(KWin::input(), &KWin::InputRedirection::deviceAdded, this, &TouchpadContactReader::add);
    connect(KWin::input(), &KWin::InputRedirection::deviceRemoved, this, &TouchpadContactReader::remove);
}

TouchpadContactReader::~TouchpadContactReader()
{
    for (const std::shared_ptr<Source> &source : std::as_const(m_sources)) {
        source->notifier.reset();
        close(source->fd);
    }
}

void TouchpadContactReader::add(KWin::InputDevice *device)
{
    if (!device->isTouchpad() || m_sources.contains(device)) {
        return;
    }
    const QString node = QStringLiteral("/dev/input/") + QFileInfo(device->sysPath()).fileName();
    const int fd = open(QFile::encodeName(node).constData(), O_RDONLY | O_NONBLOCK | O_CLOEXEC);
    if (fd < 0) {
        qWarning() << "konveyor: touchpad taps are unavailable on" << device->name() << "because" << node
                   << "could not be opened:" << strerror(errno);
        return;
    }
    const double resolutionX = resolutionOf(fd, ABS_MT_POSITION_X);
    const double resolutionY = resolutionOf(fd, ABS_MT_POSITION_Y);
    if (resolutionX <= 0.0 || resolutionY <= 0.0) {
        qWarning() << "konveyor: touchpad taps are unavailable on" << device->name() << "because it reports no multitouch resolution";
        close(fd);
        return;
    }
    auto source = std::make_shared<Source>();
    source->fd = fd;
    source->base = m_nextBase;
    m_nextBase += SlotsPerSource;
    source->resolutionX = resolutionX;
    source->resolutionY = resolutionY;
    source->notifier = std::make_unique<QSocketNotifier>(fd, QSocketNotifier::Read);
    connect(source->notifier.get(), &QSocketNotifier::activated, this, [this, raw = source.get()] { read(*raw); });
    m_sources.insert(device, source);
}

void TouchpadContactReader::remove(KWin::InputDevice *device)
{
    const std::shared_ptr<Source> source = m_sources.take(device);
    if (!source) {
        return;
    }
    source->notifier.reset();
    close(source->fd);
    m_handlers.reset();
}

void TouchpadContactReader::read(Source &source)
{
    input_event event {};
    while (::read(source.fd, &event, sizeof(event)) == sizeof(event)) {
        if (event.type == EV_SYN) {
            handleSync(source, event);
        } else if (!source.dropped) {
            handleEvent(source, event);
        }
    }
}

void TouchpadContactReader::handleSync(Source &source, const input_event &event)
{
    if (event.code == SYN_DROPPED) {
        source.dropped = true;
        source.contacts.clear();
        m_handlers.reset();
    } else if (event.code == SYN_REPORT && !std::exchange(source.dropped, false)) {
        commit(source, event.input_event_sec * MillisecondsPerSecond + event.input_event_usec / MicrosecondsPerMillisecond);
    }
}

void TouchpadContactReader::handleEvent(Source &source, const input_event &event)
{
    if (event.type == EV_KEY && event.code == BTN_LEFT && event.value == 1) {
        m_handlers.press();
        return;
    }
    if (event.type != EV_ABS) {
        return;
    }
    Slot &slot = source.contacts[source.slot];
    switch (event.code) {
    case ABS_MT_SLOT:
        source.slot = event.value;
        break;
    case ABS_MT_TRACKING_ID:
        (event.value >= 0 ? slot.began : slot.ended) = true;
        break;
    case ABS_MT_POSITION_X:
        slot.raw.setX(event.value);
        slot.moved = true;
        break;
    case ABS_MT_POSITION_Y:
        slot.raw.setY(event.value);
        slot.moved = true;
        break;
    default:
        break;
    }
}

void TouchpadContactReader::commit(Source &source, qint64 timestampMs)
{
    for (auto it = source.contacts.begin(); it != source.contacts.end(); ++it) {
        Slot &slot = it.value();
        const QPointF millimeters(slot.raw.x() / source.resolutionX, slot.raw.y() / source.resolutionY);
        if (slot.began && !slot.active) {
            slot.active = true;
            m_handlers.down(source.base + it.key(), millimeters, timestampMs);
        } else if (slot.moved && slot.active) {
            m_handlers.motion(source.base + it.key(), millimeters);
        }
        slot.began = false;
        slot.moved = false;
    }
    for (auto it = source.contacts.begin(); it != source.contacts.end(); ++it) {
        Slot &slot = it.value();
        if (std::exchange(slot.ended, false) && slot.active) {
            slot.active = false;
            m_handlers.up(source.base + it.key(), timestampMs);
        }
    }
}

}
