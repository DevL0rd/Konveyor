#pragma once

#include "audio/audiomatch.h"

#include <pipewire/pipewire.h>

#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <vector>

namespace Konveyor::Audio
{

struct PlayingStream
{
    std::uint32_t id = 0;
    std::uint64_t serial = 0;
    StreamIdentity identity;
};

class StreamGraph
{
public:
    struct Callbacks
    {
        std::function<void()> changed;
        std::function<void(std::uint32_t)> removed;
    };

    StreamGraph(pw_core *core, Callbacks callbacks);
    ~StreamGraph();

    StreamGraph(const StreamGraph &) = delete;
    StreamGraph &operator=(const StreamGraph &) = delete;

    std::vector<PlayingStream> streams() const;

private:
    struct Watched
    {
        StreamGraph *graph = nullptr;
        pw_proxy *proxy = nullptr;
        spa_hook listener {};
        std::uint32_t id = 0;
        std::uint64_t serial = 0;
        std::uint32_t clientId = PW_ID_ANY;
        StreamIdentity identity;
        bool known = false;

        ~Watched();
    };

    static void onGlobal(
        void *data, std::uint32_t id, std::uint32_t permissions, const char *type, std::uint32_t version, const spa_dict *props);
    static void onGlobalRemove(void *data, std::uint32_t id);
    static void onNodeInfo(void *data, const pw_node_info *info);
    static void onClientInfo(void *data, const pw_client_info *info);

    void watchNode(std::uint32_t id, const spa_dict *props);
    void watchClient(std::uint32_t id);
    void nodeInfo(Watched &node, const pw_node_info &info);
    void clientInfo(Watched &client, const pw_client_info &info);
    StreamIdentity identityOf(const Watched &node) const;

    Callbacks m_callbacks;
    pw_registry *m_registry = nullptr;
    spa_hook m_registryListener {};
    std::map<std::uint32_t, std::unique_ptr<Watched>> m_nodes;
    std::map<std::uint32_t, std::unique_ptr<Watched>> m_clients;
};

}
