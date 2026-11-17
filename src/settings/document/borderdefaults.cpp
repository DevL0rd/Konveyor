#include "document/borderdefaults.h"

#include "config/loader.h"
#include "document/configdocument.h"
#include "document/nodecodec.h"
#include "document/nodepath.h"

#include <QStringList>
#include <QVariantList>

#include <cmath>
#include <functional>
#include <optional>

namespace Konveyor::Settings
{

namespace
{

constexpr QLatin1StringView ringPath("layout/focus-ring");
constexpr double oldRingWidth = 4;
constexpr QLatin1StringView oldInactiveColor("#505050");

struct StateDefault
{
    QString state;
    QString oldColor;
    bool needsUntouchedActive = false;
};

const QList<StateDefault> &stateDefaults()
{
    static const QList<StateDefault> states {
        {QStringLiteral("active"), QStringLiteral("accent")},
        {QStringLiteral("inactive"), QString(oldInactiveColor), true},
        {QStringLiteral("urgent"), QStringLiteral("#9b0000")},
    };
    return states;
}

std::optional<QString> stringArgument(const Kdl::Node *node)
{
    if (!node || node->arguments.size() != 1 || !node->arguments.first().isString()) {
        return std::nullopt;
    }
    return node->arguments.first().toString();
}

bool hasOldWidth(const Kdl::Node &ring)
{
    const Kdl::Node *width = ring.child(QStringLiteral("width"));
    return width && width->arguments.size() == 1 && width->arguments.first().isNumber()
        && width->arguments.first().toDouble() == oldRingWidth;
}

bool keepsOldColor(const Kdl::Node &ring, const StateDefault &state)
{
    if (ring.child(state.state + QStringLiteral("-gradient"))) {
        return false;
    }
    const Kdl::Node *color = ring.child(state.state + QStringLiteral("-color"));
    if (!color) {
        return true;
    }
    const std::optional<QString> value = stringArgument(color);
    return value && value->compare(state.oldColor, Qt::CaseInsensitive) == 0;
}

const Kdl::Node *namedStyle(const ConfigDocument &document, const QString &name)
{
    for (int index = 0; const Kdl::Node *style = document.find(QStringLiteral("border-style#") + QString::number(index)); ++index) {
        if (stringArgument(style) == name) {
            return style;
        }
    }
    return nullptr;
}

bool copyNode(ConfigDocument &document, const QString &path, const Kdl::Node &node)
{
    return document.setNode(path, nodeToVariant(node)).has_value();
}

bool copyShippedChild(ConfigDocument &document, const ConfigDocument &shipped, const QString &child)
{
    const QString path = QString(ringPath) + QLatin1Char('/') + child;
    const Kdl::Node *node = shipped.find(path);
    return node && copyNode(document, path, *node);
}

bool addStateStyle(ConfigDocument &document, const ConfigDocument &shipped, const StateDefault &state)
{
    const QString child = state.state + QStringLiteral("-style");
    const QString childPath = QString(ringPath) + QLatin1Char('/') + child;
    const Kdl::Node *shippedChild = shipped.find(childPath);
    const std::optional<QString> name = stringArgument(shippedChild);
    const Kdl::Node *ring = document.find(ringPath);
    const Kdl::Node *style = name ? namedStyle(shipped, *name) : nullptr;
    if (!style || ring->child(child) || !keepsOldColor(*ring, state) || namedStyle(document, *name)) {
        return true;
    }
    return copyNode(document, childPath, *shippedChild) && document.append(QString(), nodeToVariant(*style));
}

std::expected<QString, QString> validated(const ConfigDocument &document, const QString &fileName, const QString &original)
{
    return Config::loadString(document.text(), fileName) ? document.text() : original;
}

template<typename Edit> EditResult each(const QStringList &paths, const Edit &edit)
{
    EditResult result {QString()};
    for (const QString &path : paths) {
        result = result.and_then([&](const QString &) { return edit(path); });
    }
    return result;
}

EditResult dropOldInactiveColor(ConfigDocument &document)
{
    const QString path = QString(ringPath) + QStringLiteral("/inactive-color");
    return stringArgument(document.find(path)) != QString(oldInactiveColor) ? EditResult {QString()} : document.remove(path);
}

QVariantMap fireOnFocus()
{
    return {{QStringLiteral("name"), QStringLiteral("fire-on")}, {QStringLiteral("args"), QVariantList {QStringLiteral("focus")}}};
}

QStringList layerPaths(const ConfigDocument &document, const std::function<bool(const Kdl::Node &style)> &styleWanted,
    const std::function<bool(const Kdl::Node &layer)> &layerWanted)
{
    QStringList paths;
    for (int index = 0; const Kdl::Node *style = document.find(QStringLiteral("border-style#") + QString::number(index)); ++index) {
        if (!styleWanted(*style)) {
            continue;
        }
        qsizetype position = 0;
        for (const Kdl::Node &child : style->children) {
            if (child.name != QLatin1String("layer")) {
                continue;
            }
            if (layerWanted(child)) {
                paths.append(QStringLiteral("border-style#%1/layer#%2").arg(index).arg(position));
            }
            ++position;
        }
    }
    return paths;
}

QStringList heatLayerPaths(const ConfigDocument &document)
{
    return layerPaths(
        document, [](const Kdl::Node &) { return true; },
        [](const Kdl::Node &layer) { return stringArgument(&layer) == QStringLiteral("heat"); });
}

EditResult rewriteHeatChildren(ConfigDocument &document, const QString &path)
{
    QString signal = QStringLiteral("audio.level");
    EditResult result {QString()};
    for (int index = 0; const Kdl::Node *drive = document.find(path + QStringLiteral("/drive#") + QString::number(index)); ++index) {
        if (!drive->arguments.isEmpty() && drive->arguments.first().toString() == QStringLiteral("position")
            && drive->arguments.size() == 2) {
            signal = drive->arguments.at(1).toString();
            result = document.remove(path + QStringLiteral("/drive#") + QString::number(index));
            break;
        }
    }
    for (const QString &gone : {QStringLiteral("position"), QStringLiteral("baseline")}) {
        if (document.find(path + QLatin1Char('/') + gone)) {
            result = result.and_then([&](const QString &) { return document.remove(path + QLatin1Char('/') + gone); });
        }
    }
    const QVariantMap stillGradient {{QStringLiteral("name"), QStringLiteral("speed")}, {QStringLiteral("args"), QVariantList {0}}};
    const QVariantMap mix {{QStringLiteral("name"), QStringLiteral("drive")},
        {QStringLiteral("args"), QVariantList {QStringLiteral("color-mix"), signal}},
        {QStringLiteral("props"), QVariantMap {{QStringLiteral("amount"), 1.0}, {QStringLiteral("smooth"), 0.15}}}};
    return result.and_then([&](const QString &) { return document.append(path, stillGradient); }).and_then([&](const QString &) {
        return document.append(path, mix);
    });
}

QStringList idleRipplePaths(const ConfigDocument &document, const QString &name)
{
    return layerPaths(
        document, [&name](const Kdl::Node &style) { return stringArgument(&style) == name; },
        [](const Kdl::Node &layer) {
            return stringArgument(&layer) == QStringLiteral("ripple") && !layer.child(QStringLiteral("fire-on"));
        });
}

}

std::expected<QString, QString> migrateFocusEffects(const QString &text, const QString &fileName, const QString &)
{
    if (const auto loaded = Config::loadString(text, fileName); !loaded) {
        return std::unexpected(loaded.error().toString());
    }
    ConfigDocument document(text);
    const Kdl::Node *ring = document.find(ringPath);
    const std::optional<QString> active = ring ? stringArgument(ring->child(QStringLiteral("active-style"))) : std::nullopt;
    if (!active) {
        return text;
    }
    return each(idleRipplePaths(document, *active), [&](const QString &path) {
        return document.append(path, fireOnFocus());
    }).and_then([&](const QString &) { return validated(document, fileName, text); });
}

std::expected<QString, QString> migrateHeatLayers(const QString &text, const QString &fileName, const QString &)
{
    ConfigDocument document(text);
    return each(heatLayerPaths(document), [&](const QString &path) {
        return document
            .setNode(path,
                {{QStringLiteral("name"), QStringLiteral("layer")}, {QStringLiteral("args"), QVariantList {QStringLiteral("gradient")}},
                    {QStringLiteral("props"), QVariantMap {}}})
            .and_then([&](const QString &) { return rewriteHeatChildren(document, path); });
    }).and_then([&](const QString &) { return validated(document, fileName, text); });
}

std::expected<QString, QString> migrateBorderDefaults(const QString &text, const QString &fileName, const QString &defaults)
{
    if (const auto loaded = Config::loadString(text, fileName); !loaded) {
        return std::unexpected(loaded.error().toString());
    }
    ConfigDocument document(text);
    const ConfigDocument shipped(defaults);
    const Kdl::Node *ring = document.find(ringPath);
    if (!ring) {
        return text;
    }
    return dropOldInactiveColor(document).and_then([&](const QString &) -> std::expected<QString, QString> {
        ring = document.find(ringPath);
        const bool activeUntouched = !ring->child(QStringLiteral("active-style")) && keepsOldColor(*ring, stateDefaults().first());
        bool edited = !hasOldWidth(*ring) || copyShippedChild(document, shipped, QStringLiteral("width"));
        for (const StateDefault &state : stateDefaults()) {
            edited = edited && ((state.needsUntouchedActive && !activeUntouched) || addStateStyle(document, shipped, state));
        }
        if (!edited || !Config::loadString(document.text(), fileName)) {
            return text;
        }
        return document.text();
    });
}

}
