#include "plugin/konveyoreffect_p.h"

namespace Konveyor
{

void KonveyorEffect::onInteractive(Layout::WindowId id, bool isMove, int phase)
{
    KWin::Window *window = d->windows.windowOf(id);
    if (!window) {
        return;
    }
    const bool scrollOnDrag = d->config.config().gestures.titlebarDrag == Config::TitlebarDrag::ScrollView;
    if (!isMove) {
        handleWindowResize(id, window, phase);
    } else if (scrollOnDrag) {
        handleTitlebarDrag(id, window, phase);
    } else {
        handleWindowMove(id, window, phase);
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
    d->dragOrigin = KWin::effects->cursorPos().x();
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
    const QPointF pointer = KWin::effects->cursorPos();
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
    if (phase == interactivePhaseStart) {
        d->resizeOrigin = window->moveResizeGeometry().size();
        changeEngine().beginResize(id, 0);
    } else if (phase == interactivePhaseStep) {
        const QSizeF size = window->moveResizeGeometry().size();
        changeEngine().updateResize(QPointF(size.width() - d->resizeOrigin.width(), size.height() - d->resizeOrigin.height()));
    } else {
        changeEngine().endResize();
    }
}

}
