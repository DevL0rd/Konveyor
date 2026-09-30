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
    const Kdl::Value nameValue = requiredArgument(node, QStringLiteral("name"));
    output.name = toText(nameValue);
    for (const OutputConfig &seen : context.config.outputs) {
        if (seen.name.compare(output.name, Qt::CaseInsensitive) == 0) {
            failAt(nameValue.location, QStringLiteral("duplicate output: ") + output.name);
        }
    }
    NodeTable table;
    table.insert(QStringLiteral("layout"), [&output](const Kdl::Node &child) {
        rejectNodes(child, globalOnlyLayoutNodes(), QStringLiteral("output.layout"));
        output.layout = decodeLayoutPart(child, false);
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
    QHash<QString, Kdl::Location> locations;
    for (const auto &[name, member] : numbers) {
        table.insert(name, [&match, &locations, name, member](const Kdl::Value &value) {
            match.*member = toNumber(value, Range {0, 100000});
            locations.insert(name, value.location);
        });
    }
    decodeProperties(node, table);
    for (qsizetype index = 0; index < numbers.size(); index += 2) {
        const auto &[aboveName, above] = numbers.at(index);
        const auto &[belowName, below] = numbers.at(index + 1);
        if (match.*above && match.*below && *(match.*above) >= *(match.*below)) {
            failAt(locations.value(aboveName), quoteName(aboveName) + QStringLiteral(" must be less than ") + quoteName(belowName));
        }
    }
    return match;
}

void decodeMonitorProfile(LoadContext &context, const Kdl::Node &node)
{
    expectNoProperties(node);
    expectArgumentLimit(node, 1);
    MonitorProfile profile;
    const Kdl::Value nameValue = requiredArgument(node, QStringLiteral("name"));
    profile.name = toText(nameValue);
    for (const MonitorProfile &seen : context.config.monitorProfiles) {
        if (seen.name == profile.name) {
            failAt(nameValue.location, QStringLiteral("duplicate monitor profile: ") + profile.name);
        }
    }
    NodeTable table;
    table.insert(QStringLiteral("match"), [&profile](const Kdl::Node &child) { profile.matches.append(decodeMonitorMatch(child)); });
    table.insert(QStringLiteral("layout"), [&profile](const Kdl::Node &child) {
        rejectNodes(child, globalOnlyLayoutNodes(), QStringLiteral("monitor-profile.layout"));
        profile.layout = decodeLayoutPart(child, false);
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
        workspace.layout = decodeLayoutPart(child, false);
    });
    decodeChildren(node, table);
    context.config.workspaces.append(workspace);
}

}
