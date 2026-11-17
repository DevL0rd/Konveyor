#pragma once

#include "anim/duration.h"
#include "config/types.h"

#include <functional>
#include <memory>

namespace Konveyor::Anim
{

using TimeSource = std::function<Duration()>;

Duration monotonicTime();

class Clock
{
public:
    Clock();
    explicit Clock(TimeSource source);

    Duration now() const;
    Duration rawNow() const;
    void clear();

    double rate() const;
    void setRate(double rate);

    bool skipsAnimations() const;
    void setSkipAnimations(bool value);

    void applyConfig(const Config::Animations &config);

private:
    struct State;
    std::shared_ptr<State> m_state;
};

}
