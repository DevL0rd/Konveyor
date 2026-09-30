#pragma once

#include "input/touchpadcontactdecoder.h"

#include <QHash>
#include <QObject>

#include <memory>

class QSocketNotifier;

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
    ~TouchpadContactReader() override;

private:
    struct Source
    {
        int fd = -1;
        std::unique_ptr<TouchpadContactDecoder> decoder;
        std::unique_ptr<QSocketNotifier> notifier;
    };

    void add(KWin::InputDevice *device);
    void remove(KWin::InputDevice *device);
    static void read(Source &source);

    TouchpadContactHandlers m_handlers;
    QHash<KWin::InputDevice *, std::shared_ptr<Source>> m_sources;
    qint32 m_nextBase = 0;
};

}
