#pragma once

#include "input/touchpadcontactdecoder.h"

#include <QPointF>
#include <QSocketNotifier>
#include <QString>

#include <memory>

namespace Konveyor
{

class TouchpadEvdevSource
{
public:
    static std::unique_ptr<TouchpadEvdevSource> open(
        const QString &node, const QString &deviceName, const TouchpadContactHandlers &handlers, qint32 base);

    ~TouchpadEvdevSource();
    TouchpadEvdevSource(const TouchpadEvdevSource &) = delete;
    TouchpadEvdevSource &operator=(const TouchpadEvdevSource &) = delete;

private:
    TouchpadEvdevSource(int fd, const TouchpadContactHandlers &handlers, qint32 base, QPointF resolution);

    void drain();

    int m_fd;
    TouchpadContactHandlers m_handlers;
    TouchpadContactDecoder m_decoder;
    QSocketNotifier m_notifier;
};

}
