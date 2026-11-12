#include "kwin/windowvisibility.h"

#include "kwin/windowregistry.h"

#include <effect/effecthandler.h>
#include <effect/effectwindow.h>
#include <window.h>

#include <algorithm>

namespace Konveyor
{

WindowVisibility::WindowVisibility(const WindowRegistry &windows)
    : m_windows(windows)
{ }

WindowVisibility::~WindowVisibility()
{
    release();
}

void WindowVisibility::sync(const QList<Layout::WindowState> &states, WindowVisibilityHold hold)
{
    std::unordered_map<Layout::WindowId, KWin::EffectWindow *> desired;
    if (hold != WindowVisibilityHold::None) {
        for (const Layout::WindowState &state : states) {
            if (hold == WindowVisibilityHold::VisibleWindows && !state.visible) {
                continue;
            }
            KWin::Window *window = m_windows.windowOf(state.id);
            KWin::EffectWindow *effectWindow = window ? KWin::effects->findWindow(window->internalId()) : nullptr;
            if (effectWindow) {
                desired.emplace(state.id, effectWindow);
            }
        }
    }

    bool changed = false;
    for (auto it = m_held.begin(); it != m_held.end();) {
        const auto wanted = desired.find(it->first);
        if (wanted == desired.end() || wanted->second != it->second.window) {
            it = m_held.erase(it);
            changed = true;
        } else {
            ++it;
        }
    }

    for (const auto &[id, window] : desired) {
        if (m_held.contains(id)) {
            continue;
        }
        Held held;
        held.window = window;
        held.ref = std::make_unique<KWin::EffectWindowVisibleRef>(window, KWin::EffectWindow::PAINT_DISABLED_BY_DESKTOP);
        m_held.emplace(id, std::move(held));
        changed = true;
    }

    if (changed && KWin::effects) {
        KWin::effects->addRepaintFull();
    }
}

void WindowVisibility::forget(KWin::EffectWindow *window)
{
    if (!window) {
        return;
    }
    const auto erased = std::erase_if(m_held, [window](const auto &entry) { return entry.second.window == window; });
    if (erased > 0 && KWin::effects) {
        KWin::effects->addRepaintFull();
    }
}

void WindowVisibility::release()
{
    m_held.clear();
}

}
