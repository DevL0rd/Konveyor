#pragma once

#include "config/types/binds.h"
#include "config/types/borderstyle.h"

#include <QList>
#include <QString>

namespace Konveyor::Config
{

struct PeekDesktop
{
    bool enabled = false;
    int delayMs = 300;
    bool operator==(const PeekDesktop &) const = default;
};

struct Experiments
{
    bool preventFullscreenMinimize = false;
    bool preventFullscreenExit = false;
    PeekDesktop peekDesktop;
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
    double audioBeatThreshold = 1.4;
    Experiments experiments;
    QList<OutputConfig> outputs;
    QList<MonitorProfile> monitorProfiles;
    QList<NamedWorkspace> workspaces;
    QList<WindowRule> windowRules;
    QList<Bind> binds;
    bool configNotificationDisableFailed = false;
    QList<BorderStyle> borderStyles;
    bool operator==(const Config &) const = default;
};

}
