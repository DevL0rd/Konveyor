#pragma once

#include "framerate.h"

#include <effect/effect.h>

#include <QDBusContext>
#include <QDBusServiceWatcher>
#include <QElapsedTimer>
#include <QHash>
#include <QSet>
#include <QTimer>
#include <QVector>

namespace KWin
{
class EffectWindow;
}

namespace ProcessMonitor
{

class TelemetryEffect : public KWin::Effect, protected QDBusContext
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.devl0rd.ProcessMonitor.FrameTelemetry")

public:
    TelemetryEffect();
    ~TelemetryEffect() override;

    bool isActive() const override;

public Q_SLOTS:
    Q_SCRIPTABLE QString Frames() const;
    Q_SCRIPTABLE void Watch(const QString &pidsJson);

private:
    struct Subscriber
    {
        QSet<qint64> pids;
        QHash<qint64, QVector<QPair<qint64, double>>> pending;
    };

    void recordFrame(qint64 pid, double frametime);
    void flush();
    void track(KWin::EffectWindow *window);

    QElapsedTimer m_clock;
    QHash<KWin::EffectWindow *, FrameRateTracker> m_rates;
    QHash<QString, Subscriber> m_subscribers;
    QTimer m_flushTimer;
    qint64 m_frameNumber = 0;
    QDBusServiceWatcher *m_subscriberWatcher = nullptr;
    bool m_registered = false;
};

}
