#include "document/apprules.h"

#include "document/nodecodec.h"

#include <QHash>

#include <algorithm>

namespace Konveyor::Settings
{

namespace
{

constexpr QLatin1StringView Rule("window-rule");
constexpr QLatin1StringView Width("default-column-width");
constexpr QLatin1StringView Height("default-window-height");

const QHash<QString, QString> &optionNodes()
{
    static const QHash<QString, QString> nodes {
        {QStringLiteral("float"), QStringLiteral("open-floating")},
        {QStringLiteral("all-workspaces"), QStringLiteral("open-on-all-workspaces")},
        {QStringLiteral("stack"), QStringLiteral("group-app-windows")},
        {QStringLiteral("row"), QStringLiteral("new-window-placement")},
        {QStringLiteral("column"), QStringLiteral("open-at-column")},
        {QStringLiteral("workspace"), QStringLiteral("open-on-workspace")},
        {QStringLiteral("monitor"), QStringLiteral("open-on-output")},
    };
    return nodes;
}

QString exactly(const QString &text)
{
    static const QString special = QStringLiteral(R"(.*+?^${}()|[]\/)");
    QString pattern = QStringLiteral("^");
    for (const QChar character : text) {
        if (special.contains(character)) {
            pattern += QLatin1Char('\\');
        }
        pattern += character;
    }
    return pattern + QLatin1Char('$');
}

QVariantMap matchProps(const QString &appId, const QString &output)
{
    QVariantMap props {{QStringLiteral("app-id"), appIdPattern(appId)}};
    if (!output.isEmpty()) {
        props.insert(QStringLiteral("output"), exactly(output));
    }
    return props;
}

QVariantMap leaf(const QString &name, const QVariantList &arguments)
{
    return {{QStringLiteral("name"), name}, {QStringLiteral("args"), arguments}, {QStringLiteral("props"), QVariantMap()}};
}

QVariantMap block(const QString &name, const QVariantList &children)
{
    QVariantMap node = leaf(name, {});
    node.insert(QStringLiteral("children"), children);
    return node;
}

std::optional<QString> rulePath(const ConfigDocument &document, const QVariantMap &props)
{
    std::optional<QString> found;
    for (const QVariant &entry : document.childPaths(QString(), QString(Rule))) {
        const QVariantList children = entry.toMap().value(QStringLiteral("node")).toMap().value(QStringLiteral("children")).toList();
        int matches = 0;
        bool excludes = false;
        QVariantMap matched;
        for (const QVariant &child : children) {
            const QVariantMap node = child.toMap();
            const QString name = node.value(QStringLiteral("name")).toString();
            if (name == QLatin1String("match")) {
                ++matches;
                matched = node.value(QStringLiteral("props")).toMap();
            }
            excludes = excludes || name == QLatin1String("exclude");
        }
        if (!excludes && matches == 1 && matched == props) {
            found = entry.toMap().value(QStringLiteral("path")).toString();
        }
    }
    return found;
}

QVariant firstArgument(const ConfigDocument &document, const std::optional<QString> &rule, const QString &name)
{
    const Kdl::Node *node = rule ? document.find(*rule + QLatin1Char('/') + name) : nullptr;
    return node ? nodeToVariant(*node).value(QStringLiteral("args")).toList().value(0) : QVariant();
}

QJsonValue fixedSize(const ConfigDocument &document, const QString &rule, const QString &name)
{
    const Kdl::Node *node = document.find(rule + QLatin1Char('/') + name + QStringLiteral("/fixed"));
    return node ? QJsonValue::fromVariant(nodeToVariant(*node).value(QStringLiteral("args")).toList().value(0)) : QJsonValue();
}

EditResult ruleFor(ConfigDocument &document, const QVariantMap &props)
{
    if (const std::optional<QString> path = rulePath(document, props)) {
        return *path;
    }
    QVariantMap match = leaf(QStringLiteral("match"), {});
    match.insert(QStringLiteral("props"), props);
    return document.append(QString(), block(QString(Rule), {match}));
}

EditResult removeIfOnlyMatches(ConfigDocument &document, const QString &rule)
{
    const Kdl::Node *node = document.find(rule);
    const bool onlyMatches
        = node && std::ranges::all_of(node->children, [](const Kdl::Node &child) { return child.name == QLatin1String("match"); });
    return onlyMatches ? document.remove(rule) : EditResult(rule);
}

EditResult setSize(ConfigDocument &document, const AppPlace &place, bool enabled)
{
    const QVariantMap props = matchProps(place.appId, place.output);
    if (!enabled) {
        const std::optional<QString> path = rulePath(document, props);
        return path ? document.remove(*path) : EditResult(QString());
    }
    if (place.width <= 0) {
        return std::unexpected(QStringLiteral("the window has no size to remember"));
    }
    const EditResult rule = ruleFor(document, props);
    if (!rule) {
        return rule;
    }
    const auto sized = [](const QString &name, int value) { return block(name, {leaf(QStringLiteral("fixed"), {value})}); };
    if (const EditResult width = document.setNode(*rule + QLatin1Char('/') + Width, sized(QString(Width), place.width)); !width) {
        return width;
    }
    const QString heightPath = *rule + QLatin1Char('/') + Height;
    return place.height ? document.setNode(heightPath, sized(QString(Height), *place.height)) : document.remove(heightPath);
}

std::optional<QVariant> optionValue(const AppPlace &place, const QString &option)
{
    if (option == QLatin1String("float") || option == QLatin1String("all-workspaces")) {
        return true;
    }
    if (option == QLatin1String("stack") || option == QLatin1String("row")) {
        return QStringLiteral("stack");
    }
    if (option == QLatin1String("column")) {
        return place.column ? std::optional<QVariant>(*place.column) : std::nullopt;
    }
    if (option == QLatin1String("workspace")) {
        return place.workspaceName.isEmpty() ? QVariant(place.workspaceIndex) : QVariant(place.workspaceName);
    }
    return place.output.isEmpty() ? std::nullopt : std::optional<QVariant>(place.output);
}

}

QString appIdPattern(const QString &appId)
{
    return exactly(appId);
}

QJsonObject appRules(const ConfigDocument &document, const QString &appId, const QString &output)
{
    const std::optional<QString> rule = rulePath(document, matchProps(appId, QString()));
    const auto argument = [&](const QString &name) { return firstArgument(document, rule, name); };
    QJsonObject rules {
        {QStringLiteral("float"), argument(QStringLiteral("open-floating")) == QVariant(true)},
        {QStringLiteral("all-workspaces"), argument(QStringLiteral("open-on-all-workspaces")) == QVariant(true)},
        {QStringLiteral("stack"), argument(QStringLiteral("group-app-windows")) == QVariant(QStringLiteral("stack"))},
        {QStringLiteral("row"), argument(QStringLiteral("new-window-placement")) == QVariant(QStringLiteral("stack"))},
    };
    for (const QString &option : {QStringLiteral("column"), QStringLiteral("workspace"), QStringLiteral("monitor")}) {
        const QVariant value = argument(optionNodes().value(option));
        rules.insert(option, value.isValid() ? QJsonValue::fromVariant(value) : QJsonValue());
    }
    const std::optional<QString> sized = output.isEmpty() ? std::nullopt : rulePath(document, matchProps(appId, output));
    const QJsonValue width = sized ? fixedSize(document, *sized, Width) : QJsonValue();
    rules.insert(QStringLiteral("size"),
        width.isUndefined() || width.isNull()
            ? QJsonValue()
            : QJsonValue(QJsonObject {{QStringLiteral("width"), width}, {QStringLiteral("height"), fixedSize(document, *sized, Height)}}));
    return rules;
}

EditResult setAppRule(ConfigDocument &document, const AppPlace &place, const QString &option, bool enabled)
{
    if (place.appId.isEmpty()) {
        return std::unexpected(QStringLiteral("the window has no application id"));
    }
    if (option == QLatin1String("size")) {
        return setSize(document, place, enabled);
    }
    const QString name = optionNodes().value(option);
    if (name.isEmpty()) {
        return std::unexpected(QStringLiteral("unknown app rule option: %1").arg(option));
    }
    const QVariantMap props = matchProps(place.appId, QString());
    if (!enabled) {
        const std::optional<QString> rule = rulePath(document, props);
        if (!rule) {
            return QString();
        }
        const EditResult removed = document.remove(*rule + QLatin1Char('/') + name);
        return removed ? removeIfOnlyMatches(document, *rule) : removed;
    }
    const std::optional<QVariant> value = optionValue(place, option);
    if (!value) {
        return std::unexpected(QStringLiteral("the window has nothing to remember for %1").arg(option));
    }
    const EditResult rule = ruleFor(document, props);
    return rule ? document.setNode(*rule + QLatin1Char('/') + name, leaf(name, {*value})) : rule;
}

}
