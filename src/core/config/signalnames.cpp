#include "config/types/signal.h"

#include <array>

namespace Konveyor::Config
{

namespace
{

constexpr std::array<const char *, kSignalCount> kNames {"audio.level", "audio.bass", "audio.mid", "audio.treble", "scroll.speed",
    "scroll.distance", "scroll.direction", "pointer.speed", "typing.rate", "view.speed", "view.distance", "view.direction", "time",
    "audio.beat", "click", "scroll.step", "keypress", "view.step", "focus", "unfocus", "urgent"};

template<typename Test> bool anySignal(const SignalSet &set, Test test)
{
    for (std::size_t index = 0; index < kSignalCount; ++index) {
        if (set.test(index) && test(static_cast<Signal>(index))) {
            return true;
        }
    }
    return false;
}

}

QString signalName(Signal signal)
{
    return QLatin1String(kNames[static_cast<std::size_t>(signal)]);
}

std::optional<Signal> signalNamed(const QString &name)
{
    for (std::size_t index = 0; index < kSignalCount; ++index) {
        if (name == QLatin1String(kNames[index])) {
            return static_cast<Signal>(index);
        }
    }
    return std::nullopt;
}

QStringList signalNames()
{
    QStringList names;
    names.reserve(kSignalCount);
    for (const char *name : kNames) {
        names.append(QLatin1String(name));
    }
    return names;
}

bool isImpulse(Signal signal)
{
    return signal >= Signal::AudioBeat;
}

bool isAudioSignal(Signal signal)
{
    return signal <= Signal::AudioTreble || signal == Signal::AudioBeat;
}

bool isInputSignal(Signal signal)
{
    return (signal >= Signal::ScrollSpeed && signal <= Signal::TypingRate) || signal == Signal::Click || signal == Signal::ScrollStep
        || signal == Signal::Keypress;
}

bool isViewSignal(Signal signal)
{
    return (signal >= Signal::ViewSpeed && signal <= Signal::ViewDirection) || signal == Signal::ViewStep;
}

SignalSource sourceOf(Signal signal)
{
    switch (signal) {
    case Signal::AudioLevel:
    case Signal::AudioBass:
    case Signal::AudioMid:
    case Signal::AudioTreble:
    case Signal::AudioBeat:
        return SignalSource::Audio;
    case Signal::ScrollSpeed:
    case Signal::ScrollDistance:
    case Signal::ScrollDirection:
    case Signal::ScrollStep:
        return SignalSource::Scroll;
    case Signal::ViewSpeed:
    case Signal::ViewDistance:
    case Signal::ViewDirection:
    case Signal::ViewStep:
        return SignalSource::View;
    case Signal::PointerSpeed:
        return SignalSource::Pointer;
    default:
        return SignalSource::None;
    }
}

bool anyAudioSignal(const SignalSet &set)
{
    return anySignal(set, isAudioSignal);
}

bool anyInputSignal(const SignalSet &set)
{
    return anySignal(set, isInputSignal);
}

}
