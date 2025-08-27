#include <stdarg.h>
#include <sys/time.h>

#define VK_PROTOTYPES
#include <vulkan/vulkan.h>

#if defined(ENABLE_WAYLAND)
#include <wayland-client.h>
#include </home/tim/vcpkg/buildtrees/sdl2/x64-linux-rel/wayland-generated-protocols/xdg-shell-client-protocol.h>
#include "vulkan/vulkan_wayland.h"
#define VK_USE_PLATFORM_WAYLAND_KHR
#elif defined(ENABLE_XCB)
#include <xcb/xcb.h>
#include "vulkan/vulkan_xcb.h"
#endif

#define printflike(a, b) __attribute__((format(printf, (a), (b))))

#define MAX_NUM_IMAGES 5

struct vkcube_buffer {
    struct gbm_bo *gbm_bo;
    VkDeviceMemory mem;
    VkImage image;
    VkImageView view;
    VkFramebuffer framebuffer;
    VkFence fence;
    VkCommandBuffer cmd_buffer;

    uint32_t fb;
    uint32_t stride;
};

struct vkcube;

struct model {
    void (*init)(struct vkcube *vc);
    void (*render)(struct vkcube *vc, struct vkcube_buffer *b, bool wait_semaphore);
};

struct vkcube {
    struct model model;

#if defined(ENABLE_WAYLAND)
    struct {
        struct wl_display *display;
        struct wl_compositor *compositor;
        struct xdg_wm_base *shell;
        struct wl_keyboard *keyboard;
        struct wl_seat *seat;
        struct wl_surface *surface;
        struct xdg_surface *xdg_surface;
        struct xdg_toplevel *xdg_toplevel;
        bool wait_for_configure;
    } wl;
#elif defined(ENABLE_XCB)
    struct {
        xcb_connection_t* connection;
        xcb_window_t window;
        // xcb_atom_t atom_wm_protocols;
        // xcb_atom_t atom_wm_delete_window;
    } xcb;
#endif

    VkSwapchainKHR swap_chain;

    uint32_t width, height;

    VkInstance instance;
    VkPhysicalDevice physical_device;
    VkPhysicalDeviceMemoryProperties memory_properties;
    VkDevice device;
    VkRenderPass render_pass;
    VkQueue queue;
    VkPipelineLayout pipeline_layout;
    VkPipeline pipeline;
    VkDeviceMemory mem;
    VkBuffer buffer;
    VkDescriptorSet descriptor_set;
    VkSemaphore semaphore;
    VkCommandPool cmd_pool;

    void *map;
    uint32_t vertex_offset, colors_offset, normals_offset;

    struct timeval start_tv;
    VkSurfaceKHR surface;
    VkFormat image_format;
    struct vkcube_buffer buffers[MAX_NUM_IMAGES];
    uint32_t image_count;
};

[[noreturn]] void failv(const char *format, va_list args);
[[noreturn]] void fail(const char *format, ...) printflike(1, 2) ;
void fail_if(int cond, const char *format, ...) printflike(2, 3);
