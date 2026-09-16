#include "input/spillinputfilter.h"

#include <effect/effecthandler.h>
#include <input_event.h>
#include <pointer_input.h>
#include <wayland/seat.h>
#include <wayland_server.h>
#include <window.h>
#include <workspace.h>

#include <ranges>

namespace Konveyor
{

SpillInputFilter::SpillInputFilter(OwnsPoint ownsPoint)
    : KWin::InputEventFilter(KWin::InputFilterOrder::Effects)
    , m_ownsPoint(std::move(ownsPoint))
{
    KWin::input()->installInputEventFilter(this);
}

SpillInputFilter::~SpillInputFilter() = default;

bool SpillInputFilter::isStolen(const QPointF &position) const
{
    if (KWin::waylandServer()->isScreenLocked() || KWin::waylandServer()->seat()->isDragPointer()) {
        return false;
    }
    if (KWin::effects && KWin::effects->isMouseInterception()) {
        return false;
    }
    KWin::Window *focus = KWin::input()->pointer()->focus();
    return focus && !focus->isInteractiveMove() && !focus->isInteractiveResize() && !m_ownsPoint(focus, position);
}

KWin::Window *SpillInputFilter::ownerAt(const QPointF &position) const
{
    for (KWin::Window *window : std::views::reverse(KWin::workspace()->stackingOrder())) {
        if (window->isDeleted() || !window->isOnCurrentActivity() || !window->isOnCurrentDesktop() || window->isMinimized()
            || window->isHidden() || window->isHiddenByShowDesktop() || !window->readyForPainting()) {
            continue;
        }
        if (m_ownsPoint(window, position) && window->hitTest(position)) {
            return window;
        }
    }
    return nullptr;
}

void SpillInputFilter::redirectTo(KWin::Window *window, const QPointF &position)
{
    KWin::SeatInterface *seat = KWin::waylandServer()->seat();
    m_redirecting = true;
    m_target = window;
    if (!window || !window->surface()) {
        if (seat->focusedPointerSurface()) {
            seat->notifyPointerLeave();
        }
        return;
    }
    if (seat->focusedPointerSurface() != window->surface()) {
        seat->notifyPointerEnter(window->surface(), position, window->inputTransformation());
    }
}

void SpillInputFilter::restore(const QPointF &position)
{
    m_redirecting = false;
    m_target.clear();
    KWin::SeatInterface *seat = KWin::waylandServer()->seat();
    KWin::Window *focus = KWin::input()->pointer()->focus();
    if (focus && focus->surface()) {
        if (seat->focusedPointerSurface() != focus->surface()) {
            seat->notifyPointerEnter(focus->surface(), position, focus->inputTransformation());
        }
    } else if (seat->focusedPointerSurface()) {
        seat->notifyPointerLeave();
    }
}

bool SpillInputFilter::pointerMotion(KWin::PointerMotionEvent *event)
{
    if (!m_redirectedButtons.isEmpty()) {
        return false;
    }
    if (event->buttons != Qt::NoButton && !m_redirecting) {
        return false;
    }
    if (isStolen(event->position)) {
        redirectTo(ownerAt(event->position), event->position);
    } else if (m_redirecting) {
        restore(event->position);
    }
    return false;
}

bool SpillInputFilter::pointerButton(KWin::PointerButtonEvent *event)
{
    KWin::SeatInterface *seat = KWin::waylandServer()->seat();
    if (event->state == KWin::PointerButtonState::Released) {
        if (!m_redirectedButtons.remove(event->nativeButton)) {
            return false;
        }
        seat->setTimestamp(event->timestamp);
        seat->notifyPointerButton(event->nativeButton, event->state);
        return true;
    }
    if (!isStolen(event->position)) {
        if (m_redirecting) {
            restore(event->position);
        }
        return false;
    }
    KWin::Window *owner = ownerAt(event->position);
    redirectTo(owner, event->position);
    if (owner && owner->wantsInput() && !owner->isActive()) {
        KWin::workspace()->activateWindow(owner);
        KWin::workspace()->raiseWindow(owner);
    }
    m_redirectedButtons.insert(event->nativeButton);
    seat->setTimestamp(event->timestamp);
    seat->notifyPointerButton(event->nativeButton, event->state);
    return true;
}

bool SpillInputFilter::pointerAxis(KWin::PointerAxisEvent *event)
{
    if (!m_redirecting || !isStolen(KWin::input()->pointer()->pos())) {
        return false;
    }
    KWin::SeatInterface *seat = KWin::waylandServer()->seat();
    seat->setTimestamp(event->timestamp);
    seat->notifyPointerAxis(event->orientation, event->delta, event->deltaV120, event->source, event->inverted);
    return true;
}

}
