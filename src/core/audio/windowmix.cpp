#include "audio/windowmix.h"

#include <algorithm>
#include <utility>

namespace Konveyor::Audio
{

namespace
{

constexpr std::chrono::milliseconds kSourceIdleAfter(100);
constexpr std::size_t kLongestSilenceStep = 4096;

}

WindowMix::WindowMix(std::shared_ptr<FrameMailbox> mailbox)
    : m_mailbox(std::move(mailbox))
    , m_analyzer(kCaptureRate)
    , m_lastTick(Clock::now())
{ }

WindowMix::~WindowMix()
{
    m_mailbox->publish({});
}

void WindowMix::addSource(Source &source)
{
    source.position = m_head;
    m_sources.push_back(&source);
}

void WindowMix::removeSource(Source &source)
{
    std::erase(m_sources, &source);
}

void WindowMix::write(Source &source, const float *samples, std::size_t count, Clock::time_point now)
{
    source.lastData = now;
    source.position = std::max(source.position, m_read);
    if (count > kRingSize) {
        samples += count - kRingSize;
        source.position += count - kRingSize;
        count = kRingSize;
    }
    if (source.position + count > m_read + kRingSize) {
        consumeUpTo(source.position + count - kRingSize);
    }
    for (std::size_t index = 0; index < count; ++index) {
        m_ring[(source.position + index) % kRingSize] += samples[index];
    }
    source.position += count;
    m_head = std::max(m_head, source.position);
    flushReady(now);
}

void WindowMix::flushReady(Clock::time_point now)
{
    std::uint64_t target = m_head;
    for (const Source *source : m_sources) {
        if (now - source->lastData < kSourceIdleAfter) {
            target = std::min(target, source->position);
        }
    }
    if (target > m_read) {
        consumeUpTo(target);
    }
}

void WindowMix::consumeUpTo(std::uint64_t target)
{
    while (m_read < target) {
        const std::size_t offset = m_read % kRingSize;
        const std::size_t count = std::min<std::uint64_t>(target - m_read, kRingSize - offset);
        m_analyzer.feed(m_ring.data() + offset, count);
        std::fill_n(m_ring.data() + offset, count, 0.0F);
        m_read += count;
    }
    m_head = std::max(m_head, m_read);
    publish();
}

void WindowMix::tick(Clock::time_point now)
{
    const auto elapsed = std::chrono::duration<double>(now - m_lastTick).count();
    m_lastTick = now;
    flushReady(now);
    const bool playing = std::ranges::any_of(m_sources, [now](const Source *source) { return now - source->lastData < kSourceIdleAfter; });
    if (playing || m_analyzer.isZero()) {
        return;
    }
    m_analyzer.feedSilence(std::min(static_cast<std::size_t>(elapsed * kCaptureRate), kLongestSilenceStep));
    publish();
}

void WindowMix::publish()
{
    const bool zero = m_analyzer.isZero();
    if (zero && m_publishedZero) {
        return;
    }
    m_mailbox->publish(m_analyzer.frame());
    m_publishedZero = zero;
}

void WindowMix::setBeatThreshold(float threshold)
{
    m_analyzer.setBeatThreshold(threshold);
}

}
