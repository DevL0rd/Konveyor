#include "layout/engine/engineprivate.h"

#include <algorithm>

namespace Konveyor::Layout
{

namespace
{

QList<QRectF> localCards(const OverviewView &view)
{
    QList<QRectF> cards;
    cards.reserve(view.workspaceRects.size());
    for (const QRectF &card : view.workspaceRects) {
        cards.append(card.translated(-view.outputGeometry.topLeft()));
    }
    return cards;
}

int dropCardIndex(const OverviewView &view, const Monitor &monitor, const QPointF &local)
{
    const QList<QRectF> cards = localCards(view);
    int card = overviewCardIndexAt(cards, local).value_or(-1);
    if (card < 0) {
        card = overviewNearestCardIndex(cards, local);
    }
    if (card < 0) {
        return -1;
    }
    const auto limit = view.workspaceIndices.isEmpty() ? static_cast<int>(monitor.workspaces().size()) : view.workspaceIndices.size();
    return card < limit && card < view.workspaceRects.size() ? card : -1;
}

std::optional<int> dropWorkspaceIndex(const OverviewView &view, const Monitor &monitor, int card)
{
    if (card < 0) {
        return std::nullopt;
    }
    const int index = view.workspaceIndices.isEmpty() ? card : view.workspaceIndices.value(card, -1);
    if (index < 0 || static_cast<std::size_t>(index) >= monitor.workspaces().size()) {
        return std::nullopt;
    }
    return index;
}

void reorderWorkspace(std::vector<Monitor> &monitors, const QString &output, int from, int to)
{
    const auto found = std::ranges::find(monitors, output, &Monitor::outputName);
    if (found == monitors.end() || from < 0 || static_cast<std::size_t>(from) >= found->workspaces().size()) {
        return;
    }
    const std::size_t target = to >= 0 ? static_cast<std::size_t>(to) : 0;
    found->moveWorkspaceToIndex(static_cast<std::size_t>(from), target);
}

}

void Engine::Private::clearOverviewDrag()
{
    overview.dragWindow.reset();
    overview.dragOutput.clear();
    overview.dragPointerRatio = QPointF();
    overview.dragPos = QPointF();
    overview.drop.reset();
    overview.reorderOutput.clear();
    overview.reorderFrom = -1;
    overview.reorderTo = -1;
    overview.lastHorizontalEdgeStep.reset();
    overview.lastVerticalEdgeStep.reset();
}

void Engine::Private::updateOverviewDrop()
{
    overview.drop.reset();
    if (!windowDrag || !overview.dragWindow) {
        return;
    }
    for (const Monitor &each : monitors) {
        const OutputInfo info = outputInfos.value(each.outputName());
        if (info.geometry.isValid() && info.geometry.contains(overview.dragPos)) {
            overview.dragOutput = each.outputName();
            break;
        }
    }
    const std::optional<OverviewView> view = overviewView(overview.dragOutput);
    Monitor *monitor = monitorByName(overview.dragOutput);
    if (!view || !monitor || view->scale <= 0.0) {
        return;
    }
    if (const auto index = monitorIndexByName(overview.dragOutput)) {
        activeMonitorIndex = *index;
    }

    const QPointF local = overview.dragPos - view->outputGeometry.topLeft();
    const int card = dropCardIndex(*view, *monitor, local);
    const std::optional<int> realWorkspace = dropWorkspaceIndex(*view, *monitor, card);
    if (!realWorkspace) {
        return;
    }

    overview.drop = OverviewDrop {
        monitor->workspaces()[*realWorkspace].id(), (overview.dragPos - view->workspaceRects.at(card).topLeft()) / view->scale};
    windowDrag->pointerPos = overview.drop->local;
    windowDrag->output = overview.dragOutput;
    windowDrag->lastPointer = overview.dragPos;
}

void Engine::overviewBeginWindowDrag(WindowId id, const QString &output, const QPointF &globalPos)
{
    if (!d->builtinOverviewOpen() || d->windowDrag) {
        return;
    }
    const QList<OverviewView> views = d->overviewViews();
    const OverviewView *view = viewAt(views, output);
    if (!view) {
        return;
    }
    const OverviewWindow *target = nullptr;
    for (const OverviewWindow &window : view->windows) {
        if (window.id == id) {
            target = &window;
        }
    }
    if (!target || target->rect.width() <= 0.0 || target->rect.height() <= 0.0) {
        return;
    }

    d->overview.dragWindow = id;
    d->overview.dragOutput = output;
    d->overview.dragPos = globalPos;
    d->overview.dragPointerRatio = QPointF(std::clamp((globalPos.x() - target->rect.left()) / target->rect.width(), 0.0, 1.0),
        std::clamp((globalPos.y() - target->rect.top()) / target->rect.height(), 0.0, 1.0));
    if (!beginWindowDrag(id, globalPos)) {
        d->clearOverviewDrag();
        return;
    }
    d->beginInteractiveMoving(output);
    d->updateOverviewDrop();
    d->updateDropHint();
    d->refresh();
}

void Engine::overviewBeginWorkspaceDrag(const QString &output, int workspaceIndex)
{
    if (!d->builtinOverviewOpen()) {
        return;
    }
    const Monitor *monitor = nullptr;
    for (const Monitor &each : d->monitors) {
        if (each.outputName() == output) {
            monitor = &each;
        }
    }
    if (!monitor || workspaceIndex < 0 || static_cast<std::size_t>(workspaceIndex) >= monitor->workspaces().size()) {
        return;
    }
    d->clearOverviewDrag();
    d->overview.reorderOutput = output;
    d->overview.reorderFrom = workspaceIndex;
    d->overview.reorderTo = workspaceIndex;
}

void Engine::overviewUpdateDrag(const QPointF &globalPos)
{
    if (!d->builtinOverviewOpen()) {
        d->clearOverviewDrag();
        return;
    }
    if (d->windowDrag && d->overview.dragWindow) {
        d->overview.dragPos = globalPos;
        d->updateOverviewDrop();
        d->scrollDragEdges();
        d->updateDropHint();
        d->refresh();
        return;
    }
    if (d->overview.reorderFrom < 0) {
        return;
    }
    const QList<OverviewView> views = d->overviewViews();
    const OverviewView *view = viewAt(views, d->overview.reorderOutput);
    if (!view || view->workspaceRects.isEmpty()) {
        return;
    }
    d->overview.reorderTo = overviewReorderIndex(view->workspaceRects, globalPos.y());
    d->refresh();
}

void Engine::overviewEndDrag()
{
    if (d->windowDrag && d->overview.dragWindow) {
        if (d->overview.drop) {
            d->windowDrag->pointerPos = d->overview.drop->local;
            d->windowDrag->output = d->overview.dragOutput;
        }
        endWindowDrag();
    } else if (d->overview.reorderFrom >= 0) {
        const QList<OverviewView> views = d->overviewViews();
        const OverviewView *view = viewAt(views, d->overview.reorderOutput);
        if (view && !view->workspaceIndices.isEmpty() && d->overview.reorderFrom < view->workspaceIndices.size()
            && d->overview.reorderTo < view->workspaceIndices.size()) {
            const int realFrom = view->workspaceIndices[d->overview.reorderFrom];
            const int realTo = view->workspaceIndices[d->overview.reorderTo];
            reorderWorkspace(d->monitors, d->overview.reorderOutput, realFrom, realTo);
        } else {
            reorderWorkspace(d->monitors, d->overview.reorderOutput, d->overview.reorderFrom, d->overview.reorderTo);
        }
    }
    d->clearOverviewDrag();
    d->refresh();
}

void Engine::overviewCancelDrag()
{
    if (d->windowDrag && d->overview.dragWindow) {
        cancelWindowDrag();
    }
    d->overview.press.reset();
    d->clearOverviewDrag();
    d->refresh();
}

}
