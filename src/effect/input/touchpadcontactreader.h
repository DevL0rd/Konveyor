#pragma once

#include "input/touchpadcontactdecoder.h"
#include "input/touchpadevdevsource.h"

#include <QHash>
#include <QObject>

#include <memory>

namespace KWin
{
class InputDevice;
}

namespace Konveyor
{

class TouchpadContactReader : public QObject
{
public:
    explicit TouchpadContactReader(TouchpadContactHandlers handlers);

private:
    void add(KWin::InputDevice *device);
    void remove(KWin::InputDevice *device);

    TouchpadContactHandlers m_handlers;
    QHash<KWin::InputDevice *, std::shared_ptr<TouchpadEvdevSource>> m_sources;
    qint32 m_nextBase = 0;
};

}
