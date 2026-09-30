#pragma once

#include <QString>
#include <QStringList>

#include <expected>

namespace Konveyor::Tools
{

QString widgetToolPath(const QString &name);
bool widgetToolInstalled(const QString &name);
std::expected<void, QString> startWidgetTool(const QString &name, const QStringList &arguments);

}
