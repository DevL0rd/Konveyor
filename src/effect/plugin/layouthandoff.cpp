#include "plugin/layouthandoff.h"

#include "kwin/windowregistry.h"

#include <window.h>
#include <workspace.h>

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

#include <tuple>

namespace Konveyor
{

namespace
{

constexpr const char *handoffProperty = "konveyorLayoutHandoff";

QJsonArray rectToJson(const QRectF &rect)
{
    return {rect.x(), rect.y(), rect.width(), rect.height()};
}

QRectF rectFromJson(const QJsonArray &array)
{
    return {array.at(0).toDouble(), array.at(1).toDouble(), array.at(2).toDouble(), array.at(3).toDouble()};
}

QJsonObject entryToJson(const QUuid &window, const Layout::WindowState &state, const Layout::RestorePlacement &placement)
{
    return {
        {QStringLiteral("window"), window.toString()},
        {QStringLiteral("output"), state.output},
        {QStringLiteral("workspace"), state.workspaceIndex},
        {QStringLiteral("floating"), placement.isFloating},
        {QStringLiteral("frame"), rectToJson(placement.floatingFrame)},
        {QStringLiteral("column"), static_cast<qint64>(placement.columnIndex)},
        {QStringLiteral("tile"), placement.tileIndex ? QJsonValue(static_cast<qint64>(*placement.tileIndex)) : QJsonValue()},
        {QStringLiteral("proportion"), placement.width.isProportion},
        {QStringLiteral("width"), placement.width.value},
    };
}

}

void LayoutHandoff::give(const Layout::Engine &engine, const WindowRegistry &windows)
{
    QJsonArray entries;
    const QList<Layout::WindowState> states = engine.windowStates();
    for (const Layout::WindowState &state : states) {
        const KWin::Window *window = windows.windowOf(state.id);
        const std::optional<Layout::RestorePlacement> placement = engine.placementOf(state.id);
        if (window && placement) {
            entries.append(entryToJson(window->internalId(), state, *placement));
        }
    }
    KWin::workspace()->setProperty(handoffProperty, QJsonDocument(entries).toJson(QJsonDocument::Compact));
}

LayoutHandoff LayoutHandoff::take()
{
    const QByteArray json = KWin::workspace()->property(handoffProperty).toByteArray();
    KWin::workspace()->setProperty(handoffProperty, QVariant());
    LayoutHandoff handoff;
    const QJsonArray entries = QJsonDocument::fromJson(json).array();
    for (const QJsonValue &value : entries) {
        const QJsonObject object = value.toObject();
        Entry entry;
        entry.output = object.value(QStringLiteral("output")).toString();
        entry.workspaceIndex = object.value(QStringLiteral("workspace")).toInt();
        entry.placement.isFloating = object.value(QStringLiteral("floating")).toBool();
        entry.placement.floatingFrame = rectFromJson(object.value(QStringLiteral("frame")).toArray());
        entry.placement.columnIndex = static_cast<std::size_t>(object.value(QStringLiteral("column")).toInteger());
        if (const QJsonValue tile = object.value(QStringLiteral("tile")); !tile.isNull()) {
            entry.placement.tileIndex = static_cast<std::size_t>(tile.toInteger());
        }
        entry.placement.width = {object.value(QStringLiteral("proportion")).toBool(), object.value(QStringLiteral("width")).toDouble()};
        handoff.m_entries.insert(QUuid::fromString(object.value(QStringLiteral("window")).toString()), entry);
    }
    return handoff;
}

bool LayoutHandoff::comesBefore(KWin::Window *first, KWin::Window *second) const
{
    const auto key = [this](KWin::Window *window) {
        const auto entry = m_entries.constFind(window->internalId());
        if (entry == m_entries.constEnd()) {
            return std::tuple(true, QString(), 0, false, std::size_t(0), std::size_t(0));
        }
        return std::tuple(false, entry->output, entry->workspaceIndex, entry->placement.isFloating, entry->placement.columnIndex,
            entry->placement.tileIndex.value_or(0));
    };
    return key(first) < key(second);
}

std::optional<HandedOverPlacement> LayoutHandoff::takePlacement(KWin::Window *window, const Layout::Engine &engine)
{
    const auto entry = m_entries.constFind(window->internalId());
    if (entry == m_entries.constEnd()) {
        return std::nullopt;
    }
    const Entry handed = *entry;
    m_entries.erase(entry);
    std::optional<Layout::WorkspaceState> target;
    const QList<Layout::WorkspaceState> workspaces = engine.workspaceStates();
    for (const Layout::WorkspaceState &workspace : workspaces) {
        if (workspace.output == handed.output && workspace.index <= handed.workspaceIndex && (!target || workspace.index > target->index)) {
            target = workspace;
        }
    }
    if (!target) {
        return std::nullopt;
    }
    Layout::RestorePlacement placement = handed.placement;
    placement.workspace = target->id;
    return HandedOverPlacement {handed.output, placement};
}

}
