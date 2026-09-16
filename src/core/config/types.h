#pragma once

#include "config/types/binds.h"

#include <QList>
#include <QString>

namespace Konveyor::Config
{

struct Config
{
    Layout layout;
    Animations animations;
    Gestures gestures;
    Input input;
    Overview overview;
    bool preferNoCsd = false;
    bool hideDesktopWidgets = false;
    bool fillPanelsOnMaximize = false;
    bool disableMinimize = false;
    QList<OutputConfig> outputs;
    QList<MonitorProfile> monitorProfiles;
    QList<NamedWorkspace> workspaces;
    QList<WindowRule> windowRules;
    QList<Bind> binds;
    bool configNotificationDisableFailed = false;
    HotkeyOverlay hotkeyOverlay;
    bool operator==(const Config &) const = default;
};

}
