#include "audio/audiosignals.h"

#include <window.h>

#include <chrono>

namespace Konveyor::Audio
{

namespace
{

constexpr std::chrono::milliseconds kPumpInterval(16);

}

AudioSignals::AudioSignals(Render::SignalBus &bus, QObject *parent)
    : QObject(parent)
    , m_bus(bus)
{
    m_pump.setInterval(kPumpInterval);
    connect(&m_pump, &QTimer::timeout, this, &AudioSignals::pump);
    m_bus.onWantedChanged([this] { refresh(); });
}

void AudioSignals::setWindowLookup(const WindowLookup &lookup)
{
    m_windowOf = lookup;
}

void AudioSignals::setReduceMotion(bool reduceMotion)
{
    m_reduceMotion = reduceMotion;
    refresh();
}

void AudioSignals::setOnBattery(bool onBattery)
{
    m_onBattery = onBattery;
    refresh();
}

void AudioSignals::setBeatThreshold(float threshold)
{
    m_audio.setBeatThreshold(threshold);
}

std::map<quint64, WindowKey> AudioSignals::windowsToCapture() const
{
    std::map<quint64, WindowKey> windows;
    if (m_reduceMotion || m_onBattery || !m_windowOf || !Config::anyAudioSignal(m_bus.wantedAnywhere())) {
        return windows;
    }
    for (std::size_t index = 0; index < Config::kSignalCount; ++index) {
        const auto signal = static_cast<Config::Signal>(index);
        if (!Config::isAudioSignal(signal)) {
            continue;
        }
        for (const quint64 id : m_bus.windowsWanting(signal)) {
            const KWin::Window *window = m_windowOf(id);
            if (window && !window->isFullScreen()) {
                windows.insert_or_assign(id, WindowKey {window->pid(), window->desktopFileName()});
            }
        }
    }
    return windows;
}

void AudioSignals::refresh()
{
    std::map<quint64, WindowKey> next = windowsToCapture();
    for (const auto &[id, key] : m_captured) {
        if (!next.contains(id)) {
            m_bus.setAudio(id, {});
        }
    }
    m_captured = std::move(next);
    QList<WindowKey> keys;
    keys.reserve(static_cast<qsizetype>(m_captured.size()));
    for (const auto &[id, key] : m_captured) {
        keys.append(key);
    }
    m_audio.setWanted(keys);
    if (m_captured.empty()) {
        m_pump.stop();
    } else if (!m_pump.isActive()) {
        m_pump.start();
    }
}

void AudioSignals::pump()
{
    for (const auto &[id, key] : m_captured) {
        m_bus.setAudio(id, m_audio.frame(key));
    }
}

}
