#include "plugin/konveyoreffect_p.h"

#include <core/renderviewport.h>
#include <effect/effectwindow.h>

namespace Konveyor
{

KonveyorEffect::KonveyorEffect()
{
    d = std::make_unique<Private>(makeHooks(), [this](const Config::Bind &bind) {
        ++d->bindCount;
        d->lastBindKey = Config::bindKeyLabel(bind);
        d->lastBindAction = bind.action.name;
        performAction(bind.action);
    });
    d->flushTimer.setSingleShot(true);
    d->flushTimer.setInterval(0);
    connect(&d->flushTimer, &QTimer::timeout, this, &KonveyorEffect::flush);
    d->animationTimer.setInterval(animationIntervalMs);
    connect(&d->animationTimer, &QTimer::timeout, this, &KonveyorEffect::stepAnimations);
    d->memorySaveTimer.setSingleShot(true);
    d->memorySaveTimer.setInterval(memorySaveDelayMs);
    connect(&d->memorySaveTimer, &QTimer::timeout, this, [this] { d->memoryStore.save(readEngine().windowMemory()); });
    d->engine.setWindowMemory(d->memoryStore.load());
    connect(&d->config, &ConfigManager::configChanged, this, &KonveyorEffect::applyConfig);
    connect(&d->accent, &AccentColor::changed, this, &KonveyorEffect::scheduleFlush);
    connectRegistries();
    connectDesktopSync();
    connectOverviewSync();
    installInputFilter();
    startDBusService();
    d->windows.setWantsWindow([this](const Layout::WindowProperties &properties) { return readEngine().wantsWindow(properties); });
    d->windows.start();
    followActiveWindow();
    d->outputs.start();
    d->desktops.start();
    d->plasmaShell.start();
    d->config.start();
    scheduleFlush();
}

KonveyorEffect::~KonveyorEffect()
{
    if (d->memorySaveTimer.isActive()) {
        d->memoryStore.save(d->engine.windowMemory());
    }
    MinimizeRule::apply(false);
    Config::HotCorners disabled;
    disabled.enabled = false;
    applyHotCorners(disabled);
}

bool KonveyorEffect::isActive() const
{
    return d->animating || hasSpill();
}

bool KonveyorEffect::blocksDirectScanout() const
{
    return d->animating;
}

std::optional<QRectF> KonveyorEffect::spillHome(KWin::Window *window) const
{
    const std::optional<Layout::WindowId> id = window ? d->windows.idOf(window) : std::nullopt;
    const auto home = id ? d->homeOutputs.constFind(*id) : d->homeOutputs.constEnd();
    if (home == d->homeOutputs.constEnd()) {
        return std::nullopt;
    }
    const KWin::LogicalOutput *output = d->outputs.outputNamed(*home);
    if (!output) {
        return std::nullopt;
    }
    const QRectF homeRect = output->geometryF();
    const QRectF visible = window->visibleGeometry();
    if (homeRect.contains(visible)) {
        return std::nullopt;
    }
    for (const KWin::LogicalOutput *other : KWin::workspace()->outputs()) {
        if (other != output && visible.intersects(other->geometryF())) {
            return homeRect;
        }
    }
    return std::nullopt;
}

bool KonveyorEffect::hasSpill() const
{
    for (auto it = d->homeOutputs.constBegin(); it != d->homeOutputs.constEnd(); ++it) {
        if (KWin::Window *window = d->windows.windowOf(it.key()); window && spillHome(window)) {
            return true;
        }
    }
    return false;
}

void KonveyorEffect::prePaintWindow(KWin::RenderView *view, KWin::EffectWindow *w, KWin::WindowPrePaintData &data)
{
    if (spillHome(w->window())) {
        data.setTranslucent();
    }
    KWin::Effect::prePaintWindow(view, w, data);
}

void KonveyorEffect::paintWindow(const KWin::RenderTarget &renderTarget, const KWin::RenderViewport &viewport, KWin::EffectWindow *w,
    int mask, const KWin::Region &deviceRegion, KWin::WindowPaintData &data)
{
    const std::optional<QRectF> home = spillHome(w->window());
    if (!home) {
        KWin::Effect::paintWindow(renderTarget, viewport, w, mask, deviceRegion, data);
        return;
    }
    const KWin::Region clipped = deviceRegion & viewport.mapToDeviceCoordinatesAligned(KWin::RectF(*home));
    if (!clipped.isEmpty()) {
        KWin::Effect::paintWindow(renderTarget, viewport, w, mask, clipped, data);
    }
}

void KonveyorEffect::prePaintScreen(KWin::ScreenPrePaintData &data)
{
    KWin::Effect::prePaintScreen(data);
}

void KonveyorEffect::postPaintScreen()
{
    KWin::Effect::postPaintScreen();
}

Layout::Engine &KonveyorEffect::changeEngine()
{
    d->clock.clear();
    scheduleFlush();
    return d->engine;
}

const Layout::Engine &KonveyorEffect::readEngine() const
{
    d->clock.clear();
    return d->engine;
}

void KonveyorEffect::stepAnimations()
{
    changeEngine().tickAnimations();
    flush();
    d->animating = readEngine().isAnimating();
    if (!d->animating) {
        d->animationTimer.stop();
    }
}

void KonveyorEffect::scheduleFlush()
{
    if (!d->flushTimer.isActive()) {
        d->flushTimer.start();
    }
}

void KonveyorEffect::flush()
{
    const QList<Layout::WindowState> states = readEngine().windowStates();
    d->desktops.apply(readEngine().workspaceStates(), states);
    d->fullscreenGuard.update(states);
    d->applier.apply(states);
    updateHomeOutputs(states);
    acknowledgeSettledModeChanges(states);
    updateDecorations(states);
    d->fullscreenShade.update(states);
    applyFocusRequest();
    d->plasmaShell.update(states, d->outputs.orderedNames());
    d->animating = readEngine().isAnimating();
    if (d->animating && !d->animationTimer.isActive()) {
        d->animationTimer.start();
    }
}

void KonveyorEffect::acknowledgeSettledModeChanges(const QList<Layout::WindowState> &states)
{
    for (const Layout::WindowState &state : states) {
        KWin::Window *window = d->windows.windowOf(state.id);
        if (!window || state.sizingMode == state.requestedSizingMode) {
            continue;
        }
        const QSizeF size = window->frameGeometry().size();
        if (d->applier.isEchoOfAppliedSize(state.id, size)) {
            changeEngine().windowSizeCommitted(state.id, size);
        }
    }
}

void KonveyorEffect::updateHomeOutputs(const QList<Layout::WindowState> &states)
{
    d->homeOutputs.clear();
    for (const Layout::WindowState &state : states) {
        d->homeOutputs.insert(state.id, state.output);
    }
}

void KonveyorEffect::updateDecorations(const QList<Layout::WindowState> &states)
{
    QSet<Layout::WindowId> present;
    for (const Layout::WindowState &state : states) {
        present.insert(state.id);
    }
    d->decorations.retainOnly(present);
    for (const Layout::WindowState &state : states) {
        KWin::Window *window = d->windows.windowOf(state.id);
        const KWin::LogicalOutput *home = d->outputs.outputNamed(state.output);
        if (window && home) {
            d->decorations.update(window, state, home->geometryF(), home->scale());
        }
    }
}

void KonveyorEffect::applyConfig(const Config::Config &config)
{
    d->clock.applyConfig(config.animations);
    changeEngine().setConfig(config);
    d->gestures.setConfig(config.gestures);
    d->windows.reevaluate();
    d->shortcuts.setBinds(config.binds);
    applyHotCorners(config.gestures.hotCorners);
    d->plasmaShell.setHideDesktopWidgets(config.hideDesktopWidgets);
    d->plasmaShell.setFillPanels(config.fillPanelsOnMaximize);
    MinimizeRule::apply(config.disableMinimize);
    d->fullscreenGuard.setExperiments(config.experiments.preventFullscreenMinimize, config.experiments.preventFullscreenExit);
}

void KonveyorEffect::applyHotCorners(const Config::HotCorners &corners)
{
    for (const auto &[border, flag] : hotCornerFlags()) {
        const bool wanted = corners.enabled && corners.*flag;
        if (wanted == d->reservedCorners.contains(border)) {
            continue;
        }
        if (wanted) {
            KWin::effects->reserveElectricBorder(border, this);
            d->reservedCorners.insert(border);
        } else {
            KWin::effects->unreserveElectricBorder(border, this);
            d->reservedCorners.remove(border);
        }
    }
}

bool KonveyorEffect::borderActivated(KWin::ElectricBorder border)
{
    if (!d->reservedCorners.contains(border)) {
        return false;
    }
    changeEngine().setOverviewOpen(!readEngine().isOverviewOpen());
    return true;
}

Layout::Hooks KonveyorEffect::makeHooks()
{
    Layout::Hooks hooks;
    hooks.closeWindow = [this](Layout::WindowId id) {
        if (KWin::Window *window = d->windows.windowOf(id)) {
            window->closeWindow();
        }
    };
    hooks.spawn = [](const QString &command) { QProcess::startDetached(QStringLiteral("/bin/sh"), {QStringLiteral("-c"), command}); };
    hooks.compositorAction = [](const QString &name) {
        if (name == QLatin1String("show-hotkey-overlay")) {
            QProcess::startDetached(QStringLiteral("konveyor-cheatsheet"), {});
            return;
        }
        qInfo() << "konveyor: action left to KDE:" << name;
    };
    hooks.toggleOverview = [this] { showKdeOverview(d->engine.isOverviewOpen()); };
    hooks.setOverviewOpen = [this](bool open) { showKdeOverview(open); };
    hooks.windowMemoryChanged = [this] { d->memorySaveTimer.start(); };
    hooks.focusWindow = [this](Layout::WindowId id) {
        d->focusRequest = id;
        scheduleFlush();
    };
    return hooks;
}

}
