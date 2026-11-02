#include "render/audioanalyzer.h"

#include <QTest>

#include <algorithm>
#include <cmath>
#include <numbers>
#include <vector>

using namespace Konveyor::Render;

namespace
{

constexpr int kRate = 48000;

std::vector<float> sine(double hertz, double seconds, float amplitude)
{
    std::vector<float> samples(static_cast<std::size_t>(seconds * kRate));
    for (std::size_t index = 0; index < samples.size(); ++index) {
        samples[index] = amplitude * static_cast<float>(std::sin(2.0 * std::numbers::pi * hertz * static_cast<double>(index) / kRate));
    }
    return samples;
}

std::size_t loudestBand(const AudioFrame &frame)
{
    return static_cast<std::size_t>(std::ranges::max_element(frame.bands) - frame.bands.begin());
}

double bandCenter(std::size_t band)
{
    return 40.0 * std::pow(16000.0 / 40.0, (static_cast<double>(band) + 0.5) / static_cast<double>(AudioFrame {}.bands.size()));
}

}

class TestAudioAnalyzer : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void sineLandsInItsBand_data();
    void sineLandsInItsBand();
    void splitsBassMidAndTreble();
    void clickTrainGivesBeats();
    void steadyToneGivesNoBeats();
    void silenceGivesZero();
    void fadesToZeroAfterSound();
    void quietSoundIsAutoGained();
    void waveStaysInRange();
};

void TestAudioAnalyzer::sineLandsInItsBand_data()
{
    QTest::addColumn<double>("hertz");
    QTest::newRow("low") << 80.0;
    QTest::newRow("middle") << 1000.0;
    QTest::newRow("high") << 9000.0;
}

void TestAudioAnalyzer::sineLandsInItsBand()
{
    QFETCH(double, hertz);
    AudioAnalyzer analyzer(kRate);
    const auto samples = sine(hertz, 1.0, 0.5F);
    analyzer.feed(samples.data(), samples.size());
    const double ratio = bandCenter(loudestBand(analyzer.frame())) / hertz;
    QVERIFY2(ratio > 0.7 && ratio < 1.4, qPrintable(QString::number(ratio)));
}

void TestAudioAnalyzer::splitsBassMidAndTreble()
{
    AudioAnalyzer bass(kRate);
    const auto low = sine(80.0, 1.0, 0.5F);
    bass.feed(low.data(), low.size());
    QVERIFY(bass.frame().bass > bass.frame().mid);
    QVERIFY(bass.frame().bass > bass.frame().treble);

    AudioAnalyzer treble(kRate);
    const auto high = sine(9000.0, 1.0, 0.5F);
    treble.feed(high.data(), high.size());
    QVERIFY(treble.frame().treble > treble.frame().bass);
    QVERIFY(treble.frame().treble > treble.frame().mid);
}

void TestAudioAnalyzer::clickTrainGivesBeats()
{
    AudioAnalyzer analyzer(kRate);
    constexpr std::size_t chunk = 512;
    constexpr std::size_t clickEvery = kRate / 2;
    std::vector<float> samples(kRate * 6);
    for (std::size_t index = 0; index < samples.size(); index += clickEvery) {
        for (std::size_t offset = 0; offset < 200; ++offset) {
            samples[index + offset] = offset % 2 == 0 ? 0.9F : -0.9F;
        }
        const auto thump = sine(70.0, 0.03, 0.9F);
        std::copy(thump.begin(), thump.end(), samples.begin() + static_cast<std::ptrdiff_t>(index));
    }
    int beats = 0;
    float previous = 0.0F;
    for (std::size_t start = 0; start + chunk <= samples.size(); start += chunk) {
        analyzer.feed(samples.data() + start, chunk);
        const float beat = analyzer.frame().beat;
        if (beat > previous && beat > 0.99F) {
            ++beats;
        }
        previous = beat;
    }
    QVERIFY2(beats >= 9 && beats <= 13, qPrintable(QString::number(beats)));
}

void TestAudioAnalyzer::steadyToneGivesNoBeats()
{
    AudioAnalyzer analyzer(kRate);
    const auto settle = sine(80.0, 2.0, 0.5F);
    analyzer.feed(settle.data(), settle.size());
    const auto steady = sine(80.0, 1.0, 0.5F);
    analyzer.feed(steady.data(), steady.size());
    QVERIFY(analyzer.frame().beat < 0.1F);
}

void TestAudioAnalyzer::silenceGivesZero()
{
    AudioAnalyzer analyzer(kRate);
    const std::vector<float> samples(kRate, 0.0F);
    analyzer.feed(samples.data(), samples.size());
    QVERIFY(analyzer.isZero());
    QCOMPARE(analyzer.frame().level, 0.0F);
    QCOMPARE(analyzer.frame().beat, 0.0F);
    QVERIFY(std::ranges::all_of(analyzer.frame().bands, [](float band) { return band == 0.0F; }));
    QVERIFY(std::ranges::all_of(analyzer.frame().wave, [](float point) { return point == 0.0F; }));
}

void TestAudioAnalyzer::fadesToZeroAfterSound()
{
    AudioAnalyzer analyzer(kRate);
    const auto samples = sine(440.0, 0.5, 0.5F);
    analyzer.feed(samples.data(), samples.size());
    QVERIFY(analyzer.frame().level > 0.5F);
    analyzer.feedSilence(kRate * 4);
    QVERIFY(analyzer.isZero());
    QCOMPARE(analyzer.frame().level, 0.0F);
}

void TestAudioAnalyzer::quietSoundIsAutoGained()
{
    AudioAnalyzer analyzer(kRate);
    const auto samples = sine(440.0, 1.0, 0.02F);
    analyzer.feed(samples.data(), samples.size());
    QVERIFY(analyzer.frame().level > 0.5F);
}

void TestAudioAnalyzer::waveStaysInRange()
{
    AudioAnalyzer analyzer(kRate);
    const auto samples = sine(200.0, 0.5, 1.0F);
    analyzer.feed(samples.data(), samples.size());
    const auto [low, high] = std::ranges::minmax(analyzer.frame().wave);
    QVERIFY(low >= -1.0F && low < -0.2F);
    QVERIFY(high <= 1.0F && high > 0.2F);
}

QTEST_APPLESS_MAIN(TestAudioAnalyzer)
#include "test_audioanalyzer.moc"
