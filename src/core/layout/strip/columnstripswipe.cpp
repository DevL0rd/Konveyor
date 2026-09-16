#include "layout/strip/columnstrip.h"

#include "layout/common/geometry.h"

#include <algorithm>
#include <cmath>

namespace Konveyor::Layout
{

namespace
{

constexpr double SwipeDistancePerWorkArea = 1200.0;

StripSwipe makeGesture(const StripScroll &offset, bool isTouchpad, std::optional<Anim::Duration> dndTime)
{
    StripSwipe gesture;
    gesture.swipePosition = offset.current();
    gesture.trackerBase = offset.current();
    gesture.restingPosition = offset.resting();
    gesture.isTouchpad = isTouchpad;
    gesture.edgeScrollLastTime = dndTime;
    return gesture;
}

}

double ColumnStrip::columnPadding(const Column &column, QRectF area) const
{
    if (column.sizingMode() == WindowMode::Maximized) {
        return 0.0;
    }
    return std::clamp((area.width() - column.width()) / 2.0, 0.0, m_options->layout.gaps);
}

std::pair<double, double> ColumnStrip::columnSnapPoints(double colX, std::size_t idx) const
{
    const Column &column = m_columns[idx];
    const double colWidth = column.width();
    const WindowMode mode = column.sizingMode();
    if (mode == WindowMode::Fullscreen) {
        return {colX, colX + colWidth};
    }

    const QRectF area = areaForMode(mode);
    const double leftStrut = area.x();
    const double rightStrut = m_area.viewSize.width() - area.width() - area.x();
    const double padding = columnPadding(column, area);
    const double center = area.width() <= colWidth ? colX - leftStrut : colX - (area.width() - colWidth) / 2.0 - leftStrut;

    const bool centerOnOverflow = m_options->layout.centerFocusedColumn == Config::CenterFocusedColumn::OnOverflow;
    const double gaps = m_options->layout.gaps;
    const auto overflows
        = [&](std::optional<double> adjacent) { return centerOnOverflow && adjacent && *adjacent + 3.0 * gaps + colWidth > area.width(); };
    std::optional<double> previousWidth;
    std::optional<double> nextWidth;
    if (idx > 0) {
        previousWidth = m_columns[idx - 1].width();
    }
    if (idx + 1 < m_columns.size()) {
        nextWidth = m_columns[idx + 1].width();
    }

    const double left = overflows(nextWidth) ? center : colX - padding - leftStrut;
    const double right = overflows(previousWidth) ? center + m_area.viewSize.width() : colX + colWidth + padding + rightStrut;
    return {left, right};
}

std::vector<ColumnStrip::Snap> ColumnStrip::centeredSnapPoints() const
{
    std::vector<Snap> snaps;
    snaps.reserve(m_columns.size());
    for (std::size_t idx = 0; idx < m_columns.size(); ++idx) {
        const Column &column = m_columns[idx];
        const double colX = columnOffset(idx);
        const double colWidth = column.width();
        const WindowMode mode = column.sizingMode();
        if (mode == WindowMode::Fullscreen) {
            snaps.push_back({colX, idx});
            continue;
        }
        const QRectF area = areaForMode(mode);
        const double viewPosition = area.width() <= colWidth ? colX - area.x() : colX - (area.width() - colWidth) / 2.0 - area.x();
        snaps.push_back({viewPosition, idx});
    }
    return snaps;
}

std::vector<ColumnStrip::Snap> ColumnStrip::edgeSnapPoints() const
{
    const double viewWidth = m_area.viewSize.width();
    const std::size_t lastIndex = m_columns.size() - 1;
    const double leftmost = columnSnapPoints(0.0, 0).first;
    const double rightmost = columnSnapPoints(columnOffset(lastIndex), lastIndex).second - viewWidth;

    std::vector<Snap> snaps;
    snaps.push_back({leftmost, 0});
    snaps.push_back({rightmost, lastIndex});
    for (std::size_t idx = 0; idx < m_columns.size(); ++idx) {
        const auto [left, right] = columnSnapPoints(columnOffset(idx), idx);
        if (leftmost < left && left < rightmost) {
            snaps.push_back({left, idx});
        }
        const double rightEdge = right - viewWidth;
        if (leftmost < rightEdge && rightEdge < rightmost) {
            snaps.push_back({rightEdge, idx});
        }
    }
    return snaps;
}

bool ColumnStrip::columnStaysInView(std::size_t idx, double snapViewPos, bool forward) const
{
    const Column &column = m_columns[idx];
    const double colX = columnOffset(idx);
    const WindowMode mode = column.sizingMode();
    const bool fullscreen = mode == WindowMode::Fullscreen;
    const QRectF area = areaForMode(mode);
    const double padding = fullscreen ? 0.0 : columnPadding(column, area);
    const double leftStrut = fullscreen ? 0.0 : area.x();
    if (!forward) {
        return colX - padding >= snapViewPos + leftStrut;
    }
    const double reach = fullscreen ? m_area.viewSize.width() : leftStrut + area.width();
    return snapViewPos + reach >= colX + column.width() + padding;
}

std::size_t ColumnStrip::furthestColumnInDirection(std::size_t snapColIndex, double snapViewPos, bool forward) const
{
    std::size_t result = snapColIndex;
    const std::size_t count = m_columns.size();
    for (std::size_t step = 1; step <= count; ++step) {
        const bool exhausted = forward ? snapColIndex + step >= count : snapColIndex < step;
        const std::size_t idx = forward ? snapColIndex + step : snapColIndex - step;
        if (exhausted || !columnStaysInView(idx, snapViewPos, forward)) {
            break;
        }
        result = idx;
    }
    return result;
}

void ColumnStrip::beginSwipe(bool isTouchpad)
{
    if (m_columns.empty() || m_resize) {
        return;
    }
    m_scroll.setSwipe(makeGesture(m_scroll, isTouchpad, std::nullopt));
}

void ColumnStrip::beginEdgeScroll()
{
    if (m_scroll.isEdgeScrolling()) {
        return;
    }
    m_scroll.setSwipe(makeGesture(m_scroll, false, m_clock.rawNow()));
    m_resize.reset();
}

std::optional<bool> ColumnStrip::updateSwipe(double deltaX, Anim::Duration timestamp, bool isTouchpad)
{
    StripSwipe *gesture = m_scroll.swipe();
    if (!gesture || gesture->isTouchpad != isTouchpad || gesture->edgeScrollLastTime) {
        return std::nullopt;
    }
    gesture->tracker.push(deltaX, timestamp);
    const double factor = gesture->isTouchpad ? m_area.workingArea.width() / SwipeDistancePerWorkArea : 1.0;
    gesture->swipePosition = gesture->tracker.pos() * factor + gesture->trackerBase;
    return true;
}

std::pair<double, double> ColumnStrip::dndScrollBounds() const
{
    if (m_columns.empty()) {
        return {0.0, 0.0};
    }
    const std::size_t lastIndex = m_columns.size() - 1;
    const double activeColX = columnOffset(m_activeColumnIndex);
    const double leftmost = -m_area.workingArea.width() - activeColX;
    const double rightmost = columnOffset(lastIndex) + m_columns[lastIndex].width() - m_area.workingArea.x() - activeColX;
    return {std::min(leftmost, rightmost), std::max(leftmost, rightmost)};
}

bool ColumnStrip::edgeScrollBy(double delta)
{
    StripSwipe *gesture = m_scroll.swipe();
    if (!gesture || !gesture->edgeScrollLastTime) {
        return false;
    }
    const EdgeScrollStep step = stepEdgeScroll(gesture->tracker, gesture->edgeScrollLastTime, gesture->edgeScrollActiveSince,
        m_clock.rawNow(), delta, m_options->gestures.dndEdgeViewScroll);
    if (!step.scrolled) {
        return step.handled;
    }
    const double viewOffset = step.position + gesture->trackerBase;
    const auto [minOffset, maxOffset] = dndScrollBounds();
    const double clamped = std::clamp(viewOffset, minOffset, maxOffset);
    gesture->trackerBase += clamped - viewOffset;
    gesture->swipePosition = clamped;
    return true;
}

void ColumnStrip::endEdgeScroll()
{
    StripSwipe *gesture = m_scroll.swipe();
    if (!gesture) {
        return;
    }
    if (!gesture->edgeScrollLastTime || gesture->tracker.pos() != 0.0) {
        endSwipe(std::nullopt);
        return;
    }

    if (auto animation = std::exchange(gesture->animation, std::nullopt)) {
        animation->offset(gesture->swipePosition);
        m_scroll.setAnimation(std::move(*animation));
    } else {
        m_scroll.setIdle(gesture->trackerBase);
    }
    if (!m_columns.empty()) {
        scrollToColumn(std::nullopt, m_activeColumnIndex, std::nullopt);
    }
}

bool ColumnStrip::endSwipe(std::optional<bool> isTouchpad, std::optional<WindowId> keepActive)
{
    StripSwipe *gesture = m_scroll.swipe();
    if (!gesture || (isTouchpad && gesture->isTouchpad != *isTouchpad)) {
        return false;
    }
    gesture->tracker.push(0.0, m_clock.rawNow());

    const double factor = gesture->isTouchpad ? m_area.workingArea.width() / SwipeDistancePerWorkArea : 1.0;
    const double velocity = gesture->tracker.velocity() * factor;
    const double swipePosition = gesture->tracker.pos() * factor + gesture->trackerBase;
    const double targetViewOffset = gesture->tracker.projectedPosition() * factor + gesture->trackerBase;

    if (m_columns.empty()) {
        m_scroll.setIdle(swipePosition);
        return true;
    }

    const Snap snap = closestSnapPoint(columnOffset(m_activeColumnIndex) + targetViewOffset);
    std::size_t newColIndex = snap.colIndex;
    if (const std::optional<Location> kept = keepActive ? locate(*keepActive) : std::nullopt) {
        newColIndex = kept->column;
    } else if (!centersActiveColumn()) {
        newColIndex = furthestColumnInDirection(newColIndex, snap.scrollPosition, targetViewOffset >= swipePosition);
    }

    const double delta = columnOffset(m_activeColumnIndex) - columnOffset(newColIndex);
    if (m_activeColumnIndex != newColIndex) {
        m_scrollToRestore.reset();
    }
    m_activeColumnIndex = newColIndex;
    m_scroll.setAnimation(Anim::Animation(m_clock, swipePosition + delta, snap.scrollPosition - columnOffset(newColIndex), velocity,
        m_options->animations.horizontalViewMovement));
    scrollToColumn(std::nullopt, newColIndex, std::nullopt);
    return true;
}

ColumnStrip::Snap ColumnStrip::closestSnapPoint(double targetScrollPosition) const
{
    const std::vector<Snap> snaps = centersActiveColumn() ? centeredSnapPoints() : edgeSnapPoints();
    const auto best = std::ranges::min_element(
        snaps, {}, [targetScrollPosition](const Snap &snap) { return std::abs(snap.scrollPosition - targetScrollPosition); });
    return *best;
}

}
