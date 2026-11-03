#include "audio/capturestream.h"
#include "config/log.h"

#include <spa/param/audio/format-utils.h>

#include <algorithm>
#include <array>

namespace Konveyor::Audio
{

namespace
{

const pw_stream_events &streamEvents(void (*process)(void *))
{
    static const pw_stream_events events = [process] {
        pw_stream_events built {};
        built.version = PW_VERSION_STREAM_EVENTS;
        built.process = process;
        return built;
    }();
    return events;
}

pw_properties *monitorProperties(std::uint64_t serial)
{
    pw_properties *properties = pw_properties_new(PW_KEY_MEDIA_TYPE, "Audio", PW_KEY_MEDIA_CATEGORY, "Capture", PW_KEY_MEDIA_ROLE, "Music",
        PW_KEY_NODE_NAME, "konveyor-window-audio", PW_KEY_NODE_DESCRIPTION, "Konveyor window effects", PW_KEY_APP_NAME, "Konveyor",
        PW_KEY_STREAM_MONITOR, "true", PW_KEY_NODE_PASSIVE, "true", PW_KEY_NODE_ASYNC, "true", PW_KEY_NODE_DONT_RECONNECT, "true",
        "node.dont-fallback", "true", "node.dont-move", "true", nullptr);
    pw_properties_setf(properties, PW_KEY_TARGET_OBJECT, "%llu", static_cast<unsigned long long>(serial));
    return properties;
}

}

CaptureStream::CaptureStream(pw_core *core, std::uint64_t serial)
{
    m_stream = pw_stream_new(core, "konveyor-window-audio", monitorProperties(serial));
    if (!m_stream) {
        return;
    }
    pw_stream_add_listener(m_stream, &m_listener, &streamEvents(&CaptureStream::onProcess), this);
    std::array<std::uint8_t, 256> storage {};
    spa_pod_builder builder {};
    spa_pod_builder_init(&builder, storage.data(), storage.size());
    spa_audio_info_raw format {};
    format.format = SPA_AUDIO_FORMAT_F32;
    format.rate = kCaptureRate;
    format.channels = 1;
    format.position[0] = SPA_AUDIO_CHANNEL_MONO;
    const spa_pod *parameters = spa_format_audio_raw_build(&builder, SPA_PARAM_EnumFormat, &format);
    const auto flags = static_cast<pw_stream_flags>(PW_STREAM_FLAG_AUTOCONNECT | PW_STREAM_FLAG_MAP_BUFFERS);
    if (pw_stream_connect(m_stream, PW_DIRECTION_INPUT, PW_ID_ANY, flags, &parameters, 1) < 0) {
        qCWarning(lcKonveyor) << "could not start capturing a window's audio";
    }
}

CaptureStream::~CaptureStream()
{
    setTargets({});
    if (m_stream) {
        pw_stream_destroy(m_stream);
    }
}

void CaptureStream::setTargets(const QList<WindowMix *> &mixes)
{
    for (auto entry = m_targets.begin(); entry != m_targets.end();) {
        if (mixes.contains(entry->first)) {
            ++entry;
            continue;
        }
        entry->first->removeSource(entry->second);
        entry = m_targets.erase(entry);
    }
    for (WindowMix *mix : mixes) {
        const auto [entry, added] = m_targets.try_emplace(mix);
        if (added) {
            mix->addSource(entry->second);
        }
    }
}

void CaptureStream::onProcess(void *data)
{
    static_cast<CaptureStream *>(data)->process();
}

void CaptureStream::process()
{
    pw_buffer *buffer = pw_stream_dequeue_buffer(m_stream);
    if (!buffer) {
        return;
    }
    const spa_data &block = buffer->buffer->datas[0];
    if (block.data && block.chunk) {
        const std::uint32_t offset = std::min(block.chunk->offset, block.maxsize);
        const std::uint32_t size = std::min(block.chunk->size, block.maxsize - offset);
        const auto *samples = reinterpret_cast<const float *>(static_cast<const char *>(block.data) + offset);
        const auto now = WindowMix::Clock::now();
        for (auto &[mix, source] : m_targets) {
            mix->write(source, samples, size / sizeof(float), now);
        }
    }
    pw_stream_queue_buffer(m_stream, buffer);
}

}
