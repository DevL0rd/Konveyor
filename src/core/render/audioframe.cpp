#include "render/audioframe.h"

#include <algorithm>
#include <cmath>
#include <numbers>

namespace Konveyor::Render
{

namespace
{

constexpr double kBeatSeconds = 0.5;
constexpr double kTau = 2.0 * std::numbers::pi;

float decayed(double since, double seconds)
{
    return static_cast<float>(std::exp(-since / seconds));
}

float bandShape(double position, double bass, double mid, double treble, double sway)
{
    const auto bump = [position](double center, double width) {
        const double distance = (position - center) / width;
        return std::exp(-distance * distance);
    };
    const double ripple = 0.5 + 0.5 * std::sin(kTau * (position * 2.0 + sway));
    const double value = bass * bump(0.08, 0.2) + mid * bump(0.45, 0.28) * (0.7 + 0.3 * ripple) + treble * bump(0.85, 0.22);
    return static_cast<float>(std::clamp(value, 0.0, 1.0));
}

}

AudioFrame simulatedAudioFrame(double seconds)
{
    const double time = std::max(seconds, 0.0);
    const double sinceBeat = std::fmod(time, kBeatSeconds);
    const double sinceHat = std::fmod(time + kBeatSeconds / 2.0, kBeatSeconds);
    const float kick = decayed(sinceBeat, 0.11);
    const float hat = decayed(sinceHat, 0.05);
    const double swell = 0.5 + 0.5 * std::sin(kTau * time / (kBeatSeconds * 8.0));

    AudioFrame frame;
    frame.beat = kick;
    frame.bass = 0.15F + 0.85F * kick;
    frame.mid = static_cast<float>(0.3 + 0.35 * swell + 0.2 * (0.5 + 0.5 * std::sin(kTau * time / kBeatSeconds)));
    frame.treble = static_cast<float>(0.15 + 0.2 * swell) + 0.6F * hat;
    frame.level = std::min(1.0F, 0.4F * frame.bass + 0.35F * frame.mid + 0.25F * frame.treble);

    const double sway = time / (kBeatSeconds * 4.0);
    const auto bandCount = frame.bands.size();
    for (std::size_t band = 0; band < bandCount; ++band) {
        const double position = static_cast<double>(band) / static_cast<double>(bandCount - 1);
        frame.bands[band] = bandShape(position, frame.bass, frame.mid, frame.treble, sway);
    }

    const auto waveCount = frame.wave.size();
    for (std::size_t point = 0; point < waveCount; ++point) {
        const double position = static_cast<double>(point) / static_cast<double>(waveCount);
        const double low = frame.bass * std::sin(kTau * position * 2.0);
        const double high
            = frame.mid * 0.4 * std::sin(kTau * (position * 7.0 + sway)) + frame.treble * 0.2 * std::sin(kTau * position * 19.0);
        frame.wave[point] = static_cast<float>(std::clamp(0.7 * (low + high), -1.0, 1.0));
    }
    return frame;
}

}
