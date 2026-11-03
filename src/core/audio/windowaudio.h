#pragma once

#include "audio/audiocapture.h"
#include "render/audioframe.h"

#include <QList>

#include <memory>
#include <vector>

namespace Konveyor::Audio
{

class WindowAudio
{
public:
    WindowAudio();
    ~WindowAudio();

    void setWanted(const QList<WindowKey> &windows);
    void setBeatThreshold(float threshold);
    Render::AudioFrame frame(const WindowKey &window) const;

private:
    WantedWindows m_windows;
    std::unique_ptr<AudioCapture> m_capture;
    std::vector<std::unique_ptr<AudioCapture>> m_stopping;
    float m_beatThreshold;
};

}
