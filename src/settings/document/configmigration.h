#pragma once

#include <QList>
#include <QString>
#include <QStringList>

#include <expected>
#include <functional>

namespace Konveyor::Settings
{

using MigrationEdit = std::function<std::expected<QString, QString>(const QString &text, const QString &fileName, const QString &defaults)>;

struct ConfigMigration
{
    QString id;
    QStringList shippedPaths;
    MigrationEdit edit;
};

const QList<ConfigMigration> &configMigrations();
bool shipsAny(const QString &defaults, const QStringList &paths);
std::expected<QString, QString> replaceBindAction(
    const QString &text, const QString &fileName, const QString &defaults, const QStringList &keys, const QString &action);
std::expected<QString, QString> applyMigration(
    const ConfigMigration &migration, const QString &text, const QString &fileName, const QString &defaults);
std::expected<QString, QString> addDefaultBinds(
    const QString &text, const QString &fileName, const QString &defaults, const QStringList &keys);

}
