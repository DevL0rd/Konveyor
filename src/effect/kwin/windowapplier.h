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
    void forget(Layout::WindowId id);

private:
    static QRectF frameFor(const Layout::WindowState &state);
    void applyGeometry(KWin::Window *window, const Layout::WindowState &state) const;
    static void applyBorderRadius(KWin::Window *window, const Layout::WindowState &state);
    void applySizingMode(KWin::Window *window, const Layout::WindowState &state) const;
    void applyStacking(const QList<Layout::WindowState> &states);

    const WindowRegistry &m_windows;
    QHash<Layout::WindowId, QSizeF> m_appliedSizes;
    bool m_applying = false;
};

}
