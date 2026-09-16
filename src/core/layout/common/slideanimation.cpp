#include "layout/common/slideanimation.h"

namespace Konveyor::Layout
{

std::optional<SlideAnimation> startSlide(const std::optional<SlideAnimation> &current, const Anim::Clock &clock, double from,
    const Config::AnimationParams &config, bool restartCurrent)
{
    const double currentOffset = current ? current->offset() : 0.0;
    if (restartCurrent && current) {
        return SlideAnimation {current->anim.retargeted(1.0, 0.0, 0.0), from + currentOffset};
    }
    return SlideAnimation {Anim::Animation(clock, 1.0, 0.0, 0.0, config), from + currentOffset};
}

void shiftSlide(std::optional<SlideAnimation> &move, double offset)
{
    if (!move) {
        return;
    }
    const double value = move->anim.value();
    if (value > 0.001) {
        move->from += offset / value;
    }
}

void clearIfDone(std::optional<SlideAnimation> &move)
{
    if (move && move->anim.isFinished()) {
        move.reset();
    }
}

void clearIfDone(std::optional<Anim::Animation> &anim)
{
    if (anim && anim->isFinished()) {
        anim.reset();
    }
}

double optionalOffset(const std::optional<SlideAnimation> &move)
{
    return move ? move->offset() : 0.0;
}

}
