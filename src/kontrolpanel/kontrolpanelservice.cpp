#include "kontrolpanelservice.h"

#include <KWindowEffects>

#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <QWindow>

namespace Konveyor
{

KontrolPanelService::KontrolPanelService(QObject *parent)
    : QObject(parent)
{ }

bool KontrolPanelService::registerOn(QDBusConnection bus)
{
    return bus.registerObject(QLatin1String(objectPath), this, QDBusConnection::ExportScriptableSlots)
        && bus.registerService(QLatin1String(serviceName));
}

void KontrolPanelService::setOpen(bool open)
{
    if (m_open == open) {
        return;
    }
    m_open = open;
    Q_EMIT openChanged();
}

void KontrolPanelService::applyBackgroundEffects(QWindow *window, const QRegion &region)
{
    if (!window) {
        return;
    }
    KWindowEffects::enableBlurBehind(window, m_theme.blurBehindEnabled(), region);
    KWindowEffects::enableBackgroundContrast(window, m_theme.backgroundContrastEnabled(), m_theme.backgroundContrast(),
        m_theme.backgroundIntensity(), m_theme.backgroundSaturation(), region);
}

bool KontrolPanelService::konveyorRunning() const
{
    const QDBusConnectionInterface *bus = QDBusConnection::sessionBus().interface();
    return bus && bus->isServiceRegistered(QStringLiteral("org.kde.Konveyor"));
}

void KontrolPanelService::Toggle()
{
    Q_EMIT toggleRequested();
}

void KontrolPanelService::Open(const QString &page)
{
    Q_EMIT openRequested(page);
}

void KontrolPanelService::Hide()
{
    Q_EMIT hideRequested();
}

void KontrolPanelService::Pin(const QStringList &files)
{
    Q_EMIT pinRequested(files);
}

void KontrolPanelService::Configure()
{
    Q_EMIT configureRequested();
}

bool KontrolPanelService::IsOpen() const
{
    return m_open;
}

}
