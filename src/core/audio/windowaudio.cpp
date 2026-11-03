#include "audio/windowaudio.h"
#include "render/audioanalyzer.h"

#include <QSet>

#include <algorithm>

namespace Konveyor::Audio
{

WindowAudio::WindowAudio()
    : m_beatThreshold(Render::AudioAnalyzer::kDefaultBeatThreshold)
{ }

WindowAudio::~WindowAudio() = default;

void WindowAudio::setWanted(const QList<WindowKey> &windows)
{
    const QSet<WindowKey> wanted(windows.begin(), windows.end());
    if (wanted.size() == m_windows.size()
        && std::ranges::all_of(wanted, [this](const WindowKey &key) { return m_windows.contains(key); })) {
        return;
    }
    WantedWindows next;
    for (const WindowKey &key : wanted) {
        const auto existing = m_windows.constFind(key);
        next.insert(key, existing != m_windows.constEnd() ? *existing : std::make_shared<FrameMailbox>());
    }
    m_windows = next;
    std::erase_if(m_stopping, [](const std::unique_ptr<AudioCapture> &capture) { return capture->hasFinished(); });
    if (m_windows.isEmpty()) {
        if (m_capture) {
            m_capture->requestStop();
            m_stopping.push_back(std::move(m_capture));
        }
        return;
    }
    if (!m_capture) {
        m_capture = std::make_unique<AudioCapture>(m_beatThreshold);
    }
    m_capture->setWanted(m_windows);
}

void WindowAudio::setBeatThreshold(float threshold)
{
    m_beatThreshold = threshold;
    if (m_capture) {
        m_capture->setBeatThreshold(threshold);
    }
}

Render::AudioFrame WindowAudio::frame(const WindowKey &window) const
{
    const auto found = m_windows.constFind(window);
    return found != m_windows.constEnd() ? (*found)->latest() : Render::AudioFrame {};
}

}
