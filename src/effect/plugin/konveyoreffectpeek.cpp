#include "plugin/konveyoreffect_p.h"

#include "config/log.h"

#include <scene/windowitem.h>

namespace Konveyor
{

namespace
{

constexpr std::chrono::milliseconds peekFadeDuration {120};

bool isMetaKey(int key)
{
    return key == Qt::Key_Meta || key == Qt::Key_Super_L || key == Qt::Key_Super_R;
}

bool keepsShowingInPeek(const KWin::Window *window)
{
    return window->isUnmanaged() || window->isDock() || window->isDesktop() || window->belongsToDesktop() || window->isInputMethod()
        || window->isLockScreen() || window->isNotification() || window->isCriticalNotification() || window->isOnScreenDisplay();
}

bool fadesInPeek(KWin::Window *window, const KWin::Window *kept, const QString &output)
{
    if (window == kept || window->isDeleted() || keepsShowingInPeek(window) || (kept && kept->hasTransient(window, true))) {
        return false;
    }
    return window->isOnCurrentDesktop() && window->isOnCurrentActivity() && !window->isMinimized() && !window->isHidden()
        && !window->isHiddenByShowDesktop() && window->readyForPainting() && outputNameOf(window) == output;
}

void setKeptVisible(PeekedWindow &faded, bool visible)
{
    KWin::WindowItem *item = faded.window ? faded.window->windowItem() : nullptr;
    if (!item || faded.keptVisible == visible) {
        return;
    }
    faded.keptVisible = visible;
    if (visible) {
        item->refVisible(KWin::WindowItem::PAINT_DISABLED_BY_HIDDEN);
    } else {
        item->unrefVisible(KWin::WindowItem::PAINT_DISABLED_BY_HIDDEN);
    }
}

Anim::Animation fadeTo(const Anim::Clock &clock, const std::optional<Anim::Animation> &current, double to)
{
    const double from = current ? current->value() : 1.0 - to;
    return Anim::Animation::ease(clock, from, to, 0.0, peekFadeDuration, Anim::Curve(Config::EasingCurve::EaseOutCubic));
}

}

bool KonveyorEffect::handlePeekKey(int key, bool pressed, Qt::KeyboardModifiers modifiers)
{
    if (!d->config.config().experiments.peekDesktop.enabled) {
        return false;
    }
    if (!isMetaKey(key)) {
        if (pressed) {
            runPeekStep(d->peekKey.otherKeyPressed());
        }
        return false;
    }
    const bool alone = !(modifiers & (Qt::ControlModifier | Qt::ShiftModifier | Qt::AltModifier));
    runPeekStep(pressed ? d->peekKey.metaPressed(alone) : d->peekKey.metaReleased(KWin::waylandServer()->seat()->isDragPointer()));
    return !pressed && d->peekKey.takePeekedWhileHeld();
}

bool KonveyorEffect::handlePeekPointer(bool used)
{
    if (used) {
        runPeekStep(d->peekKey.pointerUsed());
    }
    return d->peekKey.isPeeking();
}

void KonveyorEffect::runPeekStep(Layout::PeekKey::Step step)
{
    switch (step) {
    case Layout::PeekKey::Step::Arm:
        d->peekTimer.start(d->config.config().experiments.peekDesktop.delayMs);
        break;
    case Layout::PeekKey::Step::Disarm:
        d->peekTimer.stop();
        break;
    case Layout::PeekKey::Step::Begin:
        beginPeek();
        break;
    case Layout::PeekKey::Step::End:
        d->peekTimer.stop();
        endPeek();
        break;
    case Layout::PeekKey::Step::None:
        break;
    }
}

bool KonveyorEffect::peekBlocked() const
{
    const KWin::Window *active = KWin::workspace()->activeWindow();
    return !d->config.config().experiments.peekDesktop.enabled || readEngine().isOverviewOpen()
        || KWin::effects->hasActiveFullScreenEffect() || KWin::waylandServer()->isScreenLocked() || (active && active->isFullScreen());
}

void KonveyorEffect::beginPeek()
{
    finishPeek();
    const QPointF pointer = KWin::effects->cursorPos();
    const QString output = outputNameAt(pointer);
    KWin::Window *active = KWin::workspace()->activeWindow();
    DesktopPeek peek;
    peek.output = output;
    peek.kept = active && outputNameOf(active) == output ? active : nullptr;
    for (KWin::Window *window : KWin::workspace()->stackingOrder()) {
        if (fadesInPeek(window, peek.kept, output)) {
            window->setHiddenByShowDesktop(true);
            PeekedWindow &faded = peek.faded.emplace_back(PeekedWindow {window});
            setKeptVisible(faded, true);
        }
    }
    peek.aside.begin(peek.kept && peek.kept->frameGeometry().contains(pointer));
    peek.fade = fadeTo(d->clock, std::nullopt, 0.0);
    d->peek = std::move(peek);
    qCInfo(lcKonveyor) << "konveyor: peeking at the desktop on" << output << "faded" << d->peek->faded.size() << "windows";
    scheduleFlush();
}

void KonveyorEffect::endPeek()
{
    if (!d->peek || d->peek->restoring) {
        return;
    }
    DesktopPeek &peek = *d->peek;
    peek.restoring = true;
    for (PeekedWindow &faded : peek.faded) {
        if (faded.window) {
            faded.window->setHiddenByShowDesktop(false);
        }
        setKeptVisible(faded, false);
    }
    peek.fade = fadeTo(d->clock, peek.fade, 1.0);
    if (peek.slide) {
        peek.slide = Anim::Animation(d->clock, peek.slide->value(), 0.0, 0.0, d->config.config().animations.windowMovement);
    }
    const KWin::Window *active = KWin::workspace()->activeWindow();
    if (peek.kept && (!active || active->isDesktop())) {
        KWin::workspace()->activateWindow(peek.kept);
    }
    qCInfo(lcKonveyor) << "konveyor: peek over on" << peek.output;
    scheduleFlush();
}

void KonveyorEffect::trackPeekPointer(const QPointF &position)
{
    if (!d->peek || d->peek->restoring || !d->peek->kept || d->peek->asideFrame || KWin::waylandServer()->seat()->isDragPointer()) {
        return;
    }
    DesktopPeek &peek = *d->peek;
    const QList<Layout::OutputInfo> outputs = d->outputs.outputs();
    const auto output = std::ranges::find(outputs, peek.output, &Layout::OutputInfo::name);
    if (output == outputs.end()) {
        return;
    }
    const QRectF frame = peek.kept->frameGeometry();
    const std::optional<Layout::PeekSide> side = peek.aside.pointerAt(position, frame, output->workArea);
    if (!side) {
        return;
    }
    peek.asideFrame = Layout::peekFrame(frame, output->workArea, *side, peek.kept->minSize().width(), d->config.config().layout.gaps);
    peek.slide = Anim::Animation(d->clock, 0.0, 1.0, 0.0, d->config.config().animations.windowMovement);
    scheduleFlush();
}

void KonveyorEffect::placePeekedWindow(QList<Layout::WindowState> &states) const
{
    if (!d->peek || !d->peek->asideFrame || !d->peek->slide || !d->peek->kept) {
        return;
    }
    const std::optional<Layout::WindowId> id = d->windows.idOf(d->peek->kept);
    const auto state = std::ranges::find_if(states, [&id](const Layout::WindowState &each) { return each.id == id; });
    if (state == states.end()) {
        return;
    }
    const QRectF layout = WindowApplier::frameFor(*state);
    const QRectF &aside = *d->peek->asideFrame;
    const double progress = d->peek->slide->value();
    const QPointF position = layout.topLeft() + ((aside.topLeft() - layout.topLeft()) * progress);
    const QRectF placed(position, d->peek->restoring ? layout.size() : aside.size());
    const QPointF shift = placed.topLeft() - layout.topLeft();
    state->renderFrame = placed;
    state->targetFrame = placed;
    state->tabBar.rect.translate(shift);
    for (QRectF &tab : state->tabBar.tabRects) {
        tab.translate(shift);
    }
}

bool KonveyorEffect::stepPeek()
{
    if (!d->peek) {
        return false;
    }
    DesktopPeek &peek = *d->peek;
    const double alpha = peek.fade ? peek.fade->value() : 1.0;
    for (PeekedWindow &faded : peek.faded) {
        setDragFade(faded.window, alpha);
        if (peek.fade && peek.fade->isFinished() && !peek.restoring) {
            setKeptVisible(faded, false);
        }
    }
    const bool fading = peek.fade && !peek.fade->isFinished();
    const bool sliding = peek.slide && !peek.slide->isFinished();
    if (peek.restoring && !fading && !sliding) {
        finishPeek();
        return false;
    }
    return fading || sliding;
}

void KonveyorEffect::finishPeek()
{
    if (!d->peek) {
        return;
    }
    for (PeekedWindow &faded : d->peek->faded) {
        if (faded.window) {
            faded.window->setHiddenByShowDesktop(false);
        }
        setKeptVisible(faded, false);
        setDragFade(faded.window, std::nullopt);
    }
    d->peek.reset();
}

}
