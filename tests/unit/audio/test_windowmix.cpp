#include "audio/windowmix.h"

#include <QTest>

#include <algorithm>
#include <cmath>
#include <memory>
#include <numbers>
#include <vector>

using namespace Konveyor::Audio;
using Clock = WindowMix::Clock;

namespace
{

std::vector<float> sine(double hertz, std::size_t count, float amplitude)
{
    std::vector<float> samples(count);
    for (std::size_t index = 0; index < count; ++index) {
        samples[index]
            = amplitude * static_cast<float>(std::sin(2.0 * std::numbers::pi * hertz * static_cast<double>(index) / kCaptureRate));
    }
    return samples;
}

void writeInChunks(
    WindowMix &mix, WindowMix::Source &source, const std::vector<float> &samples, Clock::time_point now, std::size_t chunk = 480)
{
    for (std::size_t start = 0; start < samples.size(); start += chunk) {
        mix.write(source, samples.data() + start, std::min(chunk, samples.size() - start), now);
    }
}

}

class TestWindowMix : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void init();
    void aSourceFillsTheMailbox();
    void overlappingSourcesAreSummed();
    void aRecentSilentSourceHoldsTheMixBack();
    void anIdleSourceDoesNotHoldTheMixBack();
    void aHugeWriteKeepsTheNewestSamples();
    void aSourceFarAheadForcesTheOldSamplesOut();
    void silenceFadesTheMailboxToZeroOnce();
    void theBeatThresholdDecidesWhetherBeatsAreSeen();
    void destroyingTheMixClearsTheMailbox();

private:
    float levelAfter(const std::vector<float> &samples);

    std::shared_ptr<FrameMailbox> m_mailbox;
};

void TestWindowMix::init()
{
    m_mailbox = std::make_shared<FrameMailbox>();
}

float TestWindowMix::levelAfter(const std::vector<float> &samples)
{
    const auto mailbox = std::make_shared<FrameMailbox>();
    WindowMix mix(mailbox);
    WindowMix::Source source;
    mix.addSource(source);
    writeInChunks(mix, source, samples, Clock::now());
    return mailbox->latest().level;
}

void TestWindowMix::aSourceFillsTheMailbox()
{
    WindowMix mix(m_mailbox);
    WindowMix::Source source;
    mix.addSource(source);
    QCOMPARE(m_mailbox->latest().level, 0.0F);
    writeInChunks(mix, source, sine(1000.0, kCaptureRate, 0.5F), Clock::now());
    QVERIFY(m_mailbox->latest().level > 0.1F);
}

void TestWindowMix::overlappingSourcesAreSummed()
{
    const std::vector<float> half = sine(220.0, kCaptureRate, 0.1F);
    const std::vector<float> whole = sine(220.0, kCaptureRate, 0.2F);
    WindowMix::Source first;
    WindowMix::Source second;
    const auto now = Clock::now();
    WindowMix mix(m_mailbox);
    mix.addSource(first);
    mix.addSource(second);
    for (std::size_t start = 0; start + 480 <= half.size(); start += 480) {
        mix.write(first, half.data() + start, 480, now);
        mix.write(second, half.data() + start, 480, now);
    }
    const float summed = m_mailbox->latest().level;
    QVERIFY(summed > 0.0F);
    QVERIFY(std::abs(summed - levelAfter(whole)) < 0.02F);
}

void TestWindowMix::aRecentSilentSourceHoldsTheMixBack()
{
    const auto now = Clock::now();
    WindowMix mix(m_mailbox);
    WindowMix::Source waiting;
    WindowMix::Source playing;
    mix.addSource(waiting);
    mix.addSource(playing);
    waiting.lastData = now;
    writeInChunks(mix, playing, sine(440.0, 8000, 0.5F), now);
    QCOMPARE(m_mailbox->latest().level, 0.0F);

    mix.removeSource(waiting);
    mix.tick(now);
    QVERIFY(m_mailbox->latest().level > 0.1F);
}

void TestWindowMix::anIdleSourceDoesNotHoldTheMixBack()
{
    WindowMix mix(m_mailbox);
    WindowMix::Source idle;
    WindowMix::Source playing;
    mix.addSource(idle);
    mix.addSource(playing);
    writeInChunks(mix, playing, sine(440.0, kCaptureRate, 0.5F), Clock::now());
    QVERIFY(m_mailbox->latest().level > 0.1F);
}

void TestWindowMix::aHugeWriteKeepsTheNewestSamples()
{
    WindowMix mix(m_mailbox);
    WindowMix::Source source;
    mix.addSource(source);
    const std::vector<float> samples = sine(440.0, 40000, 0.5F);
    mix.write(source, samples.data(), samples.size(), Clock::now());
    QVERIFY(m_mailbox->latest().level > 0.1F);
    mix.write(source, samples.data(), 100, Clock::now());
    QVERIFY(m_mailbox->latest().level > 0.1F);
}

void TestWindowMix::aSourceFarAheadForcesTheOldSamplesOut()
{
    const auto now = Clock::now();
    WindowMix mix(m_mailbox);
    WindowMix::Source behind;
    WindowMix::Source ahead;
    mix.addSource(behind);
    mix.addSource(ahead);
    behind.lastData = now;
    const std::vector<float> samples = sine(440.0, 30000, 0.5F);
    mix.write(ahead, samples.data(), 8000, now);
    QCOMPARE(m_mailbox->latest().level, 0.0F);
    mix.write(ahead, samples.data(), 12000, now);
    QVERIFY(m_mailbox->latest().level > 0.1F);
}

void TestWindowMix::silenceFadesTheMailboxToZeroOnce()
{
    WindowMix mix(m_mailbox);
    WindowMix::Source source;
    mix.addSource(source);
    auto now = Clock::now();
    writeInChunks(mix, source, sine(1000.0, kCaptureRate, 0.5F), now);
    QVERIFY(m_mailbox->latest().level > 0.1F);

    float level = 1.0F;
    for (int step = 0; step < 400 && level > 0.0F; ++step) {
        now += std::chrono::milliseconds(100);
        mix.tick(now);
        level = m_mailbox->latest().level;
    }
    QCOMPARE(level, 0.0F);
    now += std::chrono::milliseconds(100);
    mix.tick(now);
    QCOMPARE(m_mailbox->latest().level, 0.0F);
}

void TestWindowMix::theBeatThresholdDecidesWhetherBeatsAreSeen()
{
    std::vector<float> samples(kCaptureRate * 6);
    for (std::size_t index = 0; index < samples.size(); index += kCaptureRate / 2) {
        const std::vector<float> thump = sine(70.0, 1500, 0.9F);
        std::copy(thump.begin(), thump.end(), samples.begin() + static_cast<std::ptrdiff_t>(index));
    }
    const auto beatsSeen = [&samples](float threshold) {
        const auto mailbox = std::make_shared<FrameMailbox>();
        WindowMix mix(mailbox);
        mix.setBeatThreshold(threshold);
        WindowMix::Source source;
        mix.addSource(source);
        int beats = 0;
        float previous = 0.0F;
        for (std::size_t start = 0; start + 512 <= samples.size(); start += 512) {
            mix.write(source, samples.data() + start, 512, Clock::now());
            const float beat = mailbox->latest().beat;
            beats += beat > previous && beat > 0.99F ? 1 : 0;
            previous = beat;
        }
        return beats;
    };
    QVERIFY(beatsSeen(Konveyor::Render::AudioAnalyzer::kDefaultBeatThreshold) >= 9);
    QVERIFY(beatsSeen(1000.0F) <= 1);
}

void TestWindowMix::destroyingTheMixClearsTheMailbox()
{
    {
        WindowMix mix(m_mailbox);
        WindowMix::Source source;
        mix.addSource(source);
        writeInChunks(mix, source, sine(1000.0, kCaptureRate, 0.5F), Clock::now());
        QVERIFY(m_mailbox->latest().level > 0.1F);
    }
    QCOMPARE(m_mailbox->latest().level, 0.0F);
}

QTEST_GUILESS_MAIN(TestWindowMix)
#include "test_windowmix.moc"
