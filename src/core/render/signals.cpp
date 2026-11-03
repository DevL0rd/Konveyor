#include "render/signals.h"

#include <algorithm>
#include <cmath>
#include <numbers>

namespace Konveyor::Render
{

namespace
{

constexpr double kImpulseSeconds = 0.25;
constexpr double kTimeCycle = 8.0;
constexpr double kLongestStep = 0.25;
constexpr float kBeatRise = 0.1F;
constexpr float kHeardLevel = 0.01F;
constexpr float kHeardRise = 0.0001F;
constexpr double kAudioHold = 2.0;
constexpr double kAudioFade = 1.0;
constexpr double kInputHold = 1.0;
constexpr double kInputFade = 0.5;
constexpr double kTau = 2.0 * std::numbers::pi;

double cycle(double seconds, double period)
{
    const double turns = seconds / period;
    return turns - std::floor(turns);
}

float wobble(double seconds, double period)
{
    return static_cast<float>(0.5 + 0.5 * std::sin(kTau * seconds / period));
}

void simulateImpulse(SignalFrame &frame, Signal signal, double seconds, double period)
{
    const auto index = static_cast<std::size_t>(signal);
    frame.fired[index] = static_cast<quint32>(std::floor(seconds / period));
    frame.values[index] = static_cast<float>(std::exp(-cycle(seconds, period) * period / kImpulseSeconds));
}

}

float SignalFrame::presence(SignalSource source) const
{
    return source == SignalSource::None ? 1.0F : presences[static_cast<std::size_t>(source)].level;
}

void SignalFrame::hear(SignalSource source)
{
    if (source != SignalSource::None) {
        presences[static_cast<std::size_t>(source)] = {1.0F, 0.0};
    }
}

void SignalFrame::set(Signal signal, float amount)
{
    float &held = values[static_cast<std::size_t>(signal)];
    const SignalSource source = Config::sourceOf(signal);
    if (source != SignalSource::Audio && std::abs(amount) > std::abs(held) + kHeardRise) {
        hear(source);
    }
    held = amount;
}

void SignalFrame::fire(Signal signal)
{
    const auto index = static_cast<std::size_t>(signal);
    values[index] = 1.0F;
    ++fired[index];
    hear(Config::sourceOf(signal));
}

void SignalFrame::fire(Signal signal, const SignalPoint &at)
{
    fire(signal);
    if (at.valid) {
        point = at;
    }
}

void SignalFrame::setAudio(const AudioFrame &frame)
{
    set(Signal::AudioLevel, frame.level);
    set(Signal::AudioBass, frame.bass);
    set(Signal::AudioMid, frame.mid);
    set(Signal::AudioTreble, frame.treble);
    bands = frame.bands;
    wave = frame.wave;
    if (frame.beat > value(Signal::AudioBeat) + kBeatRise) {
        fire(Signal::AudioBeat);
    }
    set(Signal::AudioBeat, std::max(frame.beat, value(Signal::AudioBeat)));
    if (frame.level > kHeardLevel) {
        hear(SignalSource::Audio);
    }
}

void SignalFrame::decayImpulses(double step)
{
    const auto fade = static_cast<float>(std::exp(-step / kImpulseSeconds));
    for (std::size_t index = static_cast<std::size_t>(Signal::AudioBeat); index < kSignalCount; ++index) {
        values[index] *= fade;
    }
}

void SignalFrame::fadePresences(double step)
{
    for (std::size_t index = 0; index < presences.size(); ++index) {
        const bool audio = index == static_cast<std::size_t>(SignalSource::Audio);
        SourcePresence &presence = presences[index];
        presence.idleSeconds += step;
        const double faded = (presence.idleSeconds - (audio ? kAudioHold : kInputHold)) / (audio ? kAudioFade : kInputFade);
        presence.level = static_cast<float>(std::clamp(1.0 - faded, 0.0, 1.0));
    }
}

SignalFrame simulatedSignals(double seconds)
{
    SignalFrame frame;
    frame.setAudio(simulatedAudioFrame(seconds));
    simulateImpulse(frame, Signal::AudioBeat, seconds, 0.5);
    simulateImpulse(frame, Signal::Click, seconds, 2.4);
    simulateImpulse(frame, Signal::ScrollStep, seconds, 0.9);
    simulateImpulse(frame, Signal::Keypress, seconds, 0.7);
    simulateImpulse(frame, Signal::ViewStep, seconds, 3.2);
    simulateImpulse(frame, Signal::Focus, seconds, 4.0);
    simulateImpulse(frame, Signal::Unfocus, seconds, 4.0);
    simulateImpulse(frame, Signal::Urgent, seconds, 6.0);
    const float scroll = std::pow(wobble(seconds, 3.0), 2.0F);
    const float view = std::pow(wobble(seconds, 5.0), 3.0F);
    frame.set(Signal::ScrollSpeed, scroll);
    frame.set(Signal::ScrollDistance, static_cast<float>(cycle(seconds, 6.0)));
    frame.set(Signal::ScrollDirection, std::sin(static_cast<float>(kTau * seconds / 6.0)) > 0.0F ? 1.0F : -1.0F);
    frame.set(Signal::PointerSpeed, wobble(seconds, 2.0));
    frame.set(Signal::TypingRate, wobble(seconds, 4.0));
    frame.set(Signal::ViewSpeed, view);
    frame.set(Signal::ViewDistance, static_cast<float>(cycle(seconds, 10.0)));
    frame.set(Signal::ViewDirection, std::sin(static_cast<float>(kTau * seconds / 10.0)) > 0.0F ? 1.0F : -1.0F);
    frame.set(Signal::Time, static_cast<float>(cycle(seconds, kTimeCycle)));
    frame.presences.fill({1.0F, 0.0});
    return frame;
}

const SignalFrame &SignalBus::frame(quint64 window) const
{
    static const SignalFrame silent;
    const auto found = m_frames.find(window);
    return found == m_frames.end() ? silent : found->second;
}

SignalFrame *SignalBus::frameFor(quint64 window, Signal signal)
{
    return wanted(window, signal) ? &m_frames[window] : nullptr;
}

void SignalBus::set(quint64 window, Signal signal, float amount)
{
    if (SignalFrame *target = frameFor(window, signal)) {
        const bool wasSilent = target->presence(Config::sourceOf(signal)) <= 0.0F;
        target->set(signal, amount);
        noteReturn(window, *target, signal, wasSilent);
    }
}

void SignalBus::fire(quint64 window, Signal signal, const SignalPoint &at)
{
    if (SignalFrame *target = frameFor(window, signal)) {
        const bool wasSilent = target->presence(Config::sourceOf(signal)) <= 0.0F;
        target->fire(signal, at);
        noteReturn(window, *target, signal, wasSilent);
    }
}

void SignalBus::setAudio(quint64 window, const AudioFrame &audio)
{
    if (SignalFrame *target = frameFor(window, Signal::AudioLevel)) {
        const bool wasSilent = target->presence(SignalSource::Audio) <= 0.0F;
        target->setAudio(audio);
        noteReturn(window, *target, Signal::AudioLevel, wasSilent);
    }
}

void SignalBus::setOnEveryWindow(Signal signal, float amount)
{
    for (quint64 window : windowsWanting(signal)) {
        set(window, signal, amount);
    }
}

void SignalBus::advance(double seconds)
{
    const double step = std::clamp(seconds - m_lastSeconds, 0.0, kLongestStep);
    m_lastSeconds = seconds;
    for (auto &[window, frame] : m_frames) {
        frame.decayImpulses(step);
        frame.fadePresences(step);
        frame.set(Signal::Time, static_cast<float>(cycle(seconds, kTimeCycle)));
    }
}

void SignalBus::setWanted(SignalClient client, quint64 window, const SignalSet &wanted)
{
    const auto index = static_cast<std::size_t>(client);
    if (wanted.none() && !m_claims.contains(window)) {
        return;
    }
    Claims &claims = m_claims[window];
    if (claims[index] == wanted) {
        return;
    }
    claims[index] = wanted;
    if (claims[0].none() && claims[1].none()) {
        m_claims.erase(window);
    }
    wantedChanged();
}

void SignalBus::setWantedEverywhere(SignalClient client, const SignalSet &wanted)
{
    SignalSet &claim = m_everywhere[static_cast<std::size_t>(client)];
    if (claim != wanted) {
        claim = wanted;
        wantedChanged();
    }
}

void SignalBus::clearWanted(SignalClient client)
{
    const auto index = static_cast<std::size_t>(client);
    for (auto &[window, claims] : m_claims) {
        claims[index].reset();
    }
    std::erase_if(m_claims, [](const auto &entry) { return entry.second[0].none() && entry.second[1].none(); });
    m_everywhere[index].reset();
    wantedChanged();
}

void SignalBus::remove(quint64 window)
{
    m_frames.erase(window);
    if (m_claims.erase(window) > 0) {
        wantedChanged();
    }
}

bool SignalBus::wanted(quint64 window, Signal signal) const
{
    const auto index = static_cast<std::size_t>(signal);
    if (m_everywhere[0].test(index) || m_everywhere[1].test(index)) {
        return true;
    }
    const auto found = m_claims.find(window);
    return found != m_claims.end() && (found->second[0].test(index) || found->second[1].test(index));
}

SignalSet SignalBus::wantedAnywhere() const
{
    return m_anywhere;
}

std::vector<quint64> SignalBus::windowsWanting(Signal signal) const
{
    std::vector<quint64> windows;
    const auto index = static_cast<std::size_t>(signal);
    for (const auto &[window, claims] : m_claims) {
        if (claims[0].test(index) || claims[1].test(index)) {
            windows.push_back(window);
        }
    }
    return windows;
}

void SignalBus::onWantedChanged(std::function<void()> listener)
{
    m_listeners.push_back(std::move(listener));
}

void SignalBus::onSourceReturned(std::function<void(quint64)> listener)
{
    m_sourceReturned = std::move(listener);
}

void SignalBus::noteReturn(quint64 window, const SignalFrame &frame, Signal signal, bool wasSilent)
{
    if (wasSilent && frame.presence(Config::sourceOf(signal)) > 0.0F && m_sourceReturned) {
        m_sourceReturned(window);
    }
}

void SignalBus::wantedChanged()
{
    m_anywhere = m_everywhere[0] | m_everywhere[1];
    for (const auto &[window, claims] : m_claims) {
        m_anywhere |= claims[0] | claims[1];
    }
    for (auto it = m_frames.begin(); it != m_frames.end();) {
        it = m_claims.contains(it->first) || m_everywhere[0].any() || m_everywhere[1].any() ? std::next(it) : m_frames.erase(it);
    }
    for (const std::function<void()> &listener : m_listeners) {
        listener();
    }
}

}
