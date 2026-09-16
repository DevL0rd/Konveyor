#include "decorations/decorationlayer.h"

#include "decorations/accentcolor.h"

#include <scene/borderoutline.h>
#include <scene/borderradius.h>
#include <scene/outlinedborderitem.h>

#include <algorithm>
#include <scene/imageitem.h>
#include <scene/windowitem.h>
#include <window.h>

namespace Konveyor
{

namespace
{

bool hasRadius(const Config::CornerRadius &radius)
{
    return radius.topLeft > 0 || radius.topRight > 0 || radius.bottomRight > 0 || radius.bottomLeft > 0;
}

double reportedRadius(KWin::Window *window)
{
    const KWin::BorderRadius radius = window->borderRadius();
    return std::max({radius.topLeft(), radius.topRight(), radius.bottomRight(), radius.bottomLeft()});
}

Config::CornerRadius expandRadius(const Config::CornerRadius &radius, double amount)
{
    const auto grow = [amount](double value) { return value > 0 ? value + amount : 0.0; };
    return {grow(radius.topLeft), grow(radius.topRight), grow(radius.bottomRight), grow(radius.bottomLeft)};
}

}

DecorationLayer::DecorationLayer(const AccentColor &accent, QObject *parent)
    : QObject(parent)
    , m_accent(accent)
{ }

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
    apply(decorations.border, parent, outline(request, state.border, state));
    apply(
        decorations.focusRing, parent, outline({request.frame, borderInset, request.radius, workspaceView, scale}, state.focusRing, state));
    const qsizetype tabCount = state.tabBar.visible ? state.tabBar.tabRects.size() : 0;
    decorations.tabs.resize(static_cast<size_t>(tabCount));
    for (qsizetype i = 0; i < tabCount; ++i) {
        apply(decorations.tabs[static_cast<size_t>(i)], parent,
            tab(frame, state.tabBar.tabRects[i], state.tabBar.tabPaints.value(i), workspaceView, scale));
    }
}

void DecorationLayer::remove(Layout::WindowId id)
{
    m_decorations.erase(id);
}

Config::CornerRadius DecorationLayer::radiusFor(KWin::Window *window, const Config::CornerRadius &fromRules)
{
    if (hasRadius(fromRules)) {
        return fromRules;
    }
    m_systemRadius = std::max(m_systemRadius, reportedRadius(window));
    return {m_systemRadius, m_systemRadius, m_systemRadius, m_systemRadius};
}

void DecorationLayer::retainOnly(const QSet<Layout::WindowId> &ids)
{
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
        slot.outline->setZ(1);
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
        slot.item->setZ(1);
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

DecorationLayer::Placement DecorationLayer::tab(
    const QRectF &frame, const QRectF &rect, const Layout::ResolvedPaint &paint, const QRectF &workspaceView, double scale) const
{
    Placement placement;
    placement.visible = !rect.isEmpty();
    placement.position = rect.topLeft() - frame.topLeft();
    placement.spec.size = rect.size();
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
