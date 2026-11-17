#include "effect.h"

#include <effect/effecthandler.h>
#include <effect/effectwindow.h>

#include <QDBusConnection>
#include <QDBusMessage>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>

namespace ProcessMonitor
{

namespace
{

constexpr auto Service = "org.devl0rd.ProcessMonitor.FrameTelemetry";
constexpr auto Path = "/FrameTelemetry";
constexpr auto FrameMember = "FrameBatch";
constexpr int FlushIntervalMs = 200;

}

TelemetryEffect::TelemetryEffect()
{
    m_clock.start();
    m_flushTimer.setInterval(FlushIntervalMs);
    connect(&m_flushTimer, &QTimer::timeout, this, &TelemetryEffect::flush);
    connect(KWin::effects, &KWin::EffectsHandler::windowAdded, this, &TelemetryEffect::track);
    connect(KWin::effects, &KWin::EffectsHandler::windowDeleted, this, [this](KWin::EffectWindow *window) { m_rates.remove(window); });
    for (KWin::EffectWindow *window : KWin::effects->stackingOrder()) {
        track(window);
    }
    QDBusConnection bus = QDBusConnection::sessionBus();
    const bool objectRegistered = bus.registerObject(QString::fromLatin1(Path), this, QDBusConnection::ExportScriptableSlots);
    m_subscriberWatcher = new QDBusServiceWatcher(this);
    m_subscriberWatcher->setConnection(bus);
    m_subscriberWatcher->setWatchMode(QDBusServiceWatcher::WatchForUnregistration);
    connect(m_subscriberWatcher, &QDBusServiceWatcher::serviceUnregistered, this, [this](const QString &service) {
        m_subscribers.remove(service);
        m_subscriberWatcher->removeWatchedService(service);
        if (m_subscribers.isEmpty()) {
            m_flushTimer.stop();
        }
    });
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

void TelemetryEffect::Watch(const QString &pidsJson)
{
    QJsonParseError error;
    const QJsonDocument document = QJsonDocument::fromJson(pidsJson.toUtf8(), &error);
    const QJsonArray pids = document.array();
    const bool valid = error.error == QJsonParseError::NoError && document.isArray()
        && std::ranges::all_of(pids, [](const QJsonValue &value) { return value.isDouble() && value.toInteger() > 0; });
    if (!valid) {
        sendErrorReply(QDBusError::InvalidArgs, QStringLiteral("Watch expects a JSON array of process ids, got: %1").arg(pidsJson));
        return;
    }
    const QString subscriber = message().service();
    QSet<qint64> watched;
    for (const auto value : pids) {
        watched.insert(value.toInteger());
    }
    if (watched.isEmpty()) {
        if (m_subscribers.remove(subscriber)) {
            m_subscriberWatcher->removeWatchedService(subscriber);
        }
        if (m_subscribers.isEmpty()) {
            m_flushTimer.stop();
        }
        return;
    }
    if (!m_subscribers.contains(subscriber)) {
        m_subscriberWatcher->addWatchedService(subscriber);
    }
    Subscriber &entry = m_subscribers[subscriber];
    entry.pids = std::move(watched);
    for (auto it = entry.pending.begin(); it != entry.pending.end();) {
        it = entry.pids.contains(it.key()) ? std::next(it) : entry.pending.erase(it);
    }
    if (!m_flushTimer.isActive()) {
        m_flushTimer.start();
    }
}

void TelemetryEffect::recordFrame(qint64 pid, double frametime)
{
    const qint64 number = ++m_frameNumber;
    for (Subscriber &subscriber : m_subscribers) {
        if (subscriber.pids.contains(pid)) {
            subscriber.pending[pid].append({number, frametime});
        }
    }
}

void TelemetryEffect::flush()
{
    QDBusConnection bus = QDBusConnection::sessionBus();
    for (auto it = m_subscribers.begin(); it != m_subscribers.end(); ++it) {
        if (it->pending.isEmpty()) {
            continue;
        }
        QJsonArray batch;
        for (auto pending = it->pending.constBegin(); pending != it->pending.constEnd(); ++pending) {
            QJsonArray frames;
            for (const auto &[number, frametime] : pending.value()) {
                frames.append(QJsonArray {number, frametime});
            }
            batch.append(QJsonObject {{QStringLiteral("pid"), pending.key()}, {QStringLiteral("frames"), frames}});
        }
        it->pending.clear();
        QDBusMessage signal = QDBusMessage::createTargetedSignal(
            it.key(), QString::fromLatin1(Path), QString::fromLatin1(Service), QString::fromLatin1(FrameMember));
        signal << QString::fromUtf8(QJsonDocument(batch).toJson(QJsonDocument::Compact));
        bus.send(signal);
    }
}

void TelemetryEffect::track(KWin::EffectWindow *window)
{
    if (!window || m_rates.contains(window)) {
        return;
    }
    m_rates.insert(window, {});
    connect(window, &KWin::EffectWindow::windowDamaged, this, [this, window] {
        const qint64 pid = window->pid();
        const std::optional<double> frametime = m_rates[window].addFrame(m_clock.nsecsElapsed());
        if (frametime && !m_subscribers.isEmpty()) {
            recordFrame(pid, *frametime);
        }
    });
}

}
