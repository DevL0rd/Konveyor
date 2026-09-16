#pragma once

#include "anim/animation.h"
#include "anim/clock.h"
#include "config/types.h"

#include <optional>

namespace Konveyor::Layout
{

struct SlideAnimation
{
    Anim::Animation anim;
    double from = 0;
    double offset() const { return from * anim.value(); }
};

std::optional<SlideAnimation> startSlide(const std::optional<SlideAnimation> &current, const Anim::Clock &clock, double from,
    const Config::AnimationParams &config, bool restartCurrent);
void shiftSlide(std::optional<SlideAnimation> &move, double offset);
void clearIfDone(std::optional<SlideAnimation> &move);
void clearIfDone(std::optional<Anim::Animation> &anim);
double optionalOffset(const std::optional<SlideAnimation> &move);

}
