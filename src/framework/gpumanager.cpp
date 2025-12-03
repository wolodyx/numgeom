#include "numgeom/gpumanager.h"

#include <cassert>
#include <vector>

#define VK_PROTOTYPES
#include "vulkan/vulkan.h"

#define MAX_NUM_IMAGES 5


namespace
{;
//! Состояние vulkan и его объектов.
struct VulkanState
{
    VkInstance instance = VK_NULL_HANDLE;
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkPhysicalDeviceMemoryProperties memoryProperties;
    VkDevice device = VK_NULL_HANDLE;
    VkSwapchainKHR swap_chain = VK_NULL_HANDLE;
    VkSemaphore semaphore = VK_NULL_HANDLE;
    VkQueue queue = VK_NULL_HANDLE;
};
}


struct GpuManager::Impl
{
    Application* app;
    VulkanState vulkanState;
};


GpuManager::GpuManager(Application* app, QWindow* window)
{
    m_pimpl = new Impl {
        .app = app,
    };
}


GpuManager::~GpuManager()
{
    delete m_pimpl;
}


namespace
{;
/*
void mainloop(
    VulkanState* vc,
    xcb_connection* xcb)
{
    VkResult result = VK_SUCCESS;
    bool quit = false;
    while(!quit) {
        //process_xcb_event(xcb);

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
*/
VkPhysicalDevice selectPhysicalDevice(VkInstance instance)
{
    VkResult r = VK_SUCCESS;
    uint32_t devicesCount = 0;
    r = vkEnumeratePhysicalDevices(instance, &devicesCount, nullptr);
    if(r != VK_SUCCESS)
        return VK_NULL_HANDLE;
    if(devicesCount == 0)
        return VK_NULL_HANDLE;
    std::vector<VkPhysicalDevice> devices(devicesCount);
    r = vkEnumeratePhysicalDevices(instance, &devicesCount, devices.data());
    if(r != VK_SUCCESS)
        return VK_NULL_HANDLE;
    return devices[0];
}

uint32_t selectQueueFamily(VkPhysicalDevice device)
{
    uint32_t count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &count, nullptr);
    if(count == 0)
        return -1;
    std::vector<VkQueueFamilyProperties> properties(count);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &count, properties.data());
    uint32_t family = -1;
    for(uint32_t i = 0; i < count; ++i) {
        if(properties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            family = i;
            break;
        }
    }
    return family;
}
}


bool GpuManager::initialize()
{
    VulkanState* state = &m_pimpl->vulkanState;
    VkResult r = VK_SUCCESS;

    std::vector<const char*> extensionNames {
        VK_KHR_SURFACE_EXTENSION_NAME,
    };

    // Создаем экземпляр vulkan.
    VkApplicationInfo applicationInfo {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pApplicationName = "app-qt",
        .applicationVersion = VK_MAKE_VERSION(0, 0, 1),
        .pEngineName = "numgeom",
        .engineVersion = VK_MAKE_VERSION(0, 0, 1),
        .apiVersion = VK_MAKE_VERSION(1, 1, 0),
    };
    VkInstanceCreateInfo ci_instance {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pApplicationInfo = &applicationInfo,
        .enabledLayerCount = 0,
        .ppEnabledLayerNames = nullptr,
        .enabledExtensionCount = static_cast<uint32_t>(extensionNames.size()),
        .ppEnabledExtensionNames = extensionNames.data()
    };
    r = vkCreateInstance(&ci_instance, nullptr, &state->instance);
    if(r != VK_SUCCESS)
        return false;

    // Выбираем физическое устройство.
    state->physicalDevice = selectPhysicalDevice(state->instance);

    // Получаем свойства памятий.
    vkGetPhysicalDeviceMemoryProperties(state->physicalDevice, &state->memoryProperties);

    // Выбираем семейство очередей.
    uint32_t queueFamilyIndex = selectQueueFamily(state->physicalDevice);

    // Создаем логическое устройство.
    float queuePriorities[] = { 1.0f };
    VkDeviceQueueCreateInfo ci_queue {
        .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        .queueFamilyIndex = queueFamilyIndex,
        .queueCount = 1,
        .pQueuePriorities = queuePriorities,
    };
    std::vector<const char*> deviceExtensionNames = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };
    VkDeviceCreateInfo ci_device {
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .queueCreateInfoCount = 1,
        .pQueueCreateInfos = &ci_queue,
        .enabledExtensionCount = static_cast<uint32_t>(deviceExtensionNames.size()),
        .ppEnabledExtensionNames = deviceExtensionNames.data(),
    };
    r = vkCreateDevice(state->physicalDevice, &ci_device, nullptr, &state->device);
    if(r != VK_SUCCESS)
        return false;

    // Извлекаем очередь.
    VkDeviceQueueInfo2 queueInfo {
        .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_INFO_2,
        .queueFamilyIndex = 0,
        .queueIndex = 0,
    };
    vkGetDeviceQueue2(state->device, &queueInfo, &state->queue);

    return true;
}


bool GpuManager::update()
{
    // Следует ли обновлять сцену?
    // Следует ли обновлять матрицы вида и перспективы?
    return false;
}
