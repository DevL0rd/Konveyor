#pragma once

#include "config/types/eventeffects.h"
#include "input/inputmotion.h"
#include "render/signals.h"

#include <QObject>
#include <QTimer>

#include <chrono>
#include <functional>
#include <map>
#include <memory>
#include <optional>

namespace KWin
{
struct KeyboardKeyEvent;
struct PointerAxisEvent;
struct PointerButtonEvent;
struct PointerMotionEvent;
class Window;
}

namespace Konveyor
{

class InputSignals : public QObject
{
public:
    struct Handlers
    {
        std::function<std::optional<quint64>(KWin::Window *)> windowId;
        std::function<void(Config::EventTrigger, quint64, const Render::SignalPoint &)> impulse;
        std::function<void()> frameWanted;
    };

    InputSignals(Render::SignalBus &bus, Handlers handlers, QObject *parent = nullptr);
    ~InputSignals() override;

    void setReduceMotion(bool reduceMotion);
    void setOnBattery(bool onBattery);
    void setFullscreen(bool fullscreen);
    void forget(quint64 window);
    bool observing() const { return m_spy != nullptr; }

private:
    class Spy;

    void refresh();
    void clearTracks();
    void tick();
    double now() const;
    std::optional<quint64> wantingWindow(KWin::Window *window, const Render::SignalSet &signalSet) const;
    InputMotion &motionOf(quint64 window);
    void pointerMoved(const KWin::PointerMotionEvent &event);
    void buttonChanged(const KWin::PointerButtonEvent &event);
    void scrolled(const KWin::PointerAxisEvent &event);
    void keyChanged(const KWin::KeyboardKeyEvent &event);

    Render::SignalBus &m_bus;
    Handlers m_handlers;
    std::unique_ptr<Spy> m_spy;
    std::map<quint64, InputMotion> m_tracks;
    std::optional<QPointF> m_lastPointer;
    QTimer m_ticker;
    std::chrono::steady_clock::time_point m_start = std::chrono::steady_clock::now();
    bool m_reduceMotion = false;
    bool m_onBattery = false;
    bool m_fullscreen = false;
};

}
