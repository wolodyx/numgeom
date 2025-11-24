#include "gpumemory.h"

#include <cassert>
#include <format>
#include <stdexcept>


namespace {;
uint32_t getHostVisibleMemoryIndex(VkPhysicalDevice physicalDevice)
{
    uint32_t hostVisibleMemIndex = 0;
    bool hostVisibleMemIndexSet = false;
    VkPhysicalDeviceMemoryProperties memProps;
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProps);
    for(uint32_t i = 0; i < memProps.memoryTypeCount; ++i) {
        VkMemoryType mt = memProps.memoryTypes[i];
        VkFlags hostVisibleAndCoherent =
              VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
            | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
        if((mt.propertyFlags & hostVisibleAndCoherent) != hostVisibleAndCoherent)
            continue;
        if(!hostVisibleMemIndexSet || mt.propertyFlags & VK_MEMORY_PROPERTY_HOST_CACHED_BIT) {
            hostVisibleMemIndexSet = true;
            hostVisibleMemIndex = i;
        }
    }
    return hostVisibleMemIndex;
}
}


GpuMemory::GpuMemory(
    const VulkanState& state,
    size_t bytesCount)
: m_vkState(state)
{
    assert(state.device != VK_NULL_HANDLE);
    assert(state.physicalDevice != VK_NULL_HANDLE);

    if(bytesCount != 0)
        this->resize(bytesCount);
}


GpuMemory::~GpuMemory()
{
    this->clear();
}


void GpuMemory::resize(size_t bytesCount)
{
    if(bytesCount <= m_bytesCount)
        return;

    this->clear();

    VkResult r;

    VkBufferCreateInfo bufferInfo {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size = bytesCount,
        .usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT
               | VK_BUFFER_USAGE_INDEX_BUFFER_BIT
               | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT
    };
    r = vkCreateBuffer(
        m_vkState.device,
        &bufferInfo,
        nullptr,
        &m_buf
    );
    if(r != VK_SUCCESS)
        throw std::runtime_error("Failed to create buffer");

    VkMemoryRequirements memReq;
    vkGetBufferMemoryRequirements(m_vkState.device, m_buf, &memReq);

    VkMemoryAllocateInfo allocInfo {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize = memReq.size,
        .memoryTypeIndex = getHostVisibleMemoryIndex(m_vkState.physicalDevice)
    };
    r = vkAllocateMemory(m_vkState.device, &allocInfo, nullptr, &m_bufMem);
    if(r != VK_SUCCESS)
        throw std::runtime_error("Failed to allocate memory");

    r = vkBindBufferMemory(m_vkState.device, m_buf, m_bufMem, 0);
    if(r != VK_SUCCESS) {
        auto msg = std::format(
            "Failed to bind buffer memory: {}",
            static_cast<int>(r)
        );
        throw std::runtime_error(msg);
    }

    m_bytesCount = bytesCount;
}


void GpuMemory::clear()
{
    if(m_buf != VK_NULL_HANDLE) {
        vkDestroyBuffer(m_vkState.device, m_buf, nullptr);
        m_buf = VK_NULL_HANDLE;
    }

    if(m_bufMem != VK_NULL_HANDLE) {
        vkFreeMemory(m_vkState.device, m_bufMem, nullptr);
        m_bufMem = VK_NULL_HANDLE;
    }

    m_bytesCount = 0;
}


GpuMemory::Mapping GpuMemory::access()
{
    if(m_bytesCount == 0)
        return Mapping();
    return Mapping(m_vkState.device, m_bufMem);
}


GpuMemory::Mapping::Mapping()
{
}


GpuMemory::Mapping::Mapping(VkDevice device, VkDeviceMemory memory)
    : m_dev(device), m_bufMem(memory)
{
}


GpuMemory::Mapping::~Mapping()
{
    vkUnmapMemory(m_dev, m_bufMem);
}


std::byte* GpuMemory::Mapping::data(size_t offset, size_t size)
{
    vkUnmapMemory(m_dev, m_bufMem);

    std::byte* data;
    VkResult r = vkMapMemory(
        m_dev,
        m_bufMem,
        offset,
        size,
        0,
        reinterpret_cast<void **>(&data)
    );
    if(r != VK_SUCCESS) {
        auto msg = std::format(
            "Failed to map memory: {}",
            static_cast<int>(r)
        );
        throw std::runtime_error(msg);
    }
    return data;
}
