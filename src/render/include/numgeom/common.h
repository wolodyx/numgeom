#include <stdarg.h>
#include <chrono>

#define VK_PROTOTYPES
#include <vulkan/vulkan.h>

#if defined(ENABLE_WAYLAND)
#  define VK_USE_PLATFORM_WAYLAND_KHR
#endif

#define MAX_NUM_IMAGES 5

struct vkcube_buffer {
    struct gbm_bo *gbm_bo;
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

    std::chrono::time_point<std::chrono::system_clock> start_tv;
    VkSurfaceKHR surface;
    VkFormat image_format;
    struct vkcube_buffer buffers[MAX_NUM_IMAGES];
    uint32_t image_count;
};
