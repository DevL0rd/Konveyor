#include "fake-input-client-protocol.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <wayland-client.h>

static struct org_kde_kwin_fake_input *fake;

static void global(void *data, struct wl_registry *registry, uint32_t name, const char *interface, uint32_t version)
{
    (void)data;
    if (strcmp(interface, org_kde_kwin_fake_input_interface.name) == 0) {
        fake = wl_registry_bind(registry, name, &org_kde_kwin_fake_input_interface, version < 4 ? version : 4);
    }
}

static void global_remove(void *data, struct wl_registry *registry, uint32_t name)
{
    (void)data;
    (void)registry;
    (void)name;
}

static const struct wl_registry_listener listener = {global, global_remove};

int main(int argc, char **argv)
{
    if (argc < 4) {
        fprintf(stderr, "usage: fakepointer move|click X Y\n");
        return 2;
    }
    struct wl_display *display = wl_display_connect(NULL);
    if (!display) {
        fprintf(stderr, "no wayland display\n");
        return 1;
    }
    struct wl_registry *registry = wl_display_get_registry(display);
    wl_registry_add_listener(registry, &listener, NULL);
    wl_display_roundtrip(display);
    if (!fake) {
        fprintf(stderr, "org_kde_kwin_fake_input is not available\n");
        return 1;
    }
    org_kde_kwin_fake_input_authenticate(fake, "konveyor-tests", "nested input");
    org_kde_kwin_fake_input_pointer_motion_absolute(fake, wl_fixed_from_double(atof(argv[2])), wl_fixed_from_double(atof(argv[3])));
    wl_display_roundtrip(display);
    if (strcmp(argv[1], "click") == 0) {
        const uint32_t leftButton = 0x110;
        const useconds_t settle = 50000;
        usleep(settle);
        org_kde_kwin_fake_input_button(fake, leftButton, 1);
        wl_display_roundtrip(display);
        usleep(settle);
        org_kde_kwin_fake_input_button(fake, leftButton, 0);
        wl_display_roundtrip(display);
        usleep(settle);
    }
    wl_display_disconnect(display);
    return 0;
}
