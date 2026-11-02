#include "decorations/styledborders.h"

#include "decorations/accentcolor.h"

#include <core/region.h>
#include <core/renderviewport.h>
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

QColor resolvedColor(const AccentColor &accent, const Config::Paint &paint)
{
    return paint.source == Config::ColorSource::Explicit ? paint.color : accent.colorFor(paint.source);
}

constexpr int kWantedHoldMilliseconds = 1000;
constexpr double kOneShotRestCycles = 3.0;

bool layerHidden(const Config::BorderLayer &layer, const Render::SignalFrame &frameSignals, bool still)
{
    return still ? Render::hiddenWhenStill(layer) : Render::layerPresence(layer, frameSignals) <= 0.0F;
}

void stepMotion(Render::LayerMotion &motion, const Config::BorderLayer &layer, const Render::SignalFrame &frameSignals, double now,
    double width, bool still)
{
    if (still) {
        Render::poseMotion(motion, layer, now, width);
    } else {
        Render::advanceMotion(motion, layer, frameSignals, now, width);
    }
}

KWin::RegionF ringRegion(QSizeF size, double outer, double inner)
{
    const double w = size.width() + 2 * outer;
    const double h = size.height() + 2 * outer;
    const double band = outer + inner;
    KWin::RegionF region(KWin::RectF(0, 0, w, band));
    region = region.united(KWin::RectF(0, h - band, w, band));
    region = region.united(KWin::RectF(0, 0, band, h));
    return region.united(KWin::RectF(w - band, 0, band, h));
}

}

QList<QColor> resolvedColors(const AccentColor &accent, const QList<Config::Paint> &paints)
{
    QList<QColor> colors;
    colors.reserve(std::max<qsizetype>(paints.size(), 1));
    for (const Config::Paint &paint : paints) {
        colors.append(resolvedColor(accent, paint));
    }
    if (colors.isEmpty()) {
        colors.append(accent.colorFor(Config::ColorSource::SystemAccent));
    }
    return colors;
}

BorderLayerPainter::Target borderPaintTarget(const StyledBorders::PaintRequest &request)
{
    const double scale = request.viewport.scale();
    const QPointF origin = request.window->windowItem()->position();
    QMatrix4x4 transform;
    transform.translate(std::round(origin.x() * scale), std::round(origin.y() * scale));
    transform *= request.data.toMatrix(scale);
    return {request.renderTarget, request.viewport, request.deviceRegion, transform};
}

void placeBorderCarrier(std::unique_ptr<KWin::Item> &carrier, KWin::Window *window, double outerReach, double innerReach)
{
    KWin::Item *parent = window->windowItem();
    if (!carrier || carrier->parentItem() != parent) {
        carrier = std::make_unique<KWin::Item>(parent);
        carrier->setZ(-1);
    }
    const QSizeF frame = window->frameGeometry().size();
    carrier->setGeometry(KWin::RectF(-outerReach, -outerReach, frame.width() + 2 * outerReach, frame.height() + 2 * outerReach));
    carrier->scheduleRepaint(ringRegion(frame, outerReach, innerReach));
}

StyledBorders::StyledBorders(const AccentColor &accent, BorderLayerPainter &painter, Render::SignalBus &signalBus)
    : m_accent(accent)
    , m_painter(painter)
    , m_signalBus(signalBus)
{
    m_clock.start();
    m_wantedExpiry.setInterval(kWantedHoldMilliseconds);
    QObject::connect(&m_wantedExpiry, &QTimer::timeout, [this] { refreshWanted(); });
    m_signalBus.onSourceReturned([this](quint64 id) { wake(id); });
}

StyledBorders::~StyledBorders() = default;

double StyledBorders::seconds() const
{
    return static_cast<double>(m_clock.nsecsElapsed()) / 1e9;
}

void StyledBorders::setStyles(const QList<Config::BorderStyle> &styles, bool reduceMotion)
{
    m_styles.clear();
    for (const Config::BorderStyle &style : styles) {
        m_styles.insert(style.name, style);
    }
    m_reduceMotion = reduceMotion;
}

void StyledBorders::setOnBattery(bool onBattery)
{
    if (m_onBattery == onBattery) {
        return;
    }
    m_onBattery = onBattery;
    for (auto &[id, parts] : m_windows) {
        for (std::optional<ShownBorder> &shown : parts.parts) {
            if (shown && parts.window) {
                repaint(parts, *shown);
            }
        }
    }
}

const Config::BorderStyle *StyledBorders::styleNamed(const QString &name)
{
    if (name.isEmpty()) {
        return nullptr;
    }
    const auto found = m_styles.constFind(name);
    if (found == m_styles.constEnd() || found->layers.isEmpty() || !m_painter.load()) {
        return nullptr;
    }
    return &*found;
}

void StyledBorders::updateReach(ShownBorder &shown, const Placement &placement)
{
    shown.inset = placement.inset;
    shown.width = placement.width;
    shown.radius = placement.radius;
    shown.outerReach = placement.inset;
    for (const Config::BorderLayer &layer : std::as_const(placement.style->layers)) {
        shown.outerReach = std::max(shown.outerReach, placement.inset + Render::layerOuterReach(layer, placement.width));
    }
    shown.innerReach
        = std::max({placement.radius.topLeft, placement.radius.topRight, placement.radius.bottomRight, placement.radius.bottomLeft});
}

void StyledBorders::place(Layout::WindowId id, KWin::Window *window, Part part, const Placement &placement)
{
    StyledWindow &parts = m_windows[id];
    parts.window = window;
    std::optional<ShownBorder> &slot = parts.parts[static_cast<std::size_t>(part)];
    if (!slot) {
        slot.emplace();
    }
    ShownBorder &shown = *slot;
    const bool restyled = shown.style != *placement.style;
    shown.style = *placement.style;
    updateReach(shown, placement);
    placeBorderCarrier(shown.carrier, window, shown.outerReach, shown.innerReach);
    if (restyled) {
        shown.motions.resize(static_cast<std::size_t>(shown.style.layers.size()));
        for (Render::LayerMotion &motion : shown.motions) {
            motion.fired = m_signalBus.frame(id).fired;
        }
        parts.paintedAt = seconds();
        refreshWanted();
    }
}

void StyledBorders::hide(Layout::WindowId id, Part part)
{
    const auto found = m_windows.find(id);
    if (found == m_windows.end()) {
        return;
    }
    found->second.parts[static_cast<std::size_t>(part)].reset();
    if (std::ranges::none_of(found->second.parts, [](const std::optional<ShownBorder> &shown) { return shown.has_value(); })) {
        m_windows.erase(found);
        m_signalBus.setWanted(Render::SignalClient::Borders, id, {});
    }
}

void StyledBorders::remove(Layout::WindowId id)
{
    m_windows.erase(id);
    m_signalBus.setWanted(Render::SignalClient::Borders, id, {});
}

void StyledBorders::retainOnly(const QSet<Layout::WindowId> &ids)
{
    for (auto it = m_windows.begin(); it != m_windows.end();) {
        if (!ids.contains(it->first)) {
            m_signalBus.setWanted(Render::SignalClient::Borders, it->first, {});
        }
        it = ids.contains(it->first) ? std::next(it) : m_windows.erase(it);
    }
}

QList<BorderLayerPainter::Layer> StyledBorders::layersFor(Layout::WindowId id, ShownBorder &shown, QSizeF frame, double opacity, bool &live)
{
    const bool still = holdsStill();
    const QRectF own(QPointF(), frame);
    LayerPass pass {shown, m_signalBus.frame(id), {}, own, opacity, still ? Render::kFrozenParticleSeconds : seconds(), still, live};
    pass.scene.subject = own;
    QList<BorderLayerPainter::Layer> layers;
    layers.reserve(shown.style.layers.size());
    shown.motions.resize(static_cast<std::size_t>(shown.style.layers.size()));
    shown.sleeping = false;
    for (std::size_t index = 0; index < static_cast<std::size_t>(shown.style.layers.size()); ++index) {
        appendLayer(layers, pass, index);
    }
    live = pass.live;
    return layers;
}

double StyledBorders::layerClock(const Config::BorderLayer &layer, const Render::LayerMotion &motion, bool reactive, const LayerPass &pass)
{
    if (pass.still) {
        return 0.0;
    }
    return reactive ? motion.phase : Render::layerPhase(layer, pass.now);
}

std::optional<Render::EventLayerFrame> StyledBorders::oneShotFor(
    const Config::BorderLayer &layer, const Config::BorderLayer &config, const Render::LayerMotion &motion, const LayerPass &pass)
{
    if (!layer.fireOn.isEmpty()) {
        return Render::oneShotFrame(config, pass.scene, pass.own, pass.now - motion.lastFire);
    }
    return pass.still ? std::nullopt : Render::restingFrame(config, pass.scene, pass.own, pass.now, kOneShotRestCycles);
}

void StyledBorders::appendLayer(QList<BorderLayerPainter::Layer> &layers, LayerPass &pass, std::size_t index)
{
    ShownBorder &shown = pass.shown;
    const Config::BorderLayer &layer = shown.style.layers[static_cast<qsizetype>(index)];
    Render::LayerMotion &motion = shown.motions[index];
    const bool reactive = Render::isReactive(layer);
    if (reactive) {
        stepMotion(motion, layer, pass.frameSignals, pass.now, shown.width, pass.still);
    }
    const Config::BorderLayer &config = reactive ? motion.resolved : layer;
    const bool rests = !pass.still && Render::layerRests(layer, pass.frameSignals);
    shown.sleeping = shown.sleeping || rests;
    if (reactive && layerHidden(layer, pass.frameSignals, pass.still)) {
        shown.sleeping = true;
        return;
    }
    pass.live = pass.live || (!pass.still && !rests && Render::layerRunsContinuously(layer));
    BorderLayerPainter::Layer drawn {&config, resolvedColors(m_accent, layer.colors), layerClock(layer, motion, reactive, pass),
        config.opacity * pass.opacity, pass.now, reactive ? &motion : nullptr, std::nullopt};
    if (Render::isOneShotLayer(layer.kind)) {
        drawn.event = oneShotFor(layer, config, motion, pass);
        if (!drawn.event) {
            return;
        }
    }
    layers.append(drawn);
}

void StyledBorders::paint(Layout::WindowId id, const PaintRequest &request)
{
    const auto found = m_windows.find(id);
    if (found == m_windows.end()) {
        return;
    }
    const KWin::Window *window = request.window->window();
    const QSizeF frame = window->frameGeometry().size();
    const double opacity = request.window->opacity() * request.data.opacity();
    const BorderLayerPainter::Target target = borderPaintTarget(request);
    for (std::optional<ShownBorder> &shown : found->second.parts) {
        if (!shown) {
            continue;
        }
        bool live = false;
        const QList<BorderLayerPainter::Layer> layers = layersFor(id, *shown, frame, opacity, live);
        found->second.paintedAt = seconds();
        m_painter.draw(target, {frame, shown->radius, shown->inset, shown->width}, layers);
        shown->paintedLive = live;
    }
}

void StyledBorders::repaint(StyledWindow &parts, ShownBorder &shown)
{
    shown.carrier->scheduleRepaint(ringRegion(parts.window->frameGeometry().size(), shown.outerReach, shown.innerReach));
}

Render::SignalSet StyledBorders::wantedFor(const StyledWindow &parts, double now) const
{
    Render::SignalSet wanted;
    const bool sleeping
        = std::ranges::any_of(parts.parts, [](const std::optional<ShownBorder> &shown) { return shown && shown->sleeping; });
    if (holdsStill() || (!sleeping && now - parts.paintedAt >= kWantedHoldMilliseconds / 1000.0)) {
        return wanted;
    }
    for (const std::optional<ShownBorder> &shown : parts.parts) {
        if (shown) {
            wanted |= Render::signalsOf(shown->style);
        }
    }
    return wanted;
}

void StyledBorders::refreshWanted()
{
    const double now = seconds();
    bool any = false;
    for (const auto &[id, parts] : m_windows) {
        const Render::SignalSet wanted = wantedFor(parts, now);
        any = any || wanted.any();
        m_signalBus.setWanted(Render::SignalClient::Borders, id, wanted);
    }
    if (any && !m_wantedExpiry.isActive()) {
        m_wantedExpiry.start();
    } else if (!any) {
        m_wantedExpiry.stop();
    }
}

void StyledBorders::wake(Layout::WindowId id)
{
    const auto found = m_windows.find(id);
    if (found == m_windows.end() || !found->second.window) {
        return;
    }
    for (std::optional<ShownBorder> &shown : found->second.parts) {
        if (shown && shown->sleeping) {
            shown->sleeping = false;
            repaint(found->second, *shown);
        }
    }
}

void StyledBorders::scheduleFrames()
{
    m_signalBus.advance(seconds());
    refreshWanted();
    for (auto &[id, parts] : m_windows) {
        if (!parts.window) {
            continue;
        }
        for (std::optional<ShownBorder> &shown : parts.parts) {
            if (shown && shown->paintedLive) {
                shown->paintedLive = false;
                repaint(parts, *shown);
            }
        }
    }
}

}
