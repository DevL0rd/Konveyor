#include "xdgclient.h"

#include <poll.h>

#include <cstdio>
#include <cstdlib>

namespace
{

enum class Request
{
    Unfullscreen,
    Minimize,
    Protocol
};

struct Client
{
    wl_display *display = nullptr;
    XdgGlobals globals;
    wl_surface *surface = nullptr;
    xdg_surface *xdgSurface = nullptr;
    xdg_toplevel *toplevel = nullptr;
    wl_buffer *buffer = nullptr;
    wl_output *fullscreenOutput = nullptr;
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
    int width = 600;
    int height = 400;
    int bufferWidth = 0;
    int bufferHeight = 0;
};

void bindOutput(void *data, wl_registry *registry, uint32_t name, const char *interface, uint32_t)
{
    auto *client = static_cast<Client *>(data);
    if (std::strcmp(interface, wl_output_interface.name) == 0) {
        if (client->fullscreenOutput) {
            wl_output_destroy(client->fullscreenOutput);
        }
        client->fullscreenOutput = static_cast<wl_output *>(wl_registry_bind(registry, name, &wl_output_interface, 1));
    }
}

constexpr wl_registry_listener outputListener {bindOutput, ignoreGlobalRemoval};

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
    if (client->request == Request::Protocol) {
        std::printf("configure:%u:%s:%dx%d\n", serial, client->fullscreen ? "fullscreen" : "normal", client->width, client->height);
        std::fflush(stdout);
        const int width = client->width > 0 ? client->width : 600;
        const int height = client->height > 0 ? client->height : 400;
        if (width != client->bufferWidth || height != client->bufferHeight) {
            if (client->buffer) {
                wl_buffer_destroy(client->buffer);
            }
            client->buffer = makeSolidBuffer(client->globals.shm, width, height, 0xffb03030u);
            client->bufferWidth = width;
            client->bufferHeight = height;
        }
        wl_surface_attach(client->surface, client->buffer, 0, 0);
        wl_surface_damage_buffer(client->surface, 0, 0, width, height);
        client->mapped = true;
    } else if (!client->mapped) {
        client->buffer = makeSolidBuffer(client->globals.shm, 600, 400, 0xffb03030u);
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

void toplevelConfigure(void *data, xdg_toplevel *, int32_t width, int32_t height, wl_array *states)
{
    auto *client = static_cast<Client *>(data);
    client->width = width;
    client->height = height;
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

void requestProtocolState(Client *client)
{
    char command[32];
    if (!std::fgets(command, sizeof(command), stdin)) {
        client->closed = true;
        return;
    }
    if (std::strcmp(command, "fullscreen\n") == 0) {
        xdg_toplevel_set_fullscreen(client->toplevel, nullptr);
    } else if (std::strcmp(command, "fullscreen-output\n") == 0) {
        xdg_toplevel_set_fullscreen(client->toplevel, client->fullscreenOutput);
    } else if (std::strcmp(command, "unset\n") == 0) {
        xdg_toplevel_unset_fullscreen(client->toplevel);
    } else if (std::strcmp(command, "minimize\n") == 0) {
        xdg_toplevel_set_minimized(client->toplevel);
    }
    wl_surface_commit(client->surface);
    wl_display_flush(client->display);
}

bool dispatch(Client *client)
{
    while (wl_display_prepare_read(client->display) != 0) {
        if (wl_display_dispatch_pending(client->display) < 0) {
            return false;
        }
    }
    wl_display_flush(client->display);
    pollfd descriptors[] {{wl_display_get_fd(client->display), POLLIN, 0}, {STDIN_FILENO, POLLIN, 0}};
    const bool awaitingRequest = client->request != Request::Protocol && client->focusPhase > 0 && client->phaseRequests < 1;
    const int timeout = awaitingRequest ? 2 : 1000;
    const int ready = poll(descriptors, client->request == Request::Protocol ? 2 : 1, timeout);
    if (ready > 0 && (descriptors[0].revents & POLLIN)) {
        if (wl_display_read_events(client->display) < 0) {
            return false;
        }
    } else {
        wl_display_cancel_read(client->display);
    }
    if (wl_display_dispatch_pending(client->display) < 0) {
        return false;
    }
    if (client->request == Request::Protocol && (descriptors[1].revents & POLLIN)) {
        requestProtocolState(client);
    }
    if (awaitingRequest) {
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
    if (std::strcmp(argv[2], "protocol") == 0) {
        client.request = Request::Protocol;
    } else if (std::strcmp(argv[2], "minimize") == 0) {
        client.request = Request::Minimize;
    }
    client.display = wl_display_connect(nullptr);
    if (!client.display) {
        return 3;
    }
    if (!bindGlobals(client.display, client.globals)) {
        return 4;
    }
    if (client.request == Request::Protocol) {
        wl_registry *registry = wl_display_get_registry(client.display);
        wl_registry_add_listener(registry, &outputListener, &client);
        wl_display_roundtrip(client.display);
    }
    client.surface = wl_compositor_create_surface(client.globals.compositor);
    client.xdgSurface = xdg_wm_base_get_xdg_surface(client.globals.shell, client.surface);
    xdg_surface_add_listener(client.xdgSurface, &surfaceListener, &client);
    client.toplevel = xdg_surface_get_toplevel(client.xdgSurface);
    xdg_toplevel_add_listener(client.toplevel, &toplevelListener, &client);
    xdg_toplevel_set_title(client.toplevel, argv[1]);
    xdg_toplevel_set_app_id(client.toplevel, "konveyor-fullscreen-guard-test");
    wl_surface_commit(client.surface);
    while (!client.closed && dispatch(&client)) { }
    return 0;
}
