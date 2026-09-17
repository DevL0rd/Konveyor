#pragma once

#include "config/types.h"
#include "layout/engine/engine.h"

#include <QJsonArray>
#include <QJsonObject>
#include <QStringList>

#include <expected>

namespace Konveyor::Ipc
{

struct WindowIdentity
{
    Layout::WindowId id = 0;
    QString title;
    QString appId;
    qint64 pid = 0;
};

struct ActionRequest
{
    Config::Action action;
    std::optional<Layout::WindowId> target;
};

QJsonObject windowToJson(const WindowIdentity &identity, const Layout::WindowState &state);
QJsonObject workspaceToJson(const Layout::WorkspaceState &state);
QJsonObject outputToJson(const Layout::OutputInfo &info);
QJsonObject bindToJson(const Config::Bind &bind);
QJsonArray gesturesToJson(const Config::Gestures &gestures);

QJsonObject actionToJson(const ActionRequest &request);
std::expected<ActionRequest, QString> actionFromJson(const QJsonObject &object);
std::expected<ActionRequest, QString> actionFromArguments(const QStringList &arguments);

}
