#include "audio/audiocapture.h"
#include "audio/capturestream.h"
#include "audio/streamgraph.h"
#include "audio/windowmix.h"
#include "config/log.h"

#include <pipewire/pipewire.h>

#include <pthread.h>
#include <signal.h>

#include <cerrno>
#include <chrono>
#include <cstdint>
#include <map>
#include <memory>
#include <mutex>
#include <tuple>

namespace Konveyor::Audio
{

namespace
{

constexpr std::chrono::milliseconds kTick(50);
constexpr std::chrono::seconds kReconnectAfter(3);

timespec asTimespec(std::chrono::nanoseconds value)
{
    const auto seconds = std::chrono::duration_cast<std::chrono::seconds>(value);
    return {static_cast<time_t>(seconds.count()), static_cast<long>((value - seconds).count())};
}

}

class AudioCapture::Session
{
public:
    Session(AudioCapture &owner, pw_main_loop *mainLoop)
        : m_owner(owner)
        , m_mainLoop(mainLoop)
        , m_loop(pw_main_loop_get_loop(mainLoop))
    {
        m_context = pw_context_new(m_loop, pw_properties_new("module.rt", "false", "context.num-data-loops", "0", nullptr), 0);
        m_tick = pw_loop_add_timer(m_loop, &Session::onTick, this);
        timespec first = asTimespec(kTick);
        timespec every = first;
        pw_loop_update_timer(m_loop, m_tick, &first, &every, false);
        connect();
    }

    ~Session()
    {
        disconnect();
        pw_loop_destroy_source(m_loop, m_tick);
        if (m_context) {
            pw_context_destroy(m_context);
        }
    }

    void refresh()
    {
        if (m_owner.stopRequested()) {
            pw_main_loop_quit(m_mainLoop);
            return;
        }
        WantedWindows wanted;
        if (m_owner.takeWanted(wanted)) {
            m_wanted = wanted;
            reconcile();
        }
    }

private:
    static void onTick(void *data, std::uint64_t) { static_cast<Session *>(data)->tick(); }

    static void onCoreError(void *data, std::uint32_t id, int, int result, const char *)
    {
        auto *session = static_cast<Session *>(data);
        if (id == PW_ID_CORE && result == -EPIPE) {
            session->m_lost = true;
        }
    }

    void tick()
    {
        const auto now = WindowMix::Clock::now();
        if (m_lost) {
            disconnect();
            m_retryAt = now + kReconnectAfter;
            m_lost = false;
        }
        if (!m_core && now >= m_retryAt) {
            connect();
        }
        const float threshold = m_owner.m_beatThreshold.load(std::memory_order_relaxed);
        for (auto &[key, mix] : m_mixes) {
            mix->setBeatThreshold(threshold);
            mix->tick(now);
        }
    }

    void connect()
    {
        m_core = m_context ? pw_context_connect(m_context, nullptr, 0) : nullptr;
        if (!m_core) {
            m_retryAt = WindowMix::Clock::now() + kReconnectAfter;
            return;
        }
        static const pw_core_events coreEvents = [] {
            pw_core_events built {};
            built.version = PW_VERSION_CORE_EVENTS;
            built.error = &Session::onCoreError;
            return built;
        }();
        pw_core_add_listener(m_core, &m_coreListener, &coreEvents, this);
        m_graph = std::make_unique<StreamGraph>(
            m_core, StreamGraph::Callbacks {[this] { reconcile(); }, [this](std::uint32_t id) { m_captures.erase(id); }});
    }

    void disconnect()
    {
        m_captures.clear();
        m_mixes.clear();
        m_graph.reset();
        if (m_core) {
            spa_hook_remove(&m_coreListener);
            pw_core_disconnect(m_core);
            m_core = nullptr;
        }
    }

    void reconcile()
    {
        if (!m_graph) {
            return;
        }
        const QList<WindowKey> keys = m_wanted.keys();
        for (const WindowKey &key : keys) {
            if (!m_mixes.contains(key)) {
                m_mixes[key] = std::make_unique<WindowMix>(m_wanted.value(key));
            }
        }
        for (const PlayingStream &stream : m_graph->streams()) {
            reconcileStream(stream, keys);
        }
        std::erase_if(m_mixes, [this](const auto &entry) { return !m_wanted.contains(entry.first); });
    }

    void reconcileStream(const PlayingStream &stream, const QList<WindowKey> &keys)
    {
        const QList<WindowKey> playing = windowsPlaying(stream.identity, keys);
        if (playing.isEmpty()) {
            m_captures.erase(stream.id);
            return;
        }
        QList<WindowMix *> mixes;
        for (const WindowKey &key : playing) {
            mixes.append(m_mixes.at(key).get());
        }
        auto &capture = m_captures[stream.id];
        if (!capture) {
            capture = std::make_unique<CaptureStream>(m_core, stream.serial);
        }
        capture->setTargets(mixes);
    }

    AudioCapture &m_owner;
    pw_main_loop *m_mainLoop = nullptr;
    pw_loop *m_loop = nullptr;
    pw_context *m_context = nullptr;
    pw_core *m_core = nullptr;
    std::unique_ptr<StreamGraph> m_graph;
    spa_hook m_coreListener {};
    spa_source *m_tick = nullptr;
    WantedWindows m_wanted;
    std::map<std::uint32_t, std::unique_ptr<CaptureStream>> m_captures;
    std::map<WindowKey, std::unique_ptr<WindowMix>, bool (*)(const WindowKey &, const WindowKey &)> m_mixes {
        [](const WindowKey &left, const WindowKey &right) {
            return std::tie(left.pid, left.desktopFile) < std::tie(right.pid, right.desktopFile);
        }};
    WindowMix::Clock::time_point m_retryAt;
    bool m_lost = false;
};

AudioCapture::AudioCapture(float beatThreshold)
    : m_beatThreshold(beatThreshold)
    , m_thread(&AudioCapture::run, this)
{ }

AudioCapture::~AudioCapture()
{
    requestStop();
    m_thread.join();
}

void AudioCapture::setWanted(const WantedWindows &windows)
{
    {
        const std::scoped_lock lock(m_mutex);
        m_pending = windows;
        m_pendingChanged = true;
    }
    wake();
}

void AudioCapture::setBeatThreshold(float threshold)
{
    m_beatThreshold.store(threshold, std::memory_order_relaxed);
}

void AudioCapture::requestStop()
{
    m_stop.store(true, std::memory_order_release);
    wake();
}

void AudioCapture::wake()
{
    const std::scoped_lock lock(m_mutex);
    if (m_wake) {
        pw_loop_signal_event(m_loop, m_wake);
    }
}

bool AudioCapture::takeWanted(WantedWindows &windows)
{
    const std::scoped_lock lock(m_mutex);
    if (!m_pendingChanged) {
        return false;
    }
    windows = m_pending;
    m_pendingChanged = false;
    return true;
}

void AudioCapture::attachLoop(pw_loop *loop, spa_source *wakeSource)
{
    const std::scoped_lock lock(m_mutex);
    m_loop = loop;
    m_wake = wakeSource;
}

void AudioCapture::detachLoop()
{
    const std::scoped_lock lock(m_mutex);
    m_wake = nullptr;
    m_loop = nullptr;
}

void AudioCapture::run()
{
    sigset_t all;
    sigfillset(&all);
    pthread_sigmask(SIG_BLOCK, &all, nullptr);
    pthread_setname_np(pthread_self(), "konveyor-audio");

    static std::once_flag initialized;
    std::call_once(initialized, [] { pw_init(nullptr, nullptr); });
    pw_main_loop *mainLoop = pw_main_loop_new(nullptr);
    if (!mainLoop) {
        qCWarning(lcKonveyor) << "could not start the window audio thread";
        m_finished.store(true, std::memory_order_release);
        return;
    }
    {
        Session session(*this, mainLoop);
        pw_loop *loop = pw_main_loop_get_loop(mainLoop);
        spa_source *wakeSource
            = pw_loop_add_event(loop, [](void *data, std::uint64_t) { static_cast<Session *>(data)->refresh(); }, &session);
        attachLoop(loop, wakeSource);
        session.refresh();
        if (!stopRequested()) {
            pw_main_loop_run(mainLoop);
        }
        detachLoop();
        pw_loop_destroy_source(loop, wakeSource);
    }
    pw_main_loop_destroy(mainLoop);
    m_finished.store(true, std::memory_order_release);
}

}
