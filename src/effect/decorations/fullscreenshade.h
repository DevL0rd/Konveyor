#pragma once

#include "layout/engine/engine.h"

#include <QImage>
#include <QPointer>

#include <map>
#include <memory>

namespace KWin
{
class ImageItem;
class Window;
}

namespace Konveyor
{

class WindowRegistry;

class FullscreenShade
{
public:
    explicit FullscreenShade(const WindowRegistry &windows);
    ~FullscreenShade();

    void update(const QList<Layout::WindowState> &states);
    void remove(Layout::WindowId id);

private:
    struct Coverage
    {
        double left = 0.0;
        double right = 0.0;
    };

    struct Side
    {
        std::unique_ptr<KWin::ImageItem> solid;
        std::unique_ptr<KWin::ImageItem> fade;
    };

    struct Shades
    {
        QPointer<KWin::Window> window;
        Side left;
        Side right;
    };

    static Coverage coverageOver(const Layout::WindowState &fullscreen, const QRectF &output, const QList<Layout::WindowState> &states);
    static void place(Side &side, KWin::Window *window, double covered, bool fromRight);
    static void placeItem(std::unique_ptr<KWin::ImageItem> &item, KWin::Window *window, const QImage &image, const QRectF &rect);

    const WindowRegistry &m_windows;
    std::map<Layout::WindowId, Shades> m_shades;
};

}
