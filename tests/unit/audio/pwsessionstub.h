#pragma once

#include <pipewire/pipewire.h>
#include <spa/param/audio/format-utils.h>
#include <spa/param/audio/raw.h>
#include <spa/param/param.h>
#include <spa/param/port-config.h>
#include <spa/pod/builder.h>

#include <array>
#include <cstdint>
#include <vector>

namespace PwHarness
{

inline pw_thread_loop *startRunningThreadLoop(const char *name)
{
    const spa_dict_item item = SPA_DICT_ITEM_INIT("thread-loop.start-signal", "true");
    const spa_dict properties = SPA_DICT_INIT(&item, 1);
    pw_thread_loop *loop = pw_thread_loop_new(name, &properties);
    if (loop) {
        pw_thread_loop_lock(loop);
        pw_thread_loop_start(loop);
        pw_thread_loop_wait(loop);
        pw_thread_loop_unlock(loop);
    }
    return loop;
}

class SessionStub
{
public:
    bool start()
    {
        m_loop = startRunningThreadLoop("konveyor-test-session");
        if (!m_loop) {
            return false;
        }
        pw_thread_loop_lock(m_loop);
        m_context = pw_context_new(pw_thread_loop_get_loop(m_loop), nullptr, 0);
        m_core = m_context ? pw_context_connect(m_context, nullptr, 0) : nullptr;
        if (m_core) {
            m_registry = pw_core_get_registry(m_core, PW_VERSION_REGISTRY, 0);
            static const pw_registry_events events = [] {
                pw_registry_events built {};
                built.version = PW_VERSION_REGISTRY_EVENTS;
                built.global = &SessionStub::onGlobal;
                return built;
            }();
            pw_registry_add_listener(m_registry, &m_listener, &events, this);
        }
        pw_thread_loop_unlock(m_loop);
        return m_registry != nullptr;
    }

    void stop()
    {
        if (!m_loop) {
            return;
        }
        pw_thread_loop_stop(m_loop);
        for (pw_proxy *node : m_nodes) {
            pw_proxy_destroy(node);
        }
        m_nodes.clear();
        if (m_registry) {
            spa_hook_remove(&m_listener);
            pw_proxy_destroy(reinterpret_cast<pw_proxy *>(m_registry));
        }
        if (m_core) {
            pw_core_disconnect(m_core);
        }
        if (m_context) {
            pw_context_destroy(m_context);
        }
        pw_thread_loop_destroy(m_loop);
        m_loop = nullptr;
        m_registry = nullptr;
        m_core = nullptr;
        m_context = nullptr;
    }

    ~SessionStub() { stop(); }

private:
    static void onGlobal(void *data, std::uint32_t id, std::uint32_t, const char *type, std::uint32_t, const spa_dict *props)
    {
        auto *stub = static_cast<SessionStub *>(data);
        const char *mediaClass = props ? spa_dict_lookup(props, PW_KEY_MEDIA_CLASS) : nullptr;
        if (!spa_streq(type, PW_TYPE_INTERFACE_Node) || !mediaClass) {
            return;
        }
        const bool output = spa_streq(mediaClass, "Stream/Output/Audio");
        if (output || spa_streq(mediaClass, "Stream/Input/Audio")) {
            stub->configure(id, output ? SPA_DIRECTION_OUTPUT : SPA_DIRECTION_INPUT);
        }
    }

    void configure(std::uint32_t id, spa_direction direction)
    {
        auto *node = static_cast<pw_proxy *>(pw_registry_bind(m_registry, id, PW_TYPE_INTERFACE_Node, PW_VERSION_NODE, 0));
        if (!node) {
            return;
        }
        m_nodes.push_back(node);
        std::array<std::uint8_t, 512> storage {};
        spa_pod_builder builder {};
        spa_pod_builder_init(&builder, storage.data(), storage.size());
        spa_audio_info_raw info {};
        info.format = SPA_AUDIO_FORMAT_F32P;
        info.rate = 48000;
        info.channels = 1;
        info.position[0] = SPA_AUDIO_CHANNEL_MONO;
        const spa_pod *format = spa_format_audio_raw_build(&builder, SPA_PARAM_Format, &info);
        auto *config = static_cast<spa_pod *>(spa_pod_builder_add_object(&builder, SPA_TYPE_OBJECT_ParamPortConfig, SPA_PARAM_PortConfig,
            SPA_PARAM_PORT_CONFIG_direction, SPA_POD_Id(direction), SPA_PARAM_PORT_CONFIG_mode, SPA_POD_Id(SPA_PARAM_PORT_CONFIG_MODE_dsp),
            SPA_PARAM_PORT_CONFIG_monitor, SPA_POD_Bool(false), SPA_PARAM_PORT_CONFIG_control, SPA_POD_Bool(false),
            SPA_PARAM_PORT_CONFIG_format, SPA_POD_Pod(format)));
        pw_node_set_param(reinterpret_cast<pw_node *>(node), SPA_PARAM_PortConfig, 0, config);
    }

    pw_thread_loop *m_loop = nullptr;
    pw_context *m_context = nullptr;
    pw_core *m_core = nullptr;
    pw_registry *m_registry = nullptr;
    spa_hook m_listener {};
    std::vector<pw_proxy *> m_nodes;
};

}
