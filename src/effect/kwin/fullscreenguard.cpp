#include "kwin/fullscreenguard.h"

#include "kwin/windowregistry.h"

#include <core/output.h>
#include <main.h>
#include <wayland/surface.h>
#include <wayland/xdgshell.h>
#include <wayland_server.h>
#include <window.h>
#include <workspace.h>
#include <x11eventfilter.h>
#include <x11window.h>

#include <QPointer>
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <utility>
#include <xcb/xcb_icccm.h>

namespace Konveyor
{

namespace
{

bool nearlyEqual(const QRectF &left, const QRectF &right)
{
    const auto close = [](double a, double b) { return std::abs(a - b) < 1.0; };
    return close(left.x(), right.x()) && close(left.y(), right.y()) && close(left.width(), right.width())
        && close(left.height(), right.height());
}

KWin::XdgToplevelInterface::Capabilities capabilitiesFor(KWin::Window *window)
{
    KWin::XdgToplevelInterface::Capabilities capabilities = KWin::XdgToplevelInterface::Capability::WindowMenu;
    capabilities.setFlag(KWin::XdgToplevelInterface::Capability::Maximize, window->isMaximizable());
    capabilities.setFlag(KWin::XdgToplevelInterface::Capability::FullScreen, window->isFullScreenable());
    capabilities.setFlag(KWin::XdgToplevelInterface::Capability::Minimize, window->isMinimizable());
    return capabilities;
}

}

class X11FullscreenGuardFilter final : public KWin::X11EventFilter
{
public:
    explicit X11FullscreenGuardFilter(
        std::function<bool(KWin::Window *)> suppressFullscreenExit, std::function<bool(KWin::Window *)> suppressMinimize)
        : KWin::X11EventFilter(QList<int> {XCB_CLIENT_MESSAGE, XCB_DESTROY_NOTIFY, XCB_FOCUS_IN, XCB_PROPERTY_NOTIFY})
        , m_suppressFullscreenExit(std::move(suppressFullscreenExit))
        , m_suppressMinimize(std::move(suppressMinimize))
    {
        m_activationConnection = QObject::connect(KWin::Workspace::self(), &KWin::Workspace::windowActivated, [this](KWin::Window *window) {
            if (auto *x11Window = qobject_cast<KWin::X11Window *>(window)) {
                releasePretendedMinimize(x11Window->window());
            }
        });
    }

    ~X11FullscreenGuardFilter() override
    {
        QObject::disconnect(m_activationConnection);
        const auto windows = m_pretendMinimized;
        for (xcb_window_t window : windows) {
            releasePretendedMinimize(window);
        }
    }

    bool event(xcb_generic_event_t *genericEvent) override
    {
        const uint8_t eventType = genericEvent->response_type & ~0x80;
        if (eventType == XCB_DESTROY_NOTIFY) {
            const xcb_window_t window = reinterpret_cast<xcb_destroy_notify_event_t *>(genericEvent)->window;
            m_repairedWindows.remove(window);
            m_acknowledgedWindows.remove(window);
            m_pretendMinimized.remove(window);
            m_leavingFullscreen.remove(window);
            return false;
        }
        if (eventType == XCB_FOCUS_IN) {
            const xcb_window_t window = reinterpret_cast<xcb_focus_in_event_t *>(genericEvent)->event;
            if (m_pretendMinimized.contains(window)) {
                KWin::X11Window *client = KWin::Workspace::self()->findClient(window);
                if (client && KWin::Workspace::self()->activeWindow() != client) {
                    KWin::Workspace::self()->restoreFocus();
                    return true;
                }
                releasePretendedMinimize(window);
            }
            m_repairedWindows.remove(window);
            m_acknowledgedWindows.remove(window);
            m_leavingFullscreen.remove(window);
            return false;
        }
        if (eventType == XCB_PROPERTY_NOTIFY) {
            auto *event = reinterpret_cast<xcb_property_notify_event_t *>(genericEvent);
            if (resolveAtoms() && event->atom == m_netWmState) {
                m_leavingFullscreen.remove(event->window);
                return false;
            }
            if (event->atom != XCB_ATOM_WM_HINTS) {
                return false;
            }
            KWin::X11Window *window = KWin::Workspace::self()->findClient(event->window);
            if (window && hasFullscreenState(window->window()) && m_suppressMinimize(window)) {
                restoreFocusHints(window->window());
                m_repairedWindows.insert(window->window());
            }
            return false;
        }
        auto *event = reinterpret_cast<xcb_client_message_event_t *>(genericEvent);
        if (event->format != 32 || !resolveAtoms()) {
            return false;
        }
        KWin::X11Window *window = KWin::Workspace::self()->findClient(event->window);
        if (!window) {
            return false;
        }
        if (event->type == m_netActiveWindow && event->data.data32[0] == 2 && event->data.data32[1] == 0 && event->data.data32[3] == 0
            && event->data.data32[4] == 0 && m_pretendMinimized.contains(window->window())) {
            return true;
        }
        if (event->type == m_wmChangeState) {
            if (event->data.data32[0] != XCB_ICCCM_WM_STATE_ICONIC) {
                return false;
            }
            if (!hasFullscreenState(window->window())) {
                releasePretendedMinimize(window->window());
                m_repairedWindows.remove(window->window());
                m_acknowledgedWindows.remove(window->window());
                m_leavingFullscreen.remove(window->window());
                return false;
            }
            if (m_leavingFullscreen.remove(window->window())) {
                return false;
            }
            if (!m_suppressMinimize(window)) {
                releasePretendedMinimize(window->window());
                return false;
            }
            if (!m_repairedWindows.contains(window->window())) {
                restoreFocusHints(window->window());
                m_repairedWindows.insert(window->window());
            }
            if (!m_acknowledgedWindows.contains(window->window())) {
                if (window->isActive()) {
                    setWmState(window->window(), XCB_ICCCM_WM_STATE_NORMAL);
                } else {
                    setWmState(window->window(), XCB_ICCCM_WM_STATE_ICONIC);
                    m_pretendMinimized.insert(window->window());
                }
                m_acknowledgedWindows.insert(window->window());
            }
            return true;
        }
        if (event->type != m_netWmState) {
            return false;
        }
        const bool fullscreen = event->data.data32[1] == m_netWmStateFullscreen || event->data.data32[2] == m_netWmStateFullscreen;
        if (!fullscreen) {
            return false;
        }
        const uint32_t action = event->data.data32[0];
        const bool leavingFullscreen = action == 0 || (action == 2 && window->isFullScreen());
        if (!leavingFullscreen) {
            return false;
        }
        if (m_suppressFullscreenExit(window)) {
            return true;
        }
        m_leavingFullscreen.insert(window->window());
        return false;
    }

    void restoreMinimizeState(KWin::Window *window)
    {
        auto *x11Window = qobject_cast<KWin::X11Window *>(window);
        if (x11Window) {
            releasePretendedMinimize(x11Window->window());
        }
    }

    void restoreAllMinimizeStates()
    {
        const auto windows = m_pretendMinimized;
        for (xcb_window_t window : windows) {
            releasePretendedMinimize(window);
        }
    }

private:
    bool hasFullscreenState(xcb_window_t window)
    {
        xcb_connection_t *connection = KWin::kwinApp()->x11Connection();
        const xcb_get_property_cookie_t cookie = xcb_get_property(connection, false, window, m_netWmState, XCB_ATOM_ATOM, 0, 64);
        xcb_get_property_reply_t *reply = xcb_get_property_reply(connection, cookie, nullptr);
        if (!reply || reply->format != 32) {
            std::free(reply);
            return false;
        }
        const auto *atoms = static_cast<const xcb_atom_t *>(xcb_get_property_value(reply));
        const int count = xcb_get_property_value_length(reply) / static_cast<int>(sizeof(xcb_atom_t));
        const bool fullscreen = std::find(atoms, atoms + count, m_netWmStateFullscreen) != atoms + count;
        std::free(reply);
        return fullscreen;
    }

    void restoreFocusHints(xcb_window_t window)
    {
        xcb_connection_t *connection = KWin::kwinApp()->x11Connection();
        const xcb_get_property_cookie_t cookie = xcb_get_property(connection, false, window, XCB_ATOM_WM_HINTS, XCB_ATOM_WM_HINTS, 0, 9);
        xcb_get_property_reply_t *reply = xcb_get_property_reply(connection, cookie, nullptr);
        if (!reply || reply->format != 32 || reply->value_len < 3) {
            std::free(reply);
            return;
        }
        auto *hints = static_cast<uint32_t *>(xcb_get_property_value(reply));
        bool changed = false;
        if ((hints[0] & 1) && hints[1] == 0) {
            hints[1] = 1;
            changed = true;
        }
        if ((hints[0] & 2) && hints[2] == XCB_ICCCM_WM_STATE_ICONIC) {
            hints[2] = XCB_ICCCM_WM_STATE_NORMAL;
            changed = true;
        }
        if (changed) {
            xcb_change_property(
                connection, XCB_PROP_MODE_REPLACE, window, XCB_ATOM_WM_HINTS, XCB_ATOM_WM_HINTS, 32, reply->value_len, hints);
            xcb_flush(connection);
        }
        std::free(reply);
    }

    bool resolveAtoms()
    {
        if (m_netActiveWindow != XCB_ATOM_NONE && m_netWmState != XCB_ATOM_NONE && m_netWmStateFullscreen != XCB_ATOM_NONE
            && m_wmChangeState != XCB_ATOM_NONE && m_wmState != XCB_ATOM_NONE) {
            return true;
        }
        xcb_connection_t *connection = KWin::kwinApp()->x11Connection();
        if (!connection) {
            return false;
        }
        const auto intern = [connection](const char *name) {
            const xcb_intern_atom_cookie_t cookie = xcb_intern_atom(connection, true, std::strlen(name), name);
            xcb_intern_atom_reply_t *reply = xcb_intern_atom_reply(connection, cookie, nullptr);
            const xcb_atom_t atom = reply ? reply->atom : static_cast<xcb_atom_t>(XCB_ATOM_NONE);
            std::free(reply);
            return atom;
        };
        m_netActiveWindow = intern("_NET_ACTIVE_WINDOW");
        m_netWmState = intern("_NET_WM_STATE");
        m_netWmStateFullscreen = intern("_NET_WM_STATE_FULLSCREEN");
        m_wmChangeState = intern("WM_CHANGE_STATE");
        m_wmState = intern("WM_STATE");
        return m_netActiveWindow != XCB_ATOM_NONE && m_netWmState != XCB_ATOM_NONE && m_netWmStateFullscreen != XCB_ATOM_NONE
            && m_wmChangeState != XCB_ATOM_NONE && m_wmState != XCB_ATOM_NONE;
    }

    void setWmState(xcb_window_t window, uint32_t state)
    {
        xcb_connection_t *connection = KWin::kwinApp()->x11Connection();
        if (!connection || m_wmState == XCB_ATOM_NONE) {
            return;
        }
        const uint32_t data[] {state, XCB_WINDOW_NONE};
        xcb_change_property(connection, XCB_PROP_MODE_REPLACE, window, m_wmState, m_wmState, 32, 2, data);
        xcb_flush(connection);
    }

    void releasePretendedMinimize(xcb_window_t window)
    {
        if (m_pretendMinimized.remove(window)) {
            setWmState(window, XCB_ICCCM_WM_STATE_NORMAL);
        }
    }

    std::function<bool(KWin::Window *)> m_suppressFullscreenExit;
    std::function<bool(KWin::Window *)> m_suppressMinimize;
    xcb_atom_t m_netActiveWindow = XCB_ATOM_NONE;
    xcb_atom_t m_netWmState = XCB_ATOM_NONE;
    xcb_atom_t m_netWmStateFullscreen = XCB_ATOM_NONE;
    xcb_atom_t m_wmChangeState = XCB_ATOM_NONE;
    xcb_atom_t m_wmState = XCB_ATOM_NONE;
    QSet<xcb_window_t> m_repairedWindows;
    QSet<xcb_window_t> m_acknowledgedWindows;
    QSet<xcb_window_t> m_pretendMinimized;
    QSet<xcb_window_t> m_leavingFullscreen;
    QMetaObject::Connection m_activationConnection;
};

struct FullscreenGuard::Entry
{
    QPointer<KWin::Window> window;
    QPointer<KWin::XdgToplevelInterface> toplevel;
    QMetaObject::Connection unfullscreenConnection;
    QMetaObject::Connection minimizeConnection;
    QMetaObject::Connection capabilityConnection;
    bool fullscreen = false;
};

FullscreenGuard::FullscreenGuard(WindowRegistry &windows, QObject *parent)
    : QObject(parent)
    , m_windows(windows)
{
    m_x11Filter = std::make_unique<X11FullscreenGuardFilter>([this](KWin::Window *window) { return suppressFullscreenExit(window); },
        [this](KWin::Window *window) {
            const std::optional<Layout::WindowId> id = m_windows.idOf(window);
            return m_preventFullscreenMinimize && id && m_fullscreenWindows.contains(*id);
        });
    connect(&m_windows, &WindowRegistry::windowAdded, this, &FullscreenGuard::observe);
    connect(&m_windows, &WindowRegistry::windowRemoved, this, &FullscreenGuard::forget);
    if (KWin::XdgShellInterface *shell = KWin::waylandServer()->findChild<KWin::XdgShellInterface *>()) {
        connect(shell, &KWin::XdgShellInterface::toplevelCreated, this, &FullscreenGuard::observeToplevel);
    }
}

FullscreenGuard::~FullscreenGuard()
{
    const auto entries = m_entries;
    m_entries.clear();
    for (const std::shared_ptr<Entry> &entry : entries) {
        restore(entry);
    }
}

void FullscreenGuard::observeToplevel(KWin::XdgToplevelInterface *toplevel)
{
    KWin::SurfaceInterface *surface = toplevel->surface();
    m_toplevels.insert(surface, toplevel);
    connect(toplevel, &QObject::destroyed, this, [this, surface] { m_toplevels.remove(surface); });
    KWin::Window *window = KWin::waylandServer()->findWindow(surface);
    const std::optional<Layout::WindowId> id = m_windows.idOf(window);
    if (id) {
        observe(*id, window);
    }
}

void FullscreenGuard::observe(Layout::WindowId id, KWin::Window *window)
{
    KWin::XdgToplevelInterface *toplevel = m_toplevels.value(window->surface());
    if (!toplevel || m_entries.contains(id)) {
        return;
    }

    QObject::disconnect(toplevel, &KWin::XdgToplevelInterface::unfullscreenRequested, window, nullptr);
    QObject::disconnect(toplevel, &KWin::XdgToplevelInterface::minimizeRequested, window, nullptr);

    auto entry = std::make_shared<Entry>();
    entry->window = window;
    entry->toplevel = toplevel;
    m_entries.insert(id, entry);
    entry->unfullscreenConnection = connect(
        toplevel, &KWin::XdgToplevelInterface::unfullscreenRequested, this, [this, id] { handleUnfullscreen(id); }, Qt::DirectConnection);
    entry->minimizeConnection
        = connect(toplevel, &KWin::XdgToplevelInterface::minimizeRequested, this, [this, id] { handleMinimize(id); }, Qt::DirectConnection);
    entry->capabilityConnection
        = connect(window, &KWin::Window::minimizeableChanged, this, [this, id] { advertiseMinimizeCapability(m_entries.value(id)); });
    advertiseMinimizeCapability(entry);
}

void FullscreenGuard::forget(Layout::WindowId id)
{
    m_fullscreenWindows.remove(id);
    const std::shared_ptr<Entry> entry = m_entries.take(id);
    if (entry) {
        restore(entry);
    }
}

void FullscreenGuard::restore(const std::shared_ptr<Entry> &entry)
{
    QObject::disconnect(entry->unfullscreenConnection);
    QObject::disconnect(entry->minimizeConnection);
    QObject::disconnect(entry->capabilityConnection);
    if (!entry->window || !entry->toplevel || entry->window->isDeleted()) {
        return;
    }
    KWin::Window *window = entry->window;
    QObject::connect(
        entry->toplevel, &KWin::XdgToplevelInterface::unfullscreenRequested, window, [window] { window->setFullScreen(false); },
        Qt::DirectConnection);
    QObject::connect(
        entry->toplevel, &KWin::XdgToplevelInterface::minimizeRequested, window, [window] { window->setMinimized(true); },
        Qt::DirectConnection);
    entry->toplevel->sendWmCapabilities(capabilitiesFor(window));
}

void FullscreenGuard::setExperiments(bool preventFullscreenMinimize, bool preventFullscreenExit)
{
    m_preventFullscreenMinimize = preventFullscreenMinimize;
    m_preventFullscreenExit = preventFullscreenExit;
    if (!m_preventFullscreenMinimize) {
        m_x11Filter->restoreAllMinimizeStates();
    }
    refreshCapabilities();
}

void FullscreenGuard::update(const QList<Layout::WindowState> &states)
{
    for (const Layout::WindowState &state : states) {
        KWin::Window *window = m_windows.windowOf(state.id);
        const bool fullscreen = window && coversOutput(window, state);
        if (fullscreen) {
            m_fullscreenWindows.insert(state.id);
        } else {
            m_fullscreenWindows.remove(state.id);
            m_x11Filter->restoreMinimizeState(window);
        }
        const std::shared_ptr<Entry> entry = m_entries.value(state.id);
        if (!entry || !entry->window) {
            continue;
        }
        setFullscreen(entry, fullscreen);
    }
}

void FullscreenGuard::setFullscreen(const std::shared_ptr<Entry> &entry, bool fullscreen)
{
    if (entry->fullscreen == fullscreen) {
        return;
    }
    entry->fullscreen = fullscreen;
    advertiseMinimizeCapability(entry);
}

void FullscreenGuard::refreshCapabilities()
{
    for (const std::shared_ptr<Entry> &entry : std::as_const(m_entries)) {
        advertiseMinimizeCapability(entry);
    }
}

void FullscreenGuard::advertiseMinimizeCapability(const std::shared_ptr<Entry> &entry)
{
    if (!entry || !entry->window || !entry->toplevel || entry->window->isDeleted()) {
        return;
    }
    KWin::XdgToplevelInterface::Capabilities capabilities = capabilitiesFor(entry->window);
    capabilities.setFlag(KWin::XdgToplevelInterface::Capability::Minimize,
        !(m_preventFullscreenMinimize && entry->fullscreen) && entry->window->isMinimizable());
    entry->toplevel->sendWmCapabilities(capabilities);
}

bool FullscreenGuard::suppressFullscreenExit(const std::shared_ptr<Entry> &entry) const
{
    return m_preventFullscreenExit && entry->window && entry->fullscreen && !entry->window->isActive();
}

bool FullscreenGuard::suppressFullscreenExit(KWin::Window *window) const
{
    const std::optional<Layout::WindowId> id = m_windows.idOf(window);
    return m_preventFullscreenExit && id && m_fullscreenWindows.contains(*id) && !window->isActive();
}

void FullscreenGuard::handleUnfullscreen(Layout::WindowId id)
{
    const std::shared_ptr<Entry> entry = m_entries.value(id);
    if (!entry || !entry->window || suppressFullscreenExit(entry)) {
        return;
    }
    entry->window->setFullScreen(false);
}

void FullscreenGuard::handleMinimize(Layout::WindowId id)
{
    const std::shared_ptr<Entry> entry = m_entries.value(id);
    if (!entry || !entry->window || (m_preventFullscreenMinimize && currentlyCoversOutput(entry->window))) {
        return;
    }
    entry->window->setMinimized(true);
}

bool FullscreenGuard::currentlyCoversOutput(KWin::Window *window)
{
    if (window->isFullScreen() || window->isRequestedFullScreen()) {
        return true;
    }
    const KWin::LogicalOutput *output = window->output();
    if (!window->noBorder() || !output) {
        return false;
    }
    return nearlyEqual(window->frameGeometry(), output->geometryF());
}

bool FullscreenGuard::coversOutput(KWin::Window *window, const Layout::WindowState &state)
{
    if (currentlyCoversOutput(window) || state.sizingMode == Layout::WindowMode::Fullscreen
        || state.requestedSizingMode == Layout::WindowMode::Fullscreen) {
        return true;
    }
    const KWin::LogicalOutput *output = window->output();
    return window->noBorder() && output && nearlyEqual(state.targetFrame, output->geometryF());
}

}
