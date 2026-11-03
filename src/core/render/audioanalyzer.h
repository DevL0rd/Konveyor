#pragma once

#include "render/audioframe.h"

#include <array>
#include <complex>
#include <cstddef>
#include <tuple>
#include <vector>

namespace Konveyor::Render
{

class AudioAnalyzer
{
public:
    static constexpr float kDefaultBeatThreshold = 1.4F;

    explicit AudioAnalyzer(int sampleRate = 48000);

    void setSampleRate(int sampleRate);
    void setBeatThreshold(float threshold) { m_beatThreshold = threshold; }
    void feed(const float *samples, std::size_t count);
    void feedSilence(std::size_t count);

    const AudioFrame &frame() const { return m_frame; }
    bool isZero() const { return m_zero; }

private:
    static constexpr std::size_t kWindowSize = 2048;
    static constexpr std::size_t kHopSize = 512;
    static constexpr std::size_t kBandCount = std::tuple_size_v<decltype(AudioFrame::bands)>;
    static constexpr std::size_t kBeatHistory = 96;

    struct BandBin
    {
        std::size_t bin = 0;
        float weight = 0.0F;
    };

    void pushSample(float sample);
    void analyzeHop();
    void measureBands();
    void updateGain(float rms);
    void updateFrame(float rms);
    void updateWave(float gain);
    void updateBeat();
    void snapToZero();
    float smoothed(float current, float target, double releaseSeconds) const;
    float recent(std::size_t age) const;

    int m_sampleRate = 48000;
    double m_hopSeconds = 0.0;
    std::array<std::vector<BandBin>, kBandCount> m_bandBins;
    std::array<float, kBandCount> m_bandCenters {};
    std::vector<BandBin> m_beatBins;

    std::array<float, kWindowSize> m_ring {};
    std::size_t m_ringHead = 0;
    std::size_t m_pending = 0;
    std::array<std::complex<float>, kWindowSize> m_spectrum {};

    std::array<float, kBandCount> m_bandAmplitude {};
    std::array<float, kBandCount> m_bandPeak {};
    float m_levelPeak = 0.0F;

    std::array<float, kBeatHistory> m_beatEnergy {};
    std::size_t m_beatCursor = 0;
    std::size_t m_beatFilled = 0;
    double m_sinceBeat = 1.0;
    float m_beatThreshold = kDefaultBeatThreshold;

    AudioFrame m_frame;
    bool m_zero = true;
};

}
