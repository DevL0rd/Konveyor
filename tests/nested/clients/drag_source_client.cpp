#include "xdgclient.h"

#include <cstdio>

namespace
{

constexpr uint32_t leftButton = 0x110;

struct Source
{
    wl_display *display = nullptr;
    XdgGlobals globals;
    wl_surface *surface = nullptr;
    xdg_toplevel *toplevel = nullptr;
    wl_data_device *dataDevice = nullptr;
    wl_data_source *dragged = nullptr;
    wl_buffer *buffer = nullptr;
    int width = 500;
    int height = 400;
    bool closed = false;
};

void surfaceConfigure(void *data, xdg_surface *surface, uint32_t serial)
{
    auto *source = static_cast<Source *>(data);
    xdg_surface_ack_configure(surface, serial);
    if (source->buffer) {
        wl_buffer_destroy(source->buffer);
    }
    source->buffer = makeSolidBuffer(source->globals.shm, source->width, source->height, 0xff1b91d5u);
    wl_surface_attach(source->surface, source->buffer, 0, 0);
    wl_surface_damage_buffer(source->surface, 0, 0, source->width, source->height);
    wl_surface_commit(source->surface);
}

constexpr xdg_surface_listener surfaceListener {surfaceConfigure};

void toplevelConfigure(void *data, xdg_toplevel *, int32_t width, int32_t height, wl_array *)
{
    auto *source = static_cast<Source *>(data);
    if (width > 0 && height > 0) {
        source->width = width;
        source->height = height;
    }
}

void toplevelClose(void *data, xdg_toplevel *)
{
    static_cast<Source *>(data)->closed = true;
}

void ignoreBounds(void *, xdg_toplevel *, int32_t, int32_t) { }

void ignoreCapabilities(void *, xdg_toplevel *, wl_array *) { }

constexpr xdg_toplevel_listener toplevelListener {toplevelConfigure, toplevelClose, ignoreBounds, ignoreCapabilities};

void ignoreTarget(void *, wl_data_source *, const char *) { }

void ignoreSend(void *, wl_data_source *, const char *, int32_t descriptor)
{
    close(descriptor);
}

void finishDrag(void *data, wl_data_source *dataSource)
{
    auto *source = static_cast<Source *>(data);
    if (source->dragged == dataSource) {
        source->dragged = nullptr;
    }
    wl_data_source_destroy(dataSource);
    std::fprintf(stderr, "konveyor-test-drag-finished\n");
}

void ignoreDropPerformed(void *, wl_data_source *) { }

void ignoreAction(void *, wl_data_source *, uint32_t) { }

constexpr wl_data_source_listener dataSourceListener {ignoreTarget, ignoreSend, finishDrag, ignoreDropPerformed, finishDrag, ignoreAction};

void pointerButton(void *data, wl_pointer *, uint32_t serial, uint32_t, uint32_t button, uint32_t state)
{
    auto *source = static_cast<Source *>(data);
    if (button != leftButton || state != WL_POINTER_BUTTON_STATE_PRESSED || source->dragged || !source->dataDevice) {
        return;
    }
    source->dragged = wl_data_device_manager_create_data_source(source->globals.dataDevices);
    wl_data_source_add_listener(source->dragged, &dataSourceListener, source);
    wl_data_source_offer(source->dragged, "text/plain");
    wl_data_source_set_actions(source->dragged, WL_DATA_DEVICE_MANAGER_DND_ACTION_COPY);
    wl_data_device_start_drag(source->dataDevice, source->dragged, source->surface, nullptr, serial);
    std::fprintf(stderr, "konveyor-test-drag-started\n");
}

void ignoreEnter(void *, wl_pointer *, uint32_t, wl_surface *, wl_fixed_t, wl_fixed_t) { }
void ignoreLeave(void *, wl_pointer *, uint32_t, wl_surface *) { }
void ignoreMotion(void *, wl_pointer *, uint32_t, wl_fixed_t, wl_fixed_t) { }
void ignoreAxis(void *, wl_pointer *, uint32_t, uint32_t, wl_fixed_t) { }
void ignoreFrame(void *, wl_pointer *) { }
void ignoreAxisSource(void *, wl_pointer *, uint32_t) { }
void ignoreAxisStop(void *, wl_pointer *, uint32_t, uint32_t) { }
void ignoreAxisDiscrete(void *, wl_pointer *, uint32_t, int32_t) { }

const wl_pointer_listener *pointerListener()
{
    static const wl_pointer_listener listener = [] {
        wl_pointer_listener events {};
        events.enter = ignoreEnter;
        events.leave = ignoreLeave;
        events.motion = ignoreMotion;
        events.button = pointerButton;
        events.axis = ignoreAxis;
        events.frame = ignoreFrame;
        events.axis_source = ignoreAxisSource;
        events.axis_stop = ignoreAxisStop;
        events.axis_discrete = ignoreAxisDiscrete;
        return events;
    }();
    return &listener;
}

}

int main(int argc, char **argv)
{
    if (argc != 2) {
        return 2;
    }
    Source source;
    source.display = wl_display_connect(nullptr);
    if (!source.display || !bindGlobals(source.display, source.globals) || !source.globals.seat || !source.globals.dataDevices) {
        return 3;
    }
    wl_pointer_add_listener(wl_seat_get_pointer(source.globals.seat), pointerListener(), &source);
    source.dataDevice = wl_data_device_manager_get_data_device(source.globals.dataDevices, source.globals.seat);
    source.surface = wl_compositor_create_surface(source.globals.compositor);
    xdg_surface *xdgSurface = xdg_wm_base_get_xdg_surface(source.globals.shell, source.surface);
    xdg_surface_add_listener(xdgSurface, &surfaceListener, &source);
    source.toplevel = xdg_surface_get_toplevel(xdgSurface);
    xdg_toplevel_add_listener(source.toplevel, &toplevelListener, &source);
    xdg_toplevel_set_title(source.toplevel, argv[1]);
    xdg_toplevel_set_app_id(source.toplevel, "konveyor-drag-source-test");
    wl_surface_commit(source.surface);
    while (!source.closed && wl_display_dispatch(source.display) >= 0) { }
    return 0;
}
