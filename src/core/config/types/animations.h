#pragma once

#include "config/types/layout.h"

#include <QList>
#include <QString>

#include <optional>

namespace Konveyor::Config
{

struct SpringParams
{
    double dampingRatio = 1.0;
    double stiffness = 800;
    double epsilon = 0.0001;
    bool operator==(const SpringParams &) const = default;
};

enum class EasingCurve
{
    Linear,
    EaseOutQuad,
    EaseOutCubic,
    EaseOutExpo,
    CubicBezier
};

struct EasingParams
{
    double durationMs = 250;
    EasingCurve curve = EasingCurve::EaseOutCubic;
    double x1 = 0;
    double y1 = 0;
    double x2 = 1;
    double y2 = 1;
    bool operator==(const EasingParams &) const = default;
};

struct AnimationParams
{
    bool enabled = true;
    std::variant<SpringParams, EasingParams> kind;
    bool operator==(const AnimationParams &) const = default;
};

struct Animations
{
    bool enabled = true;
    double slowdown = 1.0;
    AnimationParams workspaceSwitch;
    AnimationParams windowOpen;
    AnimationParams windowClose;
    AnimationParams horizontalViewMovement;
    AnimationParams windowMovement;
    AnimationParams windowResize;
    AnimationParams overviewOpenClose;
    bool operator==(const Animations &) const = default;
};

}
