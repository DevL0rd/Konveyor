#pragma once

#include "anim/clock.h"

#include <memory>

namespace Konveyor::Anim
{

class ManualClock : public Clock
{
public:
    explicit ManualClock(Duration time)
        : ManualClock(std::make_shared<Duration>(time))
    { }

    void setRawNow(Duration time)
    {
        *m_time = time;
        clear();
    }

private:
    explicit ManualClock(std::shared_ptr<Duration> time)
        : Clock([time] { return *time; })
        , m_time(std::move(time))
    { }

    std::shared_ptr<Duration> m_time;
};

}
