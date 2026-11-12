#pragma once

#include "layout/engine/engine.h"

#include <memory>
#include <unordered_map>

namespace KWin
{
class EffectWindow;
class EffectWindowVisibleRef;
}

namespace Konveyor
{

class WindowRegistry;

enum class WindowVisibilityHold
{
    None,
    VisibleWindows,
    AllWindows,
};

class WindowVisibility
{
public:
    explicit WindowVisibility(const WindowRegistry &windows);
    ~WindowVisibility();

    WindowVisibility(const WindowVisibility &) = delete;
    WindowVisibility &operator=(const WindowVisibility &) = delete;

    void sync(const QList<Layout::WindowState> &states, WindowVisibilityHold hold);
    void forget(KWin::EffectWindow *window);
    void release();

private:
    struct Held
    {
        KWin::EffectWindow *window = nullptr;
        std::unique_ptr<KWin::EffectWindowVisibleRef> ref;
    };

    const WindowRegistry &m_windows;
    std::unordered_map<Layout::WindowId, Held> m_held;
};

}
