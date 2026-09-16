#include "dbus/dbusservice.h"

#include "ipc/dbusnames.h"

#include <QDBusConnection>
#include <QDBusError>
#include <QJsonObject>

namespace Konveyor
{

DBusService::DBusService(DBusHandlers handlers, QObject *parent)
    : QObject(parent)
    , m_handlers(std::move(handlers))
{ }

DBusService::~DBusService()
{
    if (!m_registered) {
        return;
    }
    QDBusConnection bus = QDBusConnection::sessionBus();
    bus.unregisterObject(Ipc::dbusPath);
    bus.unregisterService(Ipc::dbusService);
}

bool DBusService::registerService()
{
    QDBusConnection bus = QDBusConnection::sessionBus();
    const bool objectRegistered = bus.registerObject(Ipc::dbusPath, this, QDBusConnection::ExportScriptableContents);
    m_registered = objectRegistered && bus.registerService(Ipc::dbusService);
    if (!m_registered) {
        qWarning() << "konveyor: failed to register D-Bus service" << Ipc::dbusService << bus.lastError().message();
    }
    return m_registered;
}

QString DBusService::compact(const QJsonDocument &document)
{
    return QString::fromUtf8(document.toJson(QJsonDocument::Compact));
}

QString DBusService::Version() const
{
    return compact(QJsonDocument(QJsonObject {{QStringLiteral("version"), QStringLiteral(KONVEYOR_VERSION)}}));
}

QString DBusService::Windows() const
{
    return compact(m_handlers.windows());
}

QString DBusService::Workspaces() const
{
    return compact(m_handlers.workspaces());
}

QString DBusService::Outputs() const
{
    return compact(m_handlers.outputs());
}

QString DBusService::FocusedWindow() const
{
    return compact(m_handlers.focusedWindow());
}

QString DBusService::FocusedOutput() const
{
    return compact(m_handlers.focusedOutput());
}

QString DBusService::Binds() const
{
    return compact(m_handlers.binds());
}

QString DBusService::OverviewState() const
{
    return compact(QJsonDocument(QJsonObject {{QStringLiteral("is_open"), m_handlers.overviewOpen()}}));
}

QString DBusService::Action(const QString &json)
{
    return m_handlers.action(json);
}

QString DBusService::LoadConfigFile(const QString &path)
{
    return m_handlers.loadConfigFile(path);
}

QString DBusService::LastBind() const
{
    return compact(m_handlers.lastBind());
}

}
