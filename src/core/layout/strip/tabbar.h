#pragma once

#include "anim/animation.h"
#include "anim/clock.h"
#include "config/types.h"

#include <QList>
#include <QPointF>
#include <QRectF>
#include <QSizeF>

#include <cstddef>
#include <optional>

namespace Konveyor::Layout
{

class TabBar
{
public:
    explicit TabBar(const Config::TabIndicator &config);

    void updateConfig(const Config::TabIndicator &config) { m_config = config; }
    const Config::TabIndicator &config() const { return m_config; }
    void tickAnimations();
    bool isAnimating() const { return m_openingAnimation.has_value(); }
    void animateOpening(const Anim::Clock &clock, const Config::AnimationParams &config);

    QList<QRectF> tabRects(QRectF area, std::size_t count, double scale) const;
    bool isShown(std::size_t count) const;
    std::optional<std::size_t> hit(QRectF area, std::size_t count, double scale, QPointF point) const;
    QSizeF reservedSize(std::size_t count, double scale) const;
    QPointF contentOffset(std::size_t count, double scale) const;

private:
    Config::TabIndicator m_config;
    std::optional<Anim::Animation> m_openingAnimation;
};

}
