#include "xdgclient.h"

#include "xdg-toplevel-tag-v1-client-protocol.h"

namespace
{

struct Overlay
{
    wl_display *display = nullptr;
    XdgGlobals globals;
    xdg_toplevel_tag_manager_v1 *tags = nullptr;
    wl_surface *surface = nullptr;
    wl_buffer *buffer = nullptr;
    int width = 600;
    int height = 400;
    bool closed = false;
};

void bindTagManager(void *data, wl_registry *registry, uint32_t name, const char *interface, uint32_t)
{
    if (std::strcmp(interface, xdg_toplevel_tag_manager_v1_interface.name) == 0) {
        static_cast<Overlay *>(data)->tags
            = static_cast<xdg_toplevel_tag_manager_v1 *>(wl_registry_bind(registry, name, &xdg_toplevel_tag_manager_v1_interface, 1));
    }
}

constexpr wl_registry_listener tagListener {bindTagManager, ignoreGlobalRemoval};

void surfaceConfigure(void *data, xdg_surface *surface, uint32_t serial)
{
    auto *overlay = static_cast<Overlay *>(data);
    xdg_surface_ack_configure(surface, serial);
    if (overlay->buffer) {
        wl_buffer_destroy(overlay->buffer);
    }
    overlay->buffer = makeSolidBuffer(overlay->globals.shm, overlay->width, overlay->height, 0x80204060u);
    wl_surface_attach(overlay->surface, overlay->buffer, 0, 0);
    wl_surface_damage_buffer(overlay->surface, 0, 0, overlay->width, overlay->height);
    wl_surface_commit(overlay->surface);
}

constexpr xdg_surface_listener surfaceListener {surfaceConfigure};

void toplevelConfigure(void *data, xdg_toplevel *, int32_t width, int32_t height, wl_array *)
{
    auto *overlay = static_cast<Overlay *>(data);
    if (width > 0 && height > 0) {
        overlay->width = width;
        overlay->height = height;
    }
}

void toplevelClose(void *data, xdg_toplevel *)
{
    static_cast<Overlay *>(data)->closed = true;
}

void ignoreBounds(void *, xdg_toplevel *, int32_t, int32_t) { }

void ignoreCapabilities(void *, xdg_toplevel *, wl_array *) { }

constexpr xdg_toplevel_listener toplevelListener {toplevelConfigure, toplevelClose, ignoreBounds, ignoreCapabilities};

}

int main(int argc, char **argv)
{
    if (argc != 3) {
        return 2;
    }
    Overlay overlay;
    overlay.display = wl_display_connect(nullptr);
    if (!overlay.display || !bindGlobals(overlay.display, overlay.globals)) {
        return 3;
    }
    wl_registry_add_listener(wl_display_get_registry(overlay.display), &tagListener, &overlay);
    wl_display_roundtrip(overlay.display);
    if (!overlay.tags) {
        return 4;
    }
    overlay.surface = wl_compositor_create_surface(overlay.globals.compositor);
    xdg_surface *xdgSurface = xdg_wm_base_get_xdg_surface(overlay.globals.shell, overlay.surface);
    xdg_surface_add_listener(xdgSurface, &surfaceListener, &overlay);
    xdg_toplevel *toplevel = xdg_surface_get_toplevel(xdgSurface);
    xdg_toplevel_add_listener(toplevel, &toplevelListener, &overlay);
    xdg_toplevel_set_title(toplevel, argv[1]);
    xdg_toplevel_set_app_id(toplevel, "konveyor-tagged-overlay-test");
    xdg_toplevel_tag_manager_v1_set_toplevel_tag(overlay.tags, toplevel, argv[2]);
    xdg_toplevel_set_fullscreen(toplevel, nullptr);
    wl_surface_commit(overlay.surface);
    while (!overlay.closed && wl_display_dispatch(overlay.display) >= 0) { }
    return 0;
}
