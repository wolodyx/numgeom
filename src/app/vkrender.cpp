#include "vkrender.h"

#include <cassert>

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <poll.h>
#include <linux/input.h>

#include "common.h"


[[noreturn]] void failv(const char *format, va_list args)
{
   vfprintf(stderr, format, args);
   fprintf(stderr, "\n");
   exit(1);
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

static void init_vk(struct vkcube *vc)
{
   VkApplicationInfo applicationInfo = {
      .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
      .pApplicationName = "numgeom",
      .apiVersion = VK_MAKE_VERSION(1, 1, 0),
   };
   const char* enabledExtensionNames[2]  = {
      VK_KHR_SURFACE_EXTENSION_NAME,
      VK_KHR_WAYLAND_SURFACE_EXTENSION_NAME,
   };
   VkInstanceCreateInfo instanceCreateInfo {
      .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
      .pApplicationInfo = &applicationInfo,
      .enabledExtensionCount = sizeof(enabledExtensionNames) / sizeof(enabledExtensionNames[0]),
      .ppEnabledExtensionNames = enabledExtensionNames
   };
   VkResult res = vkCreateInstance(&instanceCreateInfo,
                                   nullptr,
                                   &vc->instance);
   fail_if(res != VK_SUCCESS, "Failed to create Vulkan instance.\n");

   uint32_t count;
   res = vkEnumeratePhysicalDevices(vc->instance, &count, nullptr);
   fail_if(res != VK_SUCCESS || count == 0, "No Vulkan devices found.\n");
   VkPhysicalDevice pd[count];
   vkEnumeratePhysicalDevices(vc->instance, &count, pd);
   vc->physical_device = pd[0];

   vkGetPhysicalDeviceMemoryProperties(vc->physical_device, &vc->memory_properties);

   vkGetPhysicalDeviceQueueFamilyProperties(vc->physical_device, &count, nullptr);
   assert(count > 0);
   VkQueueFamilyProperties props[count];
   vkGetPhysicalDeviceQueueFamilyProperties(vc->physical_device, &count, props);
   assert(props[0].queueFlags & VK_QUEUE_GRAPHICS_BIT);

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
   vkCreateDevice(vc->physical_device,
                  &deviceCreateInfo,
                  nullptr,
                  &vc->device);

   VkDeviceQueueInfo2 deviceQueueInfo2 {
      .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_INFO_2,
      .queueFamilyIndex = 0,
      .queueIndex = 0,
   };
   vkGetDeviceQueue2(vc->device, &deviceQueueInfo2, &vc->queue);
}

static void
init_vk_objects(struct vkcube *vc)
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

   vkCreateRenderPass(vc->device,
                      &renderPassCreateInfo,
                      nullptr,
                      &vc->render_pass);

   vc->model.init(vc);

   VkCommandPoolCreateInfo commandPoolCreateInfo {
      .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
      .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
      .queueFamilyIndex = 0,
   };
   vkCreateCommandPool(vc->device,
                       &commandPoolCreateInfo,
                       nullptr,
                       &vc->cmd_pool);

   VkSemaphoreCreateInfo semaphoreCreateInfo {
      .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
   };
   vkCreateSemaphore(vc->device,
                     &semaphoreCreateInfo,
                     nullptr,
                     &vc->semaphore);
}

static void
init_buffer(struct vkcube *vc, struct vkcube_buffer *b)
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
   vkCreateImageView(vc->device,
                     &imageViewCreateInfo,
                     nullptr,
                     &b->view);

   VkFramebufferCreateInfo framebufferCreateInfo {
      .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
      .renderPass = vc->render_pass,
      .attachmentCount = 1,
      .pAttachments = &b->view,
      .width = vc->width,
      .height = vc->height,
      .layers = 1
   };
   vkCreateFramebuffer(vc->device,
                       &framebufferCreateInfo,
                       nullptr,
                       &b->framebuffer);

   VkFenceCreateInfo fenceCreateInfo {
      .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
      .flags = VK_FENCE_CREATE_SIGNALED_BIT
   };
   vkCreateFence(vc->device,
                 &fenceCreateInfo,
                 nullptr,
                 &b->fence);

   VkCommandBufferAllocateInfo commandBufferAllocateInfo {
      .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
      .commandPool = vc->cmd_pool,
      .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
      .commandBufferCount = 1,
   };
   vkAllocateCommandBuffers(vc->device,
                            &commandBufferAllocateInfo,
                            &b->cmd_buffer);
}

/* Swapchain-based code - shared between XCB and Wayland */

static VkFormat
choose_surface_format(struct vkcube *vc)
{
   uint32_t num_formats = 0;
   vkGetPhysicalDeviceSurfaceFormatsKHR(vc->physical_device, vc->surface,
                                        &num_formats, nullptr);
   assert(num_formats > 0);

   VkSurfaceFormatKHR formats[num_formats];

   vkGetPhysicalDeviceSurfaceFormatsKHR(vc->physical_device, vc->surface,
                                        &num_formats, formats);

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

static void
create_swapchain(struct vkcube *vc)
{
   VkSurfaceCapabilitiesKHR surface_caps;
   vkGetPhysicalDeviceSurfaceCapabilitiesKHR(vc->physical_device, vc->surface,
                                             &surface_caps);
   assert(surface_caps.supportedCompositeAlpha &
          VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR);

   VkBool32 supported;
   vkGetPhysicalDeviceSurfaceSupportKHR(vc->physical_device, 0, vc->surface,
                                        &supported);
   assert(supported);

   uint32_t count;
   vkGetPhysicalDeviceSurfacePresentModesKHR(vc->physical_device, vc->surface,
                                             &count, nullptr);
   VkPresentModeKHR present_modes[count];
   vkGetPhysicalDeviceSurfacePresentModesKHR(vc->physical_device, vc->surface,
                                             &count, present_modes);
   int i;
   VkPresentModeKHR present_mode = VK_PRESENT_MODE_FIFO_KHR;
   for (i = 0; i < count; i++) {
      if (present_modes[i] == VK_PRESENT_MODE_MAILBOX_KHR) {
         present_mode = VK_PRESENT_MODE_MAILBOX_KHR;
         break;
      }
   }

   uint32_t minImageCount = 2;
   if (minImageCount < surface_caps.minImageCount) {
      if (surface_caps.minImageCount > MAX_NUM_IMAGES)
          fail("surface_caps.minImageCount is too large (is: %d, max: %d)",
               surface_caps.minImageCount, MAX_NUM_IMAGES);
      minImageCount = surface_caps.minImageCount;
   }

   if (surface_caps.maxImageCount > 0 &&
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
   vkCreateSwapchainKHR(vc->device,
                        &swapchainCreateInfoKHR,
                        nullptr,
                        &vc->swap_chain);

   vkGetSwapchainImagesKHR(vc->device, vc->swap_chain,
                           &vc->image_count, nullptr);
   assert(vc->image_count > 0);
   VkImage swap_chain_images[vc->image_count];
   vkGetSwapchainImagesKHR(vc->device, vc->swap_chain,
                           &vc->image_count, swap_chain_images);

   assert(vc->image_count <= MAX_NUM_IMAGES);
   for (uint32_t i = 0; i < vc->image_count; i++) {
      vc->buffers[i].image = swap_chain_images[i];
      init_buffer(vc, &vc->buffers[i]);
   }
}

/* Wayland display code - render to Wayland window */

static void
handle_xdg_surface_configure(void *data, struct xdg_surface *surface,
                             uint32_t serial)
{
   struct vkcube *vc = (vkcube*)data;

   xdg_surface_ack_configure(surface, serial);

   if (vc->wl.wait_for_configure) {
      // redraw
      vc->wl.wait_for_configure = false;
   }
}

static const struct xdg_surface_listener xdg_surface_listener = {
   handle_xdg_surface_configure,
};

static void
handle_xdg_toplevel_configure(void *data, struct xdg_toplevel *toplevel,
                              int32_t width, int32_t height,
                              struct wl_array *states)
{
}

static void
handle_xdg_toplevel_close(void *data, struct xdg_toplevel *toplevel)
{
}

static const struct xdg_toplevel_listener xdg_toplevel_listener = {
   handle_xdg_toplevel_configure,
   handle_xdg_toplevel_close,
};

static void
handle_xdg_wm_base_ping(void *data, struct xdg_wm_base *shell, uint32_t serial)
{
   xdg_wm_base_pong(shell, serial);
}

static const struct xdg_wm_base_listener xdg_wm_base_listener = {
   handle_xdg_wm_base_ping,
};

static void
handle_wl_keyboard_keymap(void *data, struct wl_keyboard *wl_keyboard,
			  uint32_t format, int32_t fd, uint32_t size)
{
}

static void
handle_wl_keyboard_enter(void *data, struct wl_keyboard *wl_keyboard,
			 uint32_t serial, struct wl_surface *surface,
			 struct wl_array *keys)
{
}

static void
handle_wl_keyboard_leave(void *data, struct wl_keyboard *wl_keyboard,
			 uint32_t serial, struct wl_surface *surface)
{
}

static void
handle_wl_keyboard_key(void *data, struct wl_keyboard *wl_keyboard,
		       uint32_t serial, uint32_t time, uint32_t key,
		       uint32_t state)
{
    if (key == KEY_ESC && state == WL_KEYBOARD_KEY_STATE_PRESSED)
      exit(0);
}

static void
handle_wl_keyboard_modifiers(void *data, struct wl_keyboard *wl_keyboard,
			     uint32_t serial, uint32_t mods_depressed,
			     uint32_t mods_latched, uint32_t mods_locked,
			     uint32_t group)
{
}

static void
handle_wl_keyboard_repeat_info(void *data, struct wl_keyboard *wl_keyboard,
			       int32_t rate, int32_t delay)
{
}

static const struct wl_keyboard_listener wl_keyboard_listener = {
   .keymap = handle_wl_keyboard_keymap,
   .enter = handle_wl_keyboard_enter,
   .leave = handle_wl_keyboard_leave,
   .key = handle_wl_keyboard_key,
   .modifiers = handle_wl_keyboard_modifiers,
   .repeat_info = handle_wl_keyboard_repeat_info,
};

static void
handle_wl_seat_capabilities(void *data, struct wl_seat *wl_seat,
			    uint32_t capabilities)
{
   struct vkcube *vc = (vkcube*)data;

   if ((capabilities & WL_SEAT_CAPABILITY_KEYBOARD) && (!vc->wl.keyboard)) {
      vc->wl.keyboard = wl_seat_get_keyboard(wl_seat);
      wl_keyboard_add_listener(vc->wl.keyboard, &wl_keyboard_listener, vc);
   } else if (!(capabilities & WL_SEAT_CAPABILITY_KEYBOARD) && vc->wl.keyboard) {
      wl_keyboard_destroy(vc->wl.keyboard);
      vc->wl.keyboard = nullptr;
   }
}

static const struct wl_seat_listener wl_seat_listener = {
   handle_wl_seat_capabilities,
};

static void
registry_handle_global(void *data, struct wl_registry *registry,
		       uint32_t name, const char *interface, uint32_t version)
{
   struct vkcube *vc = (vkcube*)data;

   if (strcmp(interface, "wl_compositor") == 0) {
      vc->wl.compositor = (wl_compositor*)wl_registry_bind(registry, name,
                                           &wl_compositor_interface, 1);
   } else if (strcmp(interface, "xdg_wm_base") == 0) {
      vc->wl.shell = (xdg_wm_base*)wl_registry_bind(registry, name, &xdg_wm_base_interface, 1);
      xdg_wm_base_add_listener(vc->wl.shell, &xdg_wm_base_listener, vc);
   } else if (strcmp(interface, "wl_seat") == 0) {
      vc->wl.seat = (wl_seat*)wl_registry_bind(registry, name, &wl_seat_interface, 1);
      wl_seat_add_listener(vc->wl.seat, &wl_seat_listener, vc);
   }
}

static void
registry_handle_global_remove(void *data, struct wl_registry *registry,
			      uint32_t name)
{
}

static const struct wl_registry_listener registry_listener = {
   registry_handle_global,
   registry_handle_global_remove
};

// Return -1 on failure.
int init_display(struct vkcube *vc)
{
   vc->wl.display = wl_display_connect(nullptr);
   if (!vc->wl.display)
      return -1;

   vc->wl.seat = nullptr;
   vc->wl.keyboard = nullptr;
   vc->wl.shell = nullptr;

   struct wl_registry *registry = wl_display_get_registry(vc->wl.display);
   wl_registry_add_listener(registry, &registry_listener, vc);

   /* Round-trip to get globals */
   wl_display_roundtrip(vc->wl.display);

   /* We don't need this anymore */
   wl_registry_destroy(registry);

   vc->wl.surface = wl_compositor_create_surface(vc->wl.compositor);

   if (!vc->wl.shell)
      fail("Compositor is missing xdg_wm_base protocol support");

   vc->wl.xdg_surface = xdg_wm_base_get_xdg_surface(vc->wl.shell, vc->wl.surface);

   xdg_surface_add_listener(vc->wl.xdg_surface, &xdg_surface_listener, vc);

   vc->wl.xdg_toplevel = xdg_surface_get_toplevel(vc->wl.xdg_surface);

   xdg_toplevel_add_listener(vc->wl.xdg_toplevel, &xdg_toplevel_listener, vc);
   xdg_toplevel_set_title(vc->wl.xdg_toplevel, "vkcube");

   vc->wl.wait_for_configure = true;
   wl_surface_commit(vc->wl.surface);

   init_vk(vc);

   PFN_vkGetPhysicalDeviceWaylandPresentationSupportKHR get_wayland_presentation_support =
      (PFN_vkGetPhysicalDeviceWaylandPresentationSupportKHR)
      vkGetInstanceProcAddr(vc->instance, "vkGetPhysicalDeviceWaylandPresentationSupportKHR");
   PFN_vkCreateWaylandSurfaceKHR create_wayland_surface =
      (PFN_vkCreateWaylandSurfaceKHR)
      vkGetInstanceProcAddr(vc->instance, "vkCreateWaylandSurfaceKHR");

   if (!get_wayland_presentation_support(vc->physical_device, 0,
                                         vc->wl.display)) {
      fail("Vulkan not supported on given Wayland surface");
   }

   VkWaylandSurfaceCreateInfoKHR waylandSurfaceCreateInfoKHR {
      .sType = VK_STRUCTURE_TYPE_WAYLAND_SURFACE_CREATE_INFO_KHR,
      .display = vc->wl.display,
      .surface = vc->wl.surface,
   };
   create_wayland_surface(vc->instance,
                          &waylandSurfaceCreateInfoKHR,
                          nullptr,
                          &vc->surface);

   vc->image_format = choose_surface_format(vc);

   init_vk_objects(vc);

   create_swapchain(vc);

   return 0;
}

static void
fini_buffer(struct vkcube *vc, struct vkcube_buffer *b)
{
   vkFreeCommandBuffers(vc->device, vc->cmd_pool, 1, &b->cmd_buffer);
   vkDestroyFence(vc->device, b->fence, nullptr);
   vkDestroyFramebuffer(vc->device, b->framebuffer, nullptr);
   vkDestroyImageView(vc->device, b->view, nullptr);
}

static void
recreate_swapchain(struct vkcube *vc)
{
   VkSwapchainKHR old_chain = vc->swap_chain;

   for (uint32_t i = 0; i < vc->image_count; i++)
      fini_buffer(vc, &vc->buffers[i]);

   vkDestroySwapchainKHR(vc->device, old_chain, nullptr);
   create_swapchain(vc);
}

void mainloop(struct vkcube *vc)
{
   VkResult result = VK_SUCCESS;
   struct pollfd fds[] = {
      { wl_display_get_fd(vc->wl.display), POLLIN },
   };
   while (1) {
      uint32_t index;

      while (wl_display_prepare_read(vc->wl.display) != 0)
         wl_display_dispatch_pending(vc->wl.display);
      if (wl_display_flush(vc->wl.display) < 0 && errno != EAGAIN) {
         wl_display_cancel_read(vc->wl.display);
         return;
      }
      if (poll(fds, 1, 0) > 0) {
         wl_display_read_events(vc->wl.display);
         wl_display_dispatch_pending(vc->wl.display);
      } else {
         wl_display_cancel_read(vc->wl.display);
      }

      result = vkAcquireNextImageKHR(vc->device, vc->swap_chain, 60,
                                     vc->semaphore, VK_NULL_HANDLE, &index);
      if (result == VK_SUBOPTIMAL_KHR ||
          result == VK_ERROR_OUT_OF_DATE_KHR) {
         recreate_swapchain(vc);
         continue;
      } else if (result == VK_NOT_READY ||
                 result == VK_TIMEOUT) {
         continue;
      } else if (result != VK_SUCCESS) {
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
