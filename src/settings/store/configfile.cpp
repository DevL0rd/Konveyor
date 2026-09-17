#include "store/configfile.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSaveFile>

namespace Konveyor::Settings
{

std::optional<QString> readConfigText(const QString &path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return std::nullopt;
    }
    return QString::fromUtf8(file.readAll());
}

std::expected<void, QString> writeConfigText(const QString &path, const QString &text)
{
    QDir().mkpath(QFileInfo(path).path());
    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return std::unexpected(QStringLiteral("Could not write %1: %2").arg(path, file.errorString()));
    }
    file.write(text.toUtf8());
    if (!file.commit()) {
        return std::unexpected(QStringLiteral("Could not write %1: %2").arg(path, file.errorString()));
    }
    return {};
}

}
