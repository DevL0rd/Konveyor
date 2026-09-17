#include "values/livematching.h"

#include "layout/monitor/monitorprofiles.h"
#include "layout/rules/windowrules.h"

namespace Konveyor::Settings
{

namespace
{

QString profileForWorkspace(
    const Config::Config &config, const QVariant &workspaceId, const QVariantList &workspaces, const QVariantList &outputs)
{
    for (const QVariant &workspace : workspaces) {
        const QVariantMap entry = workspace.toMap();
        if (entry.value(QStringLiteral("id")) != workspaceId) {
            continue;
        }
        for (const QVariant &output : outputs) {
            if (output.toMap().value(QStringLiteral("name")) == entry.value(QStringLiteral("output"))) {
                return profileNameFor(config, output.toMap());
            }
        }
    }
    return QString();
}

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
        context.monitorProfile = profileForWorkspace(config, entry.value(QStringLiteral("workspace_id")), workspaces, outputs);
        if (Layout::ruleApplies(rule, context, false)) {
            matching.append(entry);
        }
    }
    return matching;
}

}
