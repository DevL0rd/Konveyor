#include "decorations/fullscreenshade.h"

#include "kwin/windowregistry.h"

#include <scene/imageitem.h>
#include <scene/windowitem.h>
#include <window.h>

#include <QSet>

#include <algorithm>

namespace Konveyor
{

namespace
{

constexpr int GradientResolution = 256;
constexpr double DarkestAlpha = 0.6;
constexpr double FadeFraction = 0.25;

const QImage &darkest()
{
    static const QImage image = [] {
        QImage pixel(1, 1, QImage::Format_ARGB32_Premultiplied);
        pixel.setPixelColor(0, 0, QColor::fromRgbF(0.0, 0.0, 0.0, static_cast<float>(DarkestAlpha)));
        return pixel;
    }();
    return image;
}

const QImage &gradient(bool fromRight)
{
    static const auto build = [](bool mirrored) {
        QImage image(GradientResolution, 1, QImage::Format_ARGB32_Premultiplied);
        for (int x = 0; x < GradientResolution; ++x) {
            const double distance = static_cast<double>(mirrored ? GradientResolution - 1 - x : x) / (GradientResolution - 1);
            image.setPixelColor(x, 0, QColor::fromRgbF(0.0, 0.0, 0.0, static_cast<float>(DarkestAlpha * (1.0 - distance))));
        }
        return image;
    };
    static const QImage left = build(false);
    static const QImage right = build(true);
    return fromRight ? right : left;
}

}

FullscreenShade::FullscreenShade(const WindowRegistry &windows)
    : m_windows(windows)
{ }

FullscreenShade::~FullscreenShade() = default;

void FullscreenShade::update(const QList<Layout::WindowState> &states)
{
    QSet<Layout::WindowId> shaded;
    for (const Layout::WindowState &state : states) {
        KWin::Window *window = m_windows.windowOf(state.id);
        if (!window || !window->isFullScreen() || window->isActive() || !state.onActiveWorkspace || !window->windowItem()) {
            continue;
        }
        const QRectF output = window->frameGeometry();
        const Coverage coverage = coverageOver(state, output, states);
        Shades &shades = m_shades[state.id];
        shades.window = window;
        place(shades.left, window, coverage.left, false);
        place(shades.right, window, coverage.right, true);
        shaded.insert(state.id);
    }
    for (auto it = m_shades.begin(); it != m_shades.end();) {
        it = shaded.contains(it->first) ? std::next(it) : m_shades.erase(it);
    }
}

void FullscreenShade::remove(Layout::WindowId id)
{
    m_shades.erase(id);
}

FullscreenShade::Coverage FullscreenShade::coverageOver(
    const Layout::WindowState &fullscreen, const QRectF &output, const QList<Layout::WindowState> &states)
{
    Coverage coverage;
    for (const Layout::WindowState &other : states) {
        const bool sameRow = other.workspace == fullscreen.workspace && !other.isFloating && other.id != fullscreen.id;
        if (!sameRow || !other.visible || !other.renderFrame.intersects(output)) {
            continue;
        }
        if (other.columnIndex < fullscreen.columnIndex) {
            coverage.left = std::max(coverage.left, other.renderFrame.right() - output.left());
        } else if (other.columnIndex > fullscreen.columnIndex) {
            coverage.right = std::max(coverage.right, output.right() - other.renderFrame.left());
        }
    }
    return coverage;
}

void FullscreenShade::place(Side &side, KWin::Window *window, double covered, bool fromRight)
{
    if (covered <= 0.0) {
        side.solid.reset();
        side.fade.reset();
        return;
    }
    const QSizeF outputSize = window->frameGeometry().size();
    const double innerEdge = std::min(covered, outputSize.width());
    const double fadeWidth = std::min(outputSize.width() - innerEdge, outputSize.width() * FadeFraction);
    const double solidStart = fromRight ? outputSize.width() - innerEdge : 0.0;
    const double fadeStart = fromRight ? solidStart - fadeWidth : innerEdge;
    placeItem(side.solid, window, darkest(), QRectF(solidStart, 0.0, innerEdge, outputSize.height()));
    placeItem(side.fade, window, gradient(fromRight), QRectF(fadeStart, 0.0, fadeWidth, outputSize.height()));
}

void FullscreenShade::placeItem(std::unique_ptr<KWin::ImageItem> &item, KWin::Window *window, const QImage &image, const QRectF &rect)
{
    if (rect.width() <= 0.0) {
        item.reset();
        return;
    }
    if (!item) {
        item = std::make_unique<KWin::ImageItem>(window->windowItem());
        item->setZ(1);
        item->setImage(image);
    }
    item->setSize(rect.size());
    item->setPosition(rect.topLeft());
}

}
