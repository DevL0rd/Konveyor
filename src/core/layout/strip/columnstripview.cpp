#include "layout/strip/columnstrip.h"

#include "layout/common/geometry.h"

#include <algorithm>
#include <cmath>
#include <utility>

namespace Konveyor::Layout
{

namespace
{

bool isFullscreenColumn(const Column &column)
{
    return column.sizingMode() == WindowMode::Fullscreen || column.requestedMode() == WindowMode::Fullscreen;
}

}

double ColumnStrip::scrollPosition() const
{
    return columnOffset(m_activeColumnIndex) + m_scroll.current();
}

double ColumnStrip::targetScrollPosition() const
{
    return columnOffset(m_activeColumnIndex) + m_scroll.target();
}

void ColumnStrip::reconcileView()
{
    if (m_columns.empty() || m_scroll.isSwiping() || m_resize) {
        return;
    }
    const Column &active = m_columns[m_activeColumnIndex];
    const double activeX = columnOffset(m_activeColumnIndex);
    if (const std::optional<double> overlay = overlayViewOffset()) {
        if (std::abs(*overlay - m_scroll.target()) > 0.5) {
            animateScroll(m_activeColumnIndex, *overlay);
        }
        return;
    }
    const double fitted = activeX + scrollForColumn(std::nullopt, m_activeColumnIndex, m_activeColumnIndex);
    const bool expanded = active.sizingMode() != WindowMode::Normal || active.requestedMode() != WindowMode::Normal;
    const double offset = (expanded ? fitted : constrainScrollPosition(fitted)) - activeX;
    if (std::abs(offset - m_scroll.target()) > 0.5) {
        animateScroll(m_activeColumnIndex, offset);
    }
}

std::optional<std::size_t> ColumnStrip::overlayNeighbour(std::size_t distance, bool forward) const
{
    const std::size_t count = m_columns.size();
    const std::size_t active = m_activeColumnIndex;
    if (forward ? active + distance < count : active >= distance) {
        return forward ? active + distance : active - distance;
    }
    if (wrapPeriod() <= 0.0) {
        return std::nullopt;
    }
    return forward ? (active + distance) % count : active + count - distance;
}

std::optional<bool> ColumnStrip::fullscreenBackgroundBefore() const
{
    const auto fullscreenAt = [&](std::size_t distance, bool forward) -> std::optional<std::size_t> {
        const std::optional<std::size_t> idx = overlayNeighbour(distance, forward);
        if (idx && isFullscreenColumn(m_columns[*idx])) {
            return idx;
        }
        return std::nullopt;
    };
    for (std::size_t distance = 1; distance < m_columns.size(); ++distance) {
        const std::optional<std::size_t> before = fullscreenAt(distance, false);
        const std::optional<std::size_t> after = fullscreenAt(distance, true);
        if (before && after && *before == *after) {
            return *before < m_activeColumnIndex;
        }
        if (before || after) {
            return before.has_value();
        }
    }
    return std::nullopt;
}

std::optional<double> ColumnStrip::overlayViewOffset() const
{
    if (isFullscreenColumn(m_columns[m_activeColumnIndex])) {
        return std::nullopt;
    }
    const std::optional<bool> backgroundBefore = fullscreenBackgroundBefore();
    if (!backgroundBefore) {
        return std::nullopt;
    }
    const QRectF area = areaForMode(WindowMode::Normal);
    const double width = m_columns[m_activeColumnIndex].width();
    const double padding = std::clamp((area.width() - width) / 2.0, 0.0, m_options->layout.gaps);
    return (*backgroundBefore ? -(area.width() - padding - width) : -padding) - area.x();
}

double ColumnStrip::constrainScrollPosition(double scrollPosition) const
{
    if (centersActiveColumn() || wrapPeriod() > 0.0) {
        return scrollPosition;
    }
    const double gaps = m_options->layout.gaps;
    const QRectF &view = m_area.workingArea;
    const std::size_t last = m_columns.size() - 1;
    const double rowWidth = columnOffset(last) + m_columns[last].width() + gaps * 2.0;
    const double alignedStart = -gaps - view.x();
    if (rowWidth > view.width()) {
        return std::clamp(scrollPosition, alignedStart, alignedStart + rowWidth - view.width());
    }
    return alignedStart;
}

bool ColumnStrip::centersActiveColumn() const
{
    return m_options->layout.centerFocusedColumn == Config::CenterFocusedColumn::Always
        || (m_options->layout.alwaysCenterSingleColumn && m_columns.size() <= 1);
}

QRectF ColumnStrip::areaForMode(WindowMode mode) const
{
    return mode == WindowMode::Maximized ? m_area.parentArea : m_area.workingArea;
}

double ColumnStrip::fitScroll(std::optional<double> targetX, double colX, double width, WindowMode mode) const
{
    if (mode == WindowMode::Fullscreen) {
        return 0.0;
    }
    const QRectF area = areaForMode(mode);
    const double padding = mode == WindowMode::Maximized ? 0.0 : m_options->layout.gaps;
    const double tx = targetX.value_or(targetScrollPosition());
    return scrollToReveal(tx + area.x(), area.width(), colX, width, padding) - area.x();
}

double ColumnStrip::centeredScroll(std::optional<double> targetX, double colX, double width, WindowMode mode) const
{
    const QRectF area = areaForMode(mode);
    if (mode == WindowMode::Fullscreen || area.width() <= width) {
        return fitScroll(targetX, colX, width, mode);
    }
    return -(area.width() - width) / 2.0 - area.x();
}

double ColumnStrip::fitScrollForColumn(std::optional<double> targetX, std::size_t idx, std::optional<std::size_t> from) const
{
    const Column &column = m_columns[idx];
    const double colX = columnXReached(targetX, idx, from);
    return fitScroll(targetX, colX, column.width(), column.sizingMode());
}

double ColumnStrip::centeredScrollForColumn(std::optional<double> targetX, std::size_t idx, std::optional<std::size_t> from) const
{
    const Column &column = m_columns[idx];
    const double colX = columnXReached(targetX, idx, from);
    return centeredScroll(targetX, colX, column.width(), column.sizingMode());
}

double ColumnStrip::overflowScrollForColumn(std::optional<double> targetX, std::size_t idx, std::size_t prevIndex) const
{
    if (prevIndex == idx) {
        return fitScrollForColumn(targetX, idx, std::nullopt);
    }
    const bool forward = columnXFrom(prevIndex, idx) > columnOffset(prevIndex);
    const std::size_t sourceIndex = neighbourIndex(idx, !forward).value_or(idx);
    const double totalWidth = m_columns[sourceIndex].width() + m_columns[idx].width() + m_options->layout.gaps * 3.0;
    if (totalWidth <= m_area.workingArea.width()) {
        return fitScrollForColumn(targetX, idx, prevIndex);
    }
    return centeredScrollForColumn(targetX, idx, prevIndex);
}

double ColumnStrip::scrollForColumn(std::optional<double> targetX, std::size_t idx, std::optional<std::size_t> prevIndex) const
{
    if (centersActiveColumn()) {
        return centeredScrollForColumn(targetX, idx, prevIndex);
    }
    if (m_options->layout.centerFocusedColumn == Config::CenterFocusedColumn::OnOverflow && prevIndex) {
        return overflowScrollForColumn(targetX, idx, *prevIndex);
    }
    return fitScrollForColumn(targetX, idx, prevIndex);
}

void ColumnStrip::animateScroll(std::size_t idx, double newViewOffset)
{
    animateScrollWith(idx, newViewOffset, m_options->animations.horizontalViewMovement);
}

void ColumnStrip::animateScrollWith(std::size_t idx, double newViewOffset, const Config::AnimationParams &config)
{
    if (idx == m_activeColumnIndex) {
        m_scroll.offset(nearestPeriodCopy(0.0, newViewOffset - m_scroll.target(), wrapPeriod()));
    } else {
        m_scroll.offset(columnOffset(m_activeColumnIndex) - columnXFrom(m_activeColumnIndex, idx));
    }

    const double pixel = 1.0 / m_area.scale;
    const double toDiff = newViewOffset - m_scroll.target();
    if (std::abs(toDiff) < pixel) {
        m_scroll.offset(toDiff);
        return;
    }

    auto *g = m_scroll.swipe();
    if (g && g->edgeScrollLastTime) {
        g->restingPosition = newViewOffset;
        const double currentPos = g->swipePosition - g->trackerBase;
        g->trackerBase = newViewOffset - currentPos;
        const double offsetDelta = newViewOffset - g->swipePosition;
        g->swipePosition = newViewOffset;
        g->startAnimationFrom(-offsetDelta, m_clock, config);
        return;
    }
    m_scroll.setAnimation(Anim::Animation(m_clock, m_scroll.current(), newViewOffset, 0.0, config));
}

void ColumnStrip::scrollToColumnCentered(std::optional<double> targetX, std::size_t idx, const Config::AnimationParams &config)
{
    animateScrollWith(idx, centeredScrollForColumn(targetX, idx, std::nullopt), config);
}

void ColumnStrip::scrollToColumnWith(
    std::optional<double> targetX, std::size_t idx, std::optional<std::size_t> prevIndex, const Config::AnimationParams &config)
{
    animateScrollWith(idx, scrollForColumn(targetX, idx, prevIndex), config);
}

void ColumnStrip::scrollToColumn(std::optional<double> targetX, std::size_t idx, std::optional<std::size_t> prevIndex)
{
    scrollToColumnWith(targetX, idx, prevIndex, m_options->animations.horizontalViewMovement);
}

void ColumnStrip::activateColumn(std::size_t idx)
{
    selectColumnWith(idx, m_options->animations.horizontalViewMovement);
}

void ColumnStrip::selectColumnWith(std::size_t idx, const Config::AnimationParams &config)
{
    if (m_activeColumnIndex == idx && (m_columns.empty() || !m_scroll.isEdgeScrolling())) {
        return;
    }
    scrollToColumnWith(std::nullopt, idx, m_activeColumnIndex, config);
    if (m_activeColumnIndex != idx) {
        m_activeColumnIndex = idx;
        m_returnColumnOnClose.reset();
        m_scrollToRestore.reset();
        m_resize.reset();
    }
}

void ColumnStrip::updateActiveColumnViewAfterUpdate(
    std::size_t colIndex, std::optional<quint8> resizeEdges, double offset, bool wasNormal, bool ongoingResizeAnim)
{
    if (offset != 0.0 && resizeEdges) {
        m_scroll.cancelSwipe();
        double delta = 0.0;
        if (centersActiveColumn()) {
            const double width = m_columns[colIndex].width();
            delta = (-(m_area.workingArea.width() - width) / 2.0 - m_area.workingArea.x()) - m_scroll.target();
        } else if ((*resizeEdges & static_cast<quint8>(ResizeEdge::Left)) != 0) {
            delta = -offset;
        }
        m_scroll.offset(delta);
    }

    const bool isNormal = m_columns[colIndex].sizingMode() == WindowMode::Normal;
    if (wasNormal && !isNormal) {
        m_scrollToRestore = m_scroll.resting();
    }
    std::optional<double> unfullscreenOffset;
    if (!wasNormal && isNormal) {
        unfullscreenOffset = std::exchange(m_scrollToRestore, std::nullopt);
    }

    if (m_resize || m_scroll.isSwiping()) {
        return;
    }
    const Config::AnimationParams config
        = ongoingResizeAnim ? m_options->animations.windowResize : m_options->animations.horizontalViewMovement;
    if (unfullscreenOffset) {
        animateScrollWith(colIndex, *unfullscreenOffset, config);
    }
    scrollToColumnWith(std::nullopt, colIndex, std::nullopt, config);
}

}
