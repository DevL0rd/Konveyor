#include "kwin/x11fullscreenguardfilter.h"

#include <main.h>
#include <window.h>
#include <workspace.h>
#include <x11window.h>

#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <utility>
#include <xcb/xcb_icccm.h>

namespace Konveyor
{

X11FullscreenGuardFilter::X11FullscreenGuardFilter(
    std::function<bool(KWin::Window *)> suppressFullscreenExit, std::function<bool(KWin::Window *)> suppressMinimize)
    : KWin::X11EventFilter(QList<int> {XCB_CLIENT_MESSAGE, XCB_DESTROY_NOTIFY, XCB_FOCUS_IN, XCB_PROPERTY_NOTIFY})
    , m_suppressFullscreenExit(std::move(suppressFullscreenExit))
    , m_suppressMinimize(std::move(suppressMinimize))
{
    m_activationConnection = QObject::connect(KWin::Workspace::self(), &KWin::Workspace::windowActivated, [this](KWin::Window *window) {
        if (auto *x11Window = qobject_cast<KWin::X11Window *>(window)) {
            releasePretendedMinimize(x11Window->window());
        }
    });
}

X11FullscreenGuardFilter::~X11FullscreenGuardFilter()
{
    QObject::disconnect(m_activationConnection);
    const auto windows = m_pretendMinimized;
    for (xcb_window_t window : windows) {
        releasePretendedMinimize(window);
    }
}

bool X11FullscreenGuardFilter::event(xcb_generic_event_t *genericEvent)
{
    const uint8_t eventType = genericEvent->response_type & ~0x80;
    switch (eventType) {
    case XCB_DESTROY_NOTIFY:
        handleDestroy(*reinterpret_cast<xcb_destroy_notify_event_t *>(genericEvent));
        return false;
    case XCB_FOCUS_IN:
        return handleFocus(*reinterpret_cast<xcb_focus_in_event_t *>(genericEvent));
    case XCB_PROPERTY_NOTIFY:
        return handleProperty(*reinterpret_cast<xcb_property_notify_event_t *>(genericEvent));
    default:
        return handleClientMessage(*reinterpret_cast<xcb_client_message_event_t *>(genericEvent));
    }
}

void X11FullscreenGuardFilter::handleDestroy(const xcb_destroy_notify_event_t &event)
{
    m_repairedWindows.remove(event.window);
    m_acknowledgedWindows.remove(event.window);
    m_pretendMinimized.remove(event.window);
    m_leavingFullscreen.remove(event.window);
}

bool X11FullscreenGuardFilter::handleFocus(const xcb_focus_in_event_t &event)
{
    const xcb_window_t window = event.event;
    if (m_pretendMinimized.contains(window)) {
        KWin::X11Window *client = KWin::Workspace::self()->findClient(window);
        if (client && KWin::Workspace::self()->activeWindow() != client) {
            KWin::Workspace::self()->restoreFocus();
            return true;
        }
        releasePretendedMinimize(window);
    }
    m_repairedWindows.remove(window);
    m_acknowledgedWindows.remove(window);
    m_leavingFullscreen.remove(window);
    return false;
}

bool X11FullscreenGuardFilter::handleProperty(const xcb_property_notify_event_t &event)
{
    if (resolveAtoms() && event.atom == m_netWmState) {
        m_leavingFullscreen.remove(event.window);
        return false;
    }
    if (event.atom != XCB_ATOM_WM_HINTS) {
        return false;
    }
    KWin::X11Window *window = KWin::Workspace::self()->findClient(event.window);
    if (window && hasFullscreenState(window->window()) && m_suppressMinimize(window)) {
        restoreFocusHints(window->window());
        m_repairedWindows.insert(window->window());
    }
    return false;
}

bool X11FullscreenGuardFilter::handleClientMessage(const xcb_client_message_event_t &event)
{
    if (event.format != 32 || !resolveAtoms()) {
        return false;
    }
    KWin::X11Window *window = KWin::Workspace::self()->findClient(event.window);
    if (!window) {
        return false;
    }
    const bool ignoredActivation = event.type == m_netActiveWindow && event.data.data32[0] == 2 && event.data.data32[1] == 0
        && event.data.data32[3] == 0 && event.data.data32[4] == 0 && m_pretendMinimized.contains(window->window());
    if (ignoredActivation) {
        return true;
    }
    if (event.type == m_wmChangeState) {
        return handleWmChangeState(event, window);
    }
    return handleNetWmState(event, window);
}

bool X11FullscreenGuardFilter::handleWmChangeState(const xcb_client_message_event_t &event, KWin::Window *window)
{
    auto *x11Window = qobject_cast<KWin::X11Window *>(window);
    const xcb_window_t id = x11Window->window();
    if (event.data.data32[0] != XCB_ICCCM_WM_STATE_ICONIC) {
        return false;
    }
    if (!hasFullscreenState(id)) {
        releasePretendedMinimize(id);
        m_repairedWindows.remove(id);
        m_acknowledgedWindows.remove(id);
        m_leavingFullscreen.remove(id);
        return false;
    }
    if (m_leavingFullscreen.remove(id)) {
        return false;
    }
    if (!m_suppressMinimize(window)) {
        releasePretendedMinimize(id);
        return false;
    }
    if (!m_repairedWindows.contains(id)) {
        restoreFocusHints(id);
        m_repairedWindows.insert(id);
    }
    if (!m_acknowledgedWindows.contains(id)) {
        if (window->isActive()) {
            setWmState(id, XCB_ICCCM_WM_STATE_NORMAL);
        } else {
            setWmState(id, XCB_ICCCM_WM_STATE_ICONIC);
            m_pretendMinimized.insert(id);
        }
        m_acknowledgedWindows.insert(id);
    }
    return true;
}

bool X11FullscreenGuardFilter::handleNetWmState(const xcb_client_message_event_t &event, KWin::Window *window)
{
    if (event.type != m_netWmState) {
        return false;
    }
    const bool fullscreen = event.data.data32[1] == m_netWmStateFullscreen || event.data.data32[2] == m_netWmStateFullscreen;
    const uint32_t action = event.data.data32[0];
    const bool leavingFullscreen = action == 0 || (action == 2 && window->isFullScreen());
    if (!fullscreen || !leavingFullscreen) {
        return false;
    }
    if (m_suppressFullscreenExit(window)) {
        return true;
    }
    auto *x11Window = qobject_cast<KWin::X11Window *>(window);
    m_leavingFullscreen.insert(x11Window->window());
    return false;
}

void X11FullscreenGuardFilter::restoreMinimizeState(KWin::Window *window)
{
    auto *x11Window = qobject_cast<KWin::X11Window *>(window);
    if (x11Window) {
        releasePretendedMinimize(x11Window->window());
    }
}

void X11FullscreenGuardFilter::restoreAllMinimizeStates()
{
    const auto windows = m_pretendMinimized;
    for (xcb_window_t window : windows) {
        releasePretendedMinimize(window);
    }
}

bool X11FullscreenGuardFilter::hasFullscreenState(xcb_window_t window)
{
    xcb_connection_t *connection = KWin::kwinApp()->x11Connection();
    const xcb_get_property_cookie_t cookie = xcb_get_property(connection, false, window, m_netWmState, XCB_ATOM_ATOM, 0, 64);
    xcb_get_property_reply_t *reply = xcb_get_property_reply(connection, cookie, nullptr);
    if (!reply || reply->format != 32) {
        std::free(reply);
        return false;
    }
    const auto *atoms = static_cast<const xcb_atom_t *>(xcb_get_property_value(reply));
    const int count = xcb_get_property_value_length(reply) / static_cast<int>(sizeof(xcb_atom_t));
    const bool fullscreen = std::find(atoms, atoms + count, m_netWmStateFullscreen) != atoms + count;
    std::free(reply);
    return fullscreen;
}

void X11FullscreenGuardFilter::restoreFocusHints(xcb_window_t window)
{
    xcb_connection_t *connection = KWin::kwinApp()->x11Connection();
    const xcb_get_property_cookie_t cookie = xcb_get_property(connection, false, window, XCB_ATOM_WM_HINTS, XCB_ATOM_WM_HINTS, 0, 9);
    xcb_get_property_reply_t *reply = xcb_get_property_reply(connection, cookie, nullptr);
    if (!reply || reply->format != 32 || reply->value_len < 3) {
        std::free(reply);
        return;
    }
    auto *hints = static_cast<uint32_t *>(xcb_get_property_value(reply));
    bool changed = false;
    if ((hints[0] & 1) && hints[1] == 0) {
        hints[1] = 1;
        changed = true;
    }
    if ((hints[0] & 2) && hints[2] == XCB_ICCCM_WM_STATE_ICONIC) {
        hints[2] = XCB_ICCCM_WM_STATE_NORMAL;
        changed = true;
    }
    if (changed) {
        xcb_change_property(connection, XCB_PROP_MODE_REPLACE, window, XCB_ATOM_WM_HINTS, XCB_ATOM_WM_HINTS, 32, reply->value_len, hints);
        xcb_flush(connection);
    }
    std::free(reply);
}

bool X11FullscreenGuardFilter::resolveAtoms()
{
    if (m_netActiveWindow != XCB_ATOM_NONE && m_netWmState != XCB_ATOM_NONE && m_netWmStateFullscreen != XCB_ATOM_NONE
        && m_wmChangeState != XCB_ATOM_NONE && m_wmState != XCB_ATOM_NONE) {
        return true;
    }
    xcb_connection_t *connection = KWin::kwinApp()->x11Connection();
    if (!connection) {
        return false;
    }
    const auto intern = [connection](const char *name) {
        const xcb_intern_atom_cookie_t cookie = xcb_intern_atom(connection, true, std::strlen(name), name);
        xcb_intern_atom_reply_t *reply = xcb_intern_atom_reply(connection, cookie, nullptr);
        const xcb_atom_t atom = reply ? reply->atom : static_cast<xcb_atom_t>(XCB_ATOM_NONE);
        std::free(reply);
        return atom;
    };
    m_netActiveWindow = intern("_NET_ACTIVE_WINDOW");
    m_netWmState = intern("_NET_WM_STATE");
    m_netWmStateFullscreen = intern("_NET_WM_STATE_FULLSCREEN");
    m_wmChangeState = intern("WM_CHANGE_STATE");
    m_wmState = intern("WM_STATE");
    return m_netActiveWindow != XCB_ATOM_NONE && m_netWmState != XCB_ATOM_NONE && m_netWmStateFullscreen != XCB_ATOM_NONE
        && m_wmChangeState != XCB_ATOM_NONE && m_wmState != XCB_ATOM_NONE;
}

void X11FullscreenGuardFilter::setWmState(xcb_window_t window, uint32_t state)
{
    xcb_connection_t *connection = KWin::kwinApp()->x11Connection();
    if (!connection || m_wmState == XCB_ATOM_NONE) {
        return;
    }
    const uint32_t data[] {state, XCB_WINDOW_NONE};
    xcb_change_property(connection, XCB_PROP_MODE_REPLACE, window, m_wmState, m_wmState, 32, 2, data);
    xcb_flush(connection);
}

void X11FullscreenGuardFilter::releasePretendedMinimize(xcb_window_t window)
{
    if (m_pretendMinimized.remove(window)) {
        setWmState(window, XCB_ICCCM_WM_STATE_NORMAL);
    }
}

}
