#pragma once

#include "decorations/eventeffects.h"

#include <QElapsedTimer>
#include <QHash>

namespace Konveyor
{

class ViewSignals
{
public:
    ViewSignals(Render::SignalBus &bus, EventEffects &events);

    bool isWanted() const;
    void update(const QList<Layout::OutputViewState> &views, const QList<Layout::WindowState> &states);
    void settle();

private:
    struct Tracked
    {
        Layout::WorkspaceId workspace = 0;
        double offset = 0.0;
        double seconds = 0.0;
        double speed = 0.0;
        double direction = 0.0;
        double distance = 0.0;
        std::size_t column = 0;
    };

    void follow(const Layout::OutputViewState &view, Tracked &tracked, double now);
    void publish(const QString &output, const Tracked &tracked);
    double seconds() const;

    Render::SignalBus &m_bus;
    EventEffects &m_events;
    QElapsedTimer m_clock;
    QHash<QString, Tracked> m_outputs;
    QHash<QString, QList<Layout::WindowId>> m_windows;
};

}
