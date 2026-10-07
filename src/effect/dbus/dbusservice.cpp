#include "dbus/dbusservice.h"

#include "ipc/dbusnames.h"

#include <QDBusConnection>
#include <QDBusError>
#include <QDBusServiceWatcher>
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
    if (tryRegister()) {
        return true;
    }
    qWarning() << "konveyor: could not register" << Ipc::dbusService
               << "yet, waiting for its current owner to let go:" << QDBusConnection::sessionBus().lastError().message();
    m_waitForName = std::make_unique<QDBusServiceWatcher>(
        Ipc::dbusService, QDBusConnection::sessionBus(), QDBusServiceWatcher::WatchForUnregistration);
    connect(m_waitForName.get(), &QDBusServiceWatcher::serviceUnregistered, this, [this] {
        if (tryRegister()) {
            m_waitForName.release()->deleteLater();
        }
    });
    return false;
}

bool DBusService::tryRegister()
{
    QDBusConnection bus = QDBusConnection::sessionBus();
    if (!bus.registerObject(Ipc::dbusPath, this, QDBusConnection::ExportScriptableContents)) {
        return false;
    }
    if (!bus.registerService(Ipc::dbusService)) {
        bus.unregisterObject(Ipc::dbusPath);
        return false;
    }
    m_registered = true;
    return true;
}

QString DBusService::compact(const QJsonDocument &document)
{
    return document.isNull() ? QStringLiteral("null") : QString::fromUtf8(document.toJson(QJsonDocument::Compact));
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

QString DBusService::Gestures() const
{
    return compact(m_handlers.gestures());
}

bool DBusService::MultiTouchActive() const
{
    return m_multiTouchActive;
}

void DBusService::setMultiTouchActive(bool active)
{
    if (active == m_multiTouchActive) {
        return;
    }
    m_multiTouchActive = active;
    Q_EMIT MultiTouchChanged(active);
}

bool DBusService::SuperHeld() const
{
    return m_superHeld;
}

void DBusService::setSuperHeld(bool held)
{
    if (held == m_superHeld) {
        return;
    }
    m_superHeld = held;
    Q_EMIT SuperHeldChanged(held);
}

void DBusService::announceLayoutChange()
{
    Q_EMIT LayoutChanged();
}

}
