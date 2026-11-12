#include "decorations/decorationlayer.h"

#include "decorations/accentcolor.h"

#include <scene/borderoutline.h>
#include <scene/borderradius.h>
#include <scene/outlinedborderitem.h>

#include <scene/imageitem.h>
#include <scene/windowitem.h>
#include <window.h>

namespace Konveyor
{

namespace
{

Config::CornerRadius expandRadius(const Config::CornerRadius &radius, double amount)
{
    const auto grow = [amount](double value) { return value > 0 ? value + amount : 0.0; };
    return {grow(radius.topLeft), grow(radius.topRight), grow(radius.bottomRight), grow(radius.bottomLeft)};
}

}

DecorationLayer::DecorationLayer(const AccentColor &accent, QObject *parent)
    : QObject(parent)
    , m_accent(accent)
    , m_audioSignals(m_signalBus)
    , m_styled(accent, m_painter, m_signalBus)
    , m_events(accent, m_painter, m_styled, m_signalBus)
    , m_viewSignals(m_signalBus, m_events)
{
    connect(&m_battery, &BatteryMonitor::onBatteryChanged, this, [this](bool onBattery) {
        m_styled.setOnBattery(onBattery);
        m_events.setOnBattery(onBattery);
        m_audioSignals.setOnBattery(onBattery);
    });
}

DecorationLayer::~DecorationLayer() = default;

void DecorationLayer::update(KWin::Window *window, const Layout::WindowState &state, const QRectF &workspaceView, double scale)
{
    KWin::Item *parent = window->windowItem();
    if (!parent) {
        return;
    }
    Decorations &decorations = m_decorations[state.id];
    decorations.window = window;
    const QRectF frame = window->frameGeometry();
    const double borderInset = state.border.enabled ? state.border.width : 0.0;
    const OutlineRequest request {frame, 0.0, radiusFor(window, state.cornerRadius), workspaceView, scale};
    updatePart({StyledBorders::Part::Border, decorations.border, window, request}, state.border, state);
    updatePart({StyledBorders::Part::FocusRing, decorations.focusRing, window, {frame, borderInset, request.radius, workspaceView, scale}},
        state.focusRing, state);
    const qsizetype tabCount = state.tabBar.visible ? state.tabBar.tabRects.size() : 0;
    decorations.tabs.resize(static_cast<size_t>(tabCount));
    for (qsizetype i = 0; i < tabCount; ++i) {
        apply(decorations.tabs[static_cast<size_t>(i)], parent,
            filled(frame, state.tabBar.tabRects[i], state.tabBar.tabRadii[i], state.tabBar.tabPaints.value(i), workspaceView, scale));
    }
}

void DecorationLayer::updatePart(const PartRequest &request, const Layout::DecorationState &decoration, const Layout::WindowState &state)
{
    Placement placement = outline(request.outline, decoration, state);
    const Config::BorderStyle *style = placement.visible ? m_styled.styleNamed(decoration.style) : nullptr;
    if (style) {
        m_styled.place(state.id, request.window, request.part, {style, request.outline.inset, decoration.width, request.outline.radius});
        placement.visible = false;
    } else {
        m_styled.hide(state.id, request.part);
        placement.visible = placement.visible && decoration.paint.isSet();
    }
    apply(request.slot, request.window->windowItem(), placement);
}

void DecorationLayer::setBorderStyles(const QList<Config::BorderStyle> &styles, bool reduceMotion)
{
    m_styled.setStyles(styles, reduceMotion);
    m_events.setReduceMotion(reduceMotion);
    m_audioSignals.setReduceMotion(reduceMotion);
}

void DecorationLayer::paintBorders(Layout::WindowId id, const StyledBorders::PaintRequest &request)
{
    m_styled.paint(id, request);
    m_events.paint(id, request);
}

void DecorationLayer::scheduleBorderFrames()
{
    m_styled.scheduleFrames();
    m_events.scheduleFrames();
    m_viewSignals.settle();
}

void DecorationLayer::updateViewSignals(const QList<Layout::OutputViewState> &views, const QList<Layout::WindowState> &states)
{
    m_viewSignals.update(views, states);
}

void DecorationLayer::fireEvent(Config::EventTrigger trigger, Layout::WindowId id, std::optional<QPointF> point, QPointF direction)
{
    m_events.fire(trigger, id, point, direction);
}

void DecorationLayer::syncEventEffects(
    const QList<Layout::WindowState> &states, const std::function<KWin::Window *(Layout::WindowId)> &windowOf)
{
    QList<EventEffects::Window> windows;
    windows.reserve(states.size());
    for (const Layout::WindowState &state : states) {
        if (KWin::Window *window = windowOf(state.id); window && window->windowItem()) {
            windows.append({window, &state, radiusFor(window, state.cornerRadius)});
        }
    }
    m_events.sync(windows);
    m_audioSignals.setWindowLookup([windowOf](quint64 id) -> std::optional<Audio::CapturedWindow> {
        const KWin::Window *window = windowOf(id);
        if (!window) {
            return std::nullopt;
        }
        return Audio::CapturedWindow {{window->pid(), window->desktopFileName()}, window->isFullScreen()};
    });
    m_audioSignals.refresh();
}

void DecorationLayer::showDropHint(Layout::WindowId carrier, KWin::Window *window, const DropHintRequest &request)
{
    KWin::Item *parent = window->windowItem();
    if (!parent) {
        hideDropHint();
        return;
    }
    Placement placement
        = filled(window->frameGeometry(), request.rect, request.radius, request.paint, request.workspaceView, request.scale);
    placement.z = -1;
    m_dropHintCarrier = carrier;
    apply(m_dropHint, parent, placement);
}

void DecorationLayer::hideDropHint()
{
    m_dropHintCarrier.reset();
    m_dropHint.item.reset();
    m_dropHint.outline.reset();
}

void DecorationLayer::remove(Layout::WindowId id)
{
    m_decorations.erase(id);
    m_styled.remove(id);
    m_events.remove(id);
    if (m_dropHintCarrier == id) {
        hideDropHint();
    }
}

Config::CornerRadius DecorationLayer::radiusFor(KWin::Window *window, const std::optional<Config::CornerRadius> &fromRules)
{
    if (fromRules) {
        return *fromRules;
    }
    const KWin::BorderRadius radius = window->borderRadius();
    return {radius.topLeft(), radius.topRight(), radius.bottomRight(), radius.bottomLeft()};
}

void DecorationLayer::retainOnly(const QSet<Layout::WindowId> &ids)
{
    if (m_dropHintCarrier && !ids.contains(*m_dropHintCarrier)) {
        hideDropHint();
    }
    m_styled.retainOnly(ids);
    for (auto it = m_decorations.begin(); it != m_decorations.end();) {
        it = ids.contains(it->first) ? std::next(it) : m_decorations.erase(it);
    }
}

void DecorationLayer::apply(Slot &slot, KWin::Item *parent, const Placement &placement)
{
    if (!placement.visible) {
        slot.item.reset();
        slot.outline.reset();
        return;
    }
    if (placement.spec.gradient || placement.filled) {
        slot.outline.reset();
        applyImage(slot, parent, placement);
        return;
    }
    slot.item.reset();
    applyOutlined(slot, parent, placement);
}

void DecorationLayer::applyOutlined(Slot &slot, KWin::Item *parent, const Placement &placement)
{
    const Config::CornerRadius &radius = placement.spec.outerRadius;
    const KWin::BorderOutline wanted(placement.thickness, placement.spec.color,
        KWin::BorderRadius(radius.topLeft, radius.topRight, radius.bottomRight, radius.bottomLeft));
    if (!slot.outline || slot.outline->parentItem() != parent) {
        slot.outline = std::make_unique<KWin::OutlinedBorderItem>(placement.innerRect, wanted, parent);
        slot.outline->setZ(placement.z);
    }
    if (slot.outline->outline() != wanted) {
        slot.outline->setOutline(wanted);
    }
    if (slot.outline->innerRect() != placement.innerRect) {
        slot.outline->setInnerRect(placement.innerRect);
    }
}

void DecorationLayer::applyImage(Slot &slot, KWin::Item *parent, const Placement &placement)
{
    if (!slot.item || slot.item->parentItem() != parent) {
        slot.item = std::make_unique<KWin::ImageItem>(parent);
        slot.item->setZ(placement.z);
        slot.spec = {};
    }
    if (!(slot.spec == placement.spec)) {
        slot.spec = placement.spec;
        slot.item->setImage(Render::renderBorder(placement.spec));
        slot.item->setSize(placement.spec.size);
    }
    slot.item->setPosition(placement.position);
}

DecorationLayer::Placement DecorationLayer::outline(
    const OutlineRequest &request, const Layout::DecorationState &decoration, const Layout::WindowState &state) const
{
    Placement placement;
    placement.visible = decoration.enabled && decoration.width > 0 && state.visible;
    if (!placement.visible) {
        return placement;
    }
    const double offset = request.inset + decoration.width;
    const QRectF globalRect = request.frame.adjusted(-offset, -offset, offset, offset);
    placement.position = QPointF(-offset, -offset);
    placement.spec.size = globalRect.size();
    placement.spec.borderWidth = decoration.width;
    placement.spec.outerRadius = expandRadius(request.radius, offset);
    placement.spec.scale = request.scale;
    placement.thickness = decoration.width;
    placement.innerRect
        = QRectF(-request.inset, -request.inset, request.frame.width() + request.inset * 2, request.frame.height() + request.inset * 2);
    fillPaint(placement.spec, decoration.paint, globalRect, request.workspaceView);
    return placement;
}

DecorationLayer::Placement DecorationLayer::filled(const QRectF &frame, const QRectF &rect, const Config::CornerRadius &radius,
    const Layout::ResolvedPaint &paint, const QRectF &workspaceView, double scale) const
{
    Placement placement;
    placement.visible = !rect.isEmpty();
    placement.position = rect.topLeft() - frame.topLeft();
    placement.spec.size = rect.size();
    placement.spec.outerRadius = radius;
    placement.spec.scale = scale;
    placement.filled = true;
    fillPaint(placement.spec, paint, rect, workspaceView);
    return placement;
}

void DecorationLayer::fillPaint(
    Render::BorderSpec &spec, const Layout::ResolvedPaint &paint, const QRectF &globalRect, const QRectF &workspaceView) const
{
    spec.color = paint.source == Config::ColorSource::Explicit ? paint.color : m_accent.colorFor(paint.source);
    spec.gradient = paint.gradient;
    if (!paint.gradient) {
        return;
    }
    const bool relativeToView = paint.gradient->relativeTo == Config::GradientRelativeTo::WorkspaceView;
    const QRectF area = relativeToView ? workspaceView : globalRect;
    spec.gradientRect = QRectF(QPointF(0, 0), area.size());
    spec.geometryOffset = globalRect.topLeft() - area.topLeft();
}

}
