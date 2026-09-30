#pragma once

#include "layout/engine/engine.h"

#include <QHash>
#include <QUuid>

#include <optional>

namespace KWin
{
class Window;
}

namespace Konveyor
{

class WindowRegistry;

struct HandedOverPlacement
{
    QString output;
    Layout::RestorePlacement placement;
};

class LayoutHandoff
{
public:
    static void give(const Layout::Engine &engine, const WindowRegistry &windows);
    static LayoutHandoff take();

    bool comesBefore(KWin::Window *first, KWin::Window *second) const;
    std::optional<HandedOverPlacement> takePlacement(KWin::Window *window, const Layout::Engine &engine);

private:
    struct Entry
    {
        QString output;
        int workspaceIndex = 0;
        Layout::RestorePlacement placement;
    };

    QHash<QUuid, Entry> m_entries;
};

}
