#pragma once

#include "anim/animation.h"

namespace Konveyor::Anim
{

struct AnimationInspector
{
    static double from(const Animation &animation) { return animation.m_from; }
    static Duration startTime(const Animation &animation) { return animation.m_startTime; }
    static Duration duration(const Animation &animation) { return animation.m_duration; }
    static Duration endTime(const Animation &animation) { return saturatingAdd(animation.m_startTime, animation.m_duration); }
};

}
