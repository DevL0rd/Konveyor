#pragma once

#include "audio/windowaudio.h"
#include "render/signals.h"

#include <QObject>
#include <QTimer>

#include <functional>
#include <map>

namespace KWin
{
class Window;
}

namespace Konveyor::Audio
{

class AudioSignals : public QObject
{
    Q_OBJECT

public:
    using WindowLookup = std::function<KWin::Window *(quint64)>;

    explicit AudioSignals(Render::SignalBus &bus, QObject *parent = nullptr);

    void setWindowLookup(const WindowLookup &lookup);
    void setReduceMotion(bool reduceMotion);
    void setOnBattery(bool onBattery);
    void setBeatThreshold(float threshold);
    void refresh();

private:
    void pump();
    std::map<quint64, WindowKey> windowsToCapture() const;

    Render::SignalBus &m_bus;
    WindowAudio m_audio;
    WindowLookup m_windowOf;
    std::map<quint64, WindowKey> m_captured;
    QTimer m_pump;
    bool m_reduceMotion = false;
    bool m_onBattery = false;
};

}
