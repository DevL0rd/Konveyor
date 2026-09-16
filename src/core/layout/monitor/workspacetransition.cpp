#include "layout/monitor/workspacetransition.h"

#include "layout/common/geometry.h"

#include <algorithm>
#include <cmath>
#include <utility>

namespace Konveyor::Layout
{

WorkspaceTransition WorkspaceTransition::withAnimation(Anim::Animation animation)
{
    WorkspaceTransition result;
    result.m_animation = std::move(animation);
    return result;
}

WorkspaceTransition WorkspaceTransition::withSwipe(WorkspaceSwipe gesture)
{
    WorkspaceTransition result;
    result.m_swipe = std::move(gesture);
    return result;
}

double WorkspaceTransition::currentIndex() const
{
    if (m_swipe) {
        return m_swipe->currentIndex + (m_swipe->animation ? m_swipe->animation->value() : 0.0);
    }
    return m_animation ? m_animation->value() : 0.0;
}

double WorkspaceTransition::targetIndex() const
{
    if (m_swipe) {
        return m_swipe->currentIndex;
    }
    return m_animation ? m_animation->to() : 0.0;
}

bool WorkspaceTransition::isAnimating() const
{
    return m_swipe ? m_swipe->animation.has_value() : m_animation.has_value();
}

void WorkspaceTransition::offset(int delta)
{
    if (!m_swipe) {
        if (m_animation) {
            m_animation->offset(delta);
        }
        return;
    }
    if (delta >= 0) {
        m_swipe->centerIndex += static_cast<std::size_t>(delta);
    } else {
        m_swipe->centerIndex -= static_cast<std::size_t>(-delta);
    }
    m_swipe->startIndex += delta;
    m_swipe->currentIndex += delta;
}

void WorkspaceSwipe::startAnimationFrom(double from, const Anim::Clock &clock, const Config::AnimationParams &config)
{
    const double current = animation ? animation->value() : 0.0;
    animation = Anim::Animation(clock, from + current, 0.0, 0.0, config);
}

std::pair<double, double> WorkspaceSwipe::positionRange(std::size_t workspaceCount) const
{
    const auto last = static_cast<double>(workspaceCount - 1);
    if (!clampToRange) {
        return {0.0, last};
    }
    const double min = centerIndex > 0 ? static_cast<double>(centerIndex - 1) : 0.0;
    return {min, std::min(static_cast<double>(centerIndex + 1), last)};
}

}
