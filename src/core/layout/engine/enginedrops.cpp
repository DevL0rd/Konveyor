#include "layout/engine/engineprivate.h"

namespace Konveyor::Layout
{

void Engine::Private::placeDragPointerOnFloatingTile()
{
    WindowDrag &move = *windowDrag;
    const Monitor *monitor = monitorOf(move.window);
    Workspace *workspace = workspaceOf(move.window);
    const Tile *tile = workspace ? workspace->tileFor(move.window) : nullptr;
    const auto tilePos = workspace ? workspace->tileRenderPosition(move.window) : std::nullopt;
    if (!monitor || !tile || !tilePos) {
        return;
    }
    const QSizeF size = tile->windowSize();
    move.output = monitor->outputName();
    move.pointerPos
        = *tilePos + tile->windowOffset() + QPointF(move.pointerRatio.x() * size.width(), move.pointerRatio.y() * size.height());
}

std::optional<QRectF> Engine::Private::dropHintRect(const Monitor &monitor) const
{
    if (!monitor.dropHint || !monitor.options()->layout.insertHint.enabled) {
        return std::nullopt;
    }
    const DropHint &hint = *monitor.dropHint;
    if (!hint.workspace.existing) {
        return std::nullopt;
    }
    const auto idx = monitor.indexOfWorkspace(hint.workspace.id);
    if (!idx) {
        return std::nullopt;
    }
    const auto area = monitor.workspaces()[*idx].dropSlotRect(hint.position);
    if (!area) {
        return std::nullopt;
    }
    const std::vector<double> offsets = workspaceRenderOffsets(monitor);
    return area->translated(originOf(monitor.outputName()) + QPointF(0.0, offsets[*idx]));
}

}
