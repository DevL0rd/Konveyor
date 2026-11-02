#pragma once

#include "render/audioframe.h"

#include <array>
#include <atomic>
#include <cstdint>

namespace Konveyor::Audio
{

class FrameMailbox
{
public:
    void publish(const Render::AudioFrame &frame)
    {
        m_frames[m_writing] = frame;
        m_writing = m_spare.exchange(static_cast<std::uint8_t>(m_writing | kFresh), std::memory_order_acq_rel) & kIndexMask;
    }

    const Render::AudioFrame &latest()
    {
        if ((m_spare.load(std::memory_order_acquire) & kFresh) != 0) {
            m_reading = m_spare.exchange(m_reading, std::memory_order_acq_rel) & kIndexMask;
        }
        return m_frames[m_reading];
    }

private:
    static constexpr std::uint8_t kFresh = 4;
    static constexpr std::uint8_t kIndexMask = 3;

    std::array<Render::AudioFrame, 3> m_frames {};
    std::uint8_t m_writing = 0;
    std::uint8_t m_reading = 1;
    std::atomic<std::uint8_t> m_spare = 2;
};

}
