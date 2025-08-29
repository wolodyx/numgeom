#include "numgeom/connect-wayland.h"

#include <cstdlib>
#include <stdexcept>

#include <string.h>

#include <vulkan/vulkan_wayland.h>

#include <linux/input.h>

#include "xdg-shell-client-protocol.h"


const char* get_wayland_extension_name()
{
    return VK_KHR_WAYLAND_SURFACE_EXTENSION_NAME;
}

static void handle_xdg_surface_configure(
    void* data,
    xdg_surface* surface,
    uint32_t serial)
{
    auto w = reinterpret_cast<wl_connection*>(data);

    xdg_surface_ack_configure(surface, serial);

    if(w->wait_for_configure) {
        // redraw
        w->wait_for_configure = false;
    }
}


static const xdg_surface_listener xdg_surface_listener = {
    handle_xdg_surface_configure,
};


static void handle_xdg_toplevel_configure(
    void *data,
    xdg_toplevel *toplevel,
    int32_t width,
    int32_t height,
    wl_array *states)
{
}


static void handle_xdg_toplevel_close(
    void *data,
    xdg_toplevel *toplevel)
{
}


static const xdg_toplevel_listener xdg_toplevel_listener = {
    handle_xdg_toplevel_configure,
    handle_xdg_toplevel_close,
};


static void handle_xdg_wm_base_ping(void *data, xdg_wm_base *shell, uint32_t serial)
{
    xdg_wm_base_pong(shell, serial);
}


static const xdg_wm_base_listener xdg_wm_base_listener = {
    handle_xdg_wm_base_ping,
};


static void handle_wl_keyboard_keymap(
    void *data,
    wl_keyboard *wl_keyboard,
    uint32_t format,
    int32_t fd,
    uint32_t size)
{
}


static void handle_wl_keyboard_enter(
    void *data,
    wl_keyboard *wl_keyboard,
    uint32_t serial,
    wl_surface *surface,
    wl_array *keys)
{
}


static void handle_wl_keyboard_leave(
    void *data,
    wl_keyboard *wl_keyboard,
    uint32_t serial,
    wl_surface *surface)
{
}


static void handle_wl_keyboard_key(
    void *data,
    wl_keyboard *wl_keyboard,
    uint32_t serial,
    uint32_t time,
    uint32_t key,
    uint32_t state)
{
    if(key == KEY_ESC && state == WL_KEYBOARD_KEY_STATE_PRESSED)
        std::exit(0);
}


static void handle_wl_keyboard_modifiers(
    void *data,
    wl_keyboard *wl_keyboard,
    uint32_t serial,
    uint32_t mods_depressed,
    uint32_t mods_latched,
    uint32_t mods_locked,
    uint32_t group)
{
}


static void handle_wl_keyboard_repeat_info(
    void *data,
    wl_keyboard *wl_keyboard,
    int32_t rate,
    int32_t delay)
{
}


static const wl_keyboard_listener wl_keyboard_listener = {
    .keymap = handle_wl_keyboard_keymap,
    .enter = handle_wl_keyboard_enter,
    .leave = handle_wl_keyboard_leave,
    .key = handle_wl_keyboard_key,
    .modifiers = handle_wl_keyboard_modifiers,
    .repeat_info = handle_wl_keyboard_repeat_info,
};


static void handle_wl_seat_capabilities(
    void* data,
    wl_seat* wl_seat,
    uint32_t capabilities)
{
    wl_connection* wl = reinterpret_cast<wl_connection*>(data);

    if ((capabilities & WL_SEAT_CAPABILITY_KEYBOARD) && (!wl->keyboard)) {
        wl->keyboard = wl_seat_get_keyboard(wl_seat);
        wl_keyboard_add_listener(wl->keyboard, &wl_keyboard_listener, wl);
    } else if (!(capabilities & WL_SEAT_CAPABILITY_KEYBOARD) && wl->keyboard) {
        wl_keyboard_destroy(wl->keyboard);
        wl->keyboard = nullptr;
    }
}


static const wl_seat_listener wl_seat_listener = {
    handle_wl_seat_capabilities,
};


static void registry_handle_global(
    void *data,
    wl_registry *registry,
    uint32_t name,
    const char *interface,
    uint32_t version)
{
    wl_connection* wl = reinterpret_cast<wl_connection*>(data);

    if (strcmp(interface, "wl_compositor") == 0) {
        wl->compositor = (wl_compositor*)wl_registry_bind(registry, name,
                                           &wl_compositor_interface, 1);
    } else if (strcmp(interface, "xdg_wm_base") == 0) {
        wl->shell = (xdg_wm_base*)wl_registry_bind(registry, name, &xdg_wm_base_interface, 1);
        xdg_wm_base_add_listener(wl->shell, &xdg_wm_base_listener, wl);
    } else if (strcmp(interface, "wl_seat") == 0) {
        wl->seat = (wl_seat*)wl_registry_bind(registry, name, &wl_seat_interface, 1);
        wl_seat_add_listener(wl->seat, &wl_seat_listener, wl);
    }
}


static void registry_handle_global_remove(
    void *data,
    wl_registry *registry,
    uint32_t name)
{
}


static const wl_registry_listener registry_listener = {
    registry_handle_global,
    registry_handle_global_remove
};


bool init_wayland_connection(wl_connection* wl)
{
    wl->surface = nullptr;

    wl->display = wl_display_connect(nullptr);
    if (!wl->display)
        return -1;

    wl->seat = nullptr;
    wl->keyboard = nullptr;
    wl->shell = nullptr;

    wl_registry *registry = wl_display_get_registry(wl->display);
    wl_registry_add_listener(registry, &registry_listener, wl);

    /* Round-trip to get globals */
    wl_display_roundtrip(wl->display);

    /* We don't need this anymore */
    wl_registry_destroy(registry);

    wl->surface = wl_compositor_create_surface(wl->compositor);

    if (!wl->shell)
        throw std::runtime_error("Compositor is missing xdg_wm_base protocol support");

    wl->xdg_surface = xdg_wm_base_get_xdg_surface(wl->shell, wl->surface);

    xdg_surface_add_listener(wl->xdg_surface, &xdg_surface_listener, wl);

    wl->xdg_toplevel = xdg_surface_get_toplevel(wl->xdg_surface);

    xdg_toplevel_add_listener(wl->xdg_toplevel, &xdg_toplevel_listener, wl);
    xdg_toplevel_set_title(wl->xdg_toplevel, "vkcube");

    wl->wait_for_configure = true;
    wl_surface_commit(wl->surface);

    return true;
}


VkSurfaceKHR create_wayland_surface(
    VkInstance instance,
    VkPhysicalDevice physicalDevice,
    wl_connection* wl)
{
    auto GetPhysicalDeviceWaylandPresentationSupport =
        (PFN_vkGetPhysicalDeviceWaylandPresentationSupportKHR)
        vkGetInstanceProcAddr(instance, "vkGetPhysicalDeviceWaylandPresentationSupportKHR");
    auto CreateWaylandSurface =
        (PFN_vkCreateWaylandSurfaceKHR)
        vkGetInstanceProcAddr(instance, "vkCreateWaylandSurfaceKHR");

    if (!GetPhysicalDeviceWaylandPresentationSupport(physicalDevice, 0,
                                                     wl->display)) {
        throw std::runtime_error("Vulkan not supported on given Wayland surface");
    }

    VkWaylandSurfaceCreateInfoKHR waylandSurfaceCreateInfoKHR {
        .sType = VK_STRUCTURE_TYPE_WAYLAND_SURFACE_CREATE_INFO_KHR,
        .display = wl->display,
        .surface = wl->surface,
    };

    VkSurfaceKHR surface;
    CreateWaylandSurface(
        instance,
        &waylandSurfaceCreateInfoKHR,
        nullptr,
        &surface
    );

    return surface;
}
