#include "plugin/konveyoreffect_p.h"

namespace Konveyor
{

KonveyorEffect::KonveyorEffect()
{
    d = std::make_unique<Private>(makeHooks(), [this](const Config::Bind &bind) {
        ++d->bindCount;
        d->lastBindKey = Config::bindKeyLabel(bind);
        d->lastBindAction = bind.action.name;
        changeEngine().perform(bind.action);
    });
    d->flushTimer.setSingleShot(true);
    d->flushTimer.setInterval(0);
    connect(&d->flushTimer, &QTimer::timeout, this, &KonveyorEffect::flush);
    d->animationTimer.setInterval(animationIntervalMs);
    connect(&d->animationTimer, &QTimer::timeout, this, &KonveyorEffect::stepAnimations);
    connect(&d->config, &ConfigManager::configChanged, this, &KonveyorEffect::applyConfig);
    connect(&d->accent, &AccentColor::changed, this, &KonveyorEffect::scheduleFlush);
    connectRegistries();
    connectDesktopSync();
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
    MinimizeRule::apply(false);
    Config::HotCorners disabled;
    disabled.enabled = false;
    applyHotCorners(disabled);
}

bool KonveyorEffect::isActive() const
{
    return d->animating;
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
    d->applier.apply(states);
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

void KonveyorEffect::updateDecorations(const QList<Layout::WindowState> &states)
{
    QSet<Layout::WindowId> present;
    for (const Layout::WindowState &state : states) {
        present.insert(state.id);
    }
    d->decorations.retainOnly(present);
    for (const Layout::WindowState &state : states) {
        if (KWin::Window *window = d->windows.windowOf(state.id)) {
            d->decorations.update(window, state, outputGeometryOf(window), outputScaleOf(window));
        }
    }
}

void KonveyorEffect::applyConfig(const Config::Config &config)
{
    d->clock.applyConfig(config.animations);
    changeEngine().setConfig(config);
    d->windows.reevaluate();
    d->shortcuts.setBinds(config.binds);
    applyHotCorners(config.gestures.hotCorners);
    d->plasmaShell.setHideDesktopWidgets(config.hideDesktopWidgets);
    d->plasmaShell.setFillPanels(config.fillPanelsOnMaximize);
    MinimizeRule::apply(config.disableMinimize);
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
    hooks.focusWindow = [this](Layout::WindowId id) {
        d->focusRequest = id;
        scheduleFlush();
    };
    return hooks;
}

}
