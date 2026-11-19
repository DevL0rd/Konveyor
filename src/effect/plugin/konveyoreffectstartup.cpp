#include "plugin/konveyoreffect_p.h"

namespace Konveyor
{

void KonveyorEffect::handleBind(const Config::Bind &bind)
{
    ++d->bindCount;
    d->lastBindKey = Config::bindKeyLabel(bind);
    d->lastBindAction = bind.action.name;
    if (!Config::bindActsOnPointerOutput(bind, d->config.config().input)) {
        performAction(bind.action);
        return;
    }
    const QString output = outputNameAt(KWin::effects->cursorPos());
    if (!output.isEmpty() && readEngine().focusedOutput() != output) {
        changeEngine().focusOutput(output);
    }
    performAction(bind.action);
}

void KonveyorEffect::connectTimers()
{
    d->flushTimer.setSingleShot(true);
    d->flushTimer.setInterval(0);
    connect(&d->flushTimer, &QTimer::timeout, this, &KonveyorEffect::flush);
    d->animationTimer.setInterval(animationIntervalMs);
    connect(&d->animationTimer, &QTimer::timeout, this, &KonveyorEffect::stepAnimations);
    d->memorySaveTimer.setSingleShot(true);
    d->memorySaveTimer.setInterval(memorySaveDelayMs);
    connect(&d->memorySaveTimer, &QTimer::timeout, this, [this] { d->memoryStore.save(readEngine().windowMemory()); });
    d->pointerFocusTimer.setSingleShot(true);
    connect(&d->pointerFocusTimer, &QTimer::timeout, this, &KonveyorEffect::followPointerFocus);
    d->edgeHoverTimer.setSingleShot(true);
    connect(&d->edgeHoverTimer, &QTimer::timeout, this, &KonveyorEffect::switchColumnAtEdge);
    d->peekTimer.setSingleShot(true);
    connect(&d->peekTimer, &QTimer::timeout, this, [this] { runPeekStep(d->peekKey.delayPassed(!peekBlocked())); });
    d->startupRulesTimer.setSingleShot(true);
    d->startupRulesTimer.setTimerType(Qt::PreciseTimer);
    connect(&d->startupRulesTimer, &QTimer::timeout, this, [this] { changeEngine().refreshRules(); });
    d->startupRulesTimer.start(std::chrono::ceil<std::chrono::milliseconds>(readEngine().timeUntilStartupEnds()));
}

void KonveyorEffect::connectKontrolPanelWindows()
{
    connect(KWin::workspace(), &KWin::Workspace::windowAdded, this, [this](KWin::Window *window) {
        if (!window || window->resourceClass() != QLatin1String("konveyor-kontrol-panel")) {
            return;
        }
        if (readEngine().isOverviewOpen() && !d->config.config().overview.allowKontrolPanel) {
            window->closeWindow();
            return;
        }
        if (!d->kontrolPanelWindows.contains(window)) {
            d->kontrolPanelWindows.append(window);
        }
        if (KWin::effects && KWin::effects->isMouseInterception()) {
            KWin::effects->stopMouseInterception(this);
        }
    });
    connect(KWin::effects, &KWin::EffectsHandler::windowDeleted, this, [this](KWin::EffectWindow *window) {
        d->visibility.forget(window);
        if (window && d->kontrolPanelWindows.removeAll(window->window()) > 0 && d->kontrolPanelWindows.isEmpty() && overviewInputActive()
            && KWin::effects && !KWin::effects->isMouseInterception()) {
            KWin::effects->startMouseInterception(this, Qt::ArrowCursor);
        }
    });
}

}
