#pragma once

#include <x11eventfilter.h>

#include <QMetaObject>
#include <QSet>

#include <functional>
#include <xcb/xcb.h>

namespace KWin
{
class Window;
}

namespace Konveyor
{

class X11FullscreenGuardFilter final : public KWin::X11EventFilter
{
public:
    explicit X11FullscreenGuardFilter(
        std::function<bool(KWin::Window *)> suppressFullscreenExit, std::function<bool(KWin::Window *)> suppressMinimize);
    ~X11FullscreenGuardFilter() override;

    bool event(xcb_generic_event_t *genericEvent) override;
    void restoreMinimizeState(KWin::Window *window);
    void restoreAllMinimizeStates();

private:
    void handleDestroy(const xcb_destroy_notify_event_t &event);
    bool handleFocus(const xcb_focus_in_event_t &event);
    bool handleProperty(const xcb_property_notify_event_t &event);
    bool handleClientMessage(const xcb_client_message_event_t &event);
    bool handleWmChangeState(const xcb_client_message_event_t &event, KWin::Window *window);
    bool handleNetWmState(const xcb_client_message_event_t &event, KWin::Window *window);
    bool hasFullscreenState(xcb_window_t window);
    void restoreFocusHints(xcb_window_t window);
    bool resolveAtoms();
    void setWmState(xcb_window_t window, uint32_t state);
    void releasePretendedMinimize(xcb_window_t window);

    std::function<bool(KWin::Window *)> m_suppressFullscreenExit;
    std::function<bool(KWin::Window *)> m_suppressMinimize;
    xcb_atom_t m_netActiveWindow = XCB_ATOM_NONE;
    xcb_atom_t m_netWmState = XCB_ATOM_NONE;
    xcb_atom_t m_netWmStateFullscreen = XCB_ATOM_NONE;
    xcb_atom_t m_wmChangeState = XCB_ATOM_NONE;
    xcb_atom_t m_wmState = XCB_ATOM_NONE;
    QSet<xcb_window_t> m_repairedWindows;
    QSet<xcb_window_t> m_acknowledgedWindows;
    QSet<xcb_window_t> m_pretendMinimized;
    QSet<xcb_window_t> m_leavingFullscreen;
    QMetaObject::Connection m_activationConnection;
};

}
