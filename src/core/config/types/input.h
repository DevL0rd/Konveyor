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
    std::optional<double> focusFollowsMouseMaxScrollPercent;
    bool warpMouseToFocus = false;
    WarpMouseMode warpMouseMode = WarpMouseMode::Separate;
    bool workspaceAutoBackAndForth = false;
    QString modKey = QStringLiteral("Super");
    std::optional<QString> modKeyNested;
    bool operator==(const Input &) const = default;
};

struct Overview
{
    double zoom = 0.5;
    QColor backdropColor {0x26, 0x26, 0x26};
    Shadow workspaceShadow {true, 40, 10, QPointF(0, 10), false, QColor(0, 0, 0, 0x50), std::nullopt};
    bool operator==(const Overview &) const = default;
};

struct HotkeyOverlay
{
    bool skipAtStartup = false;
    bool hideNotBound = false;
    bool operator==(const HotkeyOverlay &) const = default;
};

}
