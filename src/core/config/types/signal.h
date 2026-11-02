#pragma once

#include <QString>
#include <QStringList>

#include <bitset>
#include <cstddef>
#include <optional>

namespace Konveyor::Config
{

enum class Signal
{
    AudioLevel,
    AudioBass,
    AudioMid,
    AudioTreble,
    ScrollSpeed,
    ScrollDistance,
    ScrollDirection,
    PointerSpeed,
    TypingRate,
    ViewSpeed,
    ViewDistance,
    ViewDirection,
    Time,
    AudioBeat,
    Click,
    ScrollStep,
    Keypress,
    ViewStep,
    Focus,
    Unfocus,
    Urgent,
    Count
};

enum class SignalSource
{
    Audio,
    Scroll,
    View,
    Pointer,
    None
};

inline constexpr std::size_t kSignalCount = static_cast<std::size_t>(Signal::Count);
inline constexpr std::size_t kSignalSourceCount = static_cast<std::size_t>(SignalSource::None);
using SignalSet = std::bitset<kSignalCount>;

QString signalName(Signal signal);
std::optional<Signal> signalNamed(const QString &name);
QStringList signalNames();
bool isImpulse(Signal signal);
bool isAudioSignal(Signal signal);
bool isInputSignal(Signal signal);
bool isViewSignal(Signal signal);
SignalSource sourceOf(Signal signal);
bool anyAudioSignal(const SignalSet &set);
bool anyInputSignal(const SignalSet &set);

}
