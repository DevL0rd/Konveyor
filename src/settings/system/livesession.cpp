#include "system/livesession.h"

#include "ipc/dbusnames.h"

#include <KApplicationTrader>
#include <KService>

#include <QDBusInterface>
#include <QDBusReply>
#include <QJsonArray>
#include <QJsonDocument>

#include <algorithm>

namespace Konveyor::Settings
{

namespace
{

constexpr int CallTimeoutMs = 2000;

std::optional<QVariantList> callJson(QDBusInterface &interface, const QString &method)
{
    const QDBusReply<QString> reply = interface.call(method);
    if (!reply.isValid()) {
        return std::nullopt;
    }
    return QJsonDocument::fromJson(reply.value().toUtf8()).array().toVariantList();
}

KService::Ptr serviceFor(const QString &appId)
{
    if (KService::Ptr service = KService::serviceByDesktopName(appId)) {
        return service;
    }
    return KService::serviceByDesktopName(appId.toLower());
}

}

LiveSession::LiveSession(QObject *parent)
    : QObject(parent)
{
    refresh();
}

bool LiveSession::running() const
{
    return m_running;
}

QVariantList LiveSession::windows() const
{
    return m_windows;
}

QVariantList LiveSession::outputs() const
{
    return m_outputs;
}

QVariantList LiveSession::workspaces() const
{
    return m_workspaces;
}

QVariantList LiveSession::applications() const
{
    if (!m_applications.isEmpty()) {
        return m_applications;
    }
    KService::List services = KApplicationTrader::query([](const KService::Ptr &service) { return !service->noDisplay(); });
    std::ranges::sort(services, [](const KService::Ptr &a, const KService::Ptr &b) { return a->name().localeAwareCompare(b->name()) < 0; });
    for (const KService::Ptr &service : services) {
        m_applications.append(QVariantMap {
            {QStringLiteral("name"), service->name()},
            {QStringLiteral("appId"), service->desktopEntryName()},
            {QStringLiteral("icon"), service->icon()},
            {QStringLiteral("exec"), service->exec()},
        });
    }
    return m_applications;
}

void LiveSession::refresh()
{
    QDBusInterface interface(Ipc::dbusService, Ipc::dbusPath, Ipc::dbusInterface);
    interface.setTimeout(CallTimeoutMs);
    const auto windows = callJson(interface, QStringLiteral("Windows"));
    const auto outputs = callJson(interface, QStringLiteral("Outputs"));
    const auto workspaces = callJson(interface, QStringLiteral("Workspaces"));
    m_running = windows && outputs && workspaces;
    m_windows = windows.value_or(QVariantList());
    m_outputs = outputs.value_or(QVariantList());
    m_workspaces = workspaces.value_or(QVariantList());
    Q_EMIT changed();
}

QString LiveSession::iconFor(const QString &appId) const
{
    const KService::Ptr service = serviceFor(appId);
    return service && !service->icon().isEmpty() ? service->icon() : appId;
}

QString LiveSession::nameFor(const QString &appId) const
{
    const KService::Ptr service = serviceFor(appId);
    return service ? service->name() : appId;
}

}
