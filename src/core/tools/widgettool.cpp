#include "tools/widgettool.h"

#include <QDir>
#include <QFileInfo>
#include <QProcess>
#include <QStandardPaths>

namespace Konveyor::Tools
{

// ~/.local/bin first, where install.sh links the tools, then PATH for system packages.
QString widgetToolPath(const QString &name)
{
    const QString localBin = QDir::home().filePath(QStringLiteral(".local/bin"));
    QString path = QStandardPaths::findExecutable(name, {localBin});
    if (path.isEmpty()) {
        path = QStandardPaths::findExecutable(name);
    }
    return path.isEmpty() ? QDir(localBin).filePath(name) : path;
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
            "%1 is not installed at %2 or on PATH. It comes with the Konveyor widgets; run ./install.sh without --no-widgets to add them.")
                .arg(name, path));
    }
    if (!QProcess::startDetached(path, arguments)) {
        return std::unexpected(QStringLiteral("Could not start %1.").arg(path));
    }
    return {};
}

}
