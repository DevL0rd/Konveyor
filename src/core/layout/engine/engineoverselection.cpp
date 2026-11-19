#include "layout/engine/engineprivate.h"

namespace Konveyor::Layout
{

namespace
{

OverviewTarget targetAt(const OverviewWindow &window, const QRectF &outputGeometry)
{
    return OverviewTarget {window.rect.center() - outputGeometry.topLeft()};
}

}

std::optional<WindowId> Engine::overviewSelectedWindow() const
{
    if (!d->overview.selected || !windowState(*d->overview.selected)) {
        return std::nullopt;
    }
    return d->overview.selected;
}

void Engine::overviewSelectWindow(std::optional<WindowId> id)
{
    if (id && !windowState(*id)) {
        return;
    }
    d->overview.selected = id;
    if (id) {
        d->revealOverviewSelection();
    }
}

bool Engine::overviewActivateSelected()
{
    const std::optional<WindowId> id = overviewSelectedWindow();
    if (!id || !d->focusWindow(*id)) {
        return false;
    }
    if (d->overviewOpen) {
        setOverviewOpen(false);
    }
    return true;
}

void Engine::Private::revealOverviewSelection()
{
    if (!overview.selected) {
        return;
    }
    const QList<OverviewView> views = overviewViews();
    for (const OverviewView &view : views) {
        for (const OverviewWindow &window : view.windows) {
            if (window.id != *overview.selected) {
                continue;
            }
            const QRectF local = window.rect.translated(-view.outputGeometry.topLeft());
            const QRectF target = local.translated(view.camera);
            const QRectF bounds = view.contentBounds.united(QRectF(view.camera, view.outputGeometry.size()));
            animateOverviewCamera(view.output, view.camera, overviewRevealCamera(bounds, view.outputGeometry.size(), view.camera, target));
            return;
        }
    }
}

namespace
{

const OverviewView *viewOfSelection(const QList<OverviewView> &views, std::optional<WindowId> selected)
{
    for (const OverviewView &view : views) {
        if (std::ranges::any_of(view.windows, [selected](const OverviewWindow &window) { return window.id == selected; })) {
            return &view;
        }
    }
    return nullptr;
}

}

void Engine::overviewMoveSelection(OverviewDirection direction)
{
    const QList<OverviewView> views = overviewViews();
    if (views.isEmpty()) {
        return;
    }
    const OverviewView *view = viewOfSelection(views, overviewSelectedWindow());
    if (!view) {
        if (const Monitor *monitor = d->activeMonitor()) {
            view = viewAt(views, monitor->outputName());
        }
    }
    if (!view) {
        view = &views.first();
    }

    QList<OverviewTarget> targets;
    int current = -1;
    for (qsizetype index = 0; index < view->windows.size(); ++index) {
        targets.append(targetAt(view->windows.at(index), view->outputGeometry));
        if (view->windows.at(index).id == overviewSelectedWindow()) {
            current = static_cast<int>(index);
        }
    }
    const std::optional<int> next = overviewNextTarget(targets, current, direction);
    if (next) {
        overviewSelectWindow(view->windows.at(*next).id);
    }
}

void Engine::overviewCycleSelection(bool forward)
{
    const QList<OverviewView> views = overviewViews();
    const OverviewView *view = viewOfSelection(views, overviewSelectedWindow());
    if (!view && !views.isEmpty()) {
        view = &views.first();
    }
    if (!view) {
        return;
    }
    int current = -1;
    for (qsizetype index = 0; index < view->windows.size(); ++index) {
        if (view->windows.at(index).id == overviewSelectedWindow()) {
            current = static_cast<int>(index);
        }
    }
    if (const auto next = overviewCycledTarget(current, static_cast<int>(view->windows.size()), forward)) {
        overviewSelectWindow(view->windows.at(*next).id);
    }
}
}
