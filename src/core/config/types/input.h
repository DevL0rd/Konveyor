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

enum class HorizontalSwipe
{
    ScrollView,
    Off
};

enum class VerticalSwipe
{
    SwitchWorkspace,
    Off
};

enum class PinchAction
{
    ToggleOverview,
    Off
};

enum class WindowHorizontalSwipe
{
    ConsumeOrExpel,
    Off
};

enum class WindowVerticalSwipe
{
    MoveToWorkspace,
    Off
};

enum class TapAction
{
    Off,
    CycleWidth,
    KontrolPanel,
    ToggleOverview
};

struct MultiTouch
{
    bool enabled = true;
    int swipeFingers = 3;
    int pinchFingers = 4;
    bool naturalSwipe = true;
    HorizontalSwipe horizontalSwipe = HorizontalSwipe::ScrollView;
    VerticalSwipe verticalSwipe = VerticalSwipe::SwitchWorkspace;
    PinchAction pinch = PinchAction::ToggleOverview;
    int windowSwipeFingers = 4;
    WindowHorizontalSwipe windowHorizontalSwipe = WindowHorizontalSwipe::ConsumeOrExpel;
    WindowVerticalSwipe windowVerticalSwipe = WindowVerticalSwipe::MoveToWorkspace;
    bool longPressToMove = true;
    int longPressMs = 500;
    TapAction threeFingerTap = TapAction::CycleWidth;
    TapAction fourFingerTap = TapAction::KontrolPanel;
    TapAction fiveFingerTap = TapAction::Off;
    TapAction tap(int fingers) const
    {
        switch (fingers) {
        case 3:
            return threeFingerTap;
        case 4:
            return fourFingerTap;
        case 5:
            return fiveFingerTap;
        default:
            return TapAction::Off;
        }
    }
    bool operator==(const MultiTouch &) const = default;
};

struct Gestures
{
    MultiTouch touchpad;
    MultiTouch touchscreen;
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
