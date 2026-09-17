#include "config/sections.h"

#include <QRegularExpression>

namespace Konveyor::Config
{

namespace
{

const QStringList &globalOnlyLayoutNodes()
{
    static const QStringList names {
        QStringLiteral("remember-window-sizes"), QStringLiteral("remember-window-positions"), QStringLiteral("float-child-windows")};
    return names;
}

const QStringList &monitorOnlyLayoutNodes()
{
    static const QStringList names {
        QStringLiteral("group-app-windows"), QStringLiteral("max-rows-per-column"), QStringLiteral("new-window-placement")};
    return names;
}

void rejectNodes(const Kdl::Node &parent, const QStringList &names, const QString &scope)
{
    for (const Kdl::Node &inner : parent.children) {
        if (names.contains(inner.name)) {
            failAt(inner.location,
                QStringLiteral("node ") + quoteName(inner.name) + QStringLiteral(" is not allowed inside `") + scope + u'`');
        }
    }
}

}

void decodeOutput(LoadContext &context, const Kdl::Node &node)
{
    expectNoProperties(node);
    expectArgumentLimit(node, 1);
    OutputConfig output;
    output.name = toText(requiredArgument(node, QStringLiteral("name")));
    NodeTable table;
    table.insert(QStringLiteral("layout"), [&output](const Kdl::Node &child) {
        rejectNodes(child, globalOnlyLayoutNodes(), QStringLiteral("output.layout"));
        output.layoutPart = decodeLayoutPart(child, false);
    });
    table.insert(QStringLiteral("hot-corners"), [&output](const Kdl::Node &child) { output.hotCorners = decodeHotCorners(child); });
    decodeChildren(node, table);
    context.config.outputs.append(output);
}

MonitorMatch decodeMonitorMatch(const Kdl::Node &node)
{
    expectNoArguments(node);
    expectNoChildren(node);
    MonitorMatch match;
    ValueTable table;
    table.insert(QStringLiteral("name"), [&match](const Kdl::Value &value) {
        const QRegularExpression regex(toText(value));
        if (!regex.isValid()) {
            failAt(value.location, QStringLiteral("invalid regex: ") + regex.errorString());
        }
        match.name = regex;
    });
    const QList<std::pair<QString, std::optional<double> MonitorMatch::*>> numbers {
        {QStringLiteral("aspect-ratio-above"), &MonitorMatch::aspectRatioAbove},
        {QStringLiteral("aspect-ratio-below"), &MonitorMatch::aspectRatioBelow},
        {QStringLiteral("width-above"), &MonitorMatch::widthAbove},
        {QStringLiteral("width-below"), &MonitorMatch::widthBelow},
        {QStringLiteral("height-above"), &MonitorMatch::heightAbove},
        {QStringLiteral("height-below"), &MonitorMatch::heightBelow},
    };
    for (const auto &[name, member] : numbers) {
        table.insert(name, [&match, member](const Kdl::Value &value) { match.*member = toNumber(value, Range {0, 100000}); });
    }
    decodeProperties(node, table);
    return match;
}

void decodeMonitorProfile(LoadContext &context, const Kdl::Node &node)
{
    expectNoProperties(node);
    expectArgumentLimit(node, 1);
    MonitorProfile profile;
    profile.name = toText(requiredArgument(node, QStringLiteral("name")));
    NodeTable table;
    table.insert(QStringLiteral("match"), [&profile](const Kdl::Node &child) { profile.matches.append(decodeMonitorMatch(child)); });
    table.insert(QStringLiteral("layout"), [&profile](const Kdl::Node &child) {
        rejectNodes(child, globalOnlyLayoutNodes(), QStringLiteral("monitor-profile.layout"));
        profile.layoutPart = decodeLayoutPart(child, false);
    });
    decodeChildren(node, table, {QStringLiteral("match")});
    context.config.monitorProfiles.append(profile);
}

void decodeWorkspace(LoadContext &context, const Kdl::Node &node)
{
    expectNoProperties(node);
    expectArgumentLimit(node, 1);
    const Kdl::Value nameValue = requiredArgument(node, QStringLiteral("name"));
    NamedWorkspace workspace;
    workspace.name = toText(nameValue);
    for (const QString &seen : context.workspaceNames) {
        if (seen.compare(workspace.name, Qt::CaseInsensitive) == 0) {
            failAt(nameValue.location, QStringLiteral("duplicate named workspace: ") + workspace.name);
        }
    }
    context.workspaceNames.append(workspace.name);
    NodeTable table;
    table.insert(
        QStringLiteral("open-on-output"), [&workspace](const Kdl::Node &child) { workspace.openOnOutput = stringArgument(child); });
    table.insert(QStringLiteral("layout"), [&workspace](const Kdl::Node &child) {
        rejectNodes(child,
            globalOnlyLayoutNodes() + monitorOnlyLayoutNodes()
                + QStringList {QStringLiteral("empty-workspace-above-first"), QStringLiteral("insert-hint")},
            QStringLiteral("workspace.layout"));
        workspace.layoutPart = decodeLayoutPart(child, false);
    });
    decodeChildren(node, table);
    context.config.workspaces.append(workspace);
}

}
