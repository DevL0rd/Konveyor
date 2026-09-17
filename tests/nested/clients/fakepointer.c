#include "fake-input-client-protocol.h"

#include <math.h>
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

static const useconds_t frameDelay = 16000;

static void place_fingers(int fingers, double cx, double cy, double radius, double *xs, double *ys)
{
    for (int i = 0; i < fingers; ++i) {
        const double angle = 2.0 * M_PI * i / fingers;
        xs[i] = cx + radius * cos(angle);
        ys[i] = cy + radius * sin(angle);
    }
}

static void touch_frame(struct wl_display *display)
{
    org_kde_kwin_fake_input_touch_frame(fake);
    wl_display_roundtrip(display);
    usleep(frameDelay);
}

static void touch_gesture(struct wl_display *display, int fingers, double x, double y, double dx, double dy, double r0, double r1, int holdMs, int steps)
{
    double xs[10];
    double ys[10];
    place_fingers(fingers, x, y, r0, xs, ys);
    for (int i = 0; i < fingers; ++i) {
        org_kde_kwin_fake_input_touch_down(fake, i, wl_fixed_from_double(xs[i]), wl_fixed_from_double(ys[i]));
    }
    touch_frame(display);
    usleep((useconds_t)holdMs * 1000);
    for (int step = 1; step <= steps; ++step) {
        const double t = (double)step / steps;
        place_fingers(fingers, x + dx * t, y + dy * t, r0 + (r1 - r0) * t, xs, ys);
        for (int i = 0; i < fingers; ++i) {
            org_kde_kwin_fake_input_touch_motion(fake, i, wl_fixed_from_double(xs[i]), wl_fixed_from_double(ys[i]));
        }
        touch_frame(display);
    }
    for (int i = 0; i < fingers; ++i) {
        org_kde_kwin_fake_input_touch_up(fake, i);
    }
    touch_frame(display);
}

int main(int argc, char **argv)
{
    if (argc < 4) {
        fprintf(stderr, "usage: fakepointer move|click X Y | touch FINGERS X Y DX DY R0 R1 HOLD_MS STEPS\n");
        return 2;
    }
    const int touch = strcmp(argv[1], "touch") == 0;
    if (touch && argc < 11) {
        fprintf(stderr, "usage: fakepointer touch FINGERS X Y DX DY R0 R1 HOLD_MS STEPS\n");
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
    if (touch) {
        const int fingers = atoi(argv[2]);
        if (fingers < 1 || fingers > 10) {
            fprintf(stderr, "fakepointer: 1 to 10 fingers\n");
            return 2;
        }
        touch_gesture(display, fingers, atof(argv[3]), atof(argv[4]), atof(argv[5]), atof(argv[6]), atof(argv[7]), atof(argv[8]), atoi(argv[9]), atoi(argv[10]));
        wl_display_disconnect(display);
        return 0;
    }
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
