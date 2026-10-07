#include "plugin/konveyoreffect_p.h"

#include <QJsonObject>

namespace Konveyor
{

namespace
{

QJsonObject requestOf(const QString &json)
{
    return QJsonDocument::fromJson(json.toUtf8()).object();
}

Layout::WindowId windowIdOf(const QJsonObject &request)
{
    return static_cast<Layout::WindowId>(request.value(QStringLiteral("id")).toInteger(-1));
}

}

std::optional<Settings::AppPlace> KonveyorEffect::appPlace(Layout::WindowId id) const
{
    KWin::Window *window = d->windows.windowOf(id);
    const std::optional<Layout::WindowState> state = readEngine().windowState(id);
    if (!window || !state) {
        return std::nullopt;
    }
    Settings::AppPlace place;
    place.appId = d->windows.propertiesOf(window).appId;
    place.output = state->output;
    place.workspaceIndex = state->workspaceIndex;
    qsizetype rows = 1;
    const QList<Layout::WorkspaceState> workspaces = readEngine().workspaceStates();
    for (const Layout::WorkspaceState &workspace : workspaces) {
        if (workspace.id == state->workspace) {
            place.workspaceName = workspace.name;
            rows = state->isFloating ? 1 : workspace.columns.value(state->columnIndex).size();
        }
    }
    if (!state->isFloating) {
        place.column = state->columnIndex + 1;
    }
    place.width = qRound(state->targetFrame.width());
    if (state->isFloating || rows > 1) {
        place.height = qRound(state->targetFrame.height());
    }
    return place;
}

QJsonDocument KonveyorEffect::appRulesJson(const QString &json) const
{
    const std::optional<Settings::AppPlace> place = appPlace(windowIdOf(requestOf(json)));
    const auto text = d->config.text();
    if (!place || !text) {
        return QJsonDocument();
    }
    const auto optional = [](const auto &value) { return value ? QJsonValue(*value) : QJsonValue(); };
    return QJsonDocument(QJsonObject {
        {QStringLiteral("app_id"), place->appId},
        {QStringLiteral("output"), place->output},
        {QStringLiteral("workspace"), place->workspaceIndex},
        {QStringLiteral("workspace_name"), place->workspaceName.isEmpty() ? QJsonValue() : QJsonValue(place->workspaceName)},
        {QStringLiteral("column"), optional(place->column)},
        {QStringLiteral("width"), place->width},
        {QStringLiteral("height"), optional(place->height)},
        {QStringLiteral("rules"), Settings::appRules(Settings::ConfigDocument(*text), place->appId, place->output)},
    });
}

QString KonveyorEffect::setAppRuleJson(const QString &json)
{
    const QJsonObject request = requestOf(json);
    const std::optional<Settings::AppPlace> place = appPlace(windowIdOf(request));
    if (!place) {
        return QStringLiteral("no window with id %1").arg(request.value(QStringLiteral("id")).toVariant().toString());
    }
    const QString option = request.value(QStringLiteral("option")).toString();
    const bool enabled = request.value(QStringLiteral("enabled")).toBool(true);
    const auto written = d->config.rewrite([&](const QString &text) -> std::expected<QString, QString> {
        Settings::ConfigDocument document(text);
        if (const Settings::EditResult edited = Settings::setAppRule(document, *place, option, enabled); !edited) {
            return std::unexpected(edited.error());
        }
        return document.text();
    });
    return written ? d->config.load() : written.error();
}

}
