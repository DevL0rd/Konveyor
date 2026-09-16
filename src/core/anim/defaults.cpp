#include "anim/defaults.h"

namespace Konveyor::Anim
{

namespace
{

Config::AnimationParams springAnimation(double dampingRatio, double stiffness, double epsilon)
{
    Config::SpringParams params;
    params.dampingRatio = dampingRatio;
    params.stiffness = stiffness;
    params.epsilon = epsilon;
    return Config::AnimationParams {true, params};
}

Config::AnimationParams easingAnimation(double durationMs, Config::EasingCurve curve)
{
    Config::EasingParams params;
    params.durationMs = durationMs;
    params.curve = curve;
    return Config::AnimationParams {true, params};
}

}

Config::Animations defaultAnimations()
{
    Config::Animations animations;
    animations.enabled = true;
    animations.slowdown = 1.0;
    animations.workspaceSwitch = springAnimation(1.0, 1000, 0.0001);
    animations.windowOpen = easingAnimation(150, Config::EasingCurve::EaseOutExpo);
    animations.windowClose = easingAnimation(150, Config::EasingCurve::EaseOutQuad);
    animations.horizontalViewMovement = springAnimation(1.0, 800, 0.0001);
    animations.windowMovement = springAnimation(1.0, 800, 0.0001);
    animations.windowResize = springAnimation(1.0, 800, 0.0001);
    animations.overviewOpenClose = springAnimation(1.0, 800, 0.0001);
    return animations;
}

}
