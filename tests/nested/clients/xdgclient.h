#pragma once

#include "xdg-shell-client-protocol.h"

#include <sys/mman.h>
#include <unistd.h>
#include <wayland-client.h>

#include <algorithm>
#include <cstdint>
#include <cstring>

struct XdgGlobals
{
    wl_compositor *compositor = nullptr;
    wl_shm *shm = nullptr;
    xdg_wm_base *shell = nullptr;
    wl_seat *seat = nullptr;
    wl_data_device_manager *dataDevices = nullptr;
};

inline void bindGlobal(void *data, wl_registry *registry, uint32_t name, const char *interface, uint32_t version)
{
    auto *globals = static_cast<XdgGlobals *>(data);
    if (std::strcmp(interface, wl_compositor_interface.name) == 0) {
        globals->compositor
            = static_cast<wl_compositor *>(wl_registry_bind(registry, name, &wl_compositor_interface, std::min(version, 4u)));
    } else if (std::strcmp(interface, wl_shm_interface.name) == 0) {
        globals->shm = static_cast<wl_shm *>(wl_registry_bind(registry, name, &wl_shm_interface, 1));
    } else if (std::strcmp(interface, xdg_wm_base_interface.name) == 0) {
        globals->shell = static_cast<xdg_wm_base *>(wl_registry_bind(registry, name, &xdg_wm_base_interface, std::min(version, 6u)));
    } else if (std::strcmp(interface, wl_seat_interface.name) == 0) {
        globals->seat = static_cast<wl_seat *>(wl_registry_bind(registry, name, &wl_seat_interface, std::min(version, 5u)));
    } else if (std::strcmp(interface, wl_data_device_manager_interface.name) == 0) {
        globals->dataDevices = static_cast<wl_data_device_manager *>(
            wl_registry_bind(registry, name, &wl_data_device_manager_interface, std::min(version, 3u)));
    }
}

inline void ignoreGlobalRemoval(void *, wl_registry *, uint32_t) { }

inline constexpr wl_registry_listener globalsListener {bindGlobal, ignoreGlobalRemoval};

inline void answerPing(void *, xdg_wm_base *shell, uint32_t serial)
{
    xdg_wm_base_pong(shell, serial);
}

inline constexpr xdg_wm_base_listener pingListener {answerPing};

inline bool bindGlobals(wl_display *display, XdgGlobals &globals)
{
    wl_registry *registry = wl_display_get_registry(display);
    wl_registry_add_listener(registry, &globalsListener, &globals);
    wl_display_roundtrip(display);
    if (!globals.compositor || !globals.shm || !globals.shell) {
        return false;
    }
    xdg_wm_base_add_listener(globals.shell, &pingListener, nullptr);
    return true;
}

inline wl_buffer *makeSolidBuffer(wl_shm *shm, int width, int height, uint32_t color)
{
    const int stride = width * 4;
    const int size = stride * height;
    const int descriptor = memfd_create("konveyor-test-buffer", MFD_CLOEXEC);
    if (descriptor < 0 || ftruncate(descriptor, size) < 0) {
        return nullptr;
    }
    void *mapping = mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_SHARED, descriptor, 0);
    if (mapping == MAP_FAILED) {
        close(descriptor);
        return nullptr;
    }
    std::fill_n(static_cast<uint32_t *>(mapping), width * height, color);
    munmap(mapping, size);
    wl_shm_pool *pool = wl_shm_create_pool(shm, descriptor, size);
    wl_buffer *buffer = wl_shm_pool_create_buffer(pool, 0, width, height, stride, WL_SHM_FORMAT_XRGB8888);
    wl_shm_pool_destroy(pool);
    close(descriptor);
    return buffer;
}
