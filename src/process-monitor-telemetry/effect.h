#pragma once

#include "framerate.h"

#include <effect/effect.h>

#include <QElapsedTimer>
#include <QHash>

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

private:
    void track(KWin::EffectWindow *window);

    QElapsedTimer m_clock;
    QHash<KWin::EffectWindow *, FrameRateTracker> m_rates;
    bool m_registered = false;
};

}
