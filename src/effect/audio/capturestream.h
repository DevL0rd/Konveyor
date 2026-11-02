#pragma once

#include "audio/windowmix.h"

#include <pipewire/pipewire.h>

#include <QList>

#include <cstdint>
#include <map>

namespace Konveyor::Audio
{

class CaptureStream
{
public:
    CaptureStream(pw_core *core, std::uint64_t serial);
    ~CaptureStream();

    CaptureStream(const CaptureStream &) = delete;
    CaptureStream &operator=(const CaptureStream &) = delete;

    void setTargets(const QList<WindowMix *> &mixes);

private:
    static void onProcess(void *data);

    void process();

    pw_stream *m_stream = nullptr;
    spa_hook m_listener {};
    std::map<WindowMix *, WindowMix::Source> m_targets;
};

}
