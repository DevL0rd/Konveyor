#include "kwin/fullscreenguard.h"

#include "kwin/windowregistry.h"
#include "kwin/x11fullscreenguardfilter.h"

#include "config/log.h"

#include <core/output.h>
#include <main.h>
#include <wayland/clientconnection.h>
#include <wayland/surface.h>
#include <wayland/xdgshell.h>
#include <window.h>

#include <QPointer>
#include <cmath>
#include <utility>
#include <wayland-server-core.h>

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

KWin::XdgToplevelInterface *toplevelOf(KWin::Window *window)
{
    struct Search
    {
        KWin::SurfaceInterface *surface = nullptr;
        KWin::XdgToplevelInterface *toplevel = nullptr;
    };
    Search search {window->surface()};
    if (!search.surface || !search.surface->client()) {
        return nullptr;
    }
    const auto match = [](wl_resource *resource, void *data) {
        auto *search = static_cast<Search *>(data);
        KWin::XdgToplevelInterface *toplevel = KWin::XdgToplevelInterface::get(resource);
        if (toplevel && toplevel->surface() == search->surface) {
            search->toplevel = toplevel;
            return WL_ITERATOR_STOP;
        }
        return WL_ITERATOR_CONTINUE;
    };
    wl_client_for_each_resource(search.surface->client()->client(), match, &search);
    return search.toplevel;
}

}

struct FullscreenGuard::Entry
{
    Layout::WindowId id;
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
}

FullscreenGuard::~FullscreenGuard()
{
    const auto entries = m_entries;
    m_entries.clear();
    for (const std::shared_ptr<Entry> &entry : entries) {
        restore(entry);
    }
}

void FullscreenGuard::observe(Layout::WindowId id, KWin::Window *window)
{
    if (m_entries.contains(id)) {
        return;
    }
    KWin::XdgToplevelInterface *toplevel = toplevelOf(window);
    if (!toplevel) {
        return;
    }

    auto entry = std::make_shared<Entry>();
    entry->id = id;
    entry->window = window;
    entry->toplevel = toplevel;
    m_entries.insert(id, entry);
    entry->unfullscreenConnection = connect(
        toplevel, &KWin::XdgToplevelInterface::unfullscreenRequested, this, [this, id] { handleUnfullscreen(id); }, Qt::DirectConnection);
    setMinimizeGuard(entry, m_preventFullscreenMinimize);
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
    QObject::disconnect(entry->capabilityConnection);
    setMinimizeGuard(entry, false);
    if (!entry->window || !entry->toplevel || entry->window->isDeleted()) {
        return;
    }
    KWin::Window *window = entry->window;
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
        setMinimizeGuard(entry, m_preventFullscreenMinimize);
        advertiseMinimizeCapability(entry);
    }
}

void FullscreenGuard::setMinimizeGuard(const std::shared_ptr<Entry> &entry, bool enabled)
{
    if (!entry->window || !entry->toplevel || entry->window->isDeleted() || bool(entry->minimizeConnection) == enabled) {
        return;
    }
    if (enabled) {
        QObject::disconnect(entry->toplevel, &KWin::XdgToplevelInterface::minimizeRequested, entry->window, nullptr);
        entry->minimizeConnection = connect(
            entry->toplevel, &KWin::XdgToplevelInterface::minimizeRequested, this, [this, id = entry->id] { handleMinimize(id); },
            Qt::DirectConnection);
        return;
    }
    QObject::disconnect(entry->minimizeConnection);
    entry->minimizeConnection = {};
    KWin::Window *window = entry->window;
    QObject::connect(
        entry->toplevel, &KWin::XdgToplevelInterface::minimizeRequested, window, [window] { window->setMinimized(true); },
        Qt::DirectConnection);
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
    if (!entry || !entry->window) {
        return;
    }
    const bool suppressed = suppressFullscreenExit(entry);
    qCInfo(lcKonveyor) << "konveyor: Wayland" << entry->window->resourceClass()
                       << "asked to leave fullscreen, active =" << entry->window->isActive() << "suppressed =" << suppressed;
    if (suppressed && entry->window->isFullScreen()) {
        const auto restoreFrame = entry->window->fullscreenGeometryRestore();
        entry->window->setMoveResizeOutput(entry->window->output());
        entry->window->setFullScreen(true);
        entry->window->setFullscreenGeometryRestore(restoreFrame);
    }
}

void FullscreenGuard::handleMinimize(Layout::WindowId id)
{
    const std::shared_ptr<Entry> entry = m_entries.value(id);
    if (!entry || !entry->window) {
        return;
    }
    const bool suppressed = m_preventFullscreenMinimize && currentlyCoversOutput(entry->window);
    qCInfo(lcKonveyor) << "konveyor: Wayland" << entry->window->resourceClass() << "asked to minimize, suppressed =" << suppressed;
    if (suppressed) {
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
