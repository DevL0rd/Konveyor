#include "input/touchpadevdevsource.h"

#include <QDebug>
#include <QFile>

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

double resolutionOf(int fd, unsigned int axis)
{
    input_absinfo info {};
    if (ioctl(fd, EVIOCGABS(axis), &info) < 0 || info.resolution <= 0) {
        return 0.0;
    }
    return info.resolution;
}

}

std::unique_ptr<TouchpadEvdevSource> TouchpadEvdevSource::open(
    const QString &node, const QString &deviceName, const TouchpadContactHandlers &handlers, qint32 base)
{
    const int fd = ::open(QFile::encodeName(node).constData(), O_RDONLY | O_NONBLOCK | O_CLOEXEC);
    if (fd < 0) {
        qWarning() << "konveyor: touchpad taps are unavailable on" << deviceName << "because" << node
                   << "could not be opened:" << strerror(errno);
        return nullptr;
    }
    const double resolutionX = resolutionOf(fd, ABS_MT_POSITION_X);
    const double resolutionY = resolutionOf(fd, ABS_MT_POSITION_Y);
    if (resolutionX <= 0.0 || resolutionY <= 0.0) {
        qWarning() << "konveyor: touchpad taps are unavailable on" << deviceName << "because it reports no multitouch resolution";
        close(fd);
        return nullptr;
    }
    return std::unique_ptr<TouchpadEvdevSource>(new TouchpadEvdevSource(fd, handlers, base, QPointF(resolutionX, resolutionY)));
}

TouchpadEvdevSource::TouchpadEvdevSource(int fd, const TouchpadContactHandlers &handlers, qint32 base, QPointF resolution)
    : m_fd(fd)
    , m_handlers(handlers)
    , m_decoder(m_handlers, base, resolution)
    , m_notifier(fd, QSocketNotifier::Read)
{
    QObject::connect(&m_notifier, &QSocketNotifier::activated, &m_notifier, [this] { drain(); });
}

TouchpadEvdevSource::~TouchpadEvdevSource()
{
    m_notifier.setEnabled(false);
    close(m_fd);
}

void TouchpadEvdevSource::drain()
{
    input_event event {};
    while (::read(m_fd, &event, sizeof(event)) == sizeof(event)) {
        m_decoder.feed(event);
    }
}

}
