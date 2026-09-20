#pragma once

#include "layout/engine/engine.h"

#include <QHash>
#include <QObject>
#include <QSet>

#include <memory>

namespace KWin
{
class SurfaceInterface;
class Window;
class XdgToplevelInterface;
}

namespace Konveyor
{

class WindowRegistry;
class X11FullscreenGuardFilter;

class FullscreenGuard final : public QObject
{
public:
    explicit FullscreenGuard(WindowRegistry &windows, QObject *parent = nullptr);
    ~FullscreenGuard() override;

    void setExperiments(bool preventFullscreenMinimize, bool preventFullscreenExit);
    void update(const QList<Layout::WindowState> &states);

private:
    struct Entry;

    void observeToplevel(KWin::XdgToplevelInterface *toplevel);
    void observe(Layout::WindowId id, KWin::Window *window);
    void forget(Layout::WindowId id);
    void restore(const std::shared_ptr<Entry> &entry);
    void setFullscreen(const std::shared_ptr<Entry> &entry, bool fullscreen);
    void refreshCapabilities();
    void advertiseMinimizeCapability(const std::shared_ptr<Entry> &entry);
    bool suppressFullscreenExit(const std::shared_ptr<Entry> &entry) const;
    bool suppressFullscreenExit(KWin::Window *window) const;
    void handleUnfullscreen(Layout::WindowId id);
    void handleMinimize(Layout::WindowId id);
    static bool currentlyCoversOutput(KWin::Window *window);
    static bool coversOutput(KWin::Window *window, const Layout::WindowState &state);

    WindowRegistry &m_windows;
    QHash<Layout::WindowId, std::shared_ptr<Entry>> m_entries;
    QHash<KWin::SurfaceInterface *, KWin::XdgToplevelInterface *> m_toplevels;
    QSet<Layout::WindowId> m_fullscreenWindows;
    std::unique_ptr<X11FullscreenGuardFilter> m_x11Filter;
    bool m_preventFullscreenMinimize = false;
    bool m_preventFullscreenExit = false;
};

}
