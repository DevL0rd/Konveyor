#include "kwin/windowregistry.h"

#include <window.h>
#include <workspace.h>

#include <memory>

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

WindowRegistry::WindowRegistry(QObject *parent)
    : QObject(parent)
{ }

void WindowRegistry::start()
{
    connect(KWin::workspace(), &KWin::Workspace::windowAdded, this, &WindowRegistry::observe);
    connect(KWin::workspace(), &KWin::Workspace::windowRemoved, this, &WindowRegistry::forget);
    connect(KWin::workspace(), &KWin::Workspace::windowActivated, this, &WindowRegistry::activeWindowChanged);
    m_adopting = true;
    const QList<KWin::Window *> windows = KWin::workspace()->windows();
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
    if (specialType || window->isLockScreenOverlay() || window->isOnAllDesktops() || window->skipTaskbar()) {
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
    properties.wantsFullscreen = window->isFullScreen() && !m_adopting;
    properties.wantsMaximized = isMaximizeRequested(window) && !m_adopting;
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

void WindowRegistry::reevaluate()
{
    const QList<KWin::Window *> observed(m_observed.cbegin(), m_observed.cend());
    for (KWin::Window *window : observed) {
        refresh(window);
    }
}

void WindowRegistry::observe(KWin::Window *window)
{
    if (m_observed.contains(window) || !isManageable(window)) {
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

void WindowRegistry::refresh(KWin::Window *window)
{
    const bool tracked = m_ids.contains(window);
    const bool wanted = !window->isMinimized() && (!m_wantsWindow || m_wantsWindow(propertiesOf(window)));
    if (!wanted && tracked) {
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
    const auto refreshWindow = [this, window]() { refresh(window); };
    connect(window, &KWin::Window::minimizedChanged, this, refreshWindow);
    connect(window, &KWin::Window::captionChanged, this, refreshWindow);
    connect(window, &KWin::Window::desktopFileNameChanged, this, refreshWindow);
    connect(window, &KWin::Window::windowClassChanged, this, refreshWindow);
}

void WindowRegistry::connectWindow(KWin::Window *window, Layout::WindowId id)
{
    const auto emitProperties = [this, id]() { Q_EMIT propertiesChanged(id); };
    connect(window, &KWin::Window::captionChanged, this, emitProperties);
    connect(window, &KWin::Window::desktopFileNameChanged, this, emitProperties);
    connect(window, &KWin::Window::windowClassChanged, this, emitProperties);
    connect(window, &KWin::Window::transientChanged, this, emitProperties);
    connect(window, &KWin::Window::demandsAttentionChanged, this,
        [this, window, id]() { Q_EMIT urgencyChanged(id, window->isDemandingAttention()); });
    connect(
        window, &KWin::Window::fullScreenChanged, this, [this, window, id]() { Q_EMIT fullscreenRequested(id, window->isFullScreen()); });
    connect(
        window, &KWin::Window::maximizedChanged, this, [this, window, id]() { Q_EMIT maximizeRequested(id, isMaximizeRequested(window)); });
    connect(window, &KWin::Window::borderRadiusChanged, this, [this, id]() { Q_EMIT appearanceChanged(id); });
    connect(window, &KWin::Window::frameGeometryChanged, this,
        [this, window, id]() { Q_EMIT sizeCommitted(id, window->frameGeometry().size()); });
    connectInteractiveSignals(window, id);
}

void WindowRegistry::connectInteractiveSignals(KWin::Window *window, Layout::WindowId id)
{
    const auto isMove = std::make_shared<bool>(false);
    connect(window, &KWin::Window::interactiveMoveResizeStarted, this, [this, window, id, isMove]() {
        *isMove = window->isInteractiveMove();
        Q_EMIT interactiveStarted(id, *isMove);
    });
    connect(window, &KWin::Window::interactiveMoveResizeStepped, this, [this, id, isMove]() { Q_EMIT interactiveStepped(id, *isMove); });
    connect(window, &KWin::Window::interactiveMoveResizeFinished, this, [this, id, isMove]() { Q_EMIT interactiveFinished(id, *isMove); });
}

}
