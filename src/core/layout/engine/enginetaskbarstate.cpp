#include "layout/engine/engineprivate.h"

#include <algorithm>

namespace Konveyor::Layout
{

namespace
{

QList<QList<WindowId>> taskbarColumns(const Workspace &workspace)
{
    QList<QList<WindowId>> columns;
    for (const Column &column : workspace.scrolling().columns()) {
        QList<WindowId> windows;
        for (const Tile &tile : column.tiles) {
            windows.append(tile.id());
        }
        columns.append(windows);
    }
    return columns;
}

TaskbarOrder updatedOrder(
    const Workspace &workspace, TaskbarOrder previous, const QSet<WindowId> &launches, bool taskbar, quint64 &revision)
{
    const auto columns = taskbarColumns(workspace);
    if (columns != previous.columns) {
        const bool launch = std::ranges::any_of(launches, [&](WindowId id) { return workspace.hasWindow(id); });
        previous.columns = columns;
        previous.revision = ++revision;
        previous.source = taskbar ? QStringLiteral("taskbar") : (launch ? QStringLiteral("launch") : QStringLiteral("layout"));
    }
    return previous;
}

}

void Engine::Private::refreshTaskbarOrders()
{
    QHash<WorkspaceId, TaskbarOrder> next;
    for (const Workspace *workspace : allWorkspaces()) {
        next.insert(workspace->id(),
            updatedOrder(*workspace, taskbarOrders.value(workspace->id()), taskbarLaunches, taskbarAction, taskbarRevision));
    }
    taskbarOrders = std::move(next);
    taskbarLaunches.clear();
    taskbarAction = false;
}

}
