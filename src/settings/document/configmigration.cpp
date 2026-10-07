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
        {QStringLiteral("focus-column-binds"), numbered(QStringLiteral("Super+Alt+"))},
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

}
