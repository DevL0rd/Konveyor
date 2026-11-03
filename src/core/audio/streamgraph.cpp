#include "audio/streamgraph.h"

#include <QString>

#include <utility>

namespace Konveyor::Audio
{

namespace
{

QString lookup(const spa_dict *dict, const char *key)
{
    const char *value = spa_dict_lookup(dict, key);
    return value ? QString::fromUtf8(value) : QString();
}

StreamIdentity identityFrom(const spa_dict *props, const char *pidKey)
{
    return {lookup(props, pidKey).toLongLong(), lookup(props, PW_KEY_APP_PROCESS_BINARY), lookup(props, "pipewire.access.portal.app_id")};
}

}

StreamGraph::Watched::~Watched()
{
    spa_hook_remove(&listener);
    pw_proxy_destroy(proxy);
}

StreamGraph::StreamGraph(pw_core *core, Callbacks callbacks)
    : m_callbacks(std::move(callbacks))
    , m_registry(pw_core_get_registry(core, PW_VERSION_REGISTRY, 0))
{
    static const pw_registry_events events = [] {
        pw_registry_events built {};
        built.version = PW_VERSION_REGISTRY_EVENTS;
        built.global = &StreamGraph::onGlobal;
        built.global_remove = &StreamGraph::onGlobalRemove;
        return built;
    }();
    pw_registry_add_listener(m_registry, &m_registryListener, &events, this);
}

StreamGraph::~StreamGraph()
{
    m_nodes.clear();
    m_clients.clear();
    spa_hook_remove(&m_registryListener);
    pw_proxy_destroy(reinterpret_cast<pw_proxy *>(m_registry));
}

std::vector<PlayingStream> StreamGraph::streams() const
{
    std::vector<PlayingStream> playing;
    for (const auto &[id, node] : m_nodes) {
        if (node->known) {
            playing.push_back({id, node->serial, identityOf(*node)});
        }
    }
    return playing;
}

StreamIdentity StreamGraph::identityOf(const Watched &node) const
{
    const auto client = m_clients.find(node.clientId);
    return client == m_clients.end() ? node.identity : withFallback(node.identity, client->second->identity);
}

void StreamGraph::onGlobal(void *data, std::uint32_t id, std::uint32_t, const char *type, std::uint32_t, const spa_dict *props)
{
    auto *graph = static_cast<StreamGraph *>(data);
    if (spa_streq(type, PW_TYPE_INTERFACE_Client)) {
        graph->watchClient(id);
    } else if (props && spa_streq(type, PW_TYPE_INTERFACE_Node)
        && spa_streq(spa_dict_lookup(props, PW_KEY_MEDIA_CLASS), "Stream/Output/Audio")) {
        graph->watchNode(id, props);
    }
}

void StreamGraph::onGlobalRemove(void *data, std::uint32_t id)
{
    auto *graph = static_cast<StreamGraph *>(data);
    graph->m_clients.erase(id);
    if (graph->m_nodes.erase(id) > 0) {
        graph->m_callbacks.removed(id);
    }
}

void StreamGraph::watchNode(std::uint32_t id, const spa_dict *props)
{
    auto *proxy = static_cast<pw_proxy *>(pw_registry_bind(m_registry, id, PW_TYPE_INTERFACE_Node, PW_VERSION_NODE, 0));
    if (!proxy) {
        return;
    }
    auto node = std::make_unique<Watched>();
    node->graph = this;
    node->proxy = proxy;
    node->id = id;
    node->serial = lookup(props, PW_KEY_OBJECT_SERIAL).toULongLong();
    static const pw_node_events events = [] {
        pw_node_events built {};
        built.version = PW_VERSION_NODE_EVENTS;
        built.info = &StreamGraph::onNodeInfo;
        return built;
    }();
    pw_node_add_listener(reinterpret_cast<pw_node *>(proxy), &node->listener, &events, node.get());
    m_nodes[id] = std::move(node);
}

void StreamGraph::watchClient(std::uint32_t id)
{
    auto *proxy = static_cast<pw_proxy *>(pw_registry_bind(m_registry, id, PW_TYPE_INTERFACE_Client, PW_VERSION_CLIENT, 0));
    if (!proxy) {
        return;
    }
    auto client = std::make_unique<Watched>();
    client->graph = this;
    client->proxy = proxy;
    client->id = id;
    static const pw_client_events events = [] {
        pw_client_events built {};
        built.version = PW_VERSION_CLIENT_EVENTS;
        built.info = &StreamGraph::onClientInfo;
        return built;
    }();
    pw_client_add_listener(reinterpret_cast<pw_client *>(proxy), &client->listener, &events, client.get());
    m_clients[id] = std::move(client);
}

void StreamGraph::onNodeInfo(void *data, const pw_node_info *info)
{
    auto *node = static_cast<Watched *>(data);
    node->graph->nodeInfo(*node, *info);
}

void StreamGraph::onClientInfo(void *data, const pw_client_info *info)
{
    auto *client = static_cast<Watched *>(data);
    client->graph->clientInfo(*client, *info);
}

void StreamGraph::nodeInfo(Watched &node, const pw_node_info &info)
{
    if (!(info.change_mask & PW_NODE_CHANGE_MASK_PROPS) || !info.props) {
        return;
    }
    node.identity = identityFrom(info.props, PW_KEY_APP_PROCESS_ID);
    node.clientId = lookup(info.props, PW_KEY_CLIENT_ID).toUInt();
    node.known = true;
    m_callbacks.changed();
}

void StreamGraph::clientInfo(Watched &client, const pw_client_info &info)
{
    if (!(info.change_mask & PW_CLIENT_CHANGE_MASK_PROPS) || !info.props) {
        return;
    }
    client.identity = identityFrom(info.props, PW_KEY_SEC_PID);
    m_callbacks.changed();
}

}
