#pragma once

#include "audio/audiomatch.h"
#include "audio/framemailbox.h"

#include <QHash>

#include <atomic>
#include <memory>
#include <mutex>
#include <thread>

struct spa_source;
struct pw_loop;

namespace Konveyor::Audio
{

using WantedWindows = QHash<WindowKey, std::shared_ptr<FrameMailbox>>;

class AudioCapture
{
public:
    explicit AudioCapture(float beatThreshold);
    ~AudioCapture();

    AudioCapture(const AudioCapture &) = delete;
    AudioCapture &operator=(const AudioCapture &) = delete;

    void setWanted(const WantedWindows &windows);
    void setBeatThreshold(float threshold);
    void requestStop();
    bool hasFinished() const { return m_finished.load(std::memory_order_acquire); }

private:
    class Session;

    void run();
    void attachLoop(pw_loop *loop, spa_source *wake);
    void detachLoop();
    void wake();
    bool takeWanted(WantedWindows &windows);
    bool stopRequested() const { return m_stop.load(std::memory_order_acquire); }

    std::mutex m_mutex;
    WantedWindows m_pending;
    bool m_pendingChanged = false;
    pw_loop *m_loop = nullptr;
    spa_source *m_wake = nullptr;
    std::atomic<float> m_beatThreshold;
    std::atomic<bool> m_stop = false;
    std::atomic<bool> m_finished = false;
    std::thread m_thread;
};

}
