#include "xdgclient.h"

#include <poll.h>
#include <unistd.h>

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <thread>

namespace
{

struct Client
{
    wl_display *display = nullptr;
    XdgGlobals globals;
    wl_surface *surface = nullptr;
    xdg_surface *xdgSurface = nullptr;
    xdg_toplevel *toplevel = nullptr;
    wl_buffer *buffer = nullptr;
    bool slow = false;
    bool mapped = false;
    bool closed = false;
    bool strictState = false;
    int width = 500;
    int height = 400;
    int pendingWidth = 0;
    int pendingHeight = 0;
};

void announce(const Client &client, const char *event, bool acknowledged)
{
    std::printf("%s pending=%dx%d size=%dx%d strict=%d acknowledged=%d\n", event, client.pendingWidth, client.pendingHeight, client.width,
        client.height, client.strictState ? 1 : 0, acknowledged ? 1 : 0);
    std::fflush(stdout);
}

void present(Client &client)
{
    wl_buffer *previous = client.buffer;
    client.buffer = makeSolidBuffer(client.globals.shm, client.width, client.height, 0xff30a050u);
    wl_surface_attach(client.surface, client.buffer, 0, 0);
    wl_surface_damage_buffer(client.surface, 0, 0, client.width, client.height);
    xdg_surface_set_window_geometry(client.xdgSurface, 0, 0, client.width, client.height);
    xdg_toplevel_set_min_size(client.toplevel, client.width, client.height);
    xdg_toplevel_set_max_size(client.toplevel, client.width, client.height);
    wl_surface_commit(client.surface);
    if (previous) {
        wl_buffer_destroy(previous);
    }
}

void surfaceConfigure(void *data, xdg_surface *surface, uint32_t serial)
{
    auto *client = static_cast<Client *>(data);
    if (client->slow) {
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }
    const bool obeys = !client->mapped || client->strictState;
    if (!obeys) {
        announce(*client, "IGNORED", false);
        wl_surface_commit(client->surface);
        return;
    }
    xdg_surface_ack_configure(surface, serial);
    if (client->pendingWidth > 0 && client->pendingHeight > 0 && client->mapped) {
        client->width = client->pendingWidth;
        client->height = client->pendingHeight;
    }
    present(*client);
    client->mapped = true;
    announce(*client, "OBEYED", true);
}

constexpr xdg_surface_listener xdgSurfaceListener {surfaceConfigure};

void toplevelConfigure(void *data, xdg_toplevel *, int32_t width, int32_t height, wl_array *states)
{
    auto *client = static_cast<Client *>(data);
    client->pendingWidth = width;
    client->pendingHeight = height;
    client->strictState = false;
    const auto *begin = static_cast<const uint32_t *>(states->data);
    for (const uint32_t *state = begin; state < begin + states->size / sizeof(uint32_t); ++state) {
        switch (*state) {
        case XDG_TOPLEVEL_STATE_MAXIMIZED:
        case XDG_TOPLEVEL_STATE_FULLSCREEN:
        case XDG_TOPLEVEL_STATE_RESIZING:
        case XDG_TOPLEVEL_STATE_TILED_LEFT:
        case XDG_TOPLEVEL_STATE_TILED_RIGHT:
        case XDG_TOPLEVEL_STATE_TILED_TOP:
        case XDG_TOPLEVEL_STATE_TILED_BOTTOM:
            client->strictState = true;
            break;
        default:
            break;
        }
    }
}

void markClosed(void *data, xdg_toplevel *)
{
    static_cast<Client *>(data)->closed = true;
}

constexpr xdg_toplevel_listener toplevelListener {toplevelConfigure, markClosed, ignoreToplevelBounds, ignoreToplevelCapabilities};

void runCommand(Client &client, const std::string &line)
{
    int width = 0;
    int height = 0;
    if (std::sscanf(line.c_str(), "%*[a-z] %dx%d", &width, &height) == 2) {
        client.width = width;
        client.height = height;
        present(client);
        announce(client, "CHOSE", false);
    }
}

void serve(Client &client)
{
    std::string pending;
    pollfd descriptors[2] = {{wl_display_get_fd(client.display), POLLIN, 0}, {STDIN_FILENO, POLLIN, 0}};
    while (!client.closed) {
        while (wl_display_prepare_read(client.display) != 0) {
            wl_display_dispatch_pending(client.display);
        }
        wl_display_flush(client.display);
        poll(descriptors, 2, -1);
        if (!(descriptors[0].revents & POLLIN)) {
            wl_display_cancel_read(client.display);
        } else if (wl_display_read_events(client.display) == -1) {
            return;
        }
        wl_display_dispatch_pending(client.display);
        char buffer[256];
        const ssize_t count = (descriptors[1].revents & POLLIN) ? read(STDIN_FILENO, buffer, sizeof buffer) : 0;
        for (ssize_t index = 0; index < count; ++index) {
            if (buffer[index] != '\n') {
                pending += buffer[index];
                continue;
            }
            runCommand(client, pending);
            pending.clear();
        }
        if (count == 0 && (descriptors[1].revents & POLLHUP)) {
            descriptors[1].fd = -1;
        }
    }
}

}

int main(int argc, char **argv)
{
    if (argc < 3) {
        return 2;
    }
    Client client;
    client.slow = argc > 3 && std::string(argv[3]) == "slow";
    if (argc > 4) {
        std::sscanf(argv[4], "%dx%d", &client.width, &client.height);
    }
    client.display = wl_display_connect(nullptr);
    if (!client.display || !bindGlobals(client.display, client.globals)) {
        return 3;
    }
    client.surface = wl_compositor_create_surface(client.globals.compositor);
    client.xdgSurface = xdg_wm_base_get_xdg_surface(client.globals.shell, client.surface);
    xdg_surface_add_listener(client.xdgSurface, &xdgSurfaceListener, &client);
    client.toplevel = xdg_surface_get_toplevel(client.xdgSurface);
    xdg_toplevel_add_listener(client.toplevel, &toplevelListener, &client);
    xdg_toplevel_set_app_id(client.toplevel, argv[2]);
    xdg_toplevel_set_title(client.toplevel, argv[1]);
    wl_surface_commit(client.surface);
    serve(client);
    return 0;
}
