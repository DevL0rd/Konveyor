#include "plugin/konveyoreffect_p.h"

#include <core/renderviewport.h>
#include <effect/effectwindow.h>
#include <scene/shadowitem.h>
#include <scene/windowitem.h>

namespace Konveyor
{

namespace
{

void setDragFade(KWin::Window *window, std::optional<double> fade)
{
    KWin::WindowItem *item = window ? window->windowItem() : nullptr;
    if (!item) {
        return;
    }
    const double content = fade ? *fade * window->opacity() : 1.0;
    item->setOpacity(fade ? 1.0 : window->opacity());
    item->windowContainer()->setOpacity(content);
    if (KWin::ShadowItem *shadow = item->shadowItem()) {
        shadow->setOpacity(content);
    }
}

}

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
    d->startupRulesTimer.setSingleShot(true);
    d->startupRulesTimer.setTimerType(Qt::PreciseTimer);
    connect(&d->startupRulesTimer, &QTimer::timeout, this, [this] { changeEngine().refreshRules(); });
    d->startupRulesTimer.start(std::chrono::ceil<std::chrono::milliseconds>(readEngine().timeUntilStartupEnds()));
    d->engine.setWindowMemory(d->memoryStore.load());
    connect(&d->config, &ConfigManager::configChanged, this, &KonveyorEffect::applyConfig);
    connect(&d->accent, &AccentColor::changed, this, &KonveyorEffect::scheduleFlush);
    connectRegistries();
    connectDesktopSync();
    connectOverviewSync();
    installInputFilter();
    startDBusService();
    d->windows.setWantsWindow([this](const Layout::WindowProperties &properties) { return readEngine().wantsWindow(properties); });
    d->windows.setPlacementOrder([this](KWin::Window *first, KWin::Window *second) { return placedBefore(first, second); });
    d->config.start();
    d->outputs.start();
    d->desktops.start();
    d->handoff = LayoutHandoff::take();
    d->windows.start([this](KWin::Window *first, KWin::Window *second) { return d->handoff.comesBefore(first, second); });
    takeHandedOverHiddenPlacements();
    d->focusRequest.reset();
    followActiveWindow();
    d->plasmaShell.start();
    d->kontrolPanelStacking.start();
    scheduleFlush();
}

KonveyorEffect::~KonveyorEffect()
{
    LayoutHandoff::give(d->engine, d->windows, d->hiddenPlacements);
    disconnect(&d->windows, nullptr, this, nullptr);
    disconnect(&d->desktops, nullptr, this, nullptr);
    if (d->memorySaveTimer.isActive()) {
        d->memoryStore.save(d->engine.windowMemory());
    }
    if (d->draggedOpacity) {
        setDragFade(d->windows.windowOf(d->draggedOpacity->first), std::nullopt);
    }
    releaseQuickTiling();
    MinimizeRule::apply(false);
    for (const KWin::ElectricBorder border : std::as_const(d->reservedCorners)) {
        KWin::effects->unreserveElectricBorder(border, this);
    }
}

bool KonveyorEffect::placedBefore(KWin::Window *first, KWin::Window *second) const
{
    const auto key = [this](KWin::Window *window) {
        const std::optional<Layout::WindowId> id = d->windows.idOf(window);
        const auto hidden = d->hiddenPlacements.constFind(window);
        const std::optional<Layout::RestorePlacement> placement = id ? readEngine().placementOf(*id)
            : hidden != d->hiddenPlacements.constEnd()               ? std::optional(*hidden)
                                                                     : std::nullopt;
        return placement
            ? std::tuple(false, placement->workspace, placement->isFloating, placement->columnIndex, placement->tileIndex.value_or(0))
            : std::tuple(true, Layout::WorkspaceId(0), false, std::size_t(0), std::size_t(0));
    };
    return key(first) < key(second);
}

bool KonveyorEffect::isActive() const
{
    return d->animating || d->draggedOpacity || hasSpill();
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
    if (spillHome(w->window()) || isDragged(w->window())) {
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
    endMoveIntoFullscreen(states);
    updateHomeOutputs(states);
    d->desktops.apply(readEngine().workspaceStates(), states);
    d->fullscreenGuard.update(states);
    d->applier.apply(states);
    acknowledgeSettledModeChanges(states);
    updateDecorations(states);
    updateDropHint(states);
    updateDraggedOpacity(states);
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
        keepOnHomeOutput(state.id);
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

void KonveyorEffect::updateDropHint(const QList<Layout::WindowState> &states)
{
    const std::optional<Layout::WindowId> moving = readEngine().movingWindow();
    const QList<Layout::OutputState> outputs = readEngine().outputStates();
    const auto hinted = std::ranges::find_if(outputs, [](const Layout::OutputState &output) { return output.dropHint.has_value(); });
    const auto carrier = std::ranges::find_if(states, [&moving](const Layout::WindowState &state) { return state.id == moving; });
    KWin::Window *window = moving ? d->windows.windowOf(*moving) : nullptr;
    const KWin::LogicalOutput *output = hinted != outputs.end() ? d->outputs.outputNamed(hinted->name) : nullptr;
    if (!window || !output || carrier == states.end()) {
        d->decorations.hideDropHint();
        return;
    }
    d->decorations.showDropHint(*moving, window,
        {*hinted->dropHint, hinted->dropHintPaint, d->decorations.radiusFor(window, carrier->cornerRadius), output->geometryF(),
            output->scale()});
}

void KonveyorEffect::updateDraggedOpacity(const QList<Layout::WindowState> &states)
{
    const std::optional<Layout::WindowId> moving = readEngine().movingWindow();
    const auto state = std::ranges::find_if(states, [&moving](const Layout::WindowState &each) { return each.id == moving; });
    const std::optional<std::pair<Layout::WindowId, double>> wanted
        = state != states.end() ? std::optional(std::pair(state->id, state->renderAlpha)) : std::nullopt;
    if (d->draggedOpacity && (!wanted || wanted->first != d->draggedOpacity->first)) {
        setDragFade(d->windows.windowOf(d->draggedOpacity->first), std::nullopt);
    }
    if (wanted) {
        setDragFade(d->windows.windowOf(wanted->first), wanted->second);
    }
    d->draggedOpacity = wanted;
}

bool KonveyorEffect::isDragged(KWin::Window *window) const
{
    return d->draggedOpacity && window && d->windows.idOf(window) == d->draggedOpacity->first;
}

void KonveyorEffect::applyConfig(const Config::Config &config)
{
    d->clock.applyConfig(config.animations);
    changeEngine().setConfig(config);
    d->gestures.setConfig(config.gestures);
    d->windows.reevaluate();
    d->shortcuts.setBinds(config.binds);
    applyHotCorners(config);
    d->plasmaShell.setHideDesktopWidgets(config.hideDesktopWidgets);
    d->plasmaShell.setFillPanels(config.fillPanelsOnMaximize);
    MinimizeRule::apply(config.disableMinimize);
    d->fullscreenGuard.setExperiments(config.experiments.preventFullscreenMinimize, config.experiments.preventFullscreenExit);
}

void KonveyorEffect::applyHotCorners(const Config::Config &config)
{
    for (const auto &[border, flag] : hotCornerFlags()) {
        const bool wanted = cornerOn(config.gestures.hotCorners, flag)
            || std::ranges::any_of(config.outputs,
                [flag](const Config::OutputConfig &output) { return output.hotCorners && cornerOn(*output.hotCorners, flag); });
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
    const auto flags = hotCornerFlags();
    const auto corner = std::ranges::find(flags, border, &std::pair<KWin::ElectricBorder, bool Config::HotCorners::*>::first);
    const QString output = outputNameAt(KWin::effects->cursorPos());
    const Config::HotCorners &corners = hotCornersOn(d->config.config(), output);
    if (!d->reservedCorners.contains(border) || corner == flags.end() || !cornerOn(corners, corner->second)) {
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
    hooks.runWidgetTool = [](const QString &name, const QStringList &arguments) {
        if (const auto started = Tools::startWidgetTool(name, arguments); !started) {
            qWarning().noquote() << "konveyor:" << started.error();
            sendNotification(QStringLiteral("Konveyor: could not open %1").arg(name), started.error());
        }
    };
    hooks.compositorAction = [](const QString &name) {
        if (name == QLatin1String("show-hotkey-overlay")) {
            QProcess::startDetached(QStringLiteral(KONVEYOR_BINDIR "/konveyor-cheatsheet"), {});
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
