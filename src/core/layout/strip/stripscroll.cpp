#include "layout/strip/stripscroll.h"

#include "layout/common/geometry.h"

#include <algorithm>
#include <cmath>
#include <utility>

namespace Konveyor::Layout
{

void StripSwipe::startAnimationFrom(double from, const Anim::Clock &clock, const Config::AnimationParams &config)
{
    const double current = animation ? animation->value() : 0.0;
    animation = Anim::Animation(clock, from + current, 0.0, 0.0, config);
}

void StripScroll::setIdle(double value)
{
    m_idle = value;
    m_animation.reset();
    m_swipe.reset();
}

void StripScroll::setAnimation(Anim::Animation anim)
{
    m_swipe.reset();
    m_animation = std::move(anim);
}

void StripScroll::setSwipe(StripSwipe gesture)
{
    m_animation.reset();
    m_swipe = std::move(gesture);
}

double StripScroll::current() const
{
    if (m_swipe) {
        return m_swipe->swipePosition + (m_swipe->animation ? m_swipe->animation->value() : 0.0);
    }
    return m_animation ? m_animation->value() : m_idle;
}

double StripScroll::target() const
{
    if (m_swipe) {
        return m_swipe->swipePosition;
    }
    return m_animation ? m_animation->to() : m_idle;
}

double StripScroll::resting() const
{
    return m_swipe ? m_swipe->restingPosition : target();
}

bool StripScroll::isEdgeScrolling() const
{
    return m_swipe && m_swipe->edgeScrollLastTime.has_value();
}

bool StripScroll::isAnimating() const
{
    if (m_swipe) {
        return m_swipe->animation.has_value();
    }
    return m_animation.has_value();
}

void StripScroll::offset(double delta)
{
    if (m_swipe) {
        m_swipe->restingPosition += delta;
        m_swipe->trackerBase += delta;
        m_swipe->swipePosition += delta;
    } else if (m_animation) {
        m_animation->offset(delta);
    } else {
        m_idle += delta;
    }
}

void StripScroll::cancelSwipe()
{
    if (m_swipe) {
        setIdle(m_swipe->swipePosition);
    }
}

void StripScroll::halt()
{
    setIdle(current());
}

}
