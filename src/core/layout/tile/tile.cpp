#include "layout/tile/tile.h"

#include "layout/common/geometry.h"

#include <algorithm>
#include <cmath>
#include <utility>

namespace Konveyor::Layout
{

namespace
{

constexpr double MinAnimatedResizeDelta = 10.0;

QSizeF lerpSize(QSizeF from, QSizeF to, double t)
{
    return {from.width() + (to.width() - from.width()) * t, from.height() + (to.height() - from.height()) * t};
}

double maxComponentDelta(QSizeF a, QSizeF b)
{
    return std::max(std::abs(a.width() - b.width()), std::abs(a.height() - b.height()));
}

QSizeF animatedSize(QSizeF current, const std::optional<ResizeTransition> &resize, QSizeF ResizeTransition::*from, double scale)
{
    if (!resize) {
        return current;
    }
    const QSizeF lerped = lerpSize((*resize).*from, current, resize->anim.value());
    return roundSize(QSizeF(std::max(1.0, lerped.width()), std::max(1.0, lerped.height())), scale);
}

}

Tile::Tile(LayoutWindow window, QSizeF viewSize, double scale, const Anim::Clock &clock, OptionsPtr options)
    : m_window(std::move(window))
    , m_sizingMode(m_window.sizingMode())
    , m_viewSize(viewSize)
    , m_scale(scale)
    , m_clock(clock)
    , m_options(std::move(options))
{
    updateDecorations();
}

void Tile::updateConfig(QSizeF viewSize, double scale, OptionsPtr options)
{
    if (m_options->layout.presetColumnWidths != options->layout.presetColumnWidths) {
        floatingWidthPresetIndex.reset();
    }
    if (m_options->layout.presetWindowHeights != options->layout.presetWindowHeights) {
        floatingHeightPresetIndex.reset();
    }
    m_viewSize = viewSize;
    m_scale = scale;
    m_options = std::move(options);
    updateDecorations();
}

void Tile::updateDecorations()
{
    const auto &rules = m_window.rules();
    m_border = mergeBorder(m_options->layout.border, rules.border);
    m_border.width = snapToPixelsAtLeastOne(m_scale, m_border.width);
    m_focusRing = mergeBorder(m_options->layout.focusRing, rules.focusRing);
    m_focusRing.width = snapToPixelsAtLeastOne(m_scale, m_focusRing.width);
}

QSizeF Tile::outerSizeForMode(QSizeF windowSize, WindowMode mode) const
{
    if (mode == WindowMode::Fullscreen) {
        return {std::max(windowSize.width(), m_viewSize.width()), std::max(windowSize.height(), m_viewSize.height())};
    }
    if (mode == WindowMode::Normal && m_border.enabled) {
        return windowSize + QSizeF(m_border.width * 2.0, m_border.width * 2.0);
    }
    return windowSize;
}

void Tile::startResizeAnimation(QSizeF snapshot, WindowMode previousMode)
{
    QSizeF sizeFrom = snapshot;
    QSizeF tileSizeFrom = outerSizeForMode(snapshot, previousMode);
    if (auto previous = std::exchange(m_resizeAnimation, std::nullopt)) {
        const double val = previous->anim.value();
        sizeFrom = lerpSize(previous->sizeFrom, snapshot, val);
        tileSizeFrom = lerpSize(previous->tileSizeFrom, tileSizeFrom, val);
    }

    const double change = std::max(maxComponentDelta(m_window.size(), sizeFrom), maxComponentDelta(outerSize(), tileSizeFrom));
    if (change <= MinAnimatedResizeDelta) {
        return;
    }

    Anim::Animation anim(m_clock, 0.0, 1.0, 0.0, m_options->animations.windowResize);
    m_resizeAnimation = ResizeTransition {std::move(anim), sizeFrom, tileSizeFrom};
}

void Tile::updateWindow()
{
    const WindowMode previousMode = m_sizingMode;
    m_sizingMode = m_window.sizingMode();
    if (auto snapshot = m_window.takeResizeStartSize()) {
        startResizeAnimation(*snapshot, previousMode);
    }
    updateDecorations();
}

void Tile::tickAnimations()
{
    clearIfDone(m_openingAnimation);
    if (m_resizeAnimation && m_resizeAnimation->anim.isFinished()) {
        m_resizeAnimation.reset();
    }
    clearIfDone(m_slideX);
    clearIfDone(m_slideY);
    if (m_alphaAnimation && !m_alphaAnimation->holdAfterDone && m_alphaAnimation->anim.isFinished()) {
        m_alphaAnimation.reset();
    }
}

bool Tile::isAnimating() const
{
    const bool alphaOngoing = m_alphaAnimation && !m_alphaAnimation->anim.isFinished();
    return m_openingAnimation || m_resizeAnimation || m_slideX || m_slideY || alphaOngoing;
}

QPointF Tile::animationOffset() const
{
    return QPointF(optionalOffset(m_slideX), optionalOffset(m_slideY)) + dragOffset;
}

void Tile::animateOpening()
{
    m_openingAnimation = Anim::Animation(m_clock, 0.0, 1.0, 0.0, m_options->animations.windowOpen);
}

const Anim::Animation *Tile::resizeAnimation() const
{
    return m_resizeAnimation ? &m_resizeAnimation->anim : nullptr;
}

void Tile::slideFrom(QPointF from)
{
    slideFromWith(from, m_options->animations.windowMovement);
}

void Tile::slideFromWith(QPointF from, const Config::AnimationParams &config)
{
    m_slideX = startSlide(m_slideX, m_clock, from.x() + dragOffset.x(), config, true);
    slideYFromWith(from.y(), config);
}

void Tile::slideYFrom(double from)
{
    slideYFromWith(from, m_options->animations.windowMovement);
}

void Tile::slideYFromWith(double from, const Config::AnimationParams &config)
{
    m_slideY = startSlide(m_slideY, m_clock, from + dragOffset.y(), config, true);
}

void Tile::shiftSlideY(double offset)
{
    shiftSlide(m_slideY, offset);
}

void Tile::stopSlides()
{
    m_slideX.reset();
    m_slideY.reset();
}

void Tile::fadeOpacity(double from, double to, const Config::AnimationParams &config)
{
    from = std::clamp(from, 0.0, 1.0);
    to = std::clamp(to, 0.0, 1.0);
    const double current = m_alphaAnimation ? m_alphaAnimation->anim.valueWithoutOvershoot() : from;
    m_alphaAnimation = FadeAnimation {Anim::Animation(m_clock, current, to, 0.0, config), false};
}

void Tile::ensureFadesToOpaque()
{
    if (m_alphaAnimation && m_alphaAnimation->anim.to() != 1.0) {
        m_alphaAnimation.reset();
    }
}

void Tile::keepFadeAfterFinish()
{
    if (m_alphaAnimation) {
        m_alphaAnimation->holdAfterDone = true;
    }
}

double Tile::alpha() const
{
    return m_alphaAnimation ? std::clamp(m_alphaAnimation->anim.valueWithoutOvershoot(), 0.0, 1.0) : 1.0;
}

std::optional<double> Tile::borderThickness() const
{
    if (m_sizingMode != WindowMode::Normal || !m_border.enabled) {
        return std::nullopt;
    }
    return m_border.width;
}

QPointF Tile::windowOffset() const
{
    const QSizeF windowSize = visibleWindowSize();
    const QSizeF target = visibleOuterSize();
    const QPointF loc((target.width() - windowSize.width()) / 2.0, (target.height() - windowSize.height()) / 2.0);
    return roundPoint(loc, m_scale);
}

QSizeF Tile::targetWindowSize() const
{
    const auto requested = m_window.requestedSize();
    if (!requested) {
        return pendingWindowSize();
    }
    const QSizeF current = m_window.size();
    const QSizeF size(
        requested->width() > 0 ? requested->width() : current.width(), requested->height() > 0 ? requested->height() : current.height());
    return roundSize(size, m_scale);
}

QSizeF Tile::targetOuterSize() const
{
    return outerSizeForMode(targetWindowSize(), m_window.requestedMode());
}

QPointF Tile::targetWindowOffset() const
{
    const QSizeF window = targetWindowSize();
    const QSizeF target = targetOuterSize();
    return roundPoint(QPointF((target.width() - window.width()) / 2.0, (target.height() - window.height()) / 2.0), m_scale);
}

QSizeF Tile::outerSize() const
{
    return outerSizeForMode(windowSize(), m_sizingMode);
}

QSizeF Tile::pendingOuterSize() const
{
    return outerSizeForMode(pendingWindowSize(), m_sizingMode);
}

QSizeF Tile::windowSize() const
{
    return roundSize(m_window.size(), m_scale);
}

QSizeF Tile::pendingWindowSize() const
{
    const auto expected = m_window.pendingSize();
    const QSizeF size = expected ? QSizeF(*expected) : m_window.size();
    return roundSize(size, m_scale);
}

QSizeF Tile::visibleWindowSize() const
{
    return animatedSize(windowSize(), m_resizeAnimation, &ResizeTransition::sizeFrom, m_scale);
}

QSizeF Tile::visibleOuterSize() const
{
    return animatedSize(outerSize(), m_resizeAnimation, &ResizeTransition::tileSizeFrom, m_scale);
}

void Tile::requestOuterSize(QSizeF size, bool animate)
{
    if (m_border.enabled) {
        size = QSizeF(std::max(1.0, size.width() - m_border.width * 2.0), std::max(1.0, size.height() - m_border.width * 2.0));
    }
    m_window.requestSize(floorSize(size), WindowMode::Normal, animate);
}

void Tile::requestMaximized(QSizeF size, bool animate)
{
    m_window.requestSize(roundedSize(size), WindowMode::Maximized, animate);
}

void Tile::requestFullscreen(bool animate)
{
    m_window.requestSize(roundedSize(m_viewSize), WindowMode::Fullscreen, animate);
}

double Tile::outerWidthFor(double size) const
{
    return m_border.enabled ? size + m_border.width * 2.0 : size;
}

double Tile::outerHeightFor(double size) const
{
    return outerWidthFor(size);
}

double Tile::innerWidthFor(double size) const
{
    return m_border.enabled ? size - m_border.width * 2.0 : size;
}

double Tile::innerHeightFor(double size) const
{
    return innerWidthFor(size);
}

QSizeF Tile::minNormalSize() const
{
    QSizeF size = m_window.minSize();
    if (m_border.enabled) {
        const double extra = m_border.width * 2.0;
        size = QSizeF(std::max(1.0, size.width()) + extra, std::max(1.0, size.height()) + extra);
    }
    return size;
}

QSizeF Tile::maxNormalSize() const
{
    QSizeF size = m_window.maxSize();
    if (!m_border.enabled) {
        return size;
    }
    const double extra = m_border.width * 2.0;
    const double w = size.width() > 0.0 ? size.width() + extra : size.width();
    const double h = size.height() > 0.0 ? size.height() + extra : size.height();
    return {w, h};
}

}
