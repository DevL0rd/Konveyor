#include "document/configmigration.h"

#include "config/loader.h"
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

}

const QList<ConfigMigration> &configMigrations()
{
    static const QList<ConfigMigration> migrations {
        {QStringLiteral("focus-column-binds"), numbered(QStringLiteral("Super+Alt+")), {}},
        {QStringLiteral("taskbar-item-binds"), numbered(QStringLiteral("Super+Alt+")), QStringLiteral("focus-column")},
    };
    return migrations;
}

bool shipsAnyBind(const QString &defaults, const QStringList &keys)
{
    const ConfigDocument shipped(defaults);
    return std::ranges::any_of(keys, [&shipped](const QString &key) { return shipped.find(QStringLiteral("binds/") + key) != nullptr; });
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
    if (migration.replacesAction.isEmpty()) {
        return addDefaultBinds(text, fileName, defaults, migration.defaultBinds);
    }
    return replaceBindAction(text, fileName, defaults, migration.defaultBinds, migration.replacesAction);
}

}
