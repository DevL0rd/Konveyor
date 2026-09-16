#include "layout/monitor/monitor.h"

#include "layout/common/geometry.h"

#include <algorithm>
#include <cmath>
#include <utility>

namespace Konveyor::Layout
{

namespace
{

constexpr double WorkspaceSwipeDistance = 300.0;

constexpr double WorkspaceEdgeScrollDistance = 1500.0;

constexpr ElasticLimit WorkspaceSwipeResistance {0.5, 0.05};

}

void Monitor::beginWorkspaceSwipe(bool isTouchpad)
{
    WorkspaceSwipe gesture;
    gesture.centerIndex = m_activeWorkspaceIndex;
    gesture.startIndex = visibleWorkspacePosition();
    gesture.currentIndex = gesture.startIndex;
    gesture.isTouchpad = isTouchpad;
    gesture.clampToRange = !overviewOpen;
    m_transition = WorkspaceTransition::withSwipe(std::move(gesture));
}

std::optional<bool> Monitor::updateWorkspaceSwipe(double deltaY, Anim::Duration timestamp, bool isTouchpad)
{
    WorkspaceSwipe *gesture = m_transition ? m_transition->swipe() : nullptr;
    if (!gesture || gesture->isTouchpad != isTouchpad || gesture->edgeScrollLastTime) {
        return std::nullopt;
    }
    const double totalHeight = swipeDistance(*gesture);
    gesture->tracker.push(deltaY, timestamp);

    const auto [min, max] = gesture->positionRange(m_workspaces.size());
    const double newIndex = WorkspaceSwipeResistance.clamp(min, max, gesture->startIndex + gesture->tracker.pos() / totalHeight);
    if (gesture->currentIndex == newIndex) {
        return false;
    }
    gesture->currentIndex = newIndex;
    return true;
}

bool Monitor::endWorkspaceSwipe(std::optional<bool> isTouchpad)
{
    WorkspaceSwipe *gesture = m_transition ? m_transition->swipe() : nullptr;
    if (!gesture || (isTouchpad && gesture->isTouchpad != *isTouchpad)) {
        return false;
    }
    const double totalHeight = swipeDistance(*gesture);
    gesture->tracker.push(0.0, m_clock.rawNow());

    const auto [min, max] = gesture->positionRange(m_workspaces.size());
    const double currentPos = gesture->tracker.pos() / totalHeight;
    const double projected = gesture->tracker.projectedPosition() / totalHeight;
    const auto newIndex = static_cast<std::size_t>(std::round(std::clamp(gesture->startIndex + projected, min, max)));
    const double velocity
        = gesture->tracker.velocity() / totalHeight * WorkspaceSwipeResistance.clampSlope(min, max, gesture->startIndex + currentPos);
    const double fromIndex = gesture->currentIndex;

    if (m_activeWorkspaceIndex != newIndex) {
        m_previousWorkspaceId = m_workspaces[m_activeWorkspaceIndex].id();
    }
    m_activeWorkspaceIndex = newIndex;
    m_transition = WorkspaceTransition::withAnimation(
        Anim::Animation(m_clock, fromIndex, static_cast<double>(newIndex), velocity, m_options->animations.workspaceSwitch));
    return true;
}

double Monitor::swipeDistance(const WorkspaceSwipe &gesture) const
{
    if (gesture.edgeScrollLastTime) {
        return WorkspaceEdgeScrollDistance;
    }
    return gesture.isTouchpad ? WorkspaceSwipeDistance : workspaceHeightWithGap();
}

void Monitor::beginEdgeScroll()
{
    const WorkspaceSwipe *existing = m_transition ? m_transition->swipe() : nullptr;
    if ((existing && existing->edgeScrollLastTime) || !overviewOpen) {
        return;
    }
    WorkspaceSwipe gesture;
    gesture.centerIndex = m_activeWorkspaceIndex;
    gesture.startIndex = visibleWorkspacePosition();
    gesture.currentIndex = gesture.startIndex;
    gesture.clampToRange = false;
    gesture.edgeScrollLastTime = m_clock.rawNow();
    m_transition = WorkspaceTransition::withSwipe(std::move(gesture));
}

bool Monitor::edgeScrollBy(QPointF pos, double speed)
{
    WorkspaceSwipe *gesture = m_transition ? m_transition->swipe() : nullptr;
    if (!gesture || !gesture->edgeScrollLastTime) {
        return false;
    }
    const Config::DndEdgeScroll &config = m_options->gestures.dndEdgeWorkspaceSwitch;
    const double height = m_area.workingArea.height();
    const double y = std::clamp(pos.y() - m_area.workingArea.y(), 0.0, height);
    const double trigger = std::clamp(config.triggerSize, 0.0, height / 2.0);

    double delta = 0.0;
    if (pos.x() >= 0.0 && pos.x() < m_area.viewSize.width() && trigger >= 0.01) {
        if (y < trigger) {
            delta = -(trigger - y) / trigger * speed;
        } else if (height - y < trigger) {
            delta = (trigger - (height - y)) / trigger * speed;
        }
    }

    const EdgeScrollStep step
        = stepEdgeScroll(gesture->tracker, gesture->edgeScrollLastTime, gesture->edgeScrollActiveSince, m_clock.rawNow(), delta, config);
    if (!step.scrolled) {
        return step.handled;
    }
    const double unclamped = gesture->startIndex + step.position / WorkspaceEdgeScrollDistance;
    const auto [min, max] = gesture->positionRange(m_workspaces.size());
    const double clamped = std::clamp(unclamped, min, max);
    gesture->startIndex += clamped - unclamped;
    gesture->currentIndex = clamped;
    return true;
}

void Monitor::endEdgeScroll()
{
    const WorkspaceSwipe *gesture = m_transition ? m_transition->swipe() : nullptr;
    if (!gesture || !gesture->edgeScrollLastTime) {
        return;
    }
    endWorkspaceSwipe(std::nullopt);
}

}
