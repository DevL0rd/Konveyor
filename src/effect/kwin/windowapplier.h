#pragma once

#include "layout/engine/engine.h"

#include <QHash>
#include <QList>
#include <QPointer>
#include <QRectF>
#include <QSet>

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
    ~WindowApplier();

    void apply(const QList<Layout::WindowState> &states);
    bool isApplying() const;
    bool isEchoOfAppliedSize(Layout::WindowId id, const QSizeF &size) const;
    bool consumeInFlightSize(Layout::WindowId id, const QSizeF &size);
    bool isEchoOfAppliedFrame(Layout::WindowId id, const QRectF &frame) const;
    void forget(Layout::WindowId id);

private:
    bool keepsStrictSize(KWin::Window *window, const Layout::WindowState &state, const QRectF &frame);
    static QRectF frameFor(const Layout::WindowState &state);
    static QRectF placedFrame(const Layout::WindowState &state);
    void applyGeometry(KWin::Window *window, const QRectF &frame, const std::optional<QSizeF> &requestedSize, bool tiled, bool allowResize,
        bool fullscreenPresentation) const;
    void applyOpacity(KWin::Window *window, const Layout::WindowState &state);
    static void applyBorderRadius(KWin::Window *window, const Layout::WindowState &state);
    void applySizingMode(KWin::Window *window, const Layout::WindowState &state) const;
    void applyStacking(const QList<Layout::WindowState> &states);
    void logTargetChange(KWin::Window *window, const Layout::WindowState &state);
    void setParked(Layout::WindowId id, KWin::Window *window, bool parked);

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
    QHash<Layout::WindowId, QList<QSizeF>> m_inFlightSizes;
    QSet<Layout::WindowId> m_strictUntilSettled;
    QHash<Layout::WindowId, double> m_overriddenOpacity;
    QHash<Layout::WindowId, LoggedTarget> m_loggedTargets;
    QHash<Layout::WindowId, QPointer<KWin::Window>> m_parked;
    bool m_applying = false;
};

}
