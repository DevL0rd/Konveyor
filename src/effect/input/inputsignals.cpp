#include "input/inputsignals.h"

#include <effect/effecthandler.h>
#include <input.h>
#include <input_event.h>
#include <input_event_spy.h>
#include <pointer_input.h>
#include <wayland_server.h>
#include <window.h>
#include <workspace.h>

#include <QPointer>

#include <cmath>
#include <ranges>

namespace Konveyor
{

namespace
{

using Render::Signal;
using Render::SignalSet;

constexpr int kTickMs = 16;

constexpr SignalSet setOf(std::initializer_list<Signal> list)
{
    SignalSet set;
    for (Signal signal : list) {
        set.set(static_cast<std::size_t>(signal));
    }
    return set;
}

constexpr SignalSet kPointerSignals = setOf({Signal::PointerSpeed});
constexpr SignalSet kClickSignals = setOf({Signal::Click});
constexpr SignalSet kScrollSignals = setOf({Signal::ScrollSpeed, Signal::ScrollDistance, Signal::ScrollDirection, Signal::ScrollStep});
constexpr SignalSet kKeySignals = setOf({Signal::Keypress, Signal::TypingRate});

bool pointerReachesWindows()
{
    return !KWin::waylandServer()->isScreenLocked() && !(KWin::effects && KWin::effects->isMouseInterception());
}

bool keysReachWindows()
{
    return !KWin::waylandServer()->isScreenLocked() && !(KWin::effects && KWin::effects->hasKeyboardGrab());
}

QPointF scrollDirection(const KWin::PointerAxisEvent &event)
{
    const double sign = event.delta < 0.0 ? -1.0 : 1.0;
    return event.orientation == Qt::Horizontal ? QPointF(sign, 0.0) : QPointF(0.0, sign);
}

void writeReadings(Render::SignalBus &bus, quint64 window, const InputReadings &readings)
{
    bus.set(window, Signal::ScrollSpeed, readings.scrollSpeed);
    bus.set(window, Signal::ScrollDistance, readings.scrollDistance);
    bus.set(window, Signal::ScrollDirection, readings.scrollDirection);
    bus.set(window, Signal::PointerSpeed, readings.pointerSpeed);
    bus.set(window, Signal::TypingRate, readings.typingRate);
}

}

class InputSignals::Spy : public KWin::InputEventSpy
{
public:
    explicit Spy(InputSignals &owner)
        : m_owner(owner)
    {
        KWin::input()->installInputEventSpy(this);
    }

    void pointerMotion(KWin::PointerMotionEvent *event) override { m_owner.pointerMoved(*event); }
    void pointerButton(KWin::PointerButtonEvent *event) override { m_owner.buttonChanged(*event); }
    void pointerAxis(KWin::PointerAxisEvent *event) override { m_owner.scrolled(*event); }
    void keyboardKey(KWin::KeyboardKeyEvent *event) override { m_owner.keyChanged(*event); }

private:
    InputSignals &m_owner;
};

InputSignals::InputSignals(Render::SignalBus &bus, Handlers handlers, QObject *parent)
    : QObject(parent)
    , m_bus(bus)
    , m_handlers(std::move(handlers))
{
    m_ticker.setInterval(kTickMs);
    connect(&m_ticker, &QTimer::timeout, this, &InputSignals::tick);
    m_bus.onWantedChanged([self = QPointer(this)] {
        if (self) {
            self->refresh();
        }
    });
    refresh();
}

InputSignals::~InputSignals() = default;

void InputSignals::setReduceMotion(bool reduceMotion)
{
    m_reduceMotion = reduceMotion;
    refresh();
}

void InputSignals::setOnBattery(bool onBattery)
{
    m_onBattery = onBattery;
    refresh();
}

void InputSignals::setFullscreen(bool fullscreen)
{
    m_fullscreen = fullscreen;
    refresh();
}

void InputSignals::forget(quint64 window)
{
    m_tracks.erase(window);
}

void InputSignals::refresh()
{
    const bool wanted = Config::anyInputSignal(m_bus.wantedAnywhere()) && !m_reduceMotion && !m_onBattery && !m_fullscreen;
    if (wanted && !m_spy) {
        m_spy = std::make_unique<Spy>(*this);
    } else if (!wanted && m_spy) {
        m_spy.reset();
        m_lastPointer.reset();
        clearTracks();
    }
}

void InputSignals::clearTracks()
{
    for (const quint64 window : std::views::keys(m_tracks)) {
        writeReadings(m_bus, window, {});
    }
    m_tracks.clear();
    m_ticker.stop();
    m_handlers.frameWanted();
}

void InputSignals::tick()
{
    const double at = now();
    for (auto it = m_tracks.begin(); it != m_tracks.end();) {
        const InputReadings readings = it->second.read(at);
        writeReadings(m_bus, it->first, readings.settled ? InputReadings {} : readings);
        it = readings.settled ? m_tracks.erase(it) : std::next(it);
    }
    if (m_tracks.empty()) {
        m_ticker.stop();
    }
    m_handlers.frameWanted();
}

double InputSignals::now() const
{
    return std::chrono::duration<double>(std::chrono::steady_clock::now() - m_start).count();
}

std::optional<quint64> InputSignals::wantingWindow(KWin::Window *window, const SignalSet &signalSet) const
{
    if (!window || (m_bus.wantedAnywhere() & signalSet).none()) {
        return std::nullopt;
    }
    const std::optional<quint64> id = m_handlers.windowId(window);
    if (!id) {
        return std::nullopt;
    }
    for (std::size_t index = 0; index < signalSet.size(); ++index) {
        if (signalSet.test(index) && m_bus.wanted(*id, static_cast<Signal>(index))) {
            return id;
        }
    }
    return std::nullopt;
}

InputMotion &InputSignals::motionOf(quint64 window)
{
    if (!m_ticker.isActive()) {
        m_ticker.start();
    }
    return m_tracks[window];
}

void InputSignals::pointerMoved(const KWin::PointerMotionEvent &event)
{
    const QPointF travelled = m_lastPointer ? event.position - *m_lastPointer : QPointF();
    m_lastPointer = event.position;
    if (!pointerReachesWindows()) {
        return;
    }
    if (const auto window = wantingWindow(KWin::input()->pointer()->focus(), kPointerSignals)) {
        motionOf(*window).pointer(std::hypot(travelled.x(), travelled.y()), now());
    }
}

void InputSignals::buttonChanged(const KWin::PointerButtonEvent &event)
{
    if (event.state != KWin::PointerButtonState::Pressed || !pointerReachesWindows()) {
        return;
    }
    if (const auto window = wantingWindow(KWin::input()->pointer()->focus(), kClickSignals)) {
        m_handlers.impulse(Config::EventTrigger::Click, *window, {event.position, {}, true});
        m_handlers.frameWanted();
    }
}

void InputSignals::scrolled(const KWin::PointerAxisEvent &event)
{
    if (!pointerReachesWindows()) {
        return;
    }
    const auto window = wantingWindow(KWin::input()->pointer()->focus(), kScrollSignals);
    if (!window) {
        return;
    }
    const int steps = motionOf(*window).scroll(event.delta, now());
    const Render::SignalPoint at {event.position, scrollDirection(event), true};
    for (int step = 0; step < steps; ++step) {
        m_handlers.impulse(Config::EventTrigger::Scroll, *window, at);
    }
    m_handlers.frameWanted();
}

void InputSignals::keyChanged(const KWin::KeyboardKeyEvent &event)
{
    if (event.state != KWin::KeyboardKeyState::Pressed || !keysReachWindows()) {
        return;
    }
    KWin::Window *active = KWin::workspace()->activeWindow();
    if (const auto window = wantingWindow(active, kKeySignals)) {
        motionOf(*window).key(now());
        m_handlers.impulse(Config::EventTrigger::Keypress, *window, {active->frameGeometry().center(), {}, true});
        m_handlers.frameWanted();
    }
}

}
