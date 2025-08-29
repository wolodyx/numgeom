#ifndef numgeom_render_connect_xcb_h
#define numgeom_render_connect_xcb_h

#include <vulkan/vulkan.h>

#include <xcb/xcb.h>


//! Описание xcb-соединения.
struct xcb_connection
{
    xcb_connection_t* connection;
    xcb_window_t window;
    // xcb_atom_t atom_wm_protocols;
    // xcb_atom_t atom_wm_delete_window;
};


const char* get_xcb_extension_name();


//! Инициализация xcb-соединения.
bool init_xcb_connection(uint16_t width, uint16_t height, xcb_connection* xcb);


//! Создаем vulkan-поверхность для xcb-соединения.
VkSurfaceKHR create_xcb_surface(
    VkInstance instance,
    VkPhysicalDevice physicalDevice,
    xcb_connection*
);


void process_xcb_event(xcb_connection*);

#endif // !numgeom_render_connect_xcb_h
