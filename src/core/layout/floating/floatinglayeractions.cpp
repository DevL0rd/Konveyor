#include "layout/floating/floatinglayer.h"

#include "layout/common/sizelimits.h"

#include "layout/common/geometry.h"

#include <algorithm>
#include <cmath>

namespace Konveyor::Layout
{

namespace
{

constexpr double NudgeStep = 50.0;
constexpr double MinAnimatedMoveDistanceSq = 100.0;

double axisOf(QSizeF size, bool horizontal)
{
    return horizontal ? size.width() : size.height();
}

double applyAxisChange(double current, double available, double loc, PositionChange change)
{
    switch (change.kind) {
    case ChangeKind::SetFixed:
        return change.value + loc;
    case ChangeKind::SetProportion:
        return available * std::clamp(change.value / 100.0, 0.0, MaxProportion) + loc;
    case ChangeKind::AdjustFixed:
        return current + change.value;
    case ChangeKind::AdjustProportion:
        break;
    }
    const double currentProportion = (current - loc) / std::max(available, 1.0);
    return available * std::clamp(currentProportion + change.value / 100.0, 0.0, MaxProportion) + loc;
}

}

bool isAbsolutePositionChange(PositionChange change)
{
    return change.kind == ChangeKind::SetFixed || change.kind == ChangeKind::SetProportion;
}

QPointF applyPositionChange(QPointF current, QRectF area, PositionChange x, PositionChange y)
{
    return {applyAxisChange(current.x(), area.width(), area.x(), x), applyAxisChange(current.y(), area.height(), area.y(), y)};
}

void FloatingLayer::setWindowSize(std::size_t idx, SizeChange change, bool horizontal, bool animate)
{
    Tile &tile = m_tiles[idx];
    (horizontal ? tile.floatingWidthPresetIndex : tile.floatingHeightPresetIndex).reset();

    const double available = axisOf(m_workingArea.size(), horizontal);
    LayoutWindow &window = tile.window();
    const double currentWindow = axisOf(window.pendingSize().value_or(roundedSize(window.size())), horizontal);
    const double currentTile = axisOf(tile.pendingOuterSize(), horizontal);
    const auto toWindow = [&](double outerSize) { return horizontal ? tile.innerWidthFor(outerSize) : tile.innerHeightFor(outerSize); };

    double value = 0.0;
    switch (change.kind) {
    case ChangeKind::SetFixed:
        value = change.value;
        break;
    case ChangeKind::SetProportion:
        value = toWindow(available * std::clamp(change.value / 100.0, 0.0, MaxProportion));
        break;
    case ChangeKind::AdjustFixed:
        value = currentWindow + change.value;
        break;
    case ChangeKind::AdjustProportion:
        value = toWindow(available * std::clamp(currentTile / available + change.value / 100.0, 0.0, MaxProportion));
        break;
    }

    const QSize minSize = window.minSize();
    const QSize maxSize = window.maxSize();
    QSize target = window.pendingSize().value_or(QSize());
    const int resolved = roundToInt(std::clamp(std::round(value), 1.0, MaxPixelSize));
    if (horizontal) {
        target.setWidth(resolved);
    } else {
        target.setHeight(resolved);
    }
    target.setWidth(clampToSizeLimits(target.width(), minSize.width(), maxSize.width()));
    target.setHeight(clampToSizeLimits(target.height(), minSize.height(), maxSize.height()));
    window.requestSizeUntilCommit(target, animate);
}

void FloatingLayer::setWindowWidth(std::optional<WindowId> window, SizeChange change, bool animate)
{
    if (const auto idx = targetIndex(window)) {
        setWindowSize(*idx, change, true, animate);
    }
}

void FloatingLayer::setWindowHeight(std::optional<WindowId> window, SizeChange change, bool animate)
{
    if (const auto idx = targetIndex(window)) {
        setWindowSize(*idx, change, false, animate);
    }
}

std::size_t FloatingLayer::togglePresetIndex(std::size_t idx, bool horizontal, bool forwards) const
{
    const Tile &tile = m_tiles[idx];
    const auto &presets = horizontal ? m_options->layout.presetColumnWidths : m_options->layout.presetWindowHeights;
    const auto len = static_cast<std::size_t>(presets.size());
    const auto &current = horizontal ? tile.floatingWidthPresetIndex : tile.floatingHeightPresetIndex;
    if (current) {
        return (*current + (forwards ? 1 : len - 1)) % len;
    }

    const double available = axisOf(m_workingArea.size(), horizontal);
    const double currentWindow = axisOf(tile.pendingWindowSize(), horizontal);
    const double currentTile = axisOf(tile.pendingOuterSize(), horizontal);
    for (std::size_t n = 0; n < len; ++n) {
        const std::size_t i = forwards ? n : len - 1 - n;
        const PresetExtent resolved = measureFloatingPreset(presets[static_cast<qsizetype>(i)], available);
        const double value = resolved.isTile ? currentTile : currentWindow;
        if (forwards ? value + 1.0 < resolved.value : resolved.value + 1.0 < value) {
            return i;
        }
    }
    return forwards ? 0 : len - 1;
}

void FloatingLayer::toggleWindowWidth(std::optional<WindowId> window, bool forwards)
{
    const auto idx = targetIndex(window);
    if (!idx) {
        return;
    }
    const std::size_t presetIndex = togglePresetIndex(*idx, true, forwards);
    setWindowSize(*idx, sizeChangeFromPreset(m_options->layout.presetColumnWidths[static_cast<qsizetype>(presetIndex)]), true, true);
    m_tiles[*idx].floatingWidthPresetIndex = presetIndex;
    endResize(m_tiles[*idx].id());
}

void FloatingLayer::toggleWindowHeight(std::optional<WindowId> window, bool forwards)
{
    const auto idx = targetIndex(window);
    if (!idx) {
        return;
    }
    const std::size_t presetIndex = togglePresetIndex(*idx, false, forwards);
    setWindowSize(*idx, sizeChangeFromPreset(m_options->layout.presetWindowHeights[static_cast<qsizetype>(presetIndex)]), false, true);
    m_tiles[*idx].floatingHeightPresetIndex = presetIndex;
    endResize(m_tiles[*idx].id());
}

bool FloatingLayer::focusNearest(const std::function<double(QPointF, QPointF)> &distance)
{
    const auto activeIndex = targetIndex(std::nullopt);
    if (!activeIndex) {
        return false;
    }
    const QPointF center = m_data[*activeIndex].center();
    std::optional<WindowId> best;
    double bestDistance = 0.0;
    for (std::size_t i = 0; i < m_tiles.size(); ++i) {
        if (i == *activeIndex) {
            continue;
        }
        const double value = distance(center, m_data[i].center());
        if (value > 0.0 && (!best || value < bestDistance)) {
            best = m_tiles[i].id();
            bestDistance = value;
        }
    }
    if (!best) {
        return false;
    }
    activateWindow(*best);
    return true;
}

bool FloatingLayer::focusLeft()
{
    return focusNearest([](QPointF focus, QPointF other) { return focus.x() - other.x(); });
}

bool FloatingLayer::focusRight()
{
    return focusNearest([](QPointF focus, QPointF other) { return other.x() - focus.x(); });
}

bool FloatingLayer::focusUp()
{
    return focusNearest([](QPointF focus, QPointF other) { return focus.y() - other.y(); });
}

bool FloatingLayer::focusDown()
{
    return focusNearest([](QPointF focus, QPointF other) { return other.y() - focus.y(); });
}

void FloatingLayer::focusExtreme(bool horizontal, bool maximum)
{
    std::optional<WindowId> best;
    double bestValue = 0.0;
    for (std::size_t i = 0; i < m_tiles.size(); ++i) {
        const double value = horizontal ? m_data[i].absolutePos.x() : m_data[i].absolutePos.y();
        if (!best || (maximum ? value > bestValue : value < bestValue)) {
            best = m_tiles[i].id();
            bestValue = value;
        }
    }
    if (best) {
        activateWindow(*best);
    }
}

void FloatingLayer::focusLeftmost()
{
    focusExtreme(true, false);
}

void FloatingLayer::focusRightmost()
{
    focusExtreme(true, true);
}

void FloatingLayer::focusTopmost()
{
    focusExtreme(false, false);
}

void FloatingLayer::focusBottommost()
{
    focusExtreme(false, true);
}

void FloatingLayer::placeAnimated(std::size_t idx, QPointF newPos)
{
    const QPointF previous = m_data[idx].absolutePos;
    m_data[idx].setAbsolutePos(newPos);
    const QPointF diff = previous - m_data[idx].absolutePos;
    if (diff.x() * diff.x() + diff.y() * diff.y() > MinAnimatedMoveDistanceSq) {
        m_tiles[idx].slideFrom(diff);
    }
}

void FloatingLayer::moveTo(std::size_t idx, QPointF newPos, bool animate)
{
    if (animate) {
        placeAnimated(idx, newPos);
    } else {
        m_data[idx].setAbsolutePos(newPos);
    }
    endResize(std::nullopt);
}

void FloatingLayer::moveBy(QPointF amount)
{
    if (const auto idx = targetIndex(std::nullopt)) {
        moveTo(*idx, m_data[*idx].absolutePos + amount, true);
    }
}

void FloatingLayer::moveLeft()
{
    moveBy(QPointF(-NudgeStep, 0.0));
}

void FloatingLayer::moveRight()
{
    moveBy(QPointF(NudgeStep, 0.0));
}

void FloatingLayer::moveUp()
{
    moveBy(QPointF(0.0, -NudgeStep));
}

void FloatingLayer::moveDown()
{
    moveBy(QPointF(0.0, NudgeStep));
}

void FloatingLayer::moveWindow(std::optional<WindowId> window, PositionChange x, PositionChange y, bool animate)
{
    const auto idx = targetIndex(window);
    if (!idx) {
        return;
    }
    moveTo(*idx, applyPositionChange(m_data[*idx].absolutePos, m_workingArea, x, y), animate);
}

void FloatingLayer::centerWindow(std::optional<WindowId> window)
{
    const auto idx = targetIndex(window);
    if (!idx) {
        return;
    }
    moveTo(*idx, centerInArea(m_workingArea, m_data[*idx].size), true);
}

bool FloatingLayer::beginResize(WindowId window, quint8 edges)
{
    if (m_resize) {
        return false;
    }
    const auto idx = indexOf(window);
    if (!idx) {
        return false;
    }
    m_resize = ResizeSession {window, m_tiles[*idx].windowSize(), edges};
    return true;
}

bool FloatingLayer::updateResize(WindowId window, QPointF delta)
{
    if (!m_resize || m_resize->window != window) {
        return false;
    }
    const QSizeF original = m_resize->originalWindowSize;
    const quint8 edges = m_resize->edges;
    const auto has = [edges](ResizeEdge edge) { return (edges & static_cast<quint8>(edge)) != 0; };

    if (has(ResizeEdge::Left) || has(ResizeEdge::Right)) {
        const double dx = has(ResizeEdge::Left) ? -delta.x() : delta.x();
        setWindowWidth(window, SizeChange {ChangeKind::SetFixed, std::round(original.width() + dx)}, false);
    }
    if (has(ResizeEdge::Top) || has(ResizeEdge::Bottom)) {
        const double dy = has(ResizeEdge::Top) ? -delta.y() : delta.y();
        setWindowHeight(window, SizeChange {ChangeKind::SetFixed, std::round(original.height() + dy)}, false);
    }
    return true;
}

void FloatingLayer::endResize(std::optional<WindowId> window)
{
    if (!m_resize) {
        return;
    }
    if (window && *window != m_resize->window) {
        return;
    }
    m_resize.reset();
}

}
