#pragma once

#include "layout/common/velocitytracker.h"

#include "anim/animation.h"

#include <optional>
#include <variant>

namespace Konveyor::Layout
{

struct StripSwipe
{
    double swipePosition = 0;
    std::optional<Anim::Animation> animation;
    VelocityTracker tracker;
    double trackerBase = 0;
    double restingPosition = 0;
    bool isTouchpad = false;
    std::optional<Anim::Duration> edgeScrollLastTime;
    std::optional<Anim::Duration> edgeScrollActiveSince;
    void startAnimationFrom(double from, const Anim::Clock &clock, const Config::AnimationParams &config);
};

class StripScroll
{
public:
    double current() const;
    double target() const;
    double resting() const;
    bool isIdle() const { return !m_animation && !m_swipe; }
    bool isSwiping() const { return m_swipe.has_value(); }
    bool isEdgeScrolling() const;
    bool isAnimating() const;
    void offset(double delta);
    void cancelSwipe();
    void halt();
    void setIdle(double value);
    void setAnimation(Anim::Animation anim);
    void setSwipe(StripSwipe gesture);
    StripSwipe *swipe() { return m_swipe ? &*m_swipe : nullptr; }
    const StripSwipe *swipe() const { return m_swipe ? &*m_swipe : nullptr; }
    const Anim::Animation *animation() const { return m_animation ? &*m_animation : nullptr; }

private:
    double m_idle = 0.0;
    std::optional<Anim::Animation> m_animation;
    std::optional<StripSwipe> m_swipe;
};

}
