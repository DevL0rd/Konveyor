#include "plugin/konveyoreffect_p.h"

namespace Konveyor
{

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
        [this](qint32 id) { return handleTouchUp(id); },
        [this] {
            d->gestures.touchCancel();
            endTitlebarDrag();
            scheduleFlush();
        },
        [this] { routeGesture(d->gestures.resetTouches()); },
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
    connect(&d->windows, &WindowRegistry::sizeCommitted, this, [this](Layout::WindowId id, const QSizeF &size) {
        KWin::Window *window = d->windows.windowOf(id);
        if (window && adoptFloatingGeometry(id, window)) {
            scheduleFlush();
            return;
        }
        const bool acknowledgesRequest = d->applier.isEchoOfAppliedSize(id, size);
        const bool resizedByUser = !d->applier.isApplying() && window && window->isInteractiveResize();
        if (acknowledgesRequest || resizedByUser) {
            changeEngine().windowSizeCommitted(id, size);
        }
        scheduleFlush();
    });
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
    connect(&d->outputs, &OutputRegistry::activeOutputChanged, this, [this](const QString &name) { changeEngine().focusOutput(name); });
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
}

}
