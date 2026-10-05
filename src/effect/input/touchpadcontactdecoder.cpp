#include "input/touchpadcontactdecoder.h"

#include <QSet>

#include <linux/input.h>

#include <memory>
#include <utility>

namespace Konveyor
{

namespace
{

constexpr qint64 MicrosecondsPerMillisecond = 1000;
constexpr qint64 MillisecondsPerSecond = 1000;

struct Gate
{
    TouchpadContactHandlers handlers;
    std::function<bool()> accepting;
    QSet<qint32> live;
    bool blocked = false;

    bool admit()
    {
        if (accepting()) {
            blocked = false;
            return true;
        }
        live.clear();
        if (!std::exchange(blocked, true)) {
            handlers.reset();
        }
        return false;
    }
};

}

TouchpadContactHandlers gatedTouchpadContacts(const TouchpadContactHandlers &handlers, std::function<bool()> accepting)
{
    auto gate = std::make_shared<Gate>(Gate {handlers, std::move(accepting), {}, false});
    return {
        [gate](qint32 slot, const QPointF &position, qint64 timestampMs) {
            if (gate->admit()) {
                gate->live.insert(slot);
                gate->handlers.down(slot, position, timestampMs);
            }
        },
        [gate](qint32 slot, const QPointF &position) {
            if (gate->admit() && gate->live.contains(slot)) {
                gate->handlers.motion(slot, position);
            }
        },
        [gate](qint32 slot, qint64 timestampMs) {
            if (gate->admit() && gate->live.remove(slot)) {
                gate->handlers.up(slot, timestampMs);
            }
        },
        [gate] {
            if (gate->admit()) {
                gate->handlers.press();
            }
        },
        [gate] {
            gate->live.clear();
            gate->handlers.reset();
        },
    };
}

TouchpadContactDecoder::TouchpadContactDecoder(const TouchpadContactHandlers &handlers, qint32 base, QPointF resolution)
    : m_handlers(handlers)
    , m_base(base)
    , m_resolution(resolution)
{ }

void TouchpadContactDecoder::feed(const input_event &event)
{
    if (event.type == EV_SYN) {
        handleSync(event);
    } else if (!m_dropped) {
        handleEvent(event);
    }
}

void TouchpadContactDecoder::handleSync(const input_event &event)
{
    if (event.code == SYN_DROPPED) {
        m_dropped = true;
        m_contacts.clear();
        m_handlers.reset();
    } else if (event.code == SYN_REPORT && !std::exchange(m_dropped, false)) {
        commit(event.input_event_sec * MillisecondsPerSecond + event.input_event_usec / MicrosecondsPerMillisecond);
    }
}

void TouchpadContactDecoder::handleEvent(const input_event &event)
{
    if (event.type == EV_KEY && event.code == BTN_LEFT && event.value == 1) {
        m_handlers.press();
        return;
    }
    if (event.type != EV_ABS) {
        return;
    }
    Slot &slot = m_contacts[m_slot];
    switch (event.code) {
    case ABS_MT_SLOT:
        m_slot = event.value;
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

void TouchpadContactDecoder::commit(qint64 timestampMs)
{
    for (auto it = m_contacts.begin(); it != m_contacts.end(); ++it) {
        Slot &slot = it.value();
        const QPointF millimeters(slot.raw.x() / m_resolution.x(), slot.raw.y() / m_resolution.y());
        if (slot.began && !slot.active) {
            slot.active = true;
            m_handlers.down(m_base + it.key(), millimeters, timestampMs);
        } else if (slot.moved && slot.active) {
            m_handlers.motion(m_base + it.key(), millimeters);
        }
        slot.began = false;
        slot.moved = false;
    }
    for (auto it = m_contacts.begin(); it != m_contacts.end(); ++it) {
        Slot &slot = it.value();
        if (std::exchange(slot.ended, false) && slot.active) {
            slot.active = false;
            m_handlers.up(m_base + it.key(), timestampMs);
        }
    }
}

}
