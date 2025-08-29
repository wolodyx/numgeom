#include "numgeom/vkrender.h"

#include <cassert>
#include <cstdlib>

#include <stdio.h>
#include <string.h>

#include <errno.h>
#include <poll.h>
#include <linux/input.h>

#include <xcb/xproto.h>

#include "numgeom/common.h"

#include "numgeom/connect-wayland.h"
#include "numgeom/connect-xcb.h"


[[noreturn]] void failv(const char *format, va_list args)
{
    vfprintf(stderr, format, args);
    fprintf(stderr, "\n");
    std::exit(1);
}

[[noreturn]] void printflike(1,2)
fail(const char *format, ...)
{
    va_list args;

    va_start(args, format);
    failv(format, args);
    va_end(args);
}

void printflike(2, 3)
fail_if(int cond, const char *format, ...)
{
    va_list args;

    if (!cond)
        return;

    va_start(args, format);
    failv(format, args);
    va_end(args);
}

/**
\brief Инициализация и выбор объектов vulkan:
+ экземпляра;
+ физического устройства;
+ логического устройства;
+ очереди;
+ свойства памяти.
*/
void init_vulkan(
    vkcube* vc,
    const std::vector<const char*>& additionalExtensionNames)
{
    VkApplicationInfo applicationInfo = {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pApplicationName = "numgeom",
        .apiVersion = VK_MAKE_VERSION(1, 1, 0),
    };
    std::vector<const char*> enabledExtensionNames = {
        VK_KHR_SURFACE_EXTENSION_NAME
    };
    for(const auto* extName : additionalExtensionNames)
        enabledExtensionNames.push_back(extName);

    VkInstanceCreateInfo instanceCreateInfo {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pApplicationInfo = &applicationInfo,
        .enabledExtensionCount = static_cast<uint32_t>(enabledExtensionNames.size()),
        .ppEnabledExtensionNames = enabledExtensionNames.data()
    };

    // Создаем экземпляр vulkan.
    VkResult res = vkCreateInstance(
        &instanceCreateInfo,
        nullptr,
        &vc->instance
    );
    fail_if(res != VK_SUCCESS, "Failed to create Vulkan instance.\n");

    // Выбор физического устройства vulkan.
    uint32_t count;
    res = vkEnumeratePhysicalDevices(vc->instance, &count, nullptr);
    fail_if(res != VK_SUCCESS || count == 0, "No Vulkan devices found.\n");
    VkPhysicalDevice pd[count];
    vkEnumeratePhysicalDevices(vc->instance, &count, pd);
    vc->physical_device = pd[0];

    // Получаем свойства памятий.
    vkGetPhysicalDeviceMemoryProperties(vc->physical_device, &vc->memory_properties);

    // Выбираем семейство очередей.
    vkGetPhysicalDeviceQueueFamilyProperties(vc->physical_device, &count, nullptr);
    assert(count > 0);
    VkQueueFamilyProperties props[count];
    vkGetPhysicalDeviceQueueFamilyProperties(vc->physical_device, &count, props);
    assert(props[0].queueFlags & VK_QUEUE_GRAPHICS_BIT);

    // Создаем логическое устройство vulkan.
    float queuePriorities[] = { 1.0f };
    VkDeviceQueueCreateInfo queueCreateInfo {
        .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        .queueFamilyIndex = 0,
        .queueCount = 1,
        .pQueuePriorities = queuePriorities,
    };
    VkDeviceCreateInfo deviceCreateInfo {
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .queueCreateInfoCount = 1,
        .pQueueCreateInfos = &queueCreateInfo,
        .enabledExtensionCount = 1,
        .ppEnabledExtensionNames = (const char * const []) {
            VK_KHR_SWAPCHAIN_EXTENSION_NAME,
        },
    };
    vkCreateDevice(
        vc->physical_device,
        &deviceCreateInfo,
        nullptr,
        &vc->device
    );

    // Извлекаем очередь.
    VkDeviceQueueInfo2 deviceQueueInfo2 {
        .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_INFO_2,
        .queueFamilyIndex = 0,
        .queueIndex = 0,
    };
    vkGetDeviceQueue2(vc->device, &deviceQueueInfo2, &vc->queue);
}

/**
\brief Инициализирует следующие объекты vulkan:
+) объект renderpass;
+) пул команд;
+) семафор;
+) размещение конвейера (pipeline layout);
+) шейдерные модули;
+) графический конвейер;
+) буфер с выделенной памятью и загруженными данными;
+) пул дескрипторов;
+) набор дескрипторов.
*/
static void init_vk_objects(vkcube *vc)
{
    VkAttachmentReference colorAttachments[] = {
        {
            .attachment = 0,
            .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
        }
    };
    VkAttachmentReference resolveAttachments[] = {
        {
            .attachment = VK_ATTACHMENT_UNUSED,
            .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
        }
    };
    VkAttachmentDescription attachments[] = {
        {
            .format = vc->image_format,
            .samples = VK_SAMPLE_COUNT_1_BIT,
            .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
            .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
        }
    };
    VkSubpassDescription subpasses[] = {
        {
            .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
            .inputAttachmentCount = 0,
            .colorAttachmentCount = 1,
            .pColorAttachments = colorAttachments,
            .pResolveAttachments = resolveAttachments,
            .pDepthStencilAttachment = nullptr,
            .preserveAttachmentCount = 0,
            .pPreserveAttachments = nullptr,
        }
    };
    VkRenderPassCreateInfo renderPassCreateInfo {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        .attachmentCount = 1,
        .pAttachments = attachments,
        .subpassCount = 1,
        .pSubpasses = subpasses,
        .dependencyCount = 0
    };

    vkCreateRenderPass(
        vc->device,
        &renderPassCreateInfo,
        nullptr,
        &vc->render_pass
    );

    vc->model.init(vc);

    VkCommandPoolCreateInfo commandPoolCreateInfo {
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        .queueFamilyIndex = 0,
    };
    vkCreateCommandPool(
        vc->device,
        &commandPoolCreateInfo,
        nullptr,
        &vc->cmd_pool
    );

    VkSemaphoreCreateInfo semaphoreCreateInfo {
       .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
    };
    vkCreateSemaphore(
        vc->device,
        &semaphoreCreateInfo,
        nullptr,
        &vc->semaphore
    );
}

static void init_buffer(vkcube* vc, vkcube_buffer* b)
{
    VkImageViewCreateInfo imageViewCreateInfo {
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .image = b->image,
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .format = vc->image_format,
        .components = {
            .r = VK_COMPONENT_SWIZZLE_R,
            .g = VK_COMPONENT_SWIZZLE_G,
            .b = VK_COMPONENT_SWIZZLE_B,
            .a = VK_COMPONENT_SWIZZLE_A,
        },
        .subresourceRange = {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1,
        },
    };
    vkCreateImageView(
        vc->device,
        &imageViewCreateInfo,
        nullptr,
        &b->view
    );

    VkFramebufferCreateInfo framebufferCreateInfo {
        .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
        .renderPass = vc->render_pass,
        .attachmentCount = 1,
        .pAttachments = &b->view,
        .width = vc->width,
        .height = vc->height,
        .layers = 1
    };
    vkCreateFramebuffer(
        vc->device,
        &framebufferCreateInfo,
        nullptr,
        &b->framebuffer
    );

    VkFenceCreateInfo fenceCreateInfo {
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        .flags = VK_FENCE_CREATE_SIGNALED_BIT
    };
    vkCreateFence(
        vc->device,
        &fenceCreateInfo,
        nullptr,
        &b->fence
    );

    VkCommandBufferAllocateInfo commandBufferAllocateInfo {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool = vc->cmd_pool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = 1,
    };
    vkAllocateCommandBuffers(
        vc->device,
        &commandBufferAllocateInfo,
        &b->cmd_buffer
    );
}

/* Swapchain-based code - shared between XCB and Wayland */

static VkFormat
choose_surface_format(vkcube *vc)
{
    uint32_t num_formats = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(
        vc->physical_device,
        vc->surface,
        &num_formats,
        nullptr
    );
    assert(num_formats > 0);

    VkSurfaceFormatKHR formats[num_formats];

    vkGetPhysicalDeviceSurfaceFormatsKHR(
        vc->physical_device,
        vc->surface,
        &num_formats,
        formats
    );

    VkFormat format = VK_FORMAT_UNDEFINED;
    for (int i = 0; i < num_formats; i++) {
        switch (formats[i].format) {
        case VK_FORMAT_R8G8B8A8_SRGB:
        case VK_FORMAT_B8G8R8A8_SRGB:
        case VK_FORMAT_R8G8B8A8_UNORM:
        case VK_FORMAT_B8G8R8A8_UNORM:
            /* These formats are all fine */
            format = formats[i].format;
            break;
        case VK_FORMAT_R8G8B8_SRGB:
        case VK_FORMAT_B8G8R8_SRGB:
        case VK_FORMAT_R8G8B8_UNORM:
        case VK_FORMAT_R5G6B5_UNORM_PACK16:
        case VK_FORMAT_B5G6R5_UNORM_PACK16:
            /* We would like to support these but they don't seem to work. */
        default:
            continue;
        }
    }

    assert(format != VK_FORMAT_UNDEFINED);

    return format;
}


//! Инициализирует swapchain и буферы для изображений.
static void create_swapchain(vkcube* vc)
{
    VkSurfaceCapabilitiesKHR surface_caps;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
        vc->physical_device,
        vc->surface,
        &surface_caps
    );
    assert(surface_caps.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR);

    VkBool32 supported;
    vkGetPhysicalDeviceSurfaceSupportKHR(
        vc->physical_device,
        0,
        vc->surface,
        &supported
    );
    assert(supported);

    uint32_t count;
    vkGetPhysicalDeviceSurfacePresentModesKHR(
        vc->physical_device,
        vc->surface,
        &count,
        nullptr
    );
    VkPresentModeKHR present_modes[count];
    vkGetPhysicalDeviceSurfacePresentModesKHR(
        vc->physical_device,
        vc->surface,
        &count,
        present_modes
    );
    int i;
    VkPresentModeKHR present_mode = VK_PRESENT_MODE_FIFO_KHR;
    for (i = 0; i < count; i++) {
        if (present_modes[i] == VK_PRESENT_MODE_MAILBOX_KHR) {
            present_mode = VK_PRESENT_MODE_MAILBOX_KHR;
            break;
        }
    }

    uint32_t minImageCount = 2;
    if(minImageCount < surface_caps.minImageCount) {
        if (surface_caps.minImageCount > MAX_NUM_IMAGES)
            fail("surface_caps.minImageCount is too large (is: %d, max: %d)",
                 surface_caps.minImageCount, MAX_NUM_IMAGES);
        minImageCount = surface_caps.minImageCount;
    }

    if(surface_caps.maxImageCount > 0 &&
       minImageCount > surface_caps.maxImageCount) {
       minImageCount = surface_caps.maxImageCount;
    }

    uint32_t queueFamilyIndices[] = { 0 };
    VkSwapchainCreateInfoKHR swapchainCreateInfoKHR {
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .surface = vc->surface,
        .minImageCount = minImageCount,
        .imageFormat = vc->image_format,
        .imageColorSpace = VK_COLORSPACE_SRGB_NONLINEAR_KHR,
        .imageExtent = { vc->width, vc->height },
        .imageArrayLayers = 1,
        .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        .imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount = 1,
        .pQueueFamilyIndices = queueFamilyIndices,
        .preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR,
        .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        .presentMode = present_mode,
    };
    vkCreateSwapchainKHR(
        vc->device,
        &swapchainCreateInfoKHR,
        nullptr,
        &vc->swap_chain
    );

    vkGetSwapchainImagesKHR(
        vc->device,
        vc->swap_chain,
        &vc->image_count,
        nullptr
    );
    assert(vc->image_count > 0);
    VkImage swap_chain_images[vc->image_count];
    vkGetSwapchainImagesKHR(
        vc->device,
        vc->swap_chain,
        &vc->image_count,
        swap_chain_images
    );

    assert(vc->image_count <= MAX_NUM_IMAGES);
    for (uint32_t i = 0; i < vc->image_count; i++) {
        vc->buffers[i].image = swap_chain_images[i];
        init_buffer(vc, &vc->buffers[i]);
    }
}


#ifdef ENABLE_WAYLAND
int init_wayland_display(vkcube *vc, wl_connection* wl)
{
    init_wayland_connection(wl);

    init_vulkan(vc, {get_wayland_extension_name()});

    vc->surface = create_wayland_surface(vc->instance, vc->physical_device, wl);
    vc->image_format = choose_surface_format(vc);

    init_vk_objects(vc);

    create_swapchain(vc);

    return 0;
}
#endif


#ifdef ENABLE_XCB
int init_xcb_display(vkcube *vc, xcb_connection* xcb)
{
    vc->surface = nullptr;

    init_xcb_connection(vc->width, vc->height, xcb);

    init_vulkan(vc, {get_xcb_extension_name()});

    vc->surface = create_xcb_surface(vc->instance, vc->physical_device, xcb);
    vc->image_format = choose_surface_format(vc);

    init_vk_objects(vc);

    create_swapchain(vc);

    return 0;
}
#endif // ENABLE_XCB


static void
fini_buffer(vkcube *vc, vkcube_buffer *b)
{
    vkFreeCommandBuffers(vc->device, vc->cmd_pool, 1, &b->cmd_buffer);
    vkDestroyFence(vc->device, b->fence, nullptr);
    vkDestroyFramebuffer(vc->device, b->framebuffer, nullptr);
    vkDestroyImageView(vc->device, b->view, nullptr);
}

static void
recreate_swapchain(vkcube *vc)
{
    VkSwapchainKHR old_chain = vc->swap_chain;

    for (uint32_t i = 0; i < vc->image_count; i++)
        fini_buffer(vc, &vc->buffers[i]);

    vkDestroySwapchainKHR(vc->device, old_chain, nullptr);
    create_swapchain(vc);
}


#ifdef ENABLE_WAYLAND
void mainloop_wayland(vkcube *vc, wl_connection* wl)
{
    pollfd fds[] = {
        { wl_display_get_fd(wl->display), POLLIN },
    };
    while(1) {
        while(wl_display_prepare_read(wl->display) != 0)
              wl_display_dispatch_pending(wl->display);
        if(wl_display_flush(wl->display) < 0 && errno != EAGAIN) {
            wl_display_cancel_read(wl->display);
            return;
        }
        if(poll(fds, 1, 0) > 0) {
            wl_display_read_events(wl->display);
            wl_display_dispatch_pending(wl->display);
        } else {
            wl_display_cancel_read(wl->display);
        }

        uint32_t index;

        VkResult result = vkAcquireNextImageKHR(
            vc->device,
            vc->swap_chain,
            60,
            vc->semaphore,
            VK_NULL_HANDLE,
            &index
        );
        if(result == VK_SUBOPTIMAL_KHR || result == VK_ERROR_OUT_OF_DATE_KHR) {
            recreate_swapchain(vc);
            continue;
        } else if(result == VK_NOT_READY || result == VK_TIMEOUT) {
            continue;
        } else if(result != VK_SUCCESS) {
            return;
        }

        assert(index <= MAX_NUM_IMAGES);
        vc->model.render(vc, &vc->buffers[index], true);

        VkPresentInfoKHR presentInfoKHR {
            .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
            .swapchainCount = 1,
            .pSwapchains = &vc->swap_chain,
            .pImageIndices = &index,
            .pResults = &result,
        };
        vkQueuePresentKHR(vc->queue, &presentInfoKHR);
        if (result != VK_SUCCESS)
            return;

        vkQueueWaitIdle(vc->queue);
    }
}
#endif


#ifdef ENABLE_XCB
void mainloop_xcb(vkcube* vc, xcb_connection* xcb)
{
    VkResult result = VK_SUCCESS;
    bool quit = false;
    while(!quit) {
        process_xcb_event(xcb);

        uint32_t index;
        result = vkAcquireNextImageKHR(
            vc->device,
            vc->swap_chain,
            60,
            vc->semaphore,
            VK_NULL_HANDLE,
            &index
        );
        if(result == VK_SUBOPTIMAL_KHR || result == VK_ERROR_OUT_OF_DATE_KHR) {
            recreate_swapchain(vc);
            continue;
        } else if(result == VK_NOT_READY ||
            result == VK_TIMEOUT) {
            continue;
        } else if(result != VK_SUCCESS) {
            return;
        }

        assert(index <= MAX_NUM_IMAGES);
        vc->model.render(vc, &vc->buffers[index], true);

        VkPresentInfoKHR presentInfoKHR {
            .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
            .swapchainCount = 1,
            .pSwapchains = &vc->swap_chain,
            .pImageIndices = &index,
            .pResults = &result,
        };
        vkQueuePresentKHR(vc->queue, &presentInfoKHR);
        if (result != VK_SUCCESS)
            return;

        vkQueueWaitIdle(vc->queue);
    }
}
#endif // ENABLE_XCB
