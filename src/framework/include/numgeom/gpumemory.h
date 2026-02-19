#ifndef numgeom_framework_gpumemory_h
#define numgeom_framework_gpumemory_h

#include <cstddef>

#include <vulkan/vulkan.h>


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

    GpuMemory(
        VkPhysicalDevice physicalDevice,
        VkDevice logicalDevice,
        VkBufferUsageFlags usage,
        size_t bytesCount = 0
    );

    ~GpuMemory();

    void resize(size_t bytesCount);

    void clear();

    VkBuffer buffer() const { return m_buf; }

    Mapping access();

    //! Возвращает размер в байтах блока uniform-памяти, вмещающей данных
    //! размером `bytesCount`.
    VkDeviceSize uniformBlockSize(VkDeviceSize bytesCount) const;


private:

    GpuMemory(const GpuMemory&) = delete;

    GpuMemory& operator=(const GpuMemory&) = delete;


private:

    VkPhysicalDevice m_physicalDevice;
    VkDevice m_device;
    VkBufferUsageFlags m_usage;

    //! Количество выделенной памяти в байтах.
    size_t m_bytesCount = 0;

    VkDeviceMemory m_bufMem = VK_NULL_HANDLE;
    VkBuffer m_buf = VK_NULL_HANDLE;
};
#endif // !numgeom_framework_gpumemory_h
