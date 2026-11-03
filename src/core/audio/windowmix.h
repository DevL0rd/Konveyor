#pragma once

#include "audio/framemailbox.h"
#include "render/audioanalyzer.h"

#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <vector>

namespace Konveyor::Audio
{

inline constexpr int kCaptureRate = 48000;

class WindowMix
{
public:
    using Clock = std::chrono::steady_clock;

    struct Source
    {
        std::uint64_t position = 0;
        Clock::time_point lastData;
    };

    explicit WindowMix(std::shared_ptr<FrameMailbox> mailbox);
    ~WindowMix();

    void addSource(Source &source);
    void removeSource(Source &source);
    void write(Source &source, const float *samples, std::size_t count, Clock::time_point now);
    void tick(Clock::time_point now);
    void setBeatThreshold(float threshold);

private:
    static constexpr std::size_t kRingSize = 16384;

    void consumeUpTo(std::uint64_t target);
    void flushReady(Clock::time_point now);
    void publish();

    std::shared_ptr<FrameMailbox> m_mailbox;
    Render::AudioAnalyzer m_analyzer;
    std::vector<Source *> m_sources;
    std::array<float, kRingSize> m_ring {};
    std::uint64_t m_read = 0;
    std::uint64_t m_head = 0;
    Clock::time_point m_lastTick;
    bool m_publishedZero = true;
};

}
