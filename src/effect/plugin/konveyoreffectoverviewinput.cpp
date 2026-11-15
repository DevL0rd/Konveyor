#include "plugin/konveyoreffect_p.h"

#include "layout/overview/overviewinput.h"

#include <effect/effecthandler.h>

namespace Konveyor
{

namespace
{

bool isOverviewKey(int key)
{
    switch (key) {
    case Qt::Key_Escape:
    case Qt::Key_Left:
    case Qt::Key_Right:
    case Qt::Key_Up:
    case Qt::Key_Down:
    case Qt::Key_Tab:
    case Qt::Key_Backtab:
    case Qt::Key_Return:
    case Qt::Key_Enter:
    case Qt::Key_Space:
        return true;
    default:
        return false;
    }
}

std::optional<Layout::OverviewDirection> directionOf(int key)
{
    switch (key) {
    case Qt::Key_Left:
        return Layout::OverviewDirection::Left;
    case Qt::Key_Right:
        return Layout::OverviewDirection::Right;
    case Qt::Key_Up:
        return Layout::OverviewDirection::Up;
    case Qt::Key_Down:
        return Layout::OverviewDirection::Down;
    default:
        return std::nullopt;
    }
}

}

bool KonveyorEffect::overviewInputActive() const
{
    const Layout::Engine &engine = readEngine();
    return engine.isOverviewEnabled() && engine.isOverviewOpen();
}

bool KonveyorEffect::handleOverviewAxis(
    const QPointF &position, Qt::Orientation orientation, qreal delta, qint32 deltaV120, bool finger, Qt::KeyboardModifiers modifiers)
{
    if (!overviewInputActive()) {
        return false;
    }
    const QString output = outputNameAt(position);
    if (output.isEmpty()) {
        return true;
    }
    const bool shift = modifiers.testFlag(Qt::ShiftModifier);
    const QPointF pan = Layout::overviewPanDelta(orientation, delta, deltaV120, finger, shift);
    if (!pan.isNull()) {
        changeEngine().overviewPan(output, pan);
        KWin::effects->addRepaintFull();
    }
    return true;
}

bool KonveyorEffect::handleOverviewButton(const QPointF &position, Qt::MouseButton button, bool pressed)
{
    if (!overviewInputActive()) {
        return false;
    }
    if (button == Qt::LeftButton) {
        if (pressed) {
            changeEngine().overviewPointerPressed(position);
        } else {
            changeEngine().overviewPointerReleased();
        }
        KWin::effects->addRepaintFull();
    }
    return true;
}

bool KonveyorEffect::handleOverviewMotion(const QPointF &position)
{
    if (!overviewInputActive()) {
        return false;
    }
    if (changeEngine().overviewPointerMoved(position)) {
        KWin::effects->addRepaintFull();
    }
    return true;
}

bool KonveyorEffect::handleOverviewKey(int key, bool pressed, Qt::KeyboardModifiers modifiers)
{
    if (!overviewInputActive() || !isOverviewKey(key)) {
        return false;
    }
    if (!pressed) {
        return true;
    }
    if (key == Qt::Key_Escape) {
        changeEngine().setOverviewOpen(false);
    } else if (const auto direction = directionOf(key)) {
        changeEngine().overviewMoveSelection(*direction);
    } else if (key == Qt::Key_Tab || key == Qt::Key_Backtab) {
        const bool backward = key == Qt::Key_Backtab || modifiers.testFlag(Qt::ShiftModifier);
        changeEngine().overviewCycleSelection(!backward);
    } else {
        changeEngine().overviewActivateSelected();
    }
    KWin::effects->addRepaintFull();
    return true;
}

}
