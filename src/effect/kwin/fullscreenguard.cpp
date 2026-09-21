#include "kwin/fullscreenguard.h"

#include "kwin/windowregistry.h"
#include "kwin/x11fullscreenguardfilter.h"

#include <core/output.h>
#include <main.h>
#include <wayland/surface.h>
#include <wayland/xdgshell.h>
#include <wayland_server.h>
#include <window.h>

#include <QPointer>
#include <cmath>
#include <utility>

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
