#ifndef NUMGEOM_APP_VKRENDER_H
#define NUMGEOM_APP_VKRENDER_H

#include <string>
#include <vector>

struct vkcube;
struct wl_connection;
struct xcb_connection;

void init_vulkan(
    vkcube* vc,
    const std::vector<const char*>& additionalExtensionNames = {}
);

void mainloop_wayland(vkcube*, wl_connection*);

void mainloop_xcb(vkcube*, xcb_connection*);

int init_xcb_display(vkcube*, xcb_connection*);

int init_wayland_display(vkcube*, wl_connection*);

#endif // !NUMGEOM_APP_VKRENDER_H
