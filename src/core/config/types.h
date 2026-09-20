#pragma once

#include "config/types/binds.h"

#include <QList>
#include <QString>

namespace Konveyor::Config
{

struct Experiments
{
    bool preventFullscreenMinimize = false;
    bool preventFullscreenExit = false;
    bool operator==(const Experiments &) const = default;
};

struct Config
{
    Layout layout;
    Animations animations;
    Gestures gestures;
    Input input;
    bool hideDesktopWidgets = false;
    bool fillPanelsOnMaximize = false;
    bool disableMinimize = false;
    Experiments experiments;
    QList<OutputConfig> outputs;
    QList<MonitorProfile> monitorProfiles;
    QList<NamedWorkspace> workspaces;
    QList<WindowRule> windowRules;
    QList<Bind> binds;
    bool configNotificationDisableFailed = false;
    bool operator==(const Config &) const = default;
};

}
