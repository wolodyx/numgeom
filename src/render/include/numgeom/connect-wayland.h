#ifndef numgeom_render_connect_wayland_h
#define numgeom_render_connect_wayland_h

#include <vulkan/vulkan.h>

#include <wayland-client.h>


struct wl_connection
{
    struct wl_display *display;
    struct wl_compositor *compositor;
    struct xdg_wm_base *shell;
    struct wl_keyboard *keyboard;
    struct wl_seat *seat;
    struct wl_surface *surface;
    struct xdg_surface *xdg_surface;
    struct xdg_toplevel *xdg_toplevel;
    bool wait_for_configure;
};


const char* get_wayland_extension_name();


bool init_wayland_connection(wl_connection*);


VkSurfaceKHR create_wayland_surface(
    VkInstance instance,
    VkPhysicalDevice physicalDevice,
    wl_connection*
);

#endif // !numgeom_render_connect_wayland_h
