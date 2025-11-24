#ifndef numgeom_app_vulkanwindowrenderer_h
#define numgeom_app_vulkanwindowrenderer_h

// Заголовочный файл vulkan следует подключать раньше, чем те же самые файлы Qt,
// чтобы воспользоваться непосредственно функциями vulkan.
#include <vulkan/vulkan.h>

#include <QVulkanWindowRenderer>

#include "numgeom/trimesh.h"

class GpuMemory;


class VulkanWindowRenderer : public QVulkanWindowRenderer
{
public:
    VulkanWindowRenderer(QVulkanWindow* w);

    void initResources() override;
    void initSwapChainResources() override;
    void releaseSwapChainResources() override;
    void releaseResources() override;

    void startNextFrame() override;

    void setModel(CTriMesh::Ptr);

private:
    VkShaderModule createShader(const uint32_t*, size_t);
    void updateModel(CTriMesh::Ptr);

private:
    QVulkanWindow* m_window;

    bool m_beUpdateModel;
    CTriMesh::Ptr m_model;

    GpuMemory* m_mem = nullptr;
    VkDescriptorBufferInfo m_uniformBufInfo[QVulkanWindow::MAX_CONCURRENT_FRAME_COUNT];

    VkDescriptorPool m_descPool = VK_NULL_HANDLE;
    VkDescriptorSetLayout m_descSetLayout = VK_NULL_HANDLE;
    VkDescriptorSet m_descSet[QVulkanWindow::MAX_CONCURRENT_FRAME_COUNT] = {VK_NULL_HANDLE};

    VkPipelineCache m_pipelineCache = VK_NULL_HANDLE;
    VkPipelineLayout m_pipelineLayout = VK_NULL_HANDLE;
    VkPipeline m_pipeline = VK_NULL_HANDLE;

    QMatrix4x4 m_proj;
    float m_rotation = 0.0f;
    VkDeviceSize m_indexOffset;
    uint32_t m_indexCount;
};

#endif // !numgeom_app_vulkanwindowrenderer_h
