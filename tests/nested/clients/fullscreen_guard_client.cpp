#include "xdg-shell-client-protocol.h"

#include <poll.h>
#include <sys/mman.h>
#include <unistd.h>
#include <wayland-client.h>

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <cstring>

namespace
{

enum class Request
{
    Unfullscreen,
    Minimize
};

struct Client
{
    wl_display *display = nullptr;
    wl_compositor *compositor = nullptr;
    wl_shm *shm = nullptr;
    xdg_wm_base *shell = nullptr;
    wl_surface *surface = nullptr;
    xdg_surface *xdgSurface = nullptr;
    xdg_toplevel *toplevel = nullptr;
    wl_buffer *buffer = nullptr;
    Request request = Request::Unfullscreen;
    bool mapped = false;
    bool active = false;
    bool wasActive = false;
    int activations = 0;
    int focusPhase = 0;
    bool fullscreen = false;
    bool closed = false;
    bool windowedMinimizeSent = false;
    int phaseRequests = 0;
};

void registryGlobal(void *data, wl_registry *registry, uint32_t name, const char *interface, uint32_t version)
{
    auto *client = static_cast<Client *>(data);
    if (std::strcmp(interface, wl_compositor_interface.name) == 0) {
        client->compositor
            = static_cast<wl_compositor *>(wl_registry_bind(registry, name, &wl_compositor_interface, std::min(version, 4u)));
    } else if (std::strcmp(interface, wl_shm_interface.name) == 0) {
        client->shm = static_cast<wl_shm *>(wl_registry_bind(registry, name, &wl_shm_interface, 1));
    } else if (std::strcmp(interface, xdg_wm_base_interface.name) == 0) {
        client->shell = static_cast<xdg_wm_base *>(wl_registry_bind(registry, name, &xdg_wm_base_interface, std::min(version, 6u)));
    }
}

void registryRemove(void *, wl_registry *, uint32_t) { }

constexpr wl_registry_listener registryListener {registryGlobal, registryRemove};

void shellPing(void *, xdg_wm_base *shell, uint32_t serial)
{
    xdg_wm_base_pong(shell, serial);
}

constexpr xdg_wm_base_listener shellListener {shellPing};

wl_buffer *makeBuffer(Client *client)
{
    constexpr int width = 600;
    constexpr int height = 400;
    constexpr int stride = width * 4;
    constexpr int size = stride * height;
    const int descriptor = memfd_create("konveyor-fullscreen-guard", MFD_CLOEXEC);
    if (descriptor < 0 || ftruncate(descriptor, size) < 0) {
        return nullptr;
    }
    void *mapping = mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_SHARED, descriptor, 0);
    if (mapping == MAP_FAILED) {
        close(descriptor);
        return nullptr;
    }
    std::fill_n(static_cast<uint32_t *>(mapping), width * height, 0xffb03030u);
    munmap(mapping, size);
    wl_shm_pool *pool = wl_shm_create_pool(client->shm, descriptor, size);
    wl_buffer *buffer = wl_shm_pool_create_buffer(pool, 0, width, height, stride, WL_SHM_FORMAT_XRGB8888);
    wl_shm_pool_destroy(pool);
    close(descriptor);
    return buffer;
}

void requestState(Client *client)
{
    if (client->request == Request::Unfullscreen) {
        xdg_toplevel_unset_fullscreen(client->toplevel);
    } else {
        xdg_toplevel_set_minimized(client->toplevel);
    }
    wl_surface_commit(client->surface);
    ++client->phaseRequests;
}

void surfaceConfigure(void *data, xdg_surface *surface, uint32_t serial)
{
    auto *client = static_cast<Client *>(data);
    xdg_surface_ack_configure(surface, serial);
    if (!client->mapped) {
        client->buffer = makeBuffer(client);
        wl_surface_attach(client->surface, client->buffer, 0, 0);
        wl_surface_damage_buffer(client->surface, 0, 0, 600, 400);
        xdg_toplevel_set_fullscreen(client->toplevel, nullptr);
        client->mapped = true;
    }
    wl_surface_commit(client->surface);
    if (client->focusPhase == 2 && client->request == Request::Unfullscreen && !client->fullscreen && !client->windowedMinimizeSent) {
        xdg_toplevel_set_minimized(client->toplevel);
        wl_surface_commit(client->surface);
        client->windowedMinimizeSent = true;
    }
}

constexpr xdg_surface_listener surfaceListener {surfaceConfigure};

void toplevelConfigure(void *data, xdg_toplevel *, int32_t, int32_t, wl_array *states)
{
    auto *client = static_cast<Client *>(data);
    client->fullscreen = false;
    client->active = false;
    const auto *state = static_cast<const uint32_t *>(states->data);
    const auto *end = state + states->size / sizeof(uint32_t);
    for (; state != end; ++state) {
        if (*state == XDG_TOPLEVEL_STATE_FULLSCREEN) {
            client->fullscreen = true;
        } else if (*state == XDG_TOPLEVEL_STATE_ACTIVATED) {
            client->active = true;
        }
    }
    if (!client->wasActive && client->active) {
        ++client->activations;
        if (client->focusPhase == 1) {
            client->focusPhase = 2;
            client->phaseRequests = 0;
        }
    }
    if (client->wasActive && !client->active && client->focusPhase == 0 && client->activations >= 2) {
        client->focusPhase = 1;
        client->phaseRequests = 0;
    }
    client->wasActive = client->active;
}

void toplevelClose(void *data, xdg_toplevel *)
{
    static_cast<Client *>(data)->closed = true;
}

void toplevelBounds(void *, xdg_toplevel *, int32_t, int32_t) { }

void toplevelCapabilities(void *, xdg_toplevel *, wl_array *) { }

constexpr xdg_toplevel_listener toplevelListener {toplevelConfigure, toplevelClose, toplevelBounds, toplevelCapabilities};

bool dispatch(Client *client)
{
    while (wl_display_prepare_read(client->display) != 0) {
        if (wl_display_dispatch_pending(client->display) < 0) {
            return false;
        }
    }
    wl_display_flush(client->display);
    pollfd descriptor {wl_display_get_fd(client->display), POLLIN, 0};
    const int timeout = client->focusPhase > 0 && client->phaseRequests < 1 ? 2 : 1000;
    const int ready = poll(&descriptor, 1, timeout);
    if (ready > 0) {
        if (wl_display_read_events(client->display) < 0) {
            return false;
        }
    } else {
        wl_display_cancel_read(client->display);
    }
    if (wl_display_dispatch_pending(client->display) < 0) {
        return false;
    }
    if (client->focusPhase > 0 && client->phaseRequests < 1) {
        requestState(client);
    }
    return true;
}

}

int main(int argc, char **argv)
{
    if (argc != 3) {
        return 2;
    }
    Client client;
    client.request = std::strcmp(argv[2], "minimize") == 0 ? Request::Minimize : Request::Unfullscreen;
    client.display = wl_display_connect(nullptr);
    if (!client.display) {
        return 3;
    }
    wl_registry *registry = wl_display_get_registry(client.display);
    wl_registry_add_listener(registry, &registryListener, &client);
    wl_display_roundtrip(client.display);
    if (!client.compositor || !client.shm || !client.shell) {
        return 4;
    }
    xdg_wm_base_add_listener(client.shell, &shellListener, &client);
    client.surface = wl_compositor_create_surface(client.compositor);
    client.xdgSurface = xdg_wm_base_get_xdg_surface(client.shell, client.surface);
    xdg_surface_add_listener(client.xdgSurface, &surfaceListener, &client);
    client.toplevel = xdg_surface_get_toplevel(client.xdgSurface);
    xdg_toplevel_add_listener(client.toplevel, &toplevelListener, &client);
    xdg_toplevel_set_title(client.toplevel, argv[1]);
    xdg_toplevel_set_app_id(client.toplevel, "konveyor-fullscreen-guard-test");
    wl_surface_commit(client.surface);
    while (!client.closed && dispatch(&client)) { }
    return 0;
}
