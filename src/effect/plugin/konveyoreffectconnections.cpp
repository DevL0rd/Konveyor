#include "plugin/konveyoreffect_p.h"

#include "config/log.h"

#include <keyboard_input.h>
#include <virtualdesktops.h>
#include <xkb.h>

namespace Konveyor
{

namespace
{

void runMonitorOverlay(const QStringList &arguments)
{
    const QString tool = QStringLiteral("monitor-overlay");
    if (const auto started = Tools::startWidgetTool(tool, arguments); !started && Tools::widgetToolInstalled(tool)) {
        qWarning().noquote() << "konveyor:" << started.error();
    }
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
    const auto pointerBind
        = [this](Config::BindTrigger trigger, Config::BindModifiers modifiers, Config::MouseButton button,
              Config::ScrollDirection direction) { return d->shortcuts.triggerPointerBind(trigger, modifiers, button, direction); };
    d->input = std::make_unique<InputFilter>(InputHandlers {
        pointerBind,
        [this](const QPointF &position, qint64 timestamp) { handlePointerMotion(position, timestamp); },
        [this] { endTitlebarDrag(); },
        [this](const QPointF &position) { return switchToTabUnderPointer(position); },
        [this](quint32 keycode, Config::BindModifiers modifiers, bool repeat) {
            const KWin::Xkb *xkb = KWin::input()->keyboard()->xkb();
            return d->shortcuts.triggerKeyPosition(keycode, modifiers, repeat, xkb->keymap(), xkb->currentLayout());
        },
        [this] { markMoveCancelled(); },
        [this] { return toggleDragFloating(); },
    });
    d->axisInput = std::make_unique<AxisFilter>(pointerBind);
    d->dragMotion = std::make_unique<DragMotionFilter>([this](const QPointF &position, qint64 timestampMs) {
        changeEngine().dataDragEdgeScroll(outputNameAt(position), position, timestampMs);
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
        [this] { handleTouchCancel(); },
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
        if (d->quickTiling && d->quickTiling->window == id) {
            releaseQuickTiling();
        }
        changeEngine().removeWindow(id);
        followActiveWindowAfterClose();
        d->decorations.remove(id);
        d->fullscreenShade.remove(id);
        d->applier.forget(id);
        d->homeOutputs.remove(id);
    });
    connect(&d->windows, &WindowRegistry::windowHiding, this, [this](Layout::WindowId id, KWin::Window *window) {
        if (const std::optional<Layout::RestorePlacement> placement = readEngine().placementOf(id)) {
            rememberHiddenPlacement(window, *placement);
        }
    });
    connect(&d->windows, &WindowRegistry::propertiesChanged, this, [this](Layout::WindowId id) {
        if (KWin::Window *window = d->windows.windowOf(id)) {
            changeEngine().updateWindowProperties(id, d->windows.propertiesOf(window));
        }
    });
    connect(&d->windows, &WindowRegistry::activeWindowChanged, this, &KonveyorEffect::onActiveWindowChanged);
}

void KonveyorEffect::connectWindowState()
{
    connect(&d->windows, &WindowRegistry::sizeCommitted, this, &KonveyorEffect::handleWindowSizeCommitted);
    connect(&d->windows, &WindowRegistry::fullscreenRequested, this, [this](Layout::WindowId id, bool fullscreen) {
        if (d->applier.isApplying()) {
            qCInfo(lcKonveyor) << "konveyor: fullscreen change for window" << id << "=" << fullscreen << "came from Konveyor itself";
        } else {
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
    if (!acknowledgesRequest && !resizedByUser && !d->applier.isApplying()) {
        qCInfo(lcKonveyor) << "konveyor: window" << id << (window ? window->resourceClass() : QString()) << "committed unrequested size"
                           << size << "resizable =" << (window && window->isResizable()) << "taken as app resize =" << resizedByApp
                           << "target =" << (state ? state->targetFrame : QRectF());
    }
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
    connect(&d->desktops, &DesktopSync::workspaceActivatedByUser, this, [this](const QString &output, int index) {
        qCInfo(lcKonveyor) << "konveyor: KDE switched to desktop" << index << "on" << output;
        changeEngine().focusWorkspace(output, index);
    });
    connect(&d->desktops, &DesktopSync::windowMovedToWorkspaceByUser, this, [this](Layout::WindowId id, int index) {
        qCInfo(lcKonveyor) << "konveyor: KDE moved window" << id << "to desktop" << index;
        changeEngine().perform({QStringLiteral("move-window-to-workspace"), {QString::number(index)}, {}}, id);
    });
}

void KonveyorEffect::rememberHiddenPlacement(KWin::Window *window, const Layout::RestorePlacement &placement)
{
    if (!d->hiddenPlacements.contains(window)) {
        connect(window, &QObject::destroyed, this, [this, window] { d->hiddenPlacements.remove(window); });
    }
    d->hiddenPlacements.insert(window, placement);
}

void KonveyorEffect::takeHandedOverHiddenPlacements()
{
    for (KWin::Window *window : KWin::workspace()->windows()) {
        if (d->windows.idOf(window)) {
            continue;
        }
        if (const std::optional<HandedOverPlacement> handed = d->handoff.takePlacement(window, readEngine())) {
            qCInfo(lcKonveyor) << "konveyor: keeping the place of hidden window" << window->resourceClass() << "through the reload";
            rememberHiddenPlacement(window, handed->placement);
        }
    }
}

void KonveyorEffect::adoptOntoKdeDesktop(Layout::WindowId id, KWin::Window *window)
{
    if (window->isOnAllDesktops() || window->desktops().size() != 1) {
        return;
    }
    const int desktop = static_cast<int>(KWin::VirtualDesktopManager::self()->desktops().indexOf(window->desktops().constFirst())) + 1;
    const std::optional<Layout::WindowState> state = readEngine().windowState(id);
    if (desktop < 1 || !state || state->workspaceIndex == desktop) {
        return;
    }
    qCInfo(lcKonveyor) << "konveyor: adopting" << window->resourceClass() << "onto workspace" << desktop << "to match its KDE desktop";
    changeEngine().perform(
        {QStringLiteral("move-window-to-workspace"), {QString::number(desktop)}, {{QStringLiteral("focus"), QStringLiteral("false")}}}, id);
}

void KonveyorEffect::onWindowAdded(Layout::WindowId id, KWin::Window *window)
{
    const auto restore = d->hiddenPlacements.constFind(window);
    const std::optional<Layout::RestorePlacement> placement
        = restore == d->hiddenPlacements.constEnd() ? std::nullopt : std::optional(*restore);
    d->hiddenPlacements.remove(window);
    if (const std::optional<HandedOverPlacement> handed = d->handoff.takePlacement(window, readEngine())) {
        qCInfo(lcKonveyor) << "konveyor: window" << id << window->resourceClass() << "takes a handed-over placement on" << handed->output;
        changeEngine().addWindow(id, d->windows.propertiesOf(window), handed->output, Layout::ActivationPolicy::NoFocus, handed->placement);
    } else {
        changeEngine().addWindow(id, d->windows.propertiesOf(window), outputNameOf(window), Layout::ActivationPolicy::Smart, placement);
        if (!placement && d->windows.isAdopting()) {
            adoptOntoKdeDesktop(id, window);
        }
    }
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

}
