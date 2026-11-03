#include "decorations/batterymonitor.h"

#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusPendingCallWatcher>
#include <QDBusPendingReply>
#include <QDBusServiceWatcher>
#include <QDBusVariant>

namespace Konveyor
{

namespace
{

constexpr QLatin1StringView kService("org.freedesktop.UPower");
constexpr QLatin1StringView kPath("/org/freedesktop/UPower");
constexpr QLatin1StringView kPropertiesInterface("org.freedesktop.DBus.Properties");
constexpr QLatin1StringView kProperty("OnBattery");

}

BatteryMonitor::BatteryMonitor(QObject *parent)
    : QObject(parent)
{
    QDBusConnection bus = QDBusConnection::systemBus();
    bus.connect(kService, kPath, kPropertiesInterface, QStringLiteral("PropertiesChanged"), this,
        SLOT(propertiesChanged(QString, QVariantMap, QStringList)));
    auto *watcher = new QDBusServiceWatcher(kService, bus, QDBusServiceWatcher::WatchForOwnerChange, this);
    connect(watcher, &QDBusServiceWatcher::serviceRegistered, this, &BatteryMonitor::read);
    connect(watcher, &QDBusServiceWatcher::serviceUnregistered, this, [this] { set(false); });
    read();
}

void BatteryMonitor::read()
{
    QDBusMessage message = QDBusMessage::createMethodCall(kService, kPath, kPropertiesInterface, QStringLiteral("Get"));
    message << QString(kService) << QString(kProperty);
    auto *call = new QDBusPendingCallWatcher(QDBusConnection::systemBus().asyncCall(message), this);
    connect(call, &QDBusPendingCallWatcher::finished, this, &BatteryMonitor::readFinished);
}

void BatteryMonitor::readFinished(QDBusPendingCallWatcher *call)
{
    const QDBusPendingReply<QDBusVariant> reply = *call;
    call->deleteLater();
    set(!reply.isError() && reply.value().variant().toBool());
}

void BatteryMonitor::propertiesChanged(const QString &interface, const QVariantMap &changed, const QStringList &invalidated)
{
    if (interface != kService) {
        return;
    }
    if (invalidated.contains(kProperty)) {
        read();
    } else if (const auto found = changed.constFind(QString(kProperty)); found != changed.constEnd()) {
        set(found->toBool());
    }
}

void BatteryMonitor::set(bool onBattery)
{
    if (m_onBattery != onBattery) {
        m_onBattery = onBattery;
        Q_EMIT onBatteryChanged(onBattery);
    }
}

}
