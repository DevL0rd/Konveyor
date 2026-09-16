#pragma once

#include "config/types/animations.h"

#include <QList>
#include <QString>
#include <QStringList>

#include <optional>

namespace Konveyor::Config
{

struct DndEdgeScroll
{
    double triggerSize = 30;
    double delayMs = 100;
    double maxSpeed = 1500;
    bool operator==(const DndEdgeScroll &) const = default;
};

struct HotCorners
{
    bool enabled = true;
    bool topLeft = true;
    bool topRight = false;
    bool bottomLeft = false;
    bool bottomRight = false;
    bool operator==(const HotCorners &) const = default;
};

enum class TitlebarDrag
{
    ScrollView,
    MoveWindow
};

struct Gestures
{
    DndEdgeScroll dndEdgeViewScroll;
    DndEdgeScroll dndEdgeWorkspaceSwitch {50, 100, 1500};
    HotCorners hotCorners;
    TitlebarDrag titlebarDrag = TitlebarDrag::ScrollView;
    bool resizeTiledWindows = false;
    bool operator==(const Gestures &) const = default;
};

enum class WarpMouseMode
{
    Separate,
    CenterXY,
    CenterXYAlways
};

struct Input
{
    bool focusFollowsMouse = false;
    bool warpMouseToFocus = false;
    WarpMouseMode warpMouseMode = WarpMouseMode::Separate;
    bool workspaceAutoBackAndForth = false;
    QString modKey = QStringLiteral("Super");
    bool operator==(const Input &) const = default;
};

}
