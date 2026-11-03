#include "decorations/eventeffects.h"

#include <effect/effect.h>
#include <effect/effectwindow.h>
#include <scene/item.h>
#include <scene/windowitem.h>
#include <window.h>

#include <algorithm>
#include <cmath>

namespace Konveyor
{

namespace
{

constexpr double kWhileHold = 0.25;
constexpr double kWhileFade = 0.35;
constexpr double kWhileRepeat = 1.0;
constexpr double kNeighborDistance = 32.0;
constexpr double kRippleSpreadPixelsPerSecond = 1800.0;
constexpr double kSmallestWidth = 4.0;

bool nextInLayout(int columnDelta, int tileDelta)
{
    return std::abs(columnDelta) == 1 || (columnDelta == 0 && std::abs(tileDelta) == 1);
}

QRectF frameOf(const QPointer<KWin::Window> &window, const QRectF &fallback)
{
    return window ? QRectF(window->frameGeometry()) : fallback;
}

}

EventEffects::EventEffects(const AccentColor &accent, BorderLayerPainter &painter, StyledBorders &styles, Render::SignalBus &bus)
    : m_accent(accent)
    , m_painter(painter)
    , m_styles(styles)
    , m_bus(bus)
{
    m_clock.start();
}

EventEffects::~EventEffects() = default;

double EventEffects::seconds() const
{
    return static_cast<double>(m_clock.nsecsElapsed()) / 1e9;
}

void EventEffects::setReduceMotion(bool reduceMotion)
{
    m_reduceMotion = reduceMotion;
    if (holdsStill()) {
        m_plays.clear();
    }
}

void EventEffects::setOnBattery(bool onBattery)
{
    m_onBattery = onBattery;
    if (holdsStill()) {
        m_plays.clear();
    }
}

const EventEffects::Known *EventEffects::known(Layout::WindowId id) const
{
    const auto found = std::ranges::find_if(m_known, [id](const Known &window) { return window.id == id; });
    return found == m_known.end() ? nullptr : &*found;
}

void EventEffects::sync(const QList<Window> &windows)
{
    std::vector<Known> before = std::move(m_known);
    m_known.clear();
    m_known.reserve(static_cast<std::size_t>(windows.size()));
    std::optional<Layout::WindowId> focused;
    for (const Window &window : windows) {
        const Layout::WindowState &state = *window.state;
        const bool fullscreen
            = state.sizingMode == Layout::WindowMode::Fullscreen || state.requestedSizingMode == Layout::WindowMode::Fullscreen;
        const bool shown = state.visible && state.onActiveWorkspace && !window.window->isMinimized();
        m_known.push_back({window.window, state.id, state.output, state.workspace, state.columnIndex, state.tileIndex, state.isFloating,
            fullscreen, shown, state.isFocused, state.isUrgent, state.border.enabled ? state.border.width : 0.0, state.focusRing.width,
            window.radius, state.eventEffects});
        if (state.isFocused) {
            focused = state.id;
        }
    }
    updateWanted();
    refreshTargets();
    syncFocus(focused);
    syncUrgent();
    m_seeded = true;
}

void EventEffects::start(Config::EventTrigger trigger, const Known &subject, const Pair &pair)
{
    const std::optional<Config::EventEffect> &effect = subject.effects.of(trigger);
    if (!effect || holdsStill() || !subject.shown || !subject.window) {
        return;
    }
    const Config::BorderStyle *style = m_styles.styleNamed(effect->style);
    const bool covered = std::ranges::any_of(
        m_known, [&subject](const Known &window) { return window.output == subject.output && window.shown && window.fullscreen; });
    if (!style || covered) {
        return;
    }
    Play play;
    play.trigger = trigger;
    play.mode = effect->mode;
    play.subject = subject.id;
    play.style = *style;
    play.subjectWindow = subject.window;
    play.scene.subject = QRectF(subject.window->frameGeometry());
    play.scene.point = pair.point;
    if (pair.from && pair.to) {
        play.fromWindow = pair.from->window;
        play.toWindow = pair.to->window;
        play.scene.from = frameOf(pair.from->window, QRectF());
        play.scene.to = frameOf(pair.to->window, QRectF());
    }
    addTargets(play, subject, *effect);
    play.startedAt = seconds();
    play.lastFired = play.startedAt;
    play.duration = Render::styleSeconds(play.style, play.scene, effect->duration);
    if (!effect->duration) {
        const auto later = [](const Target &a, const Target &b) { return a.delay < b.delay; };
        play.duration += std::ranges::max_element(play.targets, later)->delay;
    }
    m_plays.push_back(std::move(play));
    updateClaims();
}

bool EventEffects::sharesSpace(const Known &window, const Known &subject)
{
    return window.shown && window.window && window.output == subject.output && window.workspace == subject.workspace;
}

bool EventEffects::reaches(
    const Known &window, const Known &subject, const Config::EventEffect &effect, const QRectF &own, const QRectF &frame)
{
    if (window.id == subject.id || effect.reach == Config::EventReach::All) {
        return true;
    }
    if (effect.reach != Config::EventReach::Neighbors) {
        return false;
    }
    const bool tiled = !window.floating && !subject.floating;
    return Render::distanceBetween(own, frame) <= kNeighborDistance
        || (tiled && nextInLayout(window.column - subject.column, window.tile - subject.tile));
}

void EventEffects::addTargets(Play &play, const Known &subject, const Config::EventEffect &effect)
{
    const bool hasWave = std::ranges::any_of(
        play.style.layers, [](const Config::BorderLayer &layer) { return layer.kind == Config::BorderLayerKind::Wave; });
    const QRectF own = play.scene.subject;
    const QPointF waveFrom = play.scene.from ? play.scene.from->center() : own.center();
    const QPointF waveTo = play.scene.to ? play.scene.to->center() : own.center();
    for (const Known &window : m_known) {
        if (!sharesSpace(window, subject)) {
            continue;
        }
        const QRectF frame(window.window->frameGeometry());
        const bool reached = reaches(window, subject, effect, own, frame);
        const bool crossed = hasWave && (window.id == subject.id || Render::segmentCrosses(waveFrom, waveTo, frame));
        if (!reached && !crossed) {
            continue;
        }
        Target &target = play.targets.emplace_back();
        target.id = window.id;
        target.waveOnly = !reached;
        target.delay = Render::distanceBetween(own, frame) / kRippleSpreadPixelsPerSecond;
        placeTarget(target, window, play);
        if (reached) {
            play.scene.farthest = std::max(play.scene.farthest, Render::farthestDistance(own, frame));
        }
    }
}

void EventEffects::placeTarget(Target &target, const Known &window, const Play &play) const
{
    target.window = window.window;
    target.radius = window.radius;
    target.inset = window.inset;
    target.width = std::max(window.ringWidth, kSmallestWidth);
    target.outerReach = target.inset;
    for (const Config::BorderLayer &layer : play.style.layers) {
        target.outerReach = std::max(target.outerReach, target.inset + Render::layerOuterReach(layer, target.width));
    }
    target.innerReach = std::max({window.radius.topLeft, window.radius.topRight, window.radius.bottomRight, window.radius.bottomLeft});
    placeBorderCarrier(target.carrier, window.window, target.outerReach, target.innerReach);
}

void EventEffects::refreshTargets()
{
    for (Play &play : m_plays) {
        std::erase_if(play.targets, [this](const Target &target) {
            const Known *window = known(target.id);
            return !window || !window->shown || window->fullscreen;
        });
        for (Target &target : play.targets) {
            placeTarget(target, *known(target.id), play);
        }
    }
    std::erase_if(m_plays, [](const Play &play) { return play.targets.empty(); });
}

void EventEffects::remove(Layout::WindowId id)
{
    m_urgent.remove(id);
    for (Play &play : m_plays) {
        std::erase_if(play.targets, [id](const Target &target) { return target.id == id; });
    }
    std::erase_if(m_plays, [](const Play &play) { return play.targets.empty(); });
}

double EventEffects::endsAt(const Play &play) const
{
    return play.mode == Config::EventMode::While ? play.lastFired + kWhileHold + kWhileFade : play.startedAt + play.duration;
}

QList<BorderLayerPainter::Layer> EventEffects::layersFor(const Play &play, Target &target, const QRectF &frame, double opacity)
{
    const double now = seconds();
    const double elapsed = now - play.startedAt;
    const bool whileMode = play.mode == Config::EventMode::While;
    const double standing = whileMode ? std::clamp(std::min(elapsed, endsAt(play) - now) / kWhileFade, 0.0, 1.0)
                                      : Render::standingEnvelope(elapsed, play.duration);
    target.motions.resize(static_cast<std::size_t>(play.style.layers.size()));
    const LayerPass pass {play, target, frame, opacity, standing, now, elapsed};
    QList<BorderLayerPainter::Layer> layers;
    for (qsizetype index = 0; index < play.style.layers.size(); ++index) {
        appendLayer(layers, pass, index);
    }
    return layers;
}

void EventEffects::appendLayer(QList<BorderLayerPainter::Layer> &layers, const LayerPass &pass, qsizetype index)
{
    const Config::BorderLayer &layer = pass.play.style.layers[index];
    Render::LayerMotion &motion = pass.target.motions[static_cast<std::size_t>(index)];
    const bool reactive = Render::isReactive(layer);
    if (reactive) {
        Render::advanceMotion(motion, layer, m_bus.frame(pass.target.id), pass.now, pass.target.width);
    }
    const Config::BorderLayer *config = reactive ? &motion.resolved : &layer;
    const Render::LayerMotion *moved = reactive ? &motion : nullptr;
    const BorderLayerPainter::Layer drawn {
        config, resolvedColors(m_accent, layer.colors), 0.0, config->opacity * pass.opacity, pass.now, moved, std::nullopt};
    if (Render::isOneShotLayer(layer.kind)) {
        appendOneShot(layers, pass, layer, drawn);
    } else if (!pass.target.waveOnly) {
        BorderLayerPainter::Layer standing = drawn;
        standing.phase = reactive ? motion.phase : Render::layerPhase(layer, pass.now);
        standing.opacity *= pass.standing;
        layers.append(standing);
    }
}

void EventEffects::appendOneShot(
    QList<BorderLayerPainter::Layer> &layers, const LayerPass &pass, const Config::BorderLayer &layer, BorderLayerPainter::Layer drawn)
{
    if (pass.target.waveOnly && layer.kind != Config::BorderLayerKind::Wave) {
        return;
    }
    const bool whileMode = pass.play.mode == Config::EventMode::While;
    const double delay = layer.kind == Config::BorderLayerKind::Ripple ? pass.target.delay : 0.0;
    drawn.event = whileMode ? Render::restingFrame(*drawn.config, pass.play.scene, pass.frame, pass.elapsed, kWhileRepeat)
                            : Render::oneShotFrame(*drawn.config, pass.play.scene, pass.frame, pass.elapsed - delay);
    if (drawn.event) {
        drawn.opacity *= whileMode ? pass.standing : 1.0;
        layers.append(drawn);
    }
}

void EventEffects::updateClaims()
{
    QHash<Layout::WindowId, Render::SignalSet> wanted;
    for (const Play &play : m_plays) {
        const Render::SignalSet style = Render::signalsOf(play.style);
        for (const Target &target : play.targets) {
            wanted[target.id] |= style;
        }
    }
    for (const Layout::WindowId id : std::as_const(m_claimed)) {
        if (!wanted.contains(id)) {
            m_bus.setWanted(Render::SignalClient::Events, id, {});
        }
    }
    m_claimed.clear();
    for (auto it = wanted.cbegin(); it != wanted.cend(); ++it) {
        m_bus.setWanted(Render::SignalClient::Events, it.key(), it.value());
        m_claimed.insert(it.key());
    }
}

void EventEffects::paint(Layout::WindowId id, const StyledBorders::PaintRequest &request)
{
    const QRectF frame(request.window->window()->frameGeometry());
    const double opacity = request.window->opacity() * request.data.opacity();
    for (Play &play : m_plays) {
        const auto target = std::ranges::find_if(play.targets, [id](const Target &candidate) { return candidate.id == id; });
        if (target == play.targets.end()) {
            continue;
        }
        play.scene.subject = frameOf(play.subjectWindow, play.scene.subject);
        if (play.fromWindow && play.toWindow) {
            play.scene.from = QRectF(play.fromWindow->frameGeometry());
            play.scene.to = QRectF(play.toWindow->frameGeometry());
        }
        const QList<BorderLayerPainter::Layer> layers = layersFor(play, *target, frame, opacity);
        if (!layers.isEmpty()) {
            m_painter.draw(borderPaintTarget(request), {frame.size(), target->radius, target->inset, target->width}, layers);
        }
    }
}

void EventEffects::scheduleFrames()
{
    const double now = seconds();
    if (std::erase_if(m_plays, [this, now](const Play &play) { return now >= endsAt(play); }) > 0) {
        updateClaims();
    }
    for (Play &play : m_plays) {
        for (Target &target : play.targets) {
            if (target.window) {
                placeBorderCarrier(target.carrier, target.window, target.outerReach, target.innerReach);
            }
        }
    }
}

}
