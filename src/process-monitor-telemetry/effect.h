#pragma once

#include "framerate.h"

#include <effect/effect.h>

#include <QElapsedTimer>
#include <QHash>
#include <QSet>

namespace KWin
{
class EffectWindow;
}

namespace ProcessMonitor
{

class TelemetryEffect : public KWin::Effect
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

Q_SIGNALS:
    Q_SCRIPTABLE void Frame(qint64 pid, double frametime);

private:
    void track(KWin::EffectWindow *window);

    QElapsedTimer m_clock;
    QHash<KWin::EffectWindow *, FrameRateTracker> m_rates;
    QSet<qint64> m_watchedPids;
    bool m_registered = false;
};

}
