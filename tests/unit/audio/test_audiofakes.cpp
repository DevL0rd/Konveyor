#include "audio/capturestream.h"
#include "audio/streamgraph.h"

#include <pipewire/pipewire.h>

#include <QTest>

#include <array>
#include <cerrno>
#include <cmath>
#include <numbers>
#include <optional>
#include <vector>

using namespace Konveyor::Audio;

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

struct FakeNode
{
    spa_interface iface {};
    const pw_node_events *nodeEvents = nullptr;
    const pw_client_events *clientEvents = nullptr;
    void *data = nullptr;
};

struct FakeRegistry
{
    spa_interface iface {};
    pw_registry_methods methods {};
    const pw_registry_events *events = nullptr;
    void *data = nullptr;
    bool bindFails = false;
    FakeNode node;
    FakeNode client;
    pw_node_methods nodeMethods {};
    pw_client_methods clientMethods {};
};

struct FakeCore
{
    spa_interface iface {};
    pw_core_methods methods {};
    FakeRegistry registry;
};

int registryAddListener(void *object, spa_hook *, const pw_registry_events *events, void *data)
{
    auto *registry = static_cast<FakeRegistry *>(object);
    registry->events = events;
    registry->data = data;
    return 0;
}

int nodeAddListener(void *object, spa_hook *, const pw_node_events *events, void *data)
{
    auto *node = static_cast<FakeNode *>(object);
    node->nodeEvents = events;
    node->data = data;
    return 0;
}

int clientAddListener(void *object, spa_hook *, const pw_client_events *events, void *data)
{
    auto *client = static_cast<FakeNode *>(object);
    client->clientEvents = events;
    client->data = data;
    return 0;
}

void *registryBind(void *object, std::uint32_t, const char *type, std::uint32_t, size_t)
{
    auto *registry = static_cast<FakeRegistry *>(object);
    if (registry->bindFails) {
        return nullptr;
    }
    return spa_streq(type, PW_TYPE_INTERFACE_Node) ? static_cast<void *>(&registry->node) : static_cast<void *>(&registry->client);
}

pw_registry *coreGetRegistry(void *object, std::uint32_t, size_t)
{
    auto *core = static_cast<FakeCore *>(object);
    return reinterpret_cast<pw_registry *>(&core->registry);
}

void wire(FakeCore &core)
{
    FakeRegistry &registry = core.registry;
    registry.nodeMethods.version = PW_VERSION_NODE_METHODS;
    registry.nodeMethods.add_listener = &nodeAddListener;
    registry.clientMethods.version = PW_VERSION_CLIENT_METHODS;
    registry.clientMethods.add_listener = &clientAddListener;
    registry.node.iface = SPA_INTERFACE_INIT(PW_TYPE_INTERFACE_Node, PW_VERSION_NODE, &registry.nodeMethods, &registry.node);
    registry.client.iface = SPA_INTERFACE_INIT(PW_TYPE_INTERFACE_Client, PW_VERSION_CLIENT, &registry.clientMethods, &registry.client);
    registry.methods.version = PW_VERSION_REGISTRY_METHODS;
    registry.methods.add_listener = &registryAddListener;
    registry.methods.bind = &registryBind;
    registry.iface = SPA_INTERFACE_INIT(PW_TYPE_INTERFACE_Registry, PW_VERSION_REGISTRY, &registry.methods, &registry);
    core.methods.version = PW_VERSION_CORE_METHODS;
    core.methods.get_registry = &coreGetRegistry;
    core.iface = SPA_INTERFACE_INIT(PW_TYPE_INTERFACE_Core, PW_VERSION_CORE, &core.methods, &core);
}

pw_core *coreOf(FakeCore &core)
{
    return reinterpret_cast<pw_core *>(&core);
}

spa_dict dictOf(std::vector<spa_dict_item> &items)
{
    return SPA_DICT_INIT(items.data(), static_cast<std::uint32_t>(items.size()));
}

}

extern "C" {

pw_stream *pw_stream_new(pw_core *, const char *, pw_properties *properties)
{
    pw_properties_free(properties);
    ++g_stream.created;
    return g_stream.failNew ? nullptr : reinterpret_cast<pw_stream *>(&g_streamHandle);
}

void pw_stream_add_listener(pw_stream *, spa_hook *, const pw_stream_events *events, void *data)
{
    g_stream.events = events;
    g_stream.data = data;
}

int pw_stream_connect(pw_stream *, enum pw_direction, uint32_t, enum pw_stream_flags, const spa_pod **, uint32_t)
{
    ++g_stream.connected;
    return g_stream.connectResult;
}

void pw_stream_destroy(pw_stream *)
{
    ++g_stream.destroyed;
}

pw_buffer *pw_stream_dequeue_buffer(pw_stream *)
{
    return g_stream.nextBuffer;
}

int pw_stream_queue_buffer(pw_stream *, pw_buffer *)
{
    return ++g_stream.queued;
}

void pw_proxy_destroy(pw_proxy *)
{
    ++g_proxiesDestroyed;
}
}

class TestAudioFakes : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void init();
    void aGraphIgnoresGlobalsWithoutProperties();
    void aGraphSurvivesObjectsThatCannotBeBound();
    void aGraphWaitsForNodeAndClientProperties();
    void aGraphKnowsAStreamOnceItsNodeReportsProperties();
    void aStreamThatCannotBeCreatedStaysInert();
    void aStreamThatCannotConnectWarns();
    void processingWithoutABufferQueuesNothing();
    void processingABufferWithoutDataRecyclesIt();
    void processingABufferMixesItsSamples();
};

void TestAudioFakes::init()
{
    g_stream = FakeStream {};
    g_proxiesDestroyed = 0;
}

void TestAudioFakes::aGraphIgnoresGlobalsWithoutProperties()
{
    FakeCore core;
    wire(core);
    StreamGraph graph(coreOf(core), {[] { }, [](std::uint32_t) { }});
    core.registry.events->global(core.registry.data, 7, 0, PW_TYPE_INTERFACE_Node, 0, nullptr);
    QVERIFY(core.registry.node.nodeEvents == nullptr);
    QVERIFY(graph.streams().empty());
}

struct AnnouncedOutput
{
    explicit AnnouncedOutput(bool bindFails)
    {
        wire(core);
        core.registry.bindFails = bindFails;
        graph.emplace(coreOf(core), StreamGraph::Callbacks {[this] { ++changes; }, [](std::uint32_t) { }});
        core.registry.events->global(core.registry.data, 7, 0, PW_TYPE_INTERFACE_Node, 0, &dict);
        core.registry.events->global(core.registry.data, 8, 0, PW_TYPE_INTERFACE_Client, 0, nullptr);
    }

    FakeCore core;
    int changes = 0;
    std::optional<StreamGraph> graph;
    std::vector<spa_dict_item> items {{PW_KEY_MEDIA_CLASS, "Stream/Output/Audio"}, {PW_KEY_OBJECT_SERIAL, "5"}};
    spa_dict dict = dictOf(items);
};

void TestAudioFakes::aGraphSurvivesObjectsThatCannotBeBound()
{
    AnnouncedOutput output(true);
    QVERIFY(output.core.registry.node.nodeEvents == nullptr);
    QVERIFY(output.core.registry.client.clientEvents == nullptr);
    QVERIFY(output.graph->streams().empty());
    QCOMPARE(output.changes, 0);
}

void TestAudioFakes::aGraphWaitsForNodeAndClientProperties()
{
    AnnouncedOutput output(false);
    FakeCore &core = output.core;
    spa_dict &dict = output.dict;
    QVERIFY(core.registry.node.nodeEvents != nullptr);
    QVERIFY(core.registry.client.clientEvents != nullptr);

    pw_node_info nodeInfo {};
    nodeInfo.change_mask = PW_NODE_CHANGE_MASK_PROPS;
    core.registry.node.nodeEvents->info(core.registry.node.data, &nodeInfo);
    nodeInfo.change_mask = PW_NODE_CHANGE_MASK_STATE;
    nodeInfo.props = &dict;
    core.registry.node.nodeEvents->info(core.registry.node.data, &nodeInfo);

    pw_client_info clientInfo {};
    clientInfo.change_mask = PW_CLIENT_CHANGE_MASK_PROPS;
    core.registry.client.clientEvents->info(core.registry.client.data, &clientInfo);
    clientInfo.change_mask = 0;
    clientInfo.props = &dict;
    core.registry.client.clientEvents->info(core.registry.client.data, &clientInfo);

    QCOMPARE(output.changes, 0);
    QVERIFY(output.graph->streams().empty());
}

void TestAudioFakes::aGraphKnowsAStreamOnceItsNodeReportsProperties()
{
    FakeCore core;
    wire(core);
    int changes = 0;
    std::vector<std::uint32_t> removed;
    StreamGraph graph(coreOf(core), {[&changes] { ++changes; }, [&removed](std::uint32_t id) { removed.push_back(id); }});
    std::vector<spa_dict_item> globalProps {{PW_KEY_MEDIA_CLASS, "Stream/Output/Audio"}, {PW_KEY_OBJECT_SERIAL, "5"}};
    spa_dict global = dictOf(globalProps);
    core.registry.events->global(core.registry.data, 7, 0, PW_TYPE_INTERFACE_Node, 0, &global);

    std::vector<spa_dict_item> nodeProps {{PW_KEY_APP_PROCESS_ID, "1234"}, {PW_KEY_APP_PROCESS_BINARY, "player"}};
    spa_dict nodeDict = dictOf(nodeProps);
    pw_node_info info {};
    info.change_mask = PW_NODE_CHANGE_MASK_PROPS;
    info.props = &nodeDict;
    core.registry.node.nodeEvents->info(core.registry.node.data, &info);

    QCOMPARE(changes, 1);
    const std::vector<PlayingStream> streams = graph.streams();
    QCOMPARE(streams.size(), std::size_t(1));
    QCOMPARE(streams.front().id, std::uint32_t(7));
    QCOMPARE(streams.front().serial, std::uint64_t(5));
    QCOMPARE(streams.front().identity.pid, qint64(1234));
    QCOMPARE(streams.front().identity.binary, QStringLiteral("player"));

    core.registry.events->global_remove(core.registry.data, 7);
    QCOMPARE(removed, std::vector<std::uint32_t> {7});
    QVERIFY(graph.streams().empty());
    QCOMPARE(g_proxiesDestroyed, 1);
}

void TestAudioFakes::aStreamThatCannotBeCreatedStaysInert()
{
    g_stream.failNew = true;
    {
        CaptureStream stream(nullptr, 9);
        stream.setTargets({});
        QVERIFY(g_stream.events == nullptr);
        QCOMPARE(g_stream.connected, 0);
    }
    QCOMPARE(g_stream.created, 1);
    QCOMPARE(g_stream.destroyed, 0);
}

void TestAudioFakes::aStreamThatCannotConnectWarns()
{
    g_stream.connectResult = -ENOMEM;
    QTest::ignoreMessage(QtWarningMsg, "could not start capturing a window's audio");
    {
        CaptureStream stream(nullptr, 9);
        QCOMPARE(g_stream.connected, 1);
    }
    QCOMPARE(g_stream.destroyed, 1);
}

void TestAudioFakes::processingWithoutABufferQueuesNothing()
{
    CaptureStream stream(nullptr, 9);
    QVERIFY(g_stream.events != nullptr);
    g_stream.nextBuffer = nullptr;
    g_stream.events->process(g_stream.data);
    QCOMPARE(g_stream.queued, 0);
}

void TestAudioFakes::processingABufferWithoutDataRecyclesIt()
{
    CaptureStream stream(nullptr, 9);
    spa_chunk chunk {};
    chunk.size = 16;
    spa_data block {};
    block.maxsize = 16;
    spa_buffer spaBuffer {};
    spaBuffer.n_datas = 1;
    spaBuffer.datas = &block;
    pw_buffer buffer {};
    buffer.buffer = &spaBuffer;
    g_stream.nextBuffer = &buffer;

    block.chunk = &chunk;
    block.data = nullptr;
    g_stream.events->process(g_stream.data);
    std::array<float, 4> samples {};
    block.data = samples.data();
    block.chunk = nullptr;
    g_stream.events->process(g_stream.data);
    QCOMPARE(g_stream.queued, 2);
}

void TestAudioFakes::processingABufferMixesItsSamples()
{
    auto mailbox = std::make_shared<FrameMailbox>();
    WindowMix mix(mailbox);
    {
        CaptureStream stream(nullptr, 9);
        stream.setTargets({&mix});
        std::vector<float> samples(8192);
        for (std::size_t index = 0; index < samples.size(); ++index) {
            samples[index]
                = 0.5F * static_cast<float>(std::sin(2.0 * std::numbers::pi * 440.0 * static_cast<double>(index) / kCaptureRate));
        }
        spa_chunk chunk {};
        chunk.size = static_cast<std::uint32_t>(samples.size() * sizeof(float));
        spa_data block {};
        block.maxsize = chunk.size;
        block.data = samples.data();
        block.chunk = &chunk;
        spa_buffer spaBuffer {};
        spaBuffer.n_datas = 1;
        spaBuffer.datas = &block;
        pw_buffer buffer {};
        buffer.buffer = &spaBuffer;
        g_stream.nextBuffer = &buffer;
        g_stream.events->process(g_stream.data);
        QCOMPARE(g_stream.queued, 1);
        QVERIFY(mailbox->latest().level > 0.0F);
        stream.setTargets({});
    }
}

QTEST_GUILESS_MAIN(TestAudioFakes)
#include "test_audiofakes.moc"
