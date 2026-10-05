#pragma once

#include <QHash>
#include <QPointF>

#include <functional>

struct input_event;

namespace Konveyor
{

struct TouchpadContactHandlers
{
    std::function<void(qint32, const QPointF &, qint64)> down;
    std::function<void(qint32, const QPointF &)> motion;
    std::function<void(qint32, qint64)> up;
    std::function<void()> press;
    std::function<void()> reset;
};

TouchpadContactHandlers gatedTouchpadContacts(const TouchpadContactHandlers &handlers, std::function<bool()> accepting);

class TouchpadContactDecoder
{
public:
    TouchpadContactDecoder(const TouchpadContactHandlers &handlers, qint32 base, QPointF resolution);

    void feed(const input_event &event);

private:
    struct Slot
    {
        bool active = false;
        bool began = false;
        bool ended = false;
        bool moved = false;
        QPointF raw;
    };

    void handleSync(const input_event &event);
    void handleEvent(const input_event &event);
    void commit(qint64 timestampMs);

    const TouchpadContactHandlers &m_handlers;
    qint32 m_base = 0;
    QPointF m_resolution;
    qint32 m_slot = 0;
    bool m_dropped = false;
    QHash<qint32, Slot> m_contacts;
};

}
