#include "layout/engine/engineprivate.h"

#include <QSet>

namespace Konveyor::Layout
{

namespace
{

void countWorkspaceWindows(const Workspace &workspace, QHash<WindowId, int> &counts)
{
    for (const Column &column : workspace.scrolling().columns()) {
        for (const Tile &tile : column.tiles) {
            counts[tile.id()] += 1;
        }
    }
    for (const Tile &tile : workspace.floating().tiles()) {
        counts[tile.id()] += 1;
    }
}

QString duplicateNameError(const Workspace &workspace, QSet<QString> &names)
{
    if (workspace.name().isEmpty()) {
        return {};
    }
    const QString key = workspace.name().toLower();
    if (names.contains(key)) {
        return QStringLiteral("engine: workspace name %1 is used more than once").arg(workspace.name());
    }
    names.insert(key);
    return {};
}

}

QString Engine::checkConsistency() const
{
    for (const Monitor &monitor : d->monitors) {
        if (const QString error = monitor.checkConsistency(); !error.isEmpty()) {
            return error;
        }
    }
    if (!d->monitors.empty() && d->activeMonitorIndex >= d->monitors.size()) {
        return QStringLiteral("engine: active monitor index out of range");
    }
    for (const Workspace &workspace : d->orphanWorkspaces) {
        if (const QString error = workspace.checkConsistency(); !error.isEmpty()) {
            return error;
        }
    }
    if (const QString error = d->checkWindowPlacement(); !error.isEmpty()) {
        return error;
    }
    return d->windowDrag && d->windowDrag->moving ? QString() : d->checkViews();
}

QString Engine::Private::checkViews() const
{
    for (const Monitor &monitor : monitors) {
        for (const Workspace &workspace : monitor.workspaces()) {
            if (const QString error = workspace.scrolling().verifyView(); !error.isEmpty()) {
                return error;
            }
        }
    }
    for (const Workspace &workspace : orphanWorkspaces) {
        if (const QString error = workspace.scrolling().verifyView(); !error.isEmpty()) {
            return error;
        }
    }
    return {};
}

QString Engine::Private::checkWindowPlacement() const
{
    QHash<WindowId, int> counts;
    QSet<QString> names;
    std::vector<const Workspace *> workspaces;
    for (const Monitor &monitor : monitors) {
        for (const Workspace &workspace : monitor.workspaces()) {
            workspaces.push_back(&workspace);
        }
    }
    for (const Workspace &workspace : orphanWorkspaces) {
        workspaces.push_back(&workspace);
    }
    for (const Workspace *workspace : workspaces) {
        countWorkspaceWindows(*workspace, counts);
        if (const QString error = duplicateNameError(*workspace, names); !error.isEmpty()) {
            return error;
        }
    }
    if (windowDrag && windowDrag->tile) {
        counts[windowDrag->tile->id()] += 1;
    }
    for (auto it = counts.constBegin(); it != counts.constEnd(); ++it) {
        if (it.value() != 1) {
            return QStringLiteral("engine: window %1 is placed %2 times").arg(it.key()).arg(it.value());
        }
    }
    if (focused && !counts.contains(*focused)) {
        return QStringLiteral("engine: focused window %1 is not in the layout").arg(*focused);
    }
    return {};
}

}
