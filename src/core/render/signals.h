#pragma once

#include "config/types/signal.h"
#include "render/audioframe.h"

#include <QPointF>
#include <QtGlobal>

#include <array>
#include <functional>
#include <map>
#include <vector>

namespace Konveyor::Render
{

using Config::kSignalCount;
using Config::kSignalSourceCount;
using Config::Signal;
using Config::SignalSet;
using Config::SignalSource;

struct SignalPoint
{
    QPointF position;
    QPointF direction;
    bool valid = false;
};

struct SourcePresence
{
    float level = 0.0F;
    double idleSeconds = 1e9;
};

struct SignalFrame
{
    std::array<float, kSignalCount> values {};
    std::array<quint32, kSignalCount> fired {};
    std::array<float, 16> bands {};
    std::array<float, 64> wave {};
    SignalPoint point;
    std::array<SourcePresence, kSignalSourceCount> presences {};

    float value(Signal signal) const { return values[static_cast<std::size_t>(signal)]; }
    quint32 fireCount(Signal signal) const { return fired[static_cast<std::size_t>(signal)]; }
    float presence(SignalSource source) const;
    void set(Signal signal, float amount);
    void hear(SignalSource source);
    void fire(Signal signal);
    void fire(Signal signal, const SignalPoint &at);
    void setAudio(const AudioFrame &frame);
    void decayImpulses(double step);
    void fadePresences(double step);
};

SignalFrame simulatedSignals(double seconds);

enum class SignalClient
{
    Borders,
    Events
};

class SignalBus
{
public:
    const SignalFrame &frame(quint64 window) const;

    void set(quint64 window, Signal signal, float amount);
    void fire(quint64 window, Signal signal, const SignalPoint &at = {});
    void setAudio(quint64 window, const AudioFrame &audio);
    void setOnEveryWindow(Signal signal, float amount);
    void advance(double seconds);

    void setWanted(SignalClient client, quint64 window, const SignalSet &wanted);
    void setWantedEverywhere(SignalClient client, const SignalSet &wanted);
    void clearWanted(SignalClient client);
    void remove(quint64 window);

    bool wanted(quint64 window, Signal signal) const;
    SignalSet wantedAnywhere() const;
    std::vector<quint64> windowsWanting(Signal signal) const;
    void onWantedChanged(std::function<void()> listener);
    void onSourceReturned(std::function<void(quint64)> listener);

private:
    using Claims = std::array<SignalSet, 2>;

    SignalFrame *frameFor(quint64 window, Signal signal);
    void wantedChanged();
    void noteReturn(quint64 window, const SignalFrame &frame, Signal signal, bool wasSilent);

    std::map<quint64, SignalFrame> m_frames;
    std::map<quint64, Claims> m_claims;
    Claims m_everywhere;
    SignalSet m_anywhere;
    std::vector<std::function<void()>> m_listeners;
    std::function<void(quint64)> m_sourceReturned;
    double m_lastSeconds = 0.0;
};

}
