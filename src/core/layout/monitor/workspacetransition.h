#pragma once

#include "layout/common/velocitytracker.h"

#include "anim/animation.h"

#include <cstddef>
#include <optional>
#include <utility>

namespace Konveyor::Layout
{

struct WorkspaceSwipe
{
    std::size_t centerIndex = 0;
    double startIndex = 0;
    double currentIndex = 0;
    std::optional<Anim::Animation> animation;
    VelocityTracker tracker;
    bool isTouchpad = false;
    bool clampToRange = true;
    std::optional<Anim::Duration> edgeScrollLastTime;
    std::optional<Anim::Duration> edgeScrollActiveSince;

    std::pair<double, double> positionRange(std::size_t workspaceCount) const;
    void startAnimationFrom(double from, const Anim::Clock &clock, const Config::AnimationParams &config);
};

class WorkspaceTransition
{
public:
    double currentIndex() const;
    double targetIndex() const;
    void offset(int delta);
    bool isAnimating() const;
    bool isSwiping() const { return m_swipe.has_value(); }
    WorkspaceSwipe *swipe() { return m_swipe ? &*m_swipe : nullptr; }
    const WorkspaceSwipe *swipe() const { return m_swipe ? &*m_swipe : nullptr; }
    Anim::Animation *animation() { return m_animation ? &*m_animation : nullptr; }

    static WorkspaceTransition withAnimation(Anim::Animation animation);
    static WorkspaceTransition withSwipe(WorkspaceSwipe gesture);

private:
    std::optional<Anim::Animation> m_animation;
    std::optional<WorkspaceSwipe> m_swipe;
};

}
