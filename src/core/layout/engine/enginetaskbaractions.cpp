#include "layout/engine/engineprivate.h"

#include <QJsonArray>
#include <QJsonDocument>

#include <cmath>

namespace Konveyor::Layout
{

namespace
{

ActionResult readGroup(const QJsonValue &group, int rank, QHash<WindowId, int> &ranks)
{
    if (!group.isArray()) {
        return actionError(QStringLiteral("taskbar groups must be arrays"));
    }
    const QJsonArray ids = group.toArray();
    for (const auto &value : ids) {
        const double id = value.toDouble();
        if (!value.isDouble() || !std::isfinite(id) || id < 1 || id > 9007199254740991.0 || std::floor(id) != id) {
            return actionError(QStringLiteral("invalid taskbar window id"));
        }
        const auto window = static_cast<WindowId>(id);
        if (!ranks.contains(window)) {
            ranks.insert(window, rank);
        }
    }
    return {};
}

ActionResult readRanks(const QString &text, QHash<WindowId, int> &ranks)
{
    QJsonParseError error;
    const QJsonDocument document = QJsonDocument::fromJson(text.toUtf8(), &error);
    if (error.error != QJsonParseError::NoError || !document.isArray()) {
        return actionError(QStringLiteral("taskbar order must be an array of window-id arrays"));
    }
    const QJsonArray groups = document.array();
    for (qsizetype rank = 0; rank < groups.size(); ++rank) {
        ActionResult parsed = readGroup(groups[rank], static_cast<int>(rank), ranks);
        if (!parsed.ok) {
            return parsed;
        }
    }
    return {};
}

ActionResult validateSnapshot(const Engine::Private &d, std::size_t output, const Config::Action &action)
{
    const WorkspaceId workspace = d.monitors[output].activeWorkspace().id();
    if (action.arguments.size() > 2) {
        bool valid = false;
        const quint64 expected = actionArgument(action, 2).toULongLong(&valid);
        if (!valid || expected != workspace) {
            return actionError(QStringLiteral("taskbar workspace changed"));
        }
    }
    if (action.arguments.size() > 3) {
        bool valid = false;
        const quint64 expected = actionArgument(action, 3).toULongLong(&valid);
        if (!valid || expected != d.taskbarOrders.value(workspace).revision) {
            return actionError(QStringLiteral("taskbar layout changed"));
        }
    }
    return {};
}

}

void registerTaskbarActions(ActionTable &table)
{
    addEngineAction(table, "order-taskbar-columns", [](Engine::Private &d, const Config::Action &action, std::optional<WindowId>) {
        const auto output = d.monitorIndexByName(actionArgument(action, 0));
        if (!output) {
            return actionError(QStringLiteral("unknown taskbar output"));
        }
        ActionResult snapshot = validateSnapshot(d, *output, action);
        if (!snapshot.ok) {
            return snapshot;
        }
        QHash<WindowId, int> ranks;
        ActionResult parsed = readRanks(actionArgument(action, 1), ranks);
        if (!parsed.ok) {
            return parsed;
        }
        if (!d.windowDrag && !d.resizeWindow) {
            d.monitors[*output].activeWorkspace().scrolling().orderTaskbarColumns(ranks);
        }
        return ActionResult();
    });
}

}
