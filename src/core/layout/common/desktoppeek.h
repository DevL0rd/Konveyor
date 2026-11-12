#pragma once

#include <QPointF>
#include <QRectF>

#include <optional>

namespace Konveyor::Layout
{

enum class PeekSide
{
    Left,
    Right
};

class PeekKey
{
public:
    enum class Step
    {
        None,
        Arm,
        Disarm,
        Begin,
        End
    };

    Step metaPressed(bool alone);
    Step otherKeyPressed();
    Step pointerUsed();
    Step delayPassed(bool allowed);
    Step metaReleased(bool dragging);
    Step dragEnded();
    Step reset();
    bool isPeeking() const;

private:
    enum class Phase
    {
        Idle,
        Armed,
        Peeking,
        AwaitingDrop
    };

    Phase m_phase = Phase::Idle;
};

class PeekAside
{
public:
    void begin(bool pointerInside);
    std::optional<PeekSide> pointerAt(QPointF pointer, const QRectF &frame, const QRectF &workArea);

private:
    bool m_inside = false;
    bool m_moved = false;
};

QRectF peekFrame(const QRectF &frame, const QRectF &workArea, PeekSide side, double minWidth, double gap);

}
