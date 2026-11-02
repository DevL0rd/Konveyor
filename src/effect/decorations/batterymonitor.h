#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QVariantMap>

class QDBusPendingCallWatcher;

namespace Konveyor
{

class BatteryMonitor : public QObject
{
    Q_OBJECT

public:
    explicit BatteryMonitor(QObject *parent = nullptr);

Q_SIGNALS:
    void onBatteryChanged(bool onBattery);

private Q_SLOTS:
    void propertiesChanged(const QString &interface, const QVariantMap &changed, const QStringList &invalidated);

private:
    void read();
    void readFinished(QDBusPendingCallWatcher *call);
    void set(bool onBattery);

    bool m_onBattery = false;
};

}
