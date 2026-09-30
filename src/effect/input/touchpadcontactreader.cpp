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
    source->decoder = std::make_unique<TouchpadContactDecoder>(m_handlers, m_nextBase, QPointF(resolutionX, resolutionY));
    m_nextBase += SlotsPerSource;
    source->notifier = std::make_unique<QSocketNotifier>(fd, QSocketNotifier::Read);
    connect(source->notifier.get(), &QSocketNotifier::activated, this, [raw = source.get()] { read(*raw); });
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
        source.decoder->feed(event);
    }
}

}
