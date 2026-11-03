#pragma once

#include <array>

namespace Konveyor::Render
{

struct AudioFrame
{
    float level = 0.0F;
    float bass = 0.0F;
    float mid = 0.0F;
    float treble = 0.0F;
    float beat = 0.0F;
    std::array<float, 16> bands {};
    std::array<float, 64> wave {};
};

AudioFrame simulatedAudioFrame(double seconds);

}
