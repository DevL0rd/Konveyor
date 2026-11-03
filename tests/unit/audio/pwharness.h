#pragma once

#include <pipewire/pipewire.h>
#include <spa/param/audio/format-utils.h>
#include <spa/param/audio/raw.h>
#include <spa/param/param.h>
#include <spa/param/port-config.h>
#include <spa/pod/builder.h>

#include <QByteArray>
#include <QDir>
#include <QFile>
#include <QProcess>
#include <QStandardPaths>
#include <QString>
#include <QTemporaryDir>
#include <QTest>

#include <algorithm>
#include <array>
#include <atomic>
#include <cmath>
#include <memory>
#include <numbers>

#include "pwsessionstub.h"

namespace PwHarness
{

inline constexpr const char *kRemote = "konveyor-test-pw";
inline constexpr const char *kPlayerNode = "konveyor-test-player";
inline constexpr const char *kCaptureNode = "konveyor-window-audio";
inline constexpr const char *kPortalAppId = "org.test.Player";
inline constexpr const char *kPlayerBinary = "fakeplayer";

inline constexpr const char *kConfig = R"(context.properties = {
    core.daemon = true
    core.name = konveyor-test-pw
}
context.spa-libs = {
    audio.convert.* = audioconvert/libspa-audioconvert
    audio.adapt = audioconvert/libspa-audioconvert
    support.* = support/libspa-support
}
context.modules = [
    { name = libpipewire-module-protocol-native }
    { name = libpipewire-module-metadata }
    { name = libpipewire-module-spa-node-factory }
    { name = libpipewire-module-client-node }
    { name = libpipewire-module-access }
    { name = libpipewire-module-adapter }
    { name = libpipewire-module-link-factory }
]
context.objects = [
    { factory = spa-node-factory
      args = { factory.name = support.node.driver node.name = Dummy-Driver priority.driver = 8000 } }
    { factory = adapter
      args = { factory.name = support.null-audio-sink node.name = konveyor-test-orphan media.class = Stream/Output/Audio
               object.linger = true audio.position = [ MONO ] } }
]
)";

class Daemon
{
public:
    bool start(const QString &directory)
    {
        const QString config = directory + QStringLiteral("/pipewire.conf");
        QFile file(config);
        if (!file.open(QIODevice::WriteOnly) || file.write(kConfig) < 0) {
            return false;
        }
        file.close();
        QProcessEnvironment environment = QProcessEnvironment::systemEnvironment();
        environment.insert(QStringLiteral("XDG_RUNTIME_DIR"), directory);
        environment.insert(QStringLiteral("PIPEWIRE_RUNTIME_DIR"), directory);
        environment.remove(QStringLiteral("PIPEWIRE_REMOTE"));
        m_process.setProcessEnvironment(environment);
        m_process.setProcessChannelMode(QProcess::ForwardedErrorChannel);
        m_process.start(QStringLiteral("pipewire"), {QStringLiteral("-c"), config});
        if (!m_process.waitForStarted(10000)) {
            return false;
        }
        for (int attempt = 0; attempt < 100 && !QFile::exists(directory + QLatin1Char('/') + QLatin1String(kRemote)); ++attempt) {
            QTest::qWait(50);
        }
        return QFile::exists(directory + QLatin1Char('/') + QLatin1String(kRemote));
    }

    void stop()
    {
        if (m_process.state() != QProcess::NotRunning) {
            m_process.terminate();
            if (!m_process.waitForFinished(5000)) {
                m_process.kill();
                m_process.waitForFinished(5000);
            }
        }
    }

    ~Daemon() { stop(); }

private:
    QProcess m_process;
};

inline QString isolate(const QTemporaryDir &directory)
{
    if (!directory.isValid()) {
        return QStringLiteral("no temporary directory");
    }
    for (const char *tool : {"pipewire", "pw-link"}) {
        if (QStandardPaths::findExecutable(QLatin1String(tool)).isEmpty()) {
            return QStringLiteral("%1 is required for these tests").arg(QLatin1String(tool));
        }
    }
    qputenv("XDG_RUNTIME_DIR", directory.path().toUtf8());
    qputenv("PIPEWIRE_RUNTIME_DIR", directory.path().toUtf8());
    qputenv("PIPEWIRE_REMOTE", kRemote);
    qunsetenv("WAYLAND_DISPLAY");
    qunsetenv("DISPLAY");
    pw_init(nullptr, nullptr);
    return {};
}

inline bool runTool(const QString &program, const QStringList &arguments, QByteArray *output = nullptr)
{
    QProcess process;
    process.start(program, arguments);
    if (!process.waitForFinished(5000)) {
        process.kill();
        process.waitForFinished(5000);
        return false;
    }
    if (output) {
        *output = process.readAllStandardOutput();
    }
    return process.exitStatus() == QProcess::NormalExit && process.exitCode() == 0;
}

inline QString portStartingWith(const QString &flag, const QString &prefix)
{
    QByteArray listing;
    if (!runTool(QStringLiteral("pw-link"), {flag}, &listing)) {
        return {};
    }
    for (const QByteArray &line : listing.split('\n')) {
        const QString port = QString::fromUtf8(line).trimmed();
        if (port.startsWith(prefix)) {
            return port;
        }
    }
    return {};
}

inline bool clientConnected(const QString &binary)
{
    QByteArray dump;
    return runTool(QStringLiteral("pw-dump"), {}, &dump)
        && dump.contains(QByteArray("\"application.process.binary\": \"") + binary.toUtf8() + QByteArray("\""));
}

inline bool linkPlayerToCapture()
{
    const QString output = portStartingWith(QStringLiteral("-o"), QLatin1String(kPlayerNode) + QLatin1Char(':'));
    const QString input = portStartingWith(QStringLiteral("-i"), QLatin1String(kCaptureNode) + QLatin1Char(':'));
    return !output.isEmpty() && !input.isEmpty() && runTool(QStringLiteral("pw-link"), {output, input});
}

class Player
{
public:
    enum class Mode : int
    {
        Tone,
        Silent,
        Clicks
    };

    bool start(Mode mode)
    {
        m_mode = static_cast<int>(mode);
        m_loop = pw_thread_loop_new("konveyor-test-player", nullptr);
        if (!m_loop) {
            return false;
        }
        pw_thread_loop_start(m_loop);
        pw_thread_loop_lock(m_loop);
        m_context = pw_context_new(pw_thread_loop_get_loop(m_loop), nullptr, 0);
        m_core = m_context ? pw_context_connect(m_context, nullptr, 0) : nullptr;
        if (m_core) {
            connectStream();
        }
        pw_thread_loop_unlock(m_loop);
        return m_stream != nullptr;
    }

    void setMode(Mode mode) { m_mode.store(static_cast<int>(mode)); }

    void stop()
    {
        if (!m_loop) {
            return;
        }
        pw_thread_loop_stop(m_loop);
        if (m_stream) {
            pw_stream_destroy(m_stream);
        }
        if (m_core) {
            pw_core_disconnect(m_core);
        }
        if (m_context) {
            pw_context_destroy(m_context);
        }
        pw_thread_loop_destroy(m_loop);
        m_loop = nullptr;
        m_stream = nullptr;
        m_core = nullptr;
        m_context = nullptr;
    }

    ~Player() { stop(); }

private:
    static void onProcess(void *data) { static_cast<Player *>(data)->process(); }

    void connectStream()
    {
        pw_properties *properties
            = pw_properties_new(PW_KEY_MEDIA_TYPE, "Audio", PW_KEY_MEDIA_CATEGORY, "Playback", PW_KEY_MEDIA_ROLE, "Music", PW_KEY_NODE_NAME,
                kPlayerNode, PW_KEY_APP_PROCESS_BINARY, kPlayerBinary, "pipewire.access.portal.app_id", kPortalAppId, nullptr);
        m_stream = pw_stream_new(m_core, kPlayerNode, properties);
        if (!m_stream) {
            return;
        }
        static const pw_stream_events events = [] {
            pw_stream_events built {};
            built.version = PW_VERSION_STREAM_EVENTS;
            built.process = &Player::onProcess;
            return built;
        }();
        pw_stream_add_listener(m_stream, &m_listener, &events, this);
        std::array<std::uint8_t, 256> storage {};
        spa_pod_builder builder {};
        spa_pod_builder_init(&builder, storage.data(), storage.size());
        spa_audio_info_raw format {};
        format.format = SPA_AUDIO_FORMAT_F32;
        format.rate = 48000;
        format.channels = 1;
        format.position[0] = SPA_AUDIO_CHANNEL_MONO;
        const spa_pod *parameters = spa_format_audio_raw_build(&builder, SPA_PARAM_EnumFormat, &format);
        const auto flags = static_cast<pw_stream_flags>(PW_STREAM_FLAG_AUTOCONNECT | PW_STREAM_FLAG_MAP_BUFFERS);
        if (pw_stream_connect(m_stream, PW_DIRECTION_OUTPUT, PW_ID_ANY, flags, &parameters, 1) < 0) {
            pw_stream_destroy(m_stream);
            m_stream = nullptr;
        }
    }

    float sampleAt(std::uint64_t index, Mode mode) const
    {
        const double seconds = static_cast<double>(index) / 48000.0;
        switch (mode) {
        case Mode::Silent:
            return 0.0F;
        case Mode::Clicks:
            return std::fmod(seconds, 0.5) < 0.03 ? 0.9F * static_cast<float>(std::sin(2.0 * std::numbers::pi * 70.0 * seconds)) : 0.0F;
        case Mode::Tone:
            break;
        }
        return 0.5F * static_cast<float>(std::sin(2.0 * std::numbers::pi * 1000.0 * seconds));
    }

    void process()
    {
        pw_buffer *buffer = pw_stream_dequeue_buffer(m_stream);
        if (!buffer) {
            return;
        }
        spa_data &block = buffer->buffer->datas[0];
        if (block.data) {
            const std::uint32_t capacity = block.maxsize / sizeof(float);
            const std::uint32_t count = buffer->requested ? std::min<std::uint64_t>(buffer->requested, capacity) : capacity;
            auto *samples = static_cast<float *>(block.data);
            const auto mode = static_cast<Mode>(m_mode.load());
            for (std::uint32_t index = 0; index < count; ++index) {
                samples[index] = sampleAt(m_position++, mode);
            }
            block.chunk->offset = 0;
            block.chunk->stride = sizeof(float);
            block.chunk->size = count * sizeof(float);
        }
        pw_stream_queue_buffer(m_stream, buffer);
    }

    pw_thread_loop *m_loop = nullptr;
    pw_context *m_context = nullptr;
    pw_core *m_core = nullptr;
    pw_stream *m_stream = nullptr;
    spa_hook m_listener {};
    std::atomic<int> m_mode = 0;
    std::uint64_t m_position = 0;
};

}
