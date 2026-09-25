#include "plugin/konveyoreffect_p.h"

#include <keyboard_input.h>
#include <xkb.h>

#include <functional>

namespace Konveyor
{

namespace
{

void runMonitorOverlay(const QStringList &arguments)
{
    QProcess::startDetached(QDir::home().filePath(QStringLiteral(".local/bin/monitor-overlay")), arguments);
}

void attachMonitorWindow(KWin::Window *window, KWin::Window *target)
{
    if (!window || !target || window == target) {
        return;
    }
    if (KWin::Window *previous = window->transientFor(); previous && previous != target) {
        previous->removeTransient(window);
    }
    if (!target->transients().contains(window)) {
        target->addTransient(window);
    }
    if (window->transientFor() != target) {
        window->setTransientFor(target);
    }
    window->setSkipTaskbar(true);
    window->setSkipPager(true);
    window->setSkipSwitcher(true);
}

void detachMonitorWindow(KWin::Window *window)
{
    if (window) {
        if (KWin::Window *target = window->transientFor()) {
            target->removeTransient(window);
        }
    }
}

using MonitorWindowMap = QHash<Layout::WindowId, QHash<int, QPointer<KWin::Window>>>;

bool removeMonitorWindow(MonitorWindowMap &windows, KWin::Window *window, const std::function<void(Layout::WindowId)> &place)
{
    bool removed = false;
    for (auto target = windows.begin(); target != windows.end();) {
        auto &slotMap = target.value();
        for (auto slot = slotMap.begin(); slot != slotMap.end();) {
            if (slot.value() == window) {
                slot = slotMap.erase(slot);
                removed = true;
            } else {
                ++slot;
            }
        }
        if (slotMap.isEmpty()) {
            target = windows.erase(target);
        } else {
            const Layout::WindowId id = target.key();
            ++target;
            place(id);
        }
    }
    return removed;
}

}

void KonveyorEffect::installInputFilter()
{
    d->spillInput = std::make_unique<SpillInputFilter>([this](KWin::Window *window, const QPointF &position) {
        const std::optional<Layout::WindowId> id = d->windows.idOf(window);
        const auto home = id ? d->homeOutputs.constFind(*id) : d->homeOutputs.constEnd();
        if (home == d->homeOutputs.constEnd()) {
            return true;
        }
        const KWin::LogicalOutput *output = d->outputs.outputNamed(*home);
        return !output || QRectF(output->geometryF()).contains(position);
    });
    d->input = std::make_unique<InputFilter>(InputHandlers {
        [this](Config::BindTrigger trigger, Qt::KeyboardModifiers modifiers, Config::MouseButton button,
            Config::ScrollDirection direction) { return d->shortcuts.triggerPointerBind(trigger, modifiers, button, direction); },
        [this](const QPointF &position, qint64 timestamp) { handlePointerMotion(position, timestamp); },
        [this] { endTitlebarDrag(); },
        [this](quint32 keycode, Qt::KeyboardModifiers modifiers, bool repeat) {
            const KWin::Xkb *xkb = KWin::input()->keyboard()->xkb();
            return d->shortcuts.triggerKeyPosition(keycode, modifiers, repeat, xkb->keymap(), xkb->currentLayout());
        },
    });
    d->gestureInput = std::make_unique<GestureFilter>(GestureHandlers {
        [this](int fingers) { return routeGesture(d->gestures.touchpadSwipeBegin(fingers, outputNameAt(KWin::effects->cursorPos()))); },
        [this](const QPointF &delta, qint64 timestamp) { return routeGesture(d->gestures.touchpadSwipeUpdate(delta, timestamp)); },
        [this] { return routeGesture(d->gestures.touchpadSwipeEnd()); },
        [this](int fingers) { return routeGesture(d->gestures.touchpadPinchBegin(fingers)); },
        [this](double scale) { return routeGesture(d->gestures.touchpadPinchUpdate(scale)); },
        [this] { return routeGesture(d->gestures.touchpadPinchEnd()); },
        [this](qint32 id, const QPointF &position, qint64 timestamp) { return handleTouchDown(id, position, timestamp); },
        [this](qint32 id, const QPointF &position, qint64 timestamp) { return handleTouchMotion(id, position, timestamp); },
        [this](qint32 id, qint64 timestamp) { return handleTouchUp(id, timestamp); },
        [this] {
            d->gestures.touchCancel();
            endTitlebarDrag();
            scheduleFlush();
        },
        [this] { routeGesture(d->gestures.resetTouches()); },
        [this](bool active) {
            if (d->dbus) {
                d->dbus->setMultiTouchActive(active);
            }
        },
        [this] { return d->gestures.takesTouchpadTapButton(); },
    });
    d->touchpadContacts = std::make_unique<TouchpadContactReader>(TouchpadContactHandlers {
        [this](
            qint32 slot, const QPointF &millimeters, qint64 timestamp) { d->gestures.touchpadContactDown(slot, millimeters, timestamp); },
        [this](qint32 slot, const QPointF &millimeters) { d->gestures.touchpadContactMotion(slot, millimeters); },
        [this](qint32 slot, qint64 timestamp) { routeGesture(d->gestures.touchpadContactUp(slot, timestamp)); },
        [this] { d->gestures.touchpadPhysicalClick(); },
        [this] { d->gestures.touchpadContactsReset(); },
    });
}

void KonveyorEffect::startDBusService()
{
    d->dbus = std::make_unique<DBusService>(DBusHandlers {
        [this] { return windowsJson(); },
        [this] { return workspacesJson(); },
        [this] { return outputsJson(); },
        [this] { return focusedWindowJson(); },
        [this] { return focusedOutputJson(); },
        [this] { return bindsJson(); },
        [this](const QString &json) { return performActionJson(json); },
        [this](const QString &path) { return d->config.load(path); },
        [this] { return readEngine().isOverviewOpen(); },
        [this] { return lastBindJson(); },
        [this] { return QJsonDocument(Ipc::gesturesToJson(d->gestures.config())); },
    });
    d->dbus->registerService();
}

void KonveyorEffect::connectRegistries()
{
    connectWindowLifecycle();
    connectWindowState();
    connectOutputs();
    connectDragAndDrop();
}

void KonveyorEffect::connectWindowLifecycle()
{
    connect(KWin::workspace(), &KWin::Workspace::windowAdded, this, [this](KWin::Window *window) {
        observeMonitorOverlay(window);
        connect(window, &KWin::Window::captionChanged, this, [this, window] { observeMonitorOverlay(window); });
    });
    connect(KWin::workspace(), &KWin::Workspace::windowRemoved, this, [this](KWin::Window *window) {
        if (const std::optional<Layout::WindowId> id = d->windows.idOf(window)) {
            d->monitorOverlays.remove(*id);
            d->monitorPanels.remove(*id);
            runMonitorOverlay({QStringLiteral("remove"), QString::number(*id)});
        }
        forgetMonitorOverlay(window);
    });
    for (KWin::Window *window : KWin::workspace()->windows()) {
        observeMonitorOverlay(window);
        connect(window, &KWin::Window::captionChanged, this, [this, window] { observeMonitorOverlay(window); });
    }
    connect(&d->windows, &WindowRegistry::windowAdded, this, &KonveyorEffect::onWindowAdded);
    connect(&d->windows, &WindowRegistry::windowRemoved, this, [this](Layout::WindowId id) {
        changeEngine().removeWindow(id);
        d->decorations.remove(id);
        d->fullscreenShade.remove(id);
        d->applier.forget(id);
        d->homeOutputs.remove(id);
    });
    connect(&d->windows, &WindowRegistry::windowMinimizing, this, [this](Layout::WindowId id, KWin::Window *window) {
        const std::optional<Layout::RestorePlacement> placement = readEngine().placementOf(id);
        if (!placement) {
            return;
        }
        if (!d->minimizedPlacements.contains(window)) {
            connect(window, &QObject::destroyed, this, [this, window] { d->minimizedPlacements.remove(window); });
        }
        d->minimizedPlacements.insert(window, *placement);
    });
    connect(&d->windows, &WindowRegistry::propertiesChanged, this, [this](Layout::WindowId id) {
        if (KWin::Window *window = d->windows.windowOf(id)) {
            changeEngine().updateWindowProperties(id, d->windows.propertiesOf(window));
        }
    });
    connect(&d->windows, &WindowRegistry::activeWindowChanged, this, &KonveyorEffect::followActiveWindow);
}

void KonveyorEffect::connectWindowState()
{
    connect(&d->windows, &WindowRegistry::sizeCommitted, this, &KonveyorEffect::handleWindowSizeCommitted);
    connect(&d->windows, &WindowRegistry::fullscreenRequested, this, [this](Layout::WindowId id, bool fullscreen) {
        if (!d->applier.isApplying()) {
            changeEngine().setWindowFullscreen(id, fullscreen);
        }
    });
    connect(&d->windows, &WindowRegistry::maximizeRequested, this, [this](Layout::WindowId id, bool maximized) {
        if (maximized && !d->applier.isApplying()) {
            changeEngine().toggleWindowFillWidth(id);
        }
    });
    connect(&d->windows, &WindowRegistry::appearanceChanged, this, &KonveyorEffect::scheduleFlush);
    connect(&d->windows, &WindowRegistry::urgencyChanged, this,
        [this](Layout::WindowId id, bool urgent) { changeEngine().setWindowUrgent(id, urgent); });
    const QList<std::pair<void (WindowRegistry::*)(Layout::WindowId, bool), int>> phases {
        {&WindowRegistry::interactiveStarted, interactivePhaseStart},
        {&WindowRegistry::interactiveStepped, interactivePhaseStep},
        {&WindowRegistry::interactiveFinished, interactivePhaseEnd},
    };
    for (const auto &[signal, phase] : phases) {
        connect(&d->windows, signal, this, [this, phase](Layout::WindowId id, bool isMove) { onInteractive(id, isMove, phase); });
    }
}

void KonveyorEffect::handleWindowSizeCommitted(Layout::WindowId id, const QSizeF &size)
{
    KWin::Window *window = d->windows.windowOf(id);
    if (window && adoptFloatingGeometry(id, window)) {
        scheduleFlush();
        return;
    }
    const bool acknowledgesRequest = d->applier.isEchoOfAppliedSize(id, size);
    const bool resizedByUser = !d->applier.isApplying() && window && window->isInteractiveResize();
    const std::optional<Layout::WindowState> state = readEngine().windowState(id);
    const bool resizedByApp = !d->applier.isApplying() && window && !window->isResizable() && state && !state->isForceResizable;
    if (resizedByApp) {
        changeEngine().updateWindowProperties(id, d->windows.propertiesOf(window));
    }
    if (acknowledgesRequest || resizedByUser || resizedByApp) {
        changeEngine().windowSizeCommitted(id, size);
    }
    scheduleFlush();
}

void KonveyorEffect::connectDragAndDrop()
{
    KWin::SeatInterface *seat = KWin::waylandServer()->seat();
    connect(seat, &KWin::SeatInterface::dragStarted, this, [this] { changeEngine().beginDataDrag(); });
    connect(seat, &KWin::SeatInterface::dragEnded, this, [this] { changeEngine().endDataDrag(); });
}

void KonveyorEffect::connectOutputs()
{
    connect(&d->outputs, &OutputRegistry::outputAdded, this, [this](const Layout::OutputInfo &info) { changeEngine().addOutput(info); });
    connect(
        &d->outputs, &OutputRegistry::outputChanged, this, [this](const Layout::OutputInfo &info) { changeEngine().updateOutput(info); });
    connect(&d->outputs, &OutputRegistry::outputRemoved, this, [this](const QString &name) { changeEngine().removeOutput(name); });
    connect(&d->outputs, &OutputRegistry::activeOutputChanged, this, &KonveyorEffect::followActiveOutput);
}

void KonveyorEffect::connectDesktopSync()
{
    connect(&d->desktops, &DesktopSync::workspaceActivatedByUser, this,
        [this](const QString &output, int index) { changeEngine().focusWorkspace(output, index); });
    connect(&d->desktops, &DesktopSync::windowMovedToWorkspaceByUser, this, [this](Layout::WindowId id, int index) {
        changeEngine().perform({QStringLiteral("move-window-to-workspace"), {QString::number(index)}, {}}, id);
    });
}

void KonveyorEffect::onWindowAdded(Layout::WindowId id, KWin::Window *window)
{
    const auto restore = d->minimizedPlacements.constFind(window);
    const std::optional<Layout::RestorePlacement> placement
        = restore == d->minimizedPlacements.constEnd() ? std::nullopt : std::optional(*restore);
    d->minimizedPlacements.remove(window);
    changeEngine().addWindow(id, d->windows.propertiesOf(window), outputNameOf(window), Layout::ActivationPolicy::Smart, placement);
    connect(window, &KWin::Window::frameGeometryChanged, this, [this, id] {
        placeMonitorOverlays(id);
        placeMonitorPanels(id);
    });
    connect(window, &KWin::Window::fullScreenChanged, this, [this, id, window] {
        placeMonitorOverlays(id);
        if (d->monitorOverlays.contains(id)) {
            runMonitorOverlay({QStringLiteral("update-fullscreen"), QString::number(id),
                window->isFullScreen() ? QStringLiteral("1") : QStringLiteral("0")});
        }
    });
    placeMonitorOverlays(id);
    if (window == KWin::workspace()->activeWindow()) {
        followActiveWindow();
    }
}

void KonveyorEffect::observeMonitorOverlay(KWin::Window *window)
{
    static const QRegularExpression pattern(QStringLiteral("^Konveyor Monitor (Overlay|Panel) (\\d+) ([0-2])$"));
    if (!window) {
        return;
    }
    const QRegularExpressionMatch match = pattern.match(window->caption());
    if (match.hasMatch()) {
        const bool panel = match.captured(1) == QLatin1String("Panel");
        const Layout::WindowId target = match.captured(2).toULongLong();
        const int slot = match.captured(3).toInt();
        const auto &windows = panel ? d->monitorPanels : d->monitorOverlays;
        if (windows.value(target).value(slot) == window) {
            return;
        }
    }
    forgetMonitorOverlay(window);
    if (!match.hasMatch()) {
        return;
    }
    const bool panel = match.captured(1) == QLatin1String("Panel");
    const Layout::WindowId target = match.captured(2).toULongLong();
    const int slot = match.captured(3).toInt();
    KWin::Window *targetWindow = d->windows.windowOf(target);
    if (!targetWindow) {
        return;
    }
    attachMonitorWindow(window, targetWindow);
    if (panel) {
        d->monitorPanels[target].insert(slot, window);
        connect(window, &KWin::Window::frameGeometryChanged, this, [this, target] { placeMonitorPanels(target); });
        placeMonitorPanels(target);
    } else {
        d->monitorOverlays[target].insert(slot, window);
        connect(window, &KWin::Window::frameGeometryChanged, this, [this, target] { placeMonitorOverlays(target); });
        placeMonitorOverlays(target);
    }
}

void KonveyorEffect::forgetMonitorOverlay(KWin::Window *window)
{
    bool forgotten = removeMonitorWindow(d->monitorOverlays, window, [this](Layout::WindowId id) { placeMonitorOverlays(id); });
    forgotten = removeMonitorWindow(d->monitorPanels, window, [this](Layout::WindowId id) { placeMonitorPanels(id); }) || forgotten;
    if (forgotten) {
        detachMonitorWindow(window);
    }
}

void KonveyorEffect::placeMonitorOverlays(Layout::WindowId id)
{
    if (d->placingMonitorOverlays.contains(id)) {
        return;
    }
    KWin::Window *target = d->windows.windowOf(id);
    const auto overlays = d->monitorOverlays.value(id);
    if (!target || overlays.isEmpty()) {
        return;
    }
    d->placingMonitorOverlays.insert(id);
    qreal width = 0;
    for (int slot = 0; slot < 3; ++slot) {
        if (KWin::Window *overlay = overlays.value(slot)) {
            width += overlay->frameGeometry().width();
        }
    }
    qreal x = target->frameGeometry().x() + (target->frameGeometry().width() - width) / 2;
    const qreal y = target->frameGeometry().y();
    for (int slot = 0; slot < 3; ++slot) {
        if (KWin::Window *overlay = overlays.value(slot)) {
            const QPointF position(qRound(x), qRound(y));
            if (overlay->frameGeometry().topLeft() != position) {
                overlay->move(position);
            }
            x += overlay->frameGeometry().width();
        }
    }
    d->placingMonitorOverlays.remove(id);
}

void KonveyorEffect::placeMonitorPanels(Layout::WindowId id)
{
    if (d->placingMonitorPanels.contains(id)) {
        return;
    }
    KWin::Window *target = d->windows.windowOf(id);
    const auto panels = d->monitorPanels.value(id);
    if (!target || panels.isEmpty()) {
        return;
    }
    d->placingMonitorPanels.insert(id);
    for (KWin::Window *panel : panels) {
        if (!panel) {
            continue;
        }
        const QPointF position(qRound(target->frameGeometry().x() + (target->frameGeometry().width() - panel->frameGeometry().width()) / 2),
            qRound(target->frameGeometry().y() + (target->frameGeometry().height() - panel->frameGeometry().height()) / 2));
        if (panel->frameGeometry().topLeft() != position) {
            panel->move(position);
        }
    }
    d->placingMonitorPanels.remove(id);
}

}
