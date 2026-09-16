#pragma once

#include "anim/animation.h"
#include "anim/clock.h"
#include "layout/common/layouttypes.h"
#include "layout/common/options.h"
#include "layout/common/slideanimation.h"
#include "layout/tile/window.h"

#include <QPointF>
#include <QSizeF>

#include <optional>

namespace Konveyor::Layout
{

struct ResizeTransition
{
    Anim::Animation anim;
    QSizeF sizeFrom;
    QSizeF tileSizeFrom;
};

struct FadeAnimation
{
    Anim::Animation anim;
    bool holdAfterDone = false;
};

class Tile;

struct TileRef
{
    Tile *tile = nullptr;
    QPointF pos;
    bool visible = true;
};

struct ConstTileRef
{
    const Tile *tile = nullptr;
    QPointF pos;
    bool visible = true;
};

class Tile
{
public:
    Tile(LayoutWindow window, QSizeF viewSize, double scale, const Anim::Clock &clock, OptionsPtr options);

    void updateConfig(QSizeF viewSize, double scale, OptionsPtr options);
    void updateWindow();
    void tickAnimations();
    bool isAnimating() const;

    const LayoutWindow &window() const { return m_window; }
    LayoutWindow &window() { return m_window; }
    WindowId id() const { return m_window.id(); }
    const OptionsPtr &options() const { return m_options; }
    const Anim::Clock &clock() const { return m_clock; }
    double scale() const { return m_scale; }
    QSizeF viewSize() const { return m_viewSize; }

    QPointF animationOffset() const;
    void animateOpening();
    const std::optional<Anim::Animation> &openingAnimation() const { return m_openingAnimation; }
    const Anim::Animation *resizeAnimation() const;

    void slideFrom(QPointF from);
    void slideFromWith(QPointF from, const Config::AnimationParams &config);
    void slideYFrom(double from);
    void slideYFromWith(double from, const Config::AnimationParams &config);
    void shiftSlideY(double offset);
    void stopSlides();

    void fadeOpacity(double from, double to, const Config::AnimationParams &config);
    void ensureFadesToOpaque();
    void keepFadeAfterFinish();
    const std::optional<FadeAnimation> &alphaAnimation() const { return m_alphaAnimation; }
    double alpha() const;

    WindowMode sizingMode() const { return m_sizingMode; }
    std::optional<double> borderThickness() const;
    const Config::Border &borderConfig() const { return m_border; }
    const Config::Border &focusRingConfig() const { return m_focusRing; }

    QPointF windowOffset() const;
    QPointF targetWindowOffset() const;
    QSizeF targetWindowSize() const;
    QSizeF targetOuterSize() const;
    QSizeF outerSize() const;
    QSizeF pendingOuterSize() const;
    QSizeF windowSize() const;
    QSizeF pendingWindowSize() const;
    QSizeF visibleWindowSize() const;
    QSizeF visibleOuterSize() const;

    void requestOuterSize(QSizeF size, bool animate);
    void requestMaximized(QSizeF size, bool animate);
    void requestFullscreen(bool animate);
    double outerWidthFor(double size) const;
    double outerHeightFor(double size) const;
    double innerWidthFor(double size) const;
    double innerHeightFor(double size) const;
    QSizeF minNormalSize() const;
    QSizeF maxNormalSize() const;

    bool returnsToFloating = false;
    std::optional<QSize> savedFloatingSize;
    std::optional<QPointF> savedFloatingPosition;
    std::optional<std::size_t> floatingWidthPresetIndex;
    std::optional<std::size_t> floatingHeightPresetIndex;
    QPointF dragOffset;

private:
    void updateDecorations();
    QSizeF outerSizeForMode(QSizeF windowSize, WindowMode mode) const;
    void startResizeAnimation(QSizeF snapshot, WindowMode previousMode);

    LayoutWindow m_window;
    Config::Border m_border;
    Config::Border m_focusRing;
    WindowMode m_sizingMode;
    std::optional<Anim::Animation> m_openingAnimation;
    std::optional<ResizeTransition> m_resizeAnimation;
    std::optional<SlideAnimation> m_slideX;
    std::optional<SlideAnimation> m_slideY;
    std::optional<FadeAnimation> m_alphaAnimation;
    QSizeF m_viewSize;
    double m_scale;
    Anim::Clock m_clock;
    OptionsPtr m_options;
};

struct DetachedTile
{
    Tile tile;
    ColumnWidth width;
    bool fillsWidth = false;
    bool isFloating = false;
};

}
