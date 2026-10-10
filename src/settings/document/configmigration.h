#pragma once

#include <QList>
#include <QString>
#include <QStringList>

#include <expected>

namespace Konveyor::Settings
{

struct ConfigMigration
{
    QString id;
    QStringList defaultBinds;
    QString replacesAction;
};

const QList<ConfigMigration> &configMigrations();
bool shipsAnyBind(const QString &defaults, const QStringList &keys);
std::expected<QString, QString> replaceBindAction(
    const QString &text, const QString &fileName, const QString &defaults, const QStringList &keys, const QString &action);
std::expected<QString, QString> applyMigration(
    const ConfigMigration &migration, const QString &text, const QString &fileName, const QString &defaults);
std::expected<QString, QString> addDefaultBinds(
    const QString &text, const QString &fileName, const QString &defaults, const QStringList &keys);

}
