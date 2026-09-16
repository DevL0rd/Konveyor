#include "layout/strip/tabbar.h"

#include "layout/common/geometry.h"
#include "layout/common/slideanimation.h"

#include <algorithm>
#include <cmath>
#include <utility>

namespace Konveyor::Layout
{

namespace
{

bool isVertical(Config::TabIndicatorPosition position)
{
    return position == Config::TabIndicatorPosition::Left || position == Config::TabIndicatorPosition::Right;
}

QPointF firstTabLocation(QRectF area, const Config::TabIndicator &config, double width, double gap, double along)
{
    QPointF loc(-gap - width, along);
    switch (config.position) {
    case Config::TabIndicatorPosition::Left:
        break;
    case Config::TabIndicatorPosition::Right:
        loc.setX(area.width() + gap);
        break;
    case Config::TabIndicatorPosition::Top:
        loc = QPointF(loc.y(), loc.x());
        break;
    case Config::TabIndicatorPosition::Bottom:
        loc = QPointF(loc.y(), area.height() + gap);
        break;
    }
    return loc + area.topLeft();
}

struct TabLengths
{
    double perTab;
    double gapsBetween;
    double total;
    std::size_t onesLeft;
};

TabLengths computeLengths(const Config::TabIndicator &config, double side, std::size_t count, double scale, double progress)
{
    const double pixel = 1.0 / scale;
    const double count_ = static_cast<double>(count);
    const double gapsBetween = snapToPixelsAtLeastOne(scale, config.gapsBetweenTabs);
    const double minLength = snapToPixels(scale, side * std::clamp(config.lengthTotalProportion, 0.0, 2.0));
    const double shortest = count_ * (pixel + gapsBetween) - gapsBetween;
    const double length = std::max(minLength, shortest);
    const double perTab = ((length + gapsBetween) / count_ - gapsBetween) * progress;
    const double animatedGaps = snapToPixels(scale, config.gapsBetweenTabs * progress);
    const double animatedLength = count_ * (perTab + animatedGaps) - animatedGaps;
    const double floored = floorToPixelsAtLeastOne(scale, perTab);
    const double flooredLength = count_ * (floored + animatedGaps) - animatedGaps;
    const auto ones = static_cast<std::size_t>(std::max(0.0, std::round((animatedLength - flooredLength) / pixel)));
    return {floored, animatedGaps, animatedLength, ones};
}

}

TabBar::TabBar(const Config::TabIndicator &config)
    : m_config(config)
{ }

void TabBar::tickAnimations()
{
    clearIfDone(m_openingAnimation);
}

void TabBar::animateOpening(const Anim::Clock &clock, const Config::AnimationParams &config)
{
    m_openingAnimation = Anim::Animation(clock, 0.0, 1.0, 0.0, config);
}

QList<QRectF> TabBar::tabRects(QRectF area, std::size_t count, double scale) const
{
    QList<QRectF> rects;
    if (count == 0) {
        return rects;
    }
    const double progress = m_openingAnimation ? std::max(m_openingAnimation->value(), 0.0) : 1.0;
    const double width = snapToPixelsAtLeastOne(scale, m_config.width);
    const double gap = std::copysign(snapToPixelsAtLeastOne(scale, std::abs(m_config.gap)), m_config.gap);
    const bool vertical = isVertical(m_config.position);
    const double side = vertical ? area.height() : area.width();
    TabLengths lengths = computeLengths(m_config, side, count, scale, progress);
    const double along = snapToPixels(scale, (side - lengths.total) / 2.0);
    QPointF loc = firstTabLocation(area, m_config, width, gap, along);
    const double pixel = 1.0 / scale;
    for (std::size_t i = 0; i < count; ++i) {
        const double perTab = lengths.perTab + (lengths.onesLeft > 0 ? pixel : 0.0);
        lengths.onesLeft -= lengths.onesLeft > 0 ? 1 : 0;
        const QSizeF size = vertical ? QSizeF(width, perTab) : QSizeF(perTab, width);
        rects.append(QRectF(loc, size));
        loc += vertical ? QPointF(0.0, perTab + lengths.gapsBetween) : QPointF(perTab + lengths.gapsBetween, 0.0);
    }
    return rects;
}

bool TabBar::isShown(std::size_t count) const
{
    return m_config.enabled && (!m_config.hideWhenSingleTab || count != 1);
}

std::optional<std::size_t> TabBar::hit(QRectF area, std::size_t count, double scale, QPointF point) const
{
    if (!isShown(count)) {
        return std::nullopt;
    }
    const QList<QRectF> rects = tabRects(area, count, scale);
    for (qsizetype i = 0; i < rects.size(); ++i) {
        if (rects[i].contains(point)) {
            return static_cast<std::size_t>(i);
        }
    }
    return std::nullopt;
}

QSizeF TabBar::reservedSize(std::size_t count, double scale) const
{
    if (!isShown(count) || !m_config.placeWithinColumn) {
        return {0.0, 0.0};
    }
    const double size = std::max(0.0, snapToPixels(scale, m_config.width) + snapToPixels(scale, m_config.gap));
    return isVertical(m_config.position) ? QSizeF(size, 0.0) : QSizeF(0.0, size);
}

QPointF TabBar::contentOffset(std::size_t count, double scale) const
{
    const auto position = m_config.position;
    if (position == Config::TabIndicatorPosition::Left || position == Config::TabIndicatorPosition::Top) {
        const QSizeF extra = reservedSize(count, scale);
        return {extra.width(), extra.height()};
    }
    return {0.0, 0.0};
}

}
