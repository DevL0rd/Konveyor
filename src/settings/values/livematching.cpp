#include "values/livematching.h"

#include "config/loader.h"
#include "layout/monitor/monitorprofiles.h"
#include "layout/rules/windowrules.h"

namespace Konveyor::Settings
{

namespace
{

QString outputOfWorkspaceId(const QVariant &workspaceId, const QVariantList &workspaces)
{
    for (const QVariant &workspace : workspaces) {
        const QVariantMap entry = workspace.toMap();
        if (entry.value(QStringLiteral("id")) == workspaceId) {
            return entry.value(QStringLiteral("output")).toString();
        }
    }
    return QString();
}

QString profileForOutput(const Config::Config &config, const QString &name, const QVariantList &outputs)
{
    for (const QVariant &output : outputs) {
        if (!name.isEmpty() && output.toMap().value(QStringLiteral("name")) == name) {
            return profileNameFor(config, output.toMap());
        }
    }
    return QString();
}

template<typename Entry> const Entry *entryNamed(const QList<Entry> &entries, const QString &name, Qt::CaseSensitivity sensitivity)
{
    const auto found = std::ranges::find_if(entries, [&](const Entry &entry) { return entry.name.compare(name, sensitivity) == 0; });
    return found == entries.end() ? nullptr : &*found;
}

Config::Layout withPart(const Config::Layout &layout, const std::optional<Config::LayoutPart> &part)
{
    return part ? Config::mergedLayout(layout, *part) : layout;
}

Config::Layout outputLayout(const Config::Config &config, const QString &name, const QVariantList &outputs)
{
    Config::Layout layout = config.layout;
    for (const QVariant &output : outputs) {
        if (output.toMap().value(QStringLiteral("name")).toString().compare(name, Qt::CaseInsensitive) != 0) {
            continue;
        }
        if (const auto *profile = entryNamed(config.monitorProfiles, profileNameFor(config, output.toMap()), Qt::CaseSensitive)) {
            layout = withPart(layout, profile->layout);
        }
    }
    const auto *entry = entryNamed(config.outputs, name, Qt::CaseInsensitive);
    return entry ? withPart(layout, entry->layout) : layout;
}

QString outputOfWorkspace(const Config::NamedWorkspace &named, const QVariantList &workspaces)
{
    for (const QVariant &workspace : workspaces) {
        const QVariantMap entry = workspace.toMap();
        if (entry.value(QStringLiteral("name")).toString().compare(named.name, Qt::CaseInsensitive) == 0) {
            return entry.value(QStringLiteral("output")).toString();
        }
    }
    return named.openOnOutput.value_or(QString());
}

}

Config::Layout scopedLayout(
    const Config::Config &config, const QString &kind, const QString &name, const QVariantList &outputs, const QVariantList &workspaces)
{
    if (kind == QLatin1String("output")) {
        return outputLayout(config, name, outputs);
    }
    if (kind == QLatin1String("monitor-profile")) {
        const auto *profile = entryNamed(config.monitorProfiles, name, Qt::CaseSensitive);
        return profile ? withPart(config.layout, profile->layout) : config.layout;
    }
    if (kind == QLatin1String("workspace")) {
        const auto *named = entryNamed(config.workspaces, name, Qt::CaseInsensitive);
        if (!named) {
            return config.layout;
        }
        const QString output = outputOfWorkspace(*named, workspaces);
        return withPart(output.isEmpty() ? config.layout : outputLayout(config, output, outputs), named->layout);
    }
    return config.layout;
}

QString profileNameFor(const Config::Config &config, const QVariantMap &output)
{
    const QVariantMap logical = output.value(QStringLiteral("logical")).toMap();
    const QSizeF size(logical.value(QStringLiteral("width")).toDouble(), logical.value(QStringLiteral("height")).toDouble());
    const Config::MonitorProfile *profile = Layout::monitorProfileFor(config, output.value(QStringLiteral("name")).toString(), size);
    return profile ? profile->name : QString();
}

QVariantList windowsMatchingRule(const Config::Config &config, const Config::WindowRule &rule, const QVariantList &windows,
    const QVariantList &workspaces, const QVariantList &outputs)
{
    QVariantList matching;
    for (const QVariant &window : windows) {
        const QVariantMap entry = window.toMap();
        Layout::MatchContext context;
        context.appId = entry.value(QStringLiteral("app_id")).toString();
        context.title = entry.value(QStringLiteral("title")).toString();
        context.isFocused = entry.value(QStringLiteral("is_focused")).toBool();
        context.isActive = context.isFocused;
        context.isFloating = entry.value(QStringLiteral("is_floating")).toBool();
        context.isUrgent = entry.value(QStringLiteral("is_urgent")).toBool();
        context.output = outputOfWorkspaceId(entry.value(QStringLiteral("workspace_id")), workspaces);
        context.monitorProfile = profileForOutput(config, context.output, outputs);
        if (Layout::ruleApplies(rule, context, false)) {
            matching.append(entry);
        }
    }
    return matching;
}

}
