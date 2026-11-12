#include "layout/common/desktoppeek.h"

#include <algorithm>
#include <utility>

namespace Konveyor::Layout
{

PeekKey::Step PeekKey::metaPressed(bool alone)
{
    switch (m_phase) {
    case Phase::Idle:
        if (!alone) {
            return Step::None;
        }
        m_phase = Phase::Armed;
        return Step::Arm;
    case Phase::Armed:
        if (alone) {
            return Step::None;
        }
        m_phase = Phase::Idle;
        return Step::Disarm;
    case Phase::AwaitingDrop:
        m_phase = Phase::Peeking;
        return Step::None;
    case Phase::Peeking:
        return Step::None;
    }
    return Step::None;
}

PeekKey::Step PeekKey::otherKeyPressed()
{
    switch (m_phase) {
    case Phase::Armed:
        m_phase = Phase::Idle;
        return Step::Disarm;
    case Phase::Peeking:
        m_phase = Phase::Idle;
        return Step::End;
    case Phase::Idle:
    case Phase::AwaitingDrop:
        return Step::None;
    }
    return Step::None;
}

PeekKey::Step PeekKey::pointerUsed()
{
    if (m_phase != Phase::Armed) {
        return Step::None;
    }
    m_phase = Phase::Idle;
    return Step::Disarm;
}

PeekKey::Step PeekKey::delayPassed(bool allowed)
{
    if (m_phase != Phase::Armed) {
        return Step::None;
    }
    m_phase = allowed ? Phase::Peeking : Phase::Idle;
    return allowed ? Step::Begin : Step::None;
}

PeekKey::Step PeekKey::metaReleased(bool dragging)
{
    switch (m_phase) {
    case Phase::Armed:
        m_phase = Phase::Idle;
        return Step::Disarm;
    case Phase::Peeking:
        m_phase = dragging ? Phase::AwaitingDrop : Phase::Idle;
        return dragging ? Step::None : Step::End;
    case Phase::Idle:
    case Phase::AwaitingDrop:
        return Step::None;
    }
    return Step::None;
}

PeekKey::Step PeekKey::dragEnded()
{
    if (m_phase != Phase::AwaitingDrop) {
        return Step::None;
    }
    m_phase = Phase::Idle;
    return Step::End;
}

PeekKey::Step PeekKey::reset()
{
    const Phase was = std::exchange(m_phase, Phase::Idle);
    switch (was) {
    case Phase::Armed:
        return Step::Disarm;
    case Phase::Peeking:
    case Phase::AwaitingDrop:
        return Step::End;
    case Phase::Idle:
        return Step::None;
    }
    return Step::None;
}

bool PeekKey::isPeeking() const
{
    return m_phase == Phase::Peeking || m_phase == Phase::AwaitingDrop;
}

void PeekAside::begin(bool pointerInside)
{
    m_inside = pointerInside;
    m_moved = false;
}

std::optional<PeekSide> PeekAside::pointerAt(QPointF pointer, const QRectF &frame, const QRectF &workArea)
{
    const bool inside = frame.contains(pointer);
    const bool left = m_inside && !inside;
    m_inside = inside;
    if (m_moved || !left) {
        return std::nullopt;
    }
    m_moved = true;
    if (pointer.x() < frame.left()) {
        return PeekSide::Right;
    }
    if (pointer.x() > frame.right()) {
        return PeekSide::Left;
    }
    return pointer.x() < workArea.center().x() ? PeekSide::Right : PeekSide::Left;
}

QRectF peekFrame(const QRectF &frame, const QRectF &workArea, PeekSide side, double minWidth, double gap)
{
    const double half = (workArea.width() - (3.0 * gap)) / 2.0;
    const double width = frame.width() > half ? std::min(std::max(half, minWidth), workArea.width()) : frame.width();
    const double x = side == PeekSide::Left ? workArea.left() + gap : std::max(workArea.left(), workArea.right() - gap - width);
    return {x, frame.top(), width, frame.height()};
}

}
