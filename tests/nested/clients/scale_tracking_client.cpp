#include "fractional-scale-v1-client-protocol.h"
#include "xdgclient.h"

#include <cstdio>

namespace
{

struct Client
{
    wl_display *display = nullptr;
    XdgGlobals globals;
    wp_fractional_scale_manager_v1 *scaleManager = nullptr;
    wl_surface *surface = nullptr;
    xdg_surface *xdgSurface = nullptr;
    xdg_toplevel *toplevel = nullptr;
    wl_buffer *buffer = nullptr;
    bool mapped = false;
    bool closed = false;
    int width = 600;
    int height = 400;
};

void report(const char *event, uint32_t value)
{
    std::printf("%s %u\n", event, value);
    std::fflush(stdout);
}

void bindScaleManager(void *data, wl_registry *registry, uint32_t name, const char *interface, uint32_t)
{
    if (std::strcmp(interface, wp_fractional_scale_manager_v1_interface.name) == 0) {
        static_cast<Client *>(data)->scaleManager
            = static_cast<wp_fractional_scale_manager_v1 *>(wl_registry_bind(registry, name, &wp_fractional_scale_manager_v1_interface, 1));
    }
}

constexpr wl_registry_listener registryListener {bindScaleManager, ignoreGlobalRemoval};

void preferredScale(void *, wp_fractional_scale_v1 *, uint32_t scale)
{
    report("SCALE", scale);
}

constexpr wp_fractional_scale_v1_listener scaleListener {preferredScale};

void surfaceConfigure(void *data, xdg_surface *surface, uint32_t serial)
{
    auto *client = static_cast<Client *>(data);
    xdg_surface_ack_configure(surface, serial);
    report("CONFIGURE", serial);
    wl_buffer *previous = client->buffer;
    client->buffer = makeSolidBuffer(client->globals.shm, client->width, client->height, 0xff3050b0u);
    wl_surface_attach(client->surface, client->buffer, 0, 0);
    wl_surface_damage_buffer(client->surface, 0, 0, client->width, client->height);
    if (previous) {
        wl_buffer_destroy(previous);
    }
    wl_surface_commit(client->surface);
    if (!client->mapped) {
        client->mapped = true;
        report("MAPPED", serial);
    }
}

constexpr xdg_surface_listener xdgSurfaceListener {surfaceConfigure};

void toplevelConfigure(void *data, xdg_toplevel *, int32_t width, int32_t height, wl_array *)
{
    auto *client = static_cast<Client *>(data);
    client->width = width > 0 ? width : client->width;
    client->height = height > 0 ? height : client->height;
}

void markClosed(void *data, xdg_toplevel *)
{
    static_cast<Client *>(data)->closed = true;
}

constexpr xdg_toplevel_listener toplevelListener {toplevelConfigure, markClosed, ignoreToplevelBounds, ignoreToplevelCapabilities};

}

int main(int argc, char **argv)
{
    if (argc != 2) {
        return 2;
    }
    Client client;
    client.display = wl_display_connect(nullptr);
    if (!client.display || !bindGlobals(client.display, client.globals)) {
        return 3;
    }
    wl_registry_add_listener(wl_display_get_registry(client.display), &registryListener, &client);
    wl_display_roundtrip(client.display);
    if (!client.scaleManager) {
        return 4;
    }
    client.surface = wl_compositor_create_surface(client.globals.compositor);
    wp_fractional_scale_v1_add_listener(
        wp_fractional_scale_manager_v1_get_fractional_scale(client.scaleManager, client.surface), &scaleListener, nullptr);
    client.xdgSurface = xdg_wm_base_get_xdg_surface(client.globals.shell, client.surface);
    xdg_surface_add_listener(client.xdgSurface, &xdgSurfaceListener, &client);
    client.toplevel = xdg_surface_get_toplevel(client.xdgSurface);
    xdg_toplevel_add_listener(client.toplevel, &toplevelListener, &client);
    xdg_toplevel_set_title(client.toplevel, argv[1]);
    xdg_toplevel_set_app_id(client.toplevel, "konveyor-scale-tracking-test");
    wl_surface_commit(client.surface);
    while (!client.closed && wl_display_dispatch(client.display) != -1) { }
    return 0;
}
