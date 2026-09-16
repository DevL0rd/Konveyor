#include "layout/floating/floatinglayer.h"

#include "layout/common/geometry.h"

#include <algorithm>
#include <cmath>
#include <utility>

namespace Konveyor::Layout
{

namespace
{

bool isRightAnchored(Config::FloatingRelativeTo relativeTo)
{
    return relativeTo == Config::FloatingRelativeTo::TopRight || relativeTo == Config::FloatingRelativeTo::BottomRight
        || relativeTo == Config::FloatingRelativeTo::Right;
}

bool isBottomAnchored(Config::FloatingRelativeTo relativeTo)
{
    return relativeTo == Config::FloatingRelativeTo::BottomLeft || relativeTo == Config::FloatingRelativeTo::BottomRight
        || relativeTo == Config::FloatingRelativeTo::Bottom;
}

QPointF resolveDefaultFloatingPosition(const Config::FloatingPosition &position, QRectF area, QSizeF size)
{
    QPointF pos(position.x, position.y);
    if (isRightAnchored(position.relativeTo)) {
        pos.setX(area.width() - size.width() - pos.x());
    }
    if (isBottomAnchored(position.relativeTo)) {
        pos.setY(area.height() - size.height() - pos.y());
    }
    const auto relativeTo = position.relativeTo;
    if (relativeTo == Config::FloatingRelativeTo::Top || relativeTo == Config::FloatingRelativeTo::Bottom) {
        pos.rx() += (area.width() - size.width()) / 2.0;
    }
    if (relativeTo == Config::FloatingRelativeTo::Left || relativeTo == Config::FloatingRelativeTo::Right) {
        pos.ry() += (area.height() - size.height()) / 2.0;
    }
    return pos + area.topLeft();
}

}

void FloatingData::update(const Tile &tile)
{
    const QSizeF outerSize = tile.outerSize();
    if (size == outerSize) {
        return;
    }
    size = outerSize;
    refreshAbsolutePos();
}

void FloatingData::updateWorkingArea(QRectF area)
{
    if (workingArea == area) {
        return;
    }
    workingArea = area;
    refreshAbsolutePos();
}

void FloatingData::refreshAbsolutePos()
{
    QPointF logical = scaleByArea(workingArea, pos) - workingArea.topLeft();
    const double maxOffHor = std::max(0.0, size.width() - std::clamp(size.width() / 4.0, 10.0, 75.0));
    const double maxOffVer = std::max(0.0, size.height() - std::clamp(size.height() / 4.0, 10.0, 75.0));
    logical.setX(std::clamp(logical.x(), -maxOffHor, workingArea.width() - size.width() + maxOffHor));
    logical.setY(std::clamp(logical.y(), -maxOffVer, workingArea.height() - size.height() + maxOffVer));
    absolutePos = logical + workingArea.topLeft();
}

void FloatingData::setAbsolutePos(QPointF newPos)
{
    pos = logicalToFracInArea(workingArea, newPos);
    refreshAbsolutePos();
}

QPointF scaleByArea(QRectF area, QPointF frac)
{
    return {frac.x() * area.width() + area.x(), frac.y() * area.height() + area.y()};
}

QPointF logicalToFracInArea(QRectF area, QPointF logical)
{
    const QPointF relative = logical - area.topLeft();
    return {relative.x() / std::max(area.width(), 1.0), relative.y() / std::max(area.height(), 1.0)};
}

QPointF FloatingLayer::relativeToAbsolute(QPointF frac) const
{
    return scaleByArea(m_workingArea, frac);
}

QPointF FloatingLayer::absoluteToRelative(QPointF logical) const
{
    return logicalToFracInArea(m_workingArea, logical);
}

QPointF FloatingLayer::keepInsideWorkArea(QPointF pos, QSizeF size) const
{
    return clampIntoArea(m_workingArea, QRectF(pos, size));
}

void FloatingLayer::prepareTileSize(Tile &tile) const
{
    LayoutWindow &window = tile.window();
    QSize size = tile.savedFloatingSize.value_or(QSize());
    if (!tile.savedFloatingSize && window.requestedMode() == WindowMode::Normal) {
        size = window.pendingSize().value_or(QSize());
    }
    const QSize minSize = window.minSize();
    const QSize maxSize = window.maxSize();
    size.setWidth(clampToSizeLimitsAllowZero(size.width(), minSize.width(), maxSize.width()));
    size.setHeight(clampToSizeLimitsAllowZero(size.height(), minSize.height(), maxSize.height()));
    window.requestSizeUntilCommit(size, true);
}

QSize FloatingLayer::initialWindowSize(const std::optional<Config::PresetSize> &width, const std::optional<Config::PresetSize> &height,
    const EffectiveWindowRules &rules) const
{
    const Config::Border border = mergeBorder(m_options->layout.border, rules.border);
    const auto resolve = [&](const std::optional<Config::PresetSize> &preset, double available) {
        if (!preset) {
            return 0;
        }
        const PresetExtent resolved = measureFloatingPreset(*preset, available);
        const double value = resolved.isTile && border.enabled ? resolved.value - border.width * 2.0 : resolved.value;
        return std::max(1, floorToInt(value));
    };
    return {resolve(width, m_workingArea.width()), resolve(height, m_workingArea.height())};
}

std::optional<QPointF> FloatingLayer::savedOrDefaultPosition(const Tile &tile) const
{
    if (tile.savedFloatingPosition) {
        return relativeToAbsolute(*tile.savedFloatingPosition);
    }
    const auto &position = tile.window().rules().defaultFloatingPosition;
    if (!position) {
        return std::nullopt;
    }
    return resolveDefaultFloatingPosition(*position, m_workingArea, tile.outerSize());
}

}
