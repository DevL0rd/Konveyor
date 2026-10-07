#pragma once

#include "document/configdocument.h"

#include <QJsonObject>
#include <QString>

#include <optional>

namespace Konveyor::Settings
{

struct AppPlace
{
    QString appId;
    QString output;
    QString workspaceName;
    int workspaceIndex = 1;
    std::optional<int> column;
    int width = 0;
    std::optional<int> height;
};

QString appIdPattern(const QString &appId);
QJsonObject appRules(const ConfigDocument &document, const QString &appId, const QString &output);
EditResult setAppRule(ConfigDocument &document, const AppPlace &place, const QString &option, bool enabled);

}
