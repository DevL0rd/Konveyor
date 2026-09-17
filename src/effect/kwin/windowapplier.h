#pragma once

#include "layout/engine/engine.h"

#include <QHash>
#include <QList>
#include <QRectF>

#include <optional>

namespace KWin
{
class Window;
}

namespace Konveyor
{

class WindowRegistry;

class WindowApplier
{
public:
    explicit WindowApplier(const WindowRegistry &windows);

    void apply(const QList<Layout::WindowState> &states);
    bool isApplying() const;
    bool isEchoOfAppliedSize(Layout::WindowId id, const QSizeF &size) const;
    bool isEchoOfAppliedFrame(Layout::WindowId id, const QRectF &frame) const;
    void forget(Layout::WindowId id);

private:
    static QRectF frameFor(const Layout::WindowState &state);
    static QRectF placedFrame(const Layout::WindowState &state);
    void applyGeometry(KWin::Window *window, const QRectF &frame, const std::optional<QSizeF> &requestedSize, bool tiled) const;
    static void applyBorderRadius(KWin::Window *window, const Layout::WindowState &state);
    void applySizingMode(KWin::Window *window, const Layout::WindowState &state) const;
    void applyStacking(const QList<Layout::WindowState> &states);

    const WindowRegistry &m_windows;
    QHash<Layout::WindowId, QRectF> m_appliedFrames;
    bool m_applying = false;
};

}
