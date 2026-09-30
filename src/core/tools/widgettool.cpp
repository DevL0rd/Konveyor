#include "tools/widgettool.h"

#include <QDir>
#include <QFileInfo>
#include <QProcess>

namespace Konveyor::Tools
{

QString widgetToolPath(const QString &name)
{
    return QDir::home().filePath(QStringLiteral(".local/bin/") + name);
}

bool widgetToolInstalled(const QString &name)
{
    const QFileInfo tool(widgetToolPath(name));
    return tool.isFile() && tool.isExecutable();
}

std::expected<void, QString> startWidgetTool(const QString &name, const QStringList &arguments)
{
    const QString path = widgetToolPath(name);
    if (!widgetToolInstalled(name)) {
        return std::unexpected(QStringLiteral(
            "%1 is not installed at %2. It comes with the Konveyor widgets; run ./install.sh without --no-widgets to add them.")
                .arg(name, path));
    }
    if (!QProcess::startDetached(path, arguments)) {
        return std::unexpected(QStringLiteral("Could not start %1.").arg(path));
    }
    return {};
}

}
