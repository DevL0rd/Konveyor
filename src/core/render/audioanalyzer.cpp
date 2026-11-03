#include "render/audioanalyzer.h"

#include <algorithm>
#include <bit>
#include <cmath>
#include <numbers>

namespace Konveyor::Render
{

namespace
{

constexpr std::size_t kFftSize = 2048;
constexpr double kLowestHz = 40.0;
constexpr double kHighestHz = 16000.0;
constexpr double kBassTopHz = 250.0;
constexpr double kMidTopHz = 2000.0;
constexpr double kBeatTopHz = 150.0;
constexpr double kGainReleaseSeconds = 12.0;
constexpr double kBandReleaseSeconds = 0.18;
constexpr double kLevelReleaseSeconds = 0.25;
constexpr double kBeatDecaySeconds = 0.18;
constexpr double kBeatRefractorySeconds = 0.15;
constexpr float kSilenceRms = 1e-4F;
constexpr float kPeakFloor = 2e-3F;
constexpr float kBandPeakShare = 0.05F;
constexpr float kBeatSpread = 1.2F;
constexpr float kZeroEpsilon = 0.002F;

struct FftTables
{
    std::array<std::size_t, kFftSize> reversed {};
    std::array<std::complex<float>, kFftSize / 2> twiddle {};
    std::array<float, kFftSize> hann {};

    FftTables()
    {
        const auto bits = static_cast<std::size_t>(std::countr_zero(kFftSize));
        for (std::size_t index = 0; index < kFftSize; ++index) {
            std::size_t mirrored = 0;
            for (std::size_t bit = 0; bit < bits; ++bit) {
                mirrored |= ((index >> bit) & 1U) << (bits - 1 - bit);
            }
            reversed[index] = mirrored;
            hann[index] = static_cast<float>(0.5 - 0.5 * std::cos(2.0 * std::numbers::pi * static_cast<double>(index) / kFftSize));
        }
        for (std::size_t index = 0; index < kFftSize / 2; ++index) {
            twiddle[index] = std::polar(1.0F, static_cast<float>(-2.0 * std::numbers::pi * static_cast<double>(index) / kFftSize));
        }
    }
};

const FftTables &fftTables()
{
    static const FftTables tables;
    return tables;
}

void transform(std::array<std::complex<float>, kFftSize> &values)
{
    const FftTables &tables = fftTables();
    for (std::size_t index = 0; index < kFftSize; ++index) {
        if (index < tables.reversed[index]) {
            std::swap(values[index], values[tables.reversed[index]]);
        }
    }
    for (std::size_t size = 2; size <= kFftSize; size *= 2) {
        const std::size_t half = size / 2;
        const std::size_t stride = kFftSize / size;
        for (std::size_t start = 0; start < kFftSize; start += size) {
            for (std::size_t offset = 0; offset < half; ++offset) {
                const std::complex<float> &turn = tables.twiddle[offset * stride];
                const std::complex<float> &odd = values[start + offset + half];
                const std::complex<float> rotated(
                    odd.real() * turn.real() - odd.imag() * turn.imag(), odd.real() * turn.imag() + odd.imag() * turn.real());
                const std::complex<float> even = values[start + offset];
                values[start + offset] = even + rotated;
                values[start + offset + half] = even - rotated;
            }
        }
    }
}

template<typename Bins> void fillBins(Bins &bins, double lowHz, double highHz, double binHz)
{
    bins.clear();
    const double low = lowHz / binHz;
    const double high = highHz / binHz;
    const auto first = static_cast<std::size_t>(std::max(1.0, std::floor(low + 0.5)));
    const auto last = std::min(static_cast<std::size_t>(std::floor(high + 0.5)), kFftSize / 2 - 1);
    for (std::size_t bin = first; bin <= last; ++bin) {
        const double from = std::max(low, static_cast<double>(bin) - 0.5);
        const double to = std::min(high, static_cast<double>(bin) + 0.5);
        if (to > from) {
            bins.push_back({bin, static_cast<float>(to - from)});
        }
    }
}

}

AudioAnalyzer::AudioAnalyzer(int sampleRate)
{
    setSampleRate(sampleRate);
}

void AudioAnalyzer::setSampleRate(int sampleRate)
{
    m_sampleRate = std::max(sampleRate, 1);
    m_hopSeconds = static_cast<double>(kHopSize) / m_sampleRate;
    const double binHz = static_cast<double>(m_sampleRate) / kFftSize;
    const double topHz = std::min(kHighestHz, m_sampleRate / 2.0);
    const double ratio = std::pow(topHz / kLowestHz, 1.0 / kBandCount);
    double low = kLowestHz;
    for (std::size_t band = 0; band < kBandCount; ++band) {
        fillBins(m_bandBins[band], low, low * ratio, binHz);
        m_bandCenters[band] = static_cast<float>(low * std::sqrt(ratio));
        low *= ratio;
    }
    fillBins(m_beatBins, kLowestHz, kBeatTopHz, binHz);
}

void AudioAnalyzer::feed(const float *samples, std::size_t count)
{
    for (std::size_t index = 0; index < count; ++index) {
        pushSample(std::isfinite(samples[index]) ? samples[index] : 0.0F);
    }
}

void AudioAnalyzer::feedSilence(std::size_t count)
{
    for (std::size_t index = 0; index < count; ++index) {
        pushSample(0.0F);
    }
}

void AudioAnalyzer::pushSample(float sample)
{
    m_ring[m_ringHead] = sample;
    m_ringHead = (m_ringHead + 1) % kWindowSize;
    if (++m_pending == kHopSize) {
        m_pending = 0;
        analyzeHop();
    }
}

float AudioAnalyzer::recent(std::size_t age) const
{
    return m_ring[(m_ringHead + kWindowSize - 1 - age) % kWindowSize];
}

void AudioAnalyzer::analyzeHop()
{
    float energy = 0.0F;
    for (const float sample : m_ring) {
        energy += sample * sample;
    }
    const float rms = std::sqrt(energy / kWindowSize);
    if (rms < kSilenceRms && m_zero) {
        return;
    }
    measureBands();
    updateGain(rms);
    updateFrame(rms);
    updateBeat();
    snapToZero();
}

void AudioAnalyzer::measureBands()
{
    const FftTables &tables = fftTables();
    for (std::size_t index = 0; index < kWindowSize; ++index) {
        m_spectrum[index] = {recent(kWindowSize - 1 - index) * tables.hann[index], 0.0F};
    }
    transform(m_spectrum);
    const float scale = 4.0F / kWindowSize;
    for (std::size_t band = 0; band < kBandCount; ++band) {
        float power = 0.0F;
        for (const BandBin &entry : m_bandBins[band]) {
            power += std::norm(m_spectrum[entry.bin] * scale) * entry.weight;
        }
        m_bandAmplitude[band] = std::sqrt(power);
    }
}

void AudioAnalyzer::updateGain(float rms)
{
    const auto release = static_cast<float>(std::exp(-m_hopSeconds / kGainReleaseSeconds));
    m_levelPeak = std::max({m_levelPeak * release, rms, kPeakFloor});
    float loudest = 0.0F;
    for (std::size_t band = 0; band < kBandCount; ++band) {
        m_bandPeak[band] = std::max(m_bandPeak[band] * release, m_bandAmplitude[band]);
        loudest = std::max(loudest, m_bandPeak[band]);
    }
    const float floor = std::max(loudest * kBandPeakShare, kPeakFloor * kBandPeakShare);
    for (float &peak : m_bandPeak) {
        peak = std::max(peak, floor);
    }
}

float AudioAnalyzer::smoothed(float current, float target, double releaseSeconds) const
{
    if (target >= current) {
        return current + (target - current) * 0.7F;
    }
    const auto keep = static_cast<float>(std::exp(-m_hopSeconds / releaseSeconds));
    return target + (current - target) * keep;
}

void AudioAnalyzer::updateFrame(float rms)
{
    const bool silent = rms < kSilenceRms;
    std::array<float, 3> totals {};
    std::array<int, 3> counts {};
    for (std::size_t band = 0; band < kBandCount; ++band) {
        const float target = silent ? 0.0F : std::sqrt(std::clamp(m_bandAmplitude[band] / m_bandPeak[band], 0.0F, 1.0F));
        m_frame.bands[band] = smoothed(m_frame.bands[band], target, kBandReleaseSeconds);
        const std::size_t group = m_bandCenters[band] < kBassTopHz ? 0 : (m_bandCenters[band] < kMidTopHz ? 1 : 2);
        totals[group] += m_frame.bands[band];
        ++counts[group];
    }
    m_frame.bass = totals[0] / static_cast<float>(counts[0]);
    m_frame.mid = counts[1] > 0 ? totals[1] / static_cast<float>(counts[1]) : 0.0F;
    m_frame.treble = counts[2] > 0 ? totals[2] / static_cast<float>(counts[2]) : 0.0F;

    const float gain = 1.0F / m_levelPeak;
    const float level = silent ? 0.0F : std::pow(std::min(rms * gain, 1.0F), 0.7F);
    m_frame.level = smoothed(m_frame.level, level, kLevelReleaseSeconds);
    updateWave(silent ? 0.0F : gain * 0.7F);
}

void AudioAnalyzer::updateWave(float gain)
{
    constexpr std::size_t points = std::tuple_size_v<decltype(AudioFrame::wave)>;
    constexpr std::size_t span = kWindowSize / 2;
    constexpr std::size_t stride = span / points;
    std::size_t start = kWindowSize - 1;
    for (std::size_t age = kWindowSize - 1; age > span; --age) {
        if (recent(age) <= 0.0F && recent(age - 1) > 0.0F) {
            start = age;
            break;
        }
    }
    for (std::size_t point = 0; point < points; ++point) {
        float sum = 0.0F;
        for (std::size_t offset = 0; offset < stride; ++offset) {
            sum += recent(start - point * stride - offset);
        }
        m_frame.wave[point] = std::clamp(sum / stride * gain, -1.0F, 1.0F);
    }
}

void AudioAnalyzer::updateBeat()
{
    float energy = 0.0F;
    for (const BandBin &entry : m_beatBins) {
        energy += std::norm(m_spectrum[entry.bin] * (4.0F / kWindowSize)) * entry.weight;
    }
    float mean = 0.0F;
    for (std::size_t index = 0; index < m_beatFilled; ++index) {
        mean += m_beatEnergy[index];
    }
    mean = m_beatFilled > 0 ? mean / static_cast<float>(m_beatFilled) : 0.0F;
    float variance = 0.0F;
    for (std::size_t index = 0; index < m_beatFilled; ++index) {
        const float distance = m_beatEnergy[index] - mean;
        variance += distance * distance;
    }
    const float spread = m_beatFilled > 0 ? std::sqrt(variance / static_cast<float>(m_beatFilled)) : 0.0F;

    m_sinceBeat += m_hopSeconds;
    const float floor = m_levelPeak * m_levelPeak * 0.01F;
    const bool onset = energy > floor && energy > mean * m_beatThreshold + spread * kBeatSpread && m_sinceBeat >= kBeatRefractorySeconds;
    m_frame.beat *= static_cast<float>(std::exp(-m_hopSeconds / kBeatDecaySeconds));
    if (onset) {
        m_frame.beat = 1.0F;
        m_sinceBeat = 0.0;
    }
    m_beatEnergy[m_beatCursor] = energy;
    m_beatCursor = (m_beatCursor + 1) % kBeatHistory;
    m_beatFilled = std::min(m_beatFilled + 1, kBeatHistory);
}

void AudioAnalyzer::snapToZero()
{
    float loudest = std::max(m_frame.level, m_frame.beat);
    for (const float band : m_frame.bands) {
        loudest = std::max(loudest, band);
    }
    for (const float point : m_frame.wave) {
        loudest = std::max(loudest, std::abs(point));
    }
    const bool quiet = loudest < kZeroEpsilon;
    m_zero = quiet;
    if (quiet) {
        m_frame = AudioFrame {};
        m_beatFilled = 0;
        m_beatCursor = 0;
    }
}

}
