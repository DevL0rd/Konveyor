#include "document/configmigration.h"

#include "config/loader.h"
#include "document/borderdefaults.h"
#include "document/configdocument.h"
#include "document/nodecodec.h"

#include <algorithm>

namespace Konveyor::Settings
{

namespace
{

QStringList numbered(const QString &prefix)
{
    QStringList keys;
    for (int number = 1; number <= 9; ++number) {
        keys.append(prefix + QString::number(number));
    }
    return keys;
}

QStringList bindPaths(const QStringList &keys)
{
    QStringList paths;
    for (const QString &key : keys) {
        paths.append(QStringLiteral("binds/") + key);
    }
    return paths;
}

ConfigMigration addBinds(const QString &id, const QStringList &keys)
{
    return {id, bindPaths(keys), [keys](const QString &text, const QString &fileName, const QString &defaults) {
                return addDefaultBinds(text, fileName, defaults, keys);
            }};
}

ConfigMigration replaceBinds(const QString &id, const QStringList &keys, const QString &action)
{
    return {id, bindPaths(keys), [keys, action](const QString &text, const QString &fileName, const QString &defaults) {
                return replaceBindAction(text, fileName, defaults, keys, action);
            }};
}

}

const QList<ConfigMigration> &configMigrations()
{
    static const QList<ConfigMigration> migrations {
        {QStringLiteral("heat-layers"), {QStringLiteral("border-style")}, migrateHeatLayers},
        addBinds(QStringLiteral("focus-column-binds"), numbered(QStringLiteral("Super+Alt+"))),
        replaceBinds(QStringLiteral("taskbar-item-binds"), numbered(QStringLiteral("Super+Alt+")), QStringLiteral("focus-column")),
        replaceBinds(QStringLiteral("fullscreen-window-bind"), {QStringLiteral("Mod+F")}, QStringLiteral("cycle-window-expansion")),
        {QStringLiteral("animated-border-defaults"), {QStringLiteral("border-style")}, migrateBorderDefaults},
        {QStringLiteral("focus-effects"), {QStringLiteral("border-style")}, migrateFocusEffects},
    };
    return migrations;
}

bool shipsAny(const QString &defaults, const QStringList &paths)
{
    const ConfigDocument shipped(defaults);
    return std::ranges::any_of(paths, [&shipped](const QString &path) { return shipped.find(path) != nullptr; });
}

std::expected<QString, QString> addDefaultBinds(
    const QString &text, const QString &fileName, const QString &defaults, const QStringList &keys)
{
    if (const auto loaded = Config::loadString(text, fileName); !loaded) {
        return std::unexpected(loaded.error().toString());
    }
    ConfigDocument document(text);
    if (!document.find(QStringLiteral("binds"))) {
        return text;
    }
    const ConfigDocument shipped(defaults);
    for (const QString &key : keys) {
        const Kdl::Node *bind = shipped.find(QStringLiteral("binds/") + key);
        if (!bind) {
            continue;
        }
        ConfigDocument attempt = document;
        if (attempt.append(QStringLiteral("binds"), nodeToVariant(*bind)) && Config::loadString(attempt.text(), fileName)) {
            document = attempt;
        }
    }
    return document.text();
}

std::expected<QString, QString> replaceBindAction(
    const QString &text, const QString &fileName, const QString &defaults, const QStringList &keys, const QString &action)
{
    if (const auto loaded = Config::loadString(text, fileName); !loaded) {
        return std::unexpected(loaded.error().toString());
    }
    ConfigDocument document(text);
    const ConfigDocument shipped(defaults);
    for (const QString &key : keys) {
        const QString path = QStringLiteral("binds/") + key;
        const Kdl::Node *current = document.find(path);
        const Kdl::Node *replacement = shipped.find(path);
        if (!current || !replacement || current->children.size() != 1 || replacement->children.size() != 1 || !current->properties.empty()
            || current->children.front().name != action
            || nodeToVariant(current->children.front()).value(QStringLiteral("args"))
                != nodeToVariant(replacement->children.front()).value(QStringLiteral("args"))) {
            continue;
        }
        ConfigDocument attempt = document;
        if (attempt.setNode(path + QLatin1Char('/') + action, nodeToVariant(replacement->children.front()))
            && Config::loadString(attempt.text(), fileName)) {
            document = attempt;
        }
    }
    return document.text();
}

std::expected<QString, QString> applyMigration(
    const ConfigMigration &migration, const QString &text, const QString &fileName, const QString &defaults)
{
    return migration.edit(text, fileName, defaults);
}

}
