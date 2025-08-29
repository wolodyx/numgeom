#include <stdarg.h>
#include <sys/time.h>

#define VK_PROTOTYPES
#include <vulkan/vulkan.h>

#if defined(ENABLE_WAYLAND)
#  define VK_USE_PLATFORM_WAYLAND_KHR
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
