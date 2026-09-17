#include "plugin/konveyoreffect_p.h"

namespace Konveyor
{

namespace
{

quint8 resizeEdgesFor(KWin::Gravity gravity)
{
    const auto edges = [](std::initializer_list<Layout::ResizeEdge> list) {
        quint8 result = 0;
        for (const Layout::ResizeEdge edge : list) {
            result |= static_cast<quint8>(edge);
        }
        return result;
    };
    switch (gravity) {
    case KWin::Gravity::Left:
        return edges({Layout::ResizeEdge::Left});
    case KWin::Gravity::Right:
        return edges({Layout::ResizeEdge::Right});
    case KWin::Gravity::Top:
        return edges({Layout::ResizeEdge::Top});
    case KWin::Gravity::Bottom:
        return edges({Layout::ResizeEdge::Bottom});
    case KWin::Gravity::TopLeft:
        return edges({Layout::ResizeEdge::Top, Layout::ResizeEdge::Left});
    case KWin::Gravity::TopRight:
        return edges({Layout::ResizeEdge::Top, Layout::ResizeEdge::Right});
    case KWin::Gravity::BottomLeft:
        return edges({Layout::ResizeEdge::Bottom, Layout::ResizeEdge::Left});
    case KWin::Gravity::BottomRight:
        return edges({Layout::ResizeEdge::Bottom, Layout::ResizeEdge::Right});
    default:
        return 0;
    }
}

}

bool KonveyorEffect::isFloatingWindow(Layout::WindowId id) const
{
    const std::optional<Layout::WindowState> state = readEngine().windowState(id);
    return state && state->isFloating;
}

bool KonveyorEffect::adoptFloatingGeometry(Layout::WindowId id, KWin::Window *window)
{
    if (d->applier.isApplying() || window->isInteractiveMove() || window->isInteractiveResize() || !isFloatingWindow(id)) {
        return false;
    }
    const QRectF frame = window->frameGeometry();
    const std::optional<Layout::WindowState> state = readEngine().windowState(id);
    const bool animating = state && state->renderFrame.topLeft() != state->targetFrame.topLeft();
    if (animating || d->applier.isEchoOfAppliedFrame(id, frame)) {
        return false;
    }
    changeEngine().setFloatingFrame(id, frame);
    return true;
}

void KonveyorEffect::onInteractive(Layout::WindowId id, bool isMove, int phase)
{
    KWin::Window *window = d->windows.windowOf(id);
    if (!window) {
        return;
    }
    if (isFloatingWindow(id)) {
        if (phase == interactivePhaseEnd) {
            changeEngine().setFloatingFrame(id, window->moveResizeGeometry());
        }
        return;
    }
    const bool scrollOnDrag = d->config.config().gestures.titlebarDrag == Config::TitlebarDrag::ScrollView;
    if (isMove && scrollOnDrag && holdsToDecide(id, phase)) {
        return;
    }
    const bool lifted = d->touchLift == id;
    if (!isMove) {
        handleWindowResize(id, window, phase);
    } else if (scrollOnDrag && !lifted) {
        handleTitlebarDrag(id, window, phase);
    } else {
        handleWindowMove(id, window, phase);
    }
    if (lifted && phase == interactivePhaseEnd) {
        d->touchLift.reset();
    }
}

void KonveyorEffect::handleTitlebarDrag(Layout::WindowId id, KWin::Window *window, int phase)
{
    if (phase != interactivePhaseStart) {
        return;
    }
    const std::optional<QString> output = readEngine().focusedOutput();
    if (!output) {
        return;
    }
    d->titlebarDrag = id;
    d->dragOrigin = interactionPoint().x();
    changeEngine().beginSwipe(*output, false);
    window->endInteractiveMoveResize();
}

void KonveyorEffect::endTitlebarDrag()
{
    if (const std::optional<Layout::WindowId> dragged = std::exchange(d->titlebarDrag, std::nullopt)) {
        changeEngine().endSwipe(false, dragged);
    }
}

void KonveyorEffect::handleWindowMove(Layout::WindowId id, KWin::Window *window, int phase)
{
    const QPointF pointer = interactionPoint();
    if (phase == interactivePhaseStart) {
        changeEngine().beginWindowDrag(id, pointer);
    } else if (phase == interactivePhaseStep) {
        changeEngine().updateWindowDrag(pointer, outputNameOf(window));
    } else {
        changeEngine().endWindowDrag();
    }
}

void KonveyorEffect::handleWindowResize(Layout::WindowId id, KWin::Window *window, int phase)
{
    if (!d->config.config().gestures.resizeTiledWindows) {
        if (phase == interactivePhaseStart) {
            window->endInteractiveMoveResize();
        }
        return;
    }
    if (phase == interactivePhaseStart) {
        d->resizeOrigin = window->moveResizeGeometry().size();
        changeEngine().beginResize(id, resizeEdgesFor(window->interactiveMoveResizeGravity()));
    } else if (phase == interactivePhaseStep) {
        const QSizeF size = window->moveResizeGeometry().size();
        changeEngine().updateResize(QPointF(size.width() - d->resizeOrigin.width(), size.height() - d->resizeOrigin.height()));
    } else {
        changeEngine().endResize();
    }
}

}
