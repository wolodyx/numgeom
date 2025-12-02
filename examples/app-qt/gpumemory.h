#ifndef numgeom_app_gpumemory_h
#define numgeom_app_gpumemory_h

#include <cstddef>

#include <vulkan/vulkan.h>


struct VulkanState
{
    VkDevice device = VK_NULL_HANDLE;
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
};


class GpuMemory
{
public:

    class Mapping
    {
    public:
        ~Mapping();
        std::byte* data(size_t offset = 0, size_t size = VK_WHOLE_SIZE);
    private:
        friend class GpuMemory;
        Mapping();
        Mapping(VkDevice,VkDeviceMemory);
    private:
        VkDevice m_dev = VK_NULL_HANDLE;
        VkDeviceMemory m_bufMem = VK_NULL_HANDLE;
    };


public:

    GpuMemory(const VulkanState& state, size_t bytesCount = 0);

    ~GpuMemory();

    void resize(size_t bytesCount);

    void clear();

    VkBuffer buffer() const { return m_buf; }

    Mapping access();


private:

    GpuMemory(const GpuMemory&) = delete;

    GpuMemory& operator=(const GpuMemory&) = delete;


private:

    const VulkanState m_vkState;

    //! Количество выделенной памяти в байтах.
    size_t m_bytesCount = 0;

    VkDeviceMemory m_bufMem = VK_NULL_HANDLE;
    VkBuffer m_buf = VK_NULL_HANDLE;
};
#endif // !numgeom_app_gpumemory_h
