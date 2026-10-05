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
    void applyGeometry(
        KWin::Window *window, const QRectF &frame, const std::optional<QSizeF> &requestedSize, bool tiled, bool allowResize) const;
    static void applyBorderRadius(KWin::Window *window, const Layout::WindowState &state);
    void applySizingMode(KWin::Window *window, const Layout::WindowState &state) const;
    void applyStacking(const QList<Layout::WindowState> &states);
    void logTargetChange(KWin::Window *window, const Layout::WindowState &state);

    struct LoggedTarget
    {
        QRectF frame;
        QString output;
        Layout::WorkspaceId workspace = 0;
        int column = 0;
        bool floating = false;
        Layout::WindowMode sizing = Layout::WindowMode::Normal;
        Layout::WindowMode requested = Layout::WindowMode::Normal;
        bool windowedFullscreen = false;
        bool kwinFullscreen = false;
        bool operator==(const LoggedTarget &) const = default;
    };

    const WindowRegistry &m_windows;
    QHash<Layout::WindowId, QRectF> m_appliedFrames;
    QHash<Layout::WindowId, LoggedTarget> m_loggedTargets;
    bool m_applying = false;
};

}
