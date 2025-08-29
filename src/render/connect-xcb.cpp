#include "numgeom/connect-xcb.h"

#include <cassert>
#include <stdexcept>

#include <string.h>

#include "vulkan/vulkan_xcb.h"


const char* get_xcb_extension_name()
{
    return VK_KHR_XCB_SURFACE_EXTENSION_NAME;
}


bool init_xcb_connection(uint16_t width, uint16_t height, xcb_connection* xcb)
{
    int screenNumber = 0;
    xcb_connection_t* connection = xcb_connect(nullptr, &screenNumber);
    if(xcb_connection_has_error(connection))
        std::runtime_error("Failed to connect to X server");

    xcb_screen_t* screen = nullptr;
    {
        xcb_screen_iterator_t it = xcb_setup_roots_iterator(xcb_get_setup(connection));
        for(int i = 0; i < screenNumber; ++i)
            xcb_screen_next(&it);
        screen = it.data;
    }
    assert(screen != nullptr);

    uint32_t value_list[2];

    xcb_window_t win = xcb_generate_id(connection);
    value_list[0] = screen->black_pixel,
    value_list[1] = XCB_EVENT_MASK_EXPOSURE |
                    XCB_EVENT_MASK_RESIZE_REDIRECT |
                    XCB_EVENT_MASK_KEY_PRESS |
                    XCB_EVENT_MASK_KEY_RELEASE |
                    XCB_EVENT_MASK_POINTER_MOTION |
                    XCB_EVENT_MASK_BUTTON_PRESS |
                    XCB_EVENT_MASK_BUTTON_RELEASE;
    xcb_void_cookie_t cookieWindow = xcb_create_window_checked(
        connection,
        XCB_COPY_FROM_PARENT,
        win,
        screen->root,
        0, 0, width, height,
        0,
        XCB_WINDOW_CLASS_INPUT_OUTPUT,
        screen->root_visual,
        XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK,
        value_list
    );

    xcb_generic_error_t* err = xcb_request_check(connection, cookieWindow);
    assert(err == nullptr);

    // Код, который в будущем позволит отправить уведомление при уничтожении окна.
    xcb_intern_atom_cookie_t cookie1 = xcb_intern_atom(connection, 1, 12, "WM_PROTOCOLS");
    xcb_intern_atom_reply_t* reply = xcb_intern_atom_reply(connection, cookie1, 0);
    xcb_intern_atom_cookie_t cookie2 = xcb_intern_atom(connection, 0, 16,"WM_DELETE_WINDOW");
    xcb_intern_atom_reply_t* atomReply = xcb_intern_atom_reply(connection, cookie2, 0);
    xcb_change_property(
        connection,
        XCB_PROP_MODE_REPLACE,
        win,
        (*reply).atom,
        4,
        32,
        1,
        &(*atomReply).atom
    );
    free(reply);

    // Устанавливаем заголовок окна.
    const char* title = "numgeom";
    xcb_change_property(
        connection,
        XCB_PROP_MODE_REPLACE,
        win,
        XCB_ATOM_WM_NAME,
        XCB_ATOM_STRING,
        8,
        strlen(title),
        title
    );
    xcb_void_cookie_t cookieMap = xcb_map_window_checked(connection, win);
    err = xcb_request_check(connection, cookieMap);
    assert(err == nullptr);

    xcb_flush(connection);

    xcb->connection = connection;
    xcb->window = win;
    return true;
}


VkSurfaceKHR create_xcb_surface(
    VkInstance instance,
    VkPhysicalDevice physicalDevice,
    xcb_connection* xcb)
{
    VkXcbSurfaceCreateInfoKHR surfaceCreateInfoKHR {
        .sType = VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR,
        .pNext = nullptr,
        .connection = xcb->connection,
        .window = xcb->window
    };
    VkSurfaceKHR surface;
    VkResult r;
    r = vkCreateXcbSurfaceKHR(
        instance,
        &surfaceCreateInfoKHR,
        nullptr,
        &surface
    );
    assert(r == VK_SUCCESS);
    return surface;
}


void process_xcb_event(xcb_connection* xcb)
{
    xcb_generic_event_t* event = xcb_poll_for_event(xcb->connection);
    bool quit = false;
    if(event) {
        switch(event->response_type & ~0x80) {
        case XCB_EXPOSE: //< Перерисовать окно.
            xcb_flush(xcb->connection);
            break;
        case XCB_KEY_PRESS: { //< Нажата клавиша.
            auto* key_event = (xcb_key_press_event_t*)(event);
            // Выходим по нажатию клавиш `esc` или `q`.
            if(key_event->detail == 0x9 || key_event->detail == 0x18)
                quit = true;
            break;
        }
        case XCB_RESIZE_REQUEST:
            break;
        default:
            //std::cout << "opcode = " << (event->response_type & ~0x80) << std::endl;
            break;
        }

        free(event);
    }
}
