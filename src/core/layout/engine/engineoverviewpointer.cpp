#include "layout/engine/engineprivate.h"

#include <algorithm>
#include <cmath>
#include <ranges>
#include <utility>

namespace Konveyor::Layout
{

namespace
{

constexpr double kOverviewDragThreshold = 4.0;

const OverviewView *viewWithPoint(const QList<OverviewView> &views, const QPointF &point)
{
    for (const OverviewView &view : views) {
        if (view.outputGeometry.contains(point)) {
            return &view;
        }
    }
    return nullptr;
}

const OverviewWindow *windowAtPoint(const OverviewView &view, const QPointF &point)
{
    for (const OverviewWindow &window : std::views::reverse(view.windows)) {
        if (window.rect.contains(point)) {
            return &window;
        }
    }
    return nullptr;
}

std::optional<int> cardAtPoint(const OverviewView &view, const QPointF &point)
{
    for (qsizetype index = 0; index < view.workspaceRects.size(); ++index) {
        if (view.workspaceRects.at(index).contains(point)) {
            return static_cast<int>(index);
        }
    }
    return std::nullopt;
}

}

bool Engine::overviewPointerActive() const
{
    return d->overview.press.has_value();
}

void Engine::overviewPointerPressed(const QPointF &globalPos)
{
    d->overview.press.reset();
    if (!d->builtinOverviewOpen() || d->windowDrag || d->overview.reorderFrom >= 0) {
        return;
    }
    const QList<OverviewView> views = d->overviewViews();
    if (views.isEmpty()) {
        return;
    }
    const OverviewView *view = viewWithPoint(views, globalPos);
    if (!view) {
        return;
    }

    OverviewPress press;
    press.output = view->output;
    press.origin = globalPos;
    if (const OverviewWindow *window = windowAtPoint(*view, globalPos)) {
        overviewSelectWindow(window->id);
        press.window = window->id;
        d->overview.press = press;
        return;
    }
    if (const std::optional<int> card = cardAtPoint(*view, globalPos)) {
        press.workspaceIndex = *card;
        d->overview.press = press;
        return;
    }

    // A press on the empty backdrop leaves the overview.
    setOverviewOpen(false);
}

bool Engine::overviewHover(const QPointF &globalPos)
{
    if (!d->builtinOverviewOpen() || d->overview.press) {
        return false;
    }
    const QList<OverviewView> views = d->overviewViews();
    const OverviewView *view = viewWithPoint(views, globalPos);
    if (!view) {
        return false;
    }
    std::optional<WindowId> target;
    if (const OverviewWindow *window = windowAtPoint(*view, globalPos)) {
        target = window->id;
    } else {
        target = d->centeredOverviewWindow(view->output);
    }
    if (d->overview.selected == target) {
        return false;
    }
    d->overview.selected = target;
    return true;
}

bool Engine::overviewPointerMoved(const QPointF &globalPos)
{
    if (!d->overview.press) {
        return false;
    }
    if (!d->builtinOverviewOpen()) {
        d->overview.press.reset();
        return false;
    }
    OverviewPress &press = *d->overview.press;
    if (!press.dragging) {
        const QPointF delta = globalPos - press.origin;
        if (std::abs(delta.x()) < kOverviewDragThreshold && std::abs(delta.y()) < kOverviewDragThreshold) {
            return false;
        }
        press.dragging = true;
        if (press.window) {
            overviewBeginWindowDrag(press.window, press.output, press.origin);
        } else {
            overviewBeginWorkspaceDrag(press.output, press.workspaceIndex);
        }
    }
    overviewUpdateDrag(globalPos);
    return true;
}

void Engine::overviewPointerReleased()
{
    const std::optional<OverviewPress> press = std::exchange(d->overview.press, std::nullopt);
    if (!press || !d->builtinOverviewOpen()) {
        return;
    }
    if (press->dragging) {
        overviewEndDrag();
        return;
    }
    if (press->window) {
        overviewActivateSelected();
        return;
    }
    if (press->workspaceIndex >= 0) {
        setOverviewOpen(false);
        focusWorkspace(press->output, press->workspaceIndex + 1);
    }
}

}
