#pragma once

#include <pipewire/pipewire.h>

namespace
{

struct FakeStream
{
    const pw_stream_events *events = nullptr;
    void *data = nullptr;
    bool failNew = false;
    int connectResult = 0;
    pw_buffer *nextBuffer = nullptr;
    int created = 0;
    int connected = 0;
    int destroyed = 0;
    int queued = 0;
};

FakeStream g_stream;
int g_proxiesDestroyed = 0;
char g_streamHandle = 0;

}
