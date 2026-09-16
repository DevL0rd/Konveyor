#pragma once

#include <QJsonDocument>
#include <QString>

namespace Konveyor::Cli
{

QString formatWindows(const QJsonDocument &document);
QString formatWorkspaces(const QJsonDocument &document);
QString formatOutputs(const QJsonDocument &document);
QString formatFocusedWindow(const QJsonDocument &document);
QString formatBinds(const QJsonDocument &document);

}
