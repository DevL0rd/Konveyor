#include "kwin/windowregistry.h"

#include "config/log.h"

#include <window.h>
#include <workspace.h>
#include <x11window.h>

#include <algorithm>
#include <memory>
#include <ranges>
#include <utility>

namespace Konveyor
{

namespace
{

QString appIdOf(KWin::Window *window)
{
    const QString desktopFile = window->desktopFileName();
    return desktopFile.isEmpty() ? window->resourceClass() : desktopFile;
}

bool isMaximizeRequested(KWin::Window *window)
{
    return window->requestedMaximizeMode() == KWin::MaximizeFull;
}

}

struct WindowRegistry::WindowSignalState
{
    Layout::WindowProperties announcedConstraints;
    bool announcedFullscreen = false;
    bool isMove = false;
};

WindowRegistry::WindowRegistry(QObject *parent)
    : QObject(parent)
    , KWin::X11EventFilter(QList<int> {XCB_PROPERTY_NOTIFY})
{ }

bool WindowRegistry::event(xcb_generic_event_t *genericEvent)
{
    const auto *event = reinterpret_cast<xcb_property_notify_event_t *>(genericEvent);
    if (event->atom != XCB_ATOM_WM_NORMAL_HINTS) {
        return false;
    }
    const QPointer<KWin::Window> window = KWin::workspace()->findClient(event->window);
    QMetaObject::invokeMethod(
        this,
        [this, window] {
            if (!window) {
                return;
            }
            if (const auto id = idOf(window)) {
                Q_EMIT propertiesChanged(*id);
            }
        },
        Qt::QueuedConnection);
    return false;
}

void WindowRegistry::start(const std::function<bool(KWin::Window *, KWin::Window *)> &adoptionOrder)
{
    connect(KWin::workspace(), &KWin::Workspace::windowAdded, this, &WindowRegistry::observe);
    connect(KWin::workspace(), &KWin::Workspace::windowRemoved, this, &WindowRegistry::forget);
    connect(KWin::workspace(), &KWin::Workspace::windowActivated, this, &WindowRegistry::activeWindowChanged);
    connect(KWin::workspace(), &KWin::Workspace::currentActivityChanged, this, &WindowRegistry::reevaluate);
    m_adopting = true;
    QList<KWin::Window *> windows = KWin::workspace()->windows();
    std::ranges::stable_sort(windows, adoptionOrder);
    for (KWin::Window *window : windows) {
        observe(window);
    }
    m_adopting = false;
}

bool WindowRegistry::isManageable(KWin::Window *window)
{
    if (!window || window->isDeleted() || !window->isClient() || window->isInternal()) {
        return false;
    }
    const bool specialType = window->isDesktop() || window->isDock() || window->isPopupWindow() || window->isSpecialWindow()
        || window->isMenu() || window->isDropdownMenu() || window->isPopupMenu() || window->isUtility() || window->isSplash()
        || window->isToolbar() || window->isTooltip() || window->isNotification() || window->isOnScreenDisplay()
        || window->isCriticalNotification() || window->isAppletPopup() || window->isLockScreen() || window->isInputMethod();
    if (specialType || window->isLockScreenOverlay() || window->skipTaskbar()) {
        return false;
    }
    return window->isNormalWindow() || window->isDialog();
}

Layout::WindowProperties WindowRegistry::propertiesOf(KWin::Window *window) const
{
    Layout::WindowProperties properties;
    properties.appId = appIdOf(window);
    properties.title = window->caption();
    properties.minSize = window->minSize();
    properties.maxSize = window->maxSize();
    properties.parent = window->transientFor() ? idOf(window->transientFor()) : std::nullopt;
    properties.isUrgent = window->isDemandingAttention();
    properties.isDialog = window->isDialog() || window->isModal();
    properties.isResizable = window->isResizable();
    properties.wantsFullscreen = window->isFullScreen();
    properties.wantsMaximized = isMaximizeRequested(window) && !m_adopting;
    properties.onAllDesktops = window->isOnAllDesktops();
    properties.frameSize = window->frameGeometry().size();
    return properties;
}

std::optional<Layout::WindowId> WindowRegistry::idOf(KWin::Window *window) const
{
    const auto it = m_ids.constFind(window);
    return it == m_ids.constEnd() ? std::nullopt : std::optional(*it);
}

KWin::Window *WindowRegistry::windowOf(Layout::WindowId id) const
{
    return m_windows.value(id).data();
}

void WindowRegistry::setWantsWindow(std::function<bool(const Layout::WindowProperties &)> wantsWindow)
{
    m_wantsWindow = std::move(wantsWindow);
}

void WindowRegistry::setPlacementOrder(std::function<bool(KWin::Window *, KWin::Window *)> placedBefore)
{
    m_placedBefore = std::move(placedBefore);
}

void WindowRegistry::reevaluate()
{
    QList<KWin::Window *> leaving;
    QList<KWin::Window *> arriving;
    for (KWin::Window *window : std::as_const(m_observed)) {
        const bool tracked = m_ids.contains(window);
        if (tracked != isWanted(window)) {
            (tracked ? leaving : arriving).append(window);
        }
    }
    std::ranges::stable_sort(leaving, m_placedBefore);
    std::ranges::stable_sort(arriving, m_placedBefore);
    for (KWin::Window *window : leaving | std::views::reverse) {
        refresh(window);
    }
    for (KWin::Window *window : std::as_const(arriving)) {
        refresh(window);
    }
}

void WindowRegistry::observe(KWin::Window *window)
{
    if (m_observed.contains(window)) {
        return;
    }
    if (!isManageable(window)) {
        if (window && !window->isDeleted() && (window->isNormalWindow() || window->isDialog())) {
            qCInfo(lcKonveyor) << "konveyor: not managing" << appIdOf(window) << window->caption() << "client =" << window->isClient()
                               << "internal =" << window->isInternal() << "skipTaskbar =" << window->skipTaskbar()
                               << "dock =" << window->isDock() << "utility =" << window->isUtility() << "splash =" << window->isSplash();
        }
        return;
    }
    m_observed.insert(window);
    connectObserved(window);
    refresh(window);
}

void WindowRegistry::forget(KWin::Window *window)
{
    if (!m_observed.remove(window)) {
        return;
    }
    remove(window);
    disconnect(window, nullptr, this, nullptr);
}

bool WindowRegistry::isWanted(KWin::Window *window) const
{
    return !window->isMinimized() && window->isOnCurrentActivity() && (!m_wantsWindow || m_wantsWindow(propertiesOf(window)));
}

void WindowRegistry::refresh(KWin::Window *window)
{
    const bool tracked = m_ids.contains(window);
    const bool wanted = isWanted(window);
    if (wanted != tracked) {
        qCInfo(lcKonveyor) << "konveyor:" << (wanted ? "tracking" : "untracking") << appIdOf(window) << window->caption()
                           << "minimized =" << window->isMinimized() << "onCurrentActivity =" << window->isOnCurrentActivity()
                           << "fullscreen =" << window->isFullScreen() << "noBorder =" << window->noBorder()
                           << "frame =" << window->frameGeometry();
    }
    if (!wanted && tracked) {
        if (window->isMinimized() || !window->isOnCurrentActivity()) {
            Q_EMIT windowHiding(m_ids.value(window), window);
        }
        remove(window);
    } else if (wanted && !tracked) {
        add(window);
    }
}

void WindowRegistry::add(KWin::Window *window)
{
    const Layout::WindowId id = m_nextId++;
    m_ids.insert(window, id);
    m_windows.insert(id, window);
    connectWindow(window, id);
    Q_EMIT windowAdded(id, window);
}

void WindowRegistry::remove(KWin::Window *window)
{
    const auto id = idOf(window);
    if (!id) {
        return;
    }
    m_ids.remove(window);
    m_windows.remove(*id);
    disconnect(window, nullptr, this, nullptr);
    if (m_observed.contains(window)) {
        connectObserved(window);
    }
    Q_EMIT windowRemoved(*id);
}

void WindowRegistry::connectObserved(KWin::Window *window)
{
    const auto refreshWindow = [this, window] { refresh(window); };
    connect(window, &KWin::Window::minimizedChanged, this, refreshWindow);
    connect(window, &KWin::Window::activitiesChanged, this, refreshWindow);
    connect(window, &KWin::Window::captionChanged, this, refreshWindow);
    connect(window, &KWin::Window::desktopFileNameChanged, this, refreshWindow);
    connect(window, &KWin::Window::windowClassChanged, this, refreshWindow);
}

void WindowRegistry::connectWindow(KWin::Window *window, Layout::WindowId id)
{
    const auto emitProperties = [this, id] { Q_EMIT propertiesChanged(id); };
    connect(window, &KWin::Window::captionChanged, this, emitProperties);
    connect(window, &KWin::Window::desktopFileNameChanged, this, emitProperties);
    connect(window, &KWin::Window::windowClassChanged, this, emitProperties);
    connect(window, &KWin::Window::transientChanged, this, emitProperties);
    connect(window, &KWin::Window::desktopsChanged, this, emitProperties);
    connect(window, &KWin::Window::demandsAttentionChanged, this,
        [this, window, id] { Q_EMIT urgencyChanged(id, window->isDemandingAttention()); });
    const auto state = std::make_shared<WindowSignalState>();
    state->announcedConstraints = propertiesOf(window);
    state->announcedFullscreen = window->isFullScreen();
    const auto announceConstraints = [this, window, id, state] {
        const auto properties = propertiesOf(window);
        const bool changed = properties.minSize != state->announcedConstraints.minSize
            || properties.maxSize != state->announcedConstraints.maxSize
            || properties.isResizable != state->announcedConstraints.isResizable;
        state->announcedConstraints = properties;
        if (changed) {
            Q_EMIT propertiesChanged(id);
        }
    };
    const auto announceFullscreen = [this, window, id, state](bool fullscreen) {
        if (std::exchange(state->announcedFullscreen, fullscreen) != fullscreen) {
            qCInfo(lcKonveyor) << "konveyor: layout told" << appIdOf(window) << "fullscreen =" << fullscreen
                               << "active =" << window->isActive() << "frame =" << window->frameGeometry();
            Q_EMIT fullscreenRequested(id, fullscreen);
        }
    };
    connect(window, &KWin::Window::fullScreenChanged, this, [window, announceConstraints, announceFullscreen] {
        announceConstraints();
        announceFullscreen(window->isFullScreen());
    });
    connect(window, &KWin::Window::maximizedChanged, this, [this, window, id] {
        qCInfo(lcKonveyor) << "konveyor:" << appIdOf(window) << "maximize changed, requested full =" << isMaximizeRequested(window);
        Q_EMIT maximizeRequested(id, isMaximizeRequested(window));
    });
    connect(window, &KWin::Window::borderRadiusChanged, this, [this, id] { Q_EMIT appearanceChanged(id); });
    connect(window, &KWin::Window::opacityChanged, this, [this, id] { Q_EMIT appearanceChanged(id); });
    connect(window, &KWin::Window::frameGeometryChanged, this, [this, window, id, announceConstraints, announceFullscreen] {
        announceConstraints();
        announceFullscreen(window->isRequestedFullScreen());
        Q_EMIT sizeCommitted(id, window->frameGeometry().size());
    });
    connectInteractiveSignals(window, id, state);
}

void WindowRegistry::connectInteractiveSignals(KWin::Window *window, Layout::WindowId id, const std::shared_ptr<WindowSignalState> &state)
{
    connect(window, &KWin::Window::interactiveMoveResizeStarted, this, [this, window, id, state] {
        state->isMove = window->isInteractiveMove();
        qCInfo(lcKonveyor) << "konveyor: interactive" << (state->isMove ? "move" : "resize") << "started on" << appIdOf(window);
        Q_EMIT interactiveStarted(id, state->isMove);
    });
    connect(window, &KWin::Window::interactiveMoveResizeStepped, this, [this, id, state] { Q_EMIT interactiveStepped(id, state->isMove); });
    connect(window, &KWin::Window::interactiveMoveResizeFinished, this, [this, window, id, state] {
        qCInfo(lcKonveyor) << "konveyor: interactive" << (state->isMove ? "move" : "resize") << "finished on" << appIdOf(window)
                           << window->frameGeometry();
        Q_EMIT interactiveFinished(id, state->isMove);
    });
}

}
