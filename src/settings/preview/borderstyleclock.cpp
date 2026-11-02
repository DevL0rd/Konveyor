#include "preview/borderstyleclock.h"

#include "preview/borderstyleitem.h"

#include <chrono>
#include <utility>

namespace Konveyor::Settings
{

namespace
{

constexpr std::chrono::milliseconds kFrameInterval(33);

}

BorderStyleClock::BorderStyleClock()
{
    m_elapsed.start();
    m_timer.setInterval(kFrameInterval);
    m_timer.setTimerType(Qt::CoarseTimer);
    connect(&m_timer, &QTimer::timeout, this, &BorderStyleClock::tick);
}

std::shared_ptr<BorderStyleClock> BorderStyleClock::shared()
{
    static std::weak_ptr<BorderStyleClock> current;
    std::shared_ptr<BorderStyleClock> clock = current.lock();
    if (!clock) {
        clock = std::make_shared<BorderStyleClock>();
        current = clock;
    }
    return clock;
}

void BorderStyleClock::setAnimating(BorderStyleItem *item, bool animating)
{
    if (animating) {
        m_items.insert(item);
    } else {
        m_items.remove(item);
    }
    if (m_items.isEmpty()) {
        m_timer.stop();
    } else if (!m_timer.isActive()) {
        m_timer.start();
    }
}

double BorderStyleClock::seconds() const
{
    return static_cast<double>(m_elapsed.nsecsElapsed()) / 1e9;
}

void BorderStyleClock::tick()
{
    for (BorderStyleItem *item : std::as_const(m_items)) {
        item->advance();
    }
}

}
