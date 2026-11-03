#include "input/touchpadcontactreader.h"

#include <core/inputdevice.h>
#include <input.h>
#include <wayland_server.h>

#include <QFileInfo>

namespace Konveyor
{

namespace
{

constexpr qint32 SlotsPerSource = 64;

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

void TouchpadContactReader::add(KWin::InputDevice *device)
{
    if (!device->isTouchpad() || m_sources.contains(device)) {
        return;
    }
    const QString node = QStringLiteral("/dev/input/") + QFileInfo(device->sysPath()).fileName();
    const TouchpadContactHandlers gated
        = gatedTouchpadContacts(m_handlers, [device] { return device->isEnabled() && !KWin::waylandServer()->isScreenLocked(); });
    std::shared_ptr<TouchpadEvdevSource> source = TouchpadEvdevSource::open(node, device->name(), gated, m_nextBase);
    if (!source) {
        return;
    }
    m_nextBase += SlotsPerSource;
    m_sources.insert(device, source);
}

void TouchpadContactReader::remove(KWin::InputDevice *device)
{
    if (m_sources.remove(device)) {
        m_handlers.reset();
    }
}

}
