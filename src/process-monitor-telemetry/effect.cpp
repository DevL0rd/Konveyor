#include "effect.h"

#include <effect/effecthandler.h>
#include <effect/effectwindow.h>

#include <QDBusConnection>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

namespace ProcessMonitor
{

namespace
{

constexpr auto Service = "org.devl0rd.ProcessMonitor.FrameTelemetry";
constexpr auto Path = "/FrameTelemetry";

}

TelemetryEffect::TelemetryEffect()
{
    m_clock.start();
    connect(KWin::effects, &KWin::EffectsHandler::windowAdded, this, &TelemetryEffect::track);
    connect(KWin::effects, &KWin::EffectsHandler::windowDeleted, this, [this](KWin::EffectWindow *window) { m_rates.remove(window); });
    for (KWin::EffectWindow *window : KWin::effects->stackingOrder()) {
        track(window);
    }
    QDBusConnection bus = QDBusConnection::sessionBus();
    const bool objectRegistered = bus.registerObject(QString::fromLatin1(Path), this, QDBusConnection::ExportScriptableSlots);
    m_registered = objectRegistered && bus.registerService(QString::fromLatin1(Service));
    if (!m_registered && objectRegistered) {
        bus.unregisterObject(QString::fromLatin1(Path));
    }
}

TelemetryEffect::~TelemetryEffect()
{
    if (!m_registered) {
        return;
    }
    QDBusConnection bus = QDBusConnection::sessionBus();
    bus.unregisterObject(QString::fromLatin1(Path));
    bus.unregisterService(QString::fromLatin1(Service));
}

bool TelemetryEffect::isActive() const
{
    return false;
}

QString TelemetryEffect::Frames() const
{
    QJsonArray frames;
    const qint64 now = m_clock.nsecsElapsed();
    for (auto it = m_rates.constBegin(); it != m_rates.constEnd(); ++it) {
        const std::optional<FrameRate> rate = it->rateAt(now);
        if (!rate || it.key()->pid() <= 0) {
            continue;
        }
        frames.append(QJsonObject {{QStringLiteral("pid"), static_cast<qint64>(it.key()->pid())}, {QStringLiteral("fps"), rate->fps},
            {QStringLiteral("frametime"), rate->frametime}, {QStringLiteral("fps_low"), rate->fpsLow}});
    }
    return QString::fromUtf8(QJsonDocument(frames).toJson(QJsonDocument::Compact));
}

void TelemetryEffect::track(KWin::EffectWindow *window)
{
    if (!window || m_rates.contains(window)) {
        return;
    }
    m_rates.insert(window, {});
    connect(window, &KWin::EffectWindow::windowDamaged, this, [this, window]() { m_rates[window].addFrame(m_clock.nsecsElapsed()); });
}

}
