#pragma once

#include <QHash>
#include <QObject>
#include <QPointF>

#include <functional>
#include <memory>

class QSocketNotifier;
struct input_event;

namespace KWin
{
class InputDevice;
}

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

class TouchpadContactReader : public QObject
{
public:
    explicit TouchpadContactReader(TouchpadContactHandlers handlers);
    ~TouchpadContactReader() override;

private:
    struct Slot
    {
        bool active = false;
        bool began = false;
        bool ended = false;
        bool moved = false;
        QPointF raw;
    };

    struct Source
    {
        int fd = -1;
        qint32 base = 0;
        double resolutionX = 1.0;
        double resolutionY = 1.0;
        qint32 slot = 0;
        bool dropped = false;
        QHash<qint32, Slot> contacts;
        std::unique_ptr<QSocketNotifier> notifier;
    };

    void add(KWin::InputDevice *device);
    void remove(KWin::InputDevice *device);
    void read(Source &source);
    void handleSync(Source &source, const input_event &event);
    void handleEvent(Source &source, const input_event &event);
    void commit(Source &source, qint64 timestampMs);

    TouchpadContactHandlers m_handlers;
    QHash<KWin::InputDevice *, std::shared_ptr<Source>> m_sources;
    qint32 m_nextBase = 0;
};

}
