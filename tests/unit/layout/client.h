#pragma once

#include "layout/engine/engine.h"

#include <QHash>
#include <QMap>
#include <QSizeF>

#include <algorithm>
#include <optional>
#include <utility>

namespace LayoutTest
{

namespace Layout = Konveyor::Layout;

struct Client
{
    QSizeF minSize;
    QSizeF maxSize;
    std::optional<QSizeF> fixedSize;
    bool commitsLate = false;

    static Client honoring(const Layout::WindowProperties &properties)
    {
        Client client;
        client.minSize = properties.minSize;
        client.maxSize = properties.maxSize;
        if (!properties.isResizable) {
            client.fixedSize = properties.frameSize;
        }
        return client;
    }

    QSizeF respond(QSizeF requested) const
    {
        if (fixedSize) {
            return *fixedSize;
        }
        return {clampExtent(requested.width(), minSize.width(), maxSize.width()),
            clampExtent(requested.height(), minSize.height(), maxSize.height())};
    }

private:
    static double clampExtent(double value, double min, double max)
    {
        if (max > 0.0) {
            value = std::min(value, max);
        }
        return std::max(value, min);
    }
};

class ClientPool
{
public:
    void set(Layout::WindowId id, const Client &client) { m_clients.insert(id, client); }

    void forget(Layout::WindowId id)
    {
        m_clients.remove(id);
        m_answered.remove(id);
        m_held.remove(id);
    }

    void commitHeld(Layout::Engine &engine)
    {
        const QMap<Layout::WindowId, QSizeF> held = std::exchange(m_held, {});
        for (auto it = held.constBegin(); it != held.constEnd(); ++it) {
            if (engine.hasWindow(it.key())) {
                engine.windowSizeCommitted(it.key(), m_clients.value(it.key()).respond(it.value()));
            }
        }
    }

    void markAnswered(Layout::WindowId id, QSizeF size) { m_answered.insert(id, size); }

    bool answer(Layout::Engine &engine, Layout::WindowId id, QSizeF requested)
    {
        if (m_answered.contains(id) && m_answered.value(id) == requested) {
            return false;
        }
        m_answered.insert(id, requested);
        const Client client = m_clients.value(id);
        if (client.commitsLate) {
            m_held.insert(id, requested);
            return false;
        }
        engine.windowSizeCommitted(id, client.respond(requested));
        return true;
    }

private:
    QHash<Layout::WindowId, Client> m_clients;
    QHash<Layout::WindowId, QSizeF> m_answered;
    QMap<Layout::WindowId, QSizeF> m_held;
};

}
