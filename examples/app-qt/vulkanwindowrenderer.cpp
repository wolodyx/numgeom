#include "vulkanwindowrenderer.h"

#include <array>
#include <vector>

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "glm/gtx/rotate_vector.hpp"

#include "numgeom/loadfromvtk.h"

#include "gpumemory.h"


static const int UNIFORM_DATA_SIZE = 16 * sizeof(float);


static inline VkDeviceSize aligned(VkDeviceSize v,VkDeviceSize byteAlign)
{
    return (v + byteAlign - 1) & ~(byteAlign - 1);
}


VulkanWindowRenderer::VulkanWindowRenderer(QVulkanWindow* w)
    : m_window(w)
{
    m_model = LoadTriMeshFromVtk("d:/projects/numgeom/tests/data/polydata-cube.vtk");
    m_beUpdateModel = true;

    bool msaa = true;
    if(msaa) {
        const QVector<int> counts = w->supportedSampleCounts();
        qDebug() << "Supported sample counts:" << counts;
        for(int s = 16; s >= 4; s /= 2) {
            if(counts.contains(s)) {
                qDebug("Requesting sample count %d",s);
                m_window->setSampleCount(s);
                break;
            }
        }
    }
}


VkShaderModule VulkanWindowRenderer::createShader(
    const uint32_t* pCode,
    size_t codeSize
) {
    VkShaderModuleCreateInfo shaderInfo {
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .codeSize = static_cast<uint32_t>(codeSize),
        .pCode = pCode,
    };
    VkShaderModule shaderModule;
    VkResult err = vkCreateShaderModule(m_window->device(),&shaderInfo,nullptr,&shaderModule);
    if(err != VK_SUCCESS) {
        qWarning("Failed to create shader module: %d",err);
        return VK_NULL_HANDLE;
    }

    return shaderModule;
}


void VulkanWindowRenderer::initResources()
{
    qDebug("initResources");

    VkDevice dev = m_window->device();
    const int concurrentFrameCount = m_window->concurrentFrameCount();

    m_mem = new GpuMemory(
        VulkanState{
            dev,
            m_window->physicalDevice()
        }
    );

    // Set up descriptor set and its layout.
    VkDescriptorPoolSize descPoolSizes {
        .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .descriptorCount = static_cast<uint32_t>(concurrentFrameCount)
    };
    VkDescriptorPoolCreateInfo descPoolInfo {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .maxSets = static_cast<uint32_t>(concurrentFrameCount),
        .poolSizeCount = 1,
        .pPoolSizes = &descPoolSizes,
    };
    VkResult err = vkCreateDescriptorPool(dev,&descPoolInfo,nullptr,&m_descPool);
    if(err != VK_SUCCESS)
        qFatal("Failed to create descriptor pool: %d",err);

    VkDescriptorSetLayoutBinding layoutBinding = {
        .binding = 0,
        .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .descriptorCount = 1,
        .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
    };
    VkDescriptorSetLayoutCreateInfo descLayoutInfo {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .bindingCount = 1,
        .pBindings = &layoutBinding
    };
    err = vkCreateDescriptorSetLayout(dev,&descLayoutInfo,nullptr,&m_descSetLayout);
    if(err != VK_SUCCESS)
        qFatal("Failed to create descriptor set layout: %d",err);

    this->updateModel(m_model);
    m_beUpdateModel = false;

    VkVertexInputBindingDescription vertexBindingDesc = {
        .binding = 0,
        .stride = 3 * sizeof(float),
        .inputRate = VK_VERTEX_INPUT_RATE_VERTEX
    };
    VkVertexInputAttributeDescription vertexAttrDesc[] = {
        { // position
            0, // location
            0, // binding
            VK_FORMAT_R32G32B32_SFLOAT,
            0
        },
    };

    VkPipelineVertexInputStateCreateInfo vertexInputInfo {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .vertexBindingDescriptionCount = 1,
        .pVertexBindingDescriptions = &vertexBindingDesc,
        .vertexAttributeDescriptionCount = 1,
        .pVertexAttributeDescriptions = vertexAttrDesc,
    };

    // Pipeline cache
    VkPipelineCacheCreateInfo pipelineCacheInfo {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO
    };
    err = vkCreatePipelineCache(dev,&pipelineCacheInfo,nullptr,&m_pipelineCache);
    if(err != VK_SUCCESS)
        qFatal("Failed to create pipeline cache: %d",err);

    // Pipeline layout
    VkPipelineLayoutCreateInfo pipelineLayoutInfo {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .setLayoutCount = 1,
        .pSetLayouts = &m_descSetLayout,
    };
    err = vkCreatePipelineLayout(dev,&pipelineLayoutInfo,nullptr,&m_pipelineLayout);
    if(err != VK_SUCCESS)
        qFatal("Failed to create pipeline layout: %d",err);

    // Shaders
    static uint32_t vs_spirv_source[] = {
        #include "color.vert.spv.h"
    };

    static uint32_t fs_spirv_source[] = {
        #include "color.frag.spv.h"
    };
    VkShaderModule vertShaderModule = createShader(vs_spirv_source, sizeof(vs_spirv_source));
    VkShaderModule fragShaderModule = createShader(fs_spirv_source, sizeof(fs_spirv_source));

    // Graphics pipeline
    VkGraphicsPipelineCreateInfo pipelineInfo;
    memset(&pipelineInfo,0,sizeof(pipelineInfo));
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;

    VkPipelineShaderStageCreateInfo shaderStages[2] = {
        {
            VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            nullptr,
            0,
            VK_SHADER_STAGE_VERTEX_BIT,
            vertShaderModule,
            "main",
            nullptr
        },
        {
            VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            nullptr,
            0,
            VK_SHADER_STAGE_FRAGMENT_BIT,
            fragShaderModule,
            "main",
            nullptr
        }
    };
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;

    pipelineInfo.pVertexInputState = &vertexInputInfo;

    VkPipelineInputAssemblyStateCreateInfo ia {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
    };
    pipelineInfo.pInputAssemblyState = &ia;

    // The viewport and scissor will be set dynamically via vkCmdSetViewport/Scissor.
    // This way the pipeline does not need to be touched when resizing the window.
    VkPipelineViewportStateCreateInfo vp {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .viewportCount = 1,
        .scissorCount = 1,
    };
    pipelineInfo.pViewportState = &vp;

    VkPipelineRasterizationStateCreateInfo rs {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .polygonMode = VK_POLYGON_MODE_FILL,
        .cullMode = VK_CULL_MODE_NONE, // we want the back face as wel,
        .frontFace = VK_FRONT_FACE_CLOCKWISE,
        .lineWidth = 1.0f,
    };
    pipelineInfo.pRasterizationState = &rs;

    VkPipelineMultisampleStateCreateInfo ms {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        // Enable multisampling.
        .rasterizationSamples = m_window->sampleCountFlagBits(),
    };
    pipelineInfo.pMultisampleState = &ms;

    VkPipelineDepthStencilStateCreateInfo ds {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
        .depthTestEnable = VK_TRUE,
        .depthWriteEnable = VK_TRUE,
        .depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL,
    };
    pipelineInfo.pDepthStencilState = &ds;

    // no blend, write out all of rgba
    VkPipelineColorBlendAttachmentState att{
        .colorWriteMask = 0xF
    };
    VkPipelineColorBlendStateCreateInfo cb {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .attachmentCount = 1,
        .pAttachments = &att,
    };
    pipelineInfo.pColorBlendState = &cb;

    VkDynamicState dynEnable[] = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
    VkPipelineDynamicStateCreateInfo dyn {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
        .dynamicStateCount = sizeof(dynEnable) / sizeof(VkDynamicState),
        .pDynamicStates = dynEnable,
    };
    pipelineInfo.pDynamicState = &dyn;

    pipelineInfo.layout = m_pipelineLayout;
    pipelineInfo.renderPass = m_window->defaultRenderPass();

    err = vkCreateGraphicsPipelines(dev,m_pipelineCache,1,&pipelineInfo,nullptr,&m_pipeline);
    if(err != VK_SUCCESS)
        qFatal("Failed to create graphics pipeline: %d",err);

    if(vertShaderModule)
        vkDestroyShaderModule(dev,vertShaderModule,nullptr);
    if(fragShaderModule)
        vkDestroyShaderModule(dev,fragShaderModule,nullptr);
}

void VulkanWindowRenderer::initSwapChainResources()
{
    qDebug("initSwapChainResources");

    // Формируем проективную матрицу.
    // adjust for Vulkan-OpenGL clip space differences
    QMatrix4x4 ccm = m_window->clipCorrectionMatrix();
    const float* src = ccm.constData();
    float* dst = glm::value_ptr(m_projectionMatrix);
    for(int i = 0; i < 16; ++i)
        dst[i] = src[i];

    const QSize sz = m_window->swapChainImageSize();
    glm::mat4 perspective = glm::perspective(
        glm::radians(45.0f),
        sz.width() / (float)sz.height(),
        0.01f,
        100.0f
    );
    m_projectionMatrix *= perspective;
}

void VulkanWindowRenderer::releaseSwapChainResources()
{
    qDebug("releaseSwapChainResources");
}

void VulkanWindowRenderer::releaseResources()
{
    qDebug("releaseResources");

    VkDevice dev = m_window->device();

    if(m_pipeline) {
        vkDestroyPipeline(dev,m_pipeline,nullptr);
        m_pipeline = VK_NULL_HANDLE;
    }

    if(m_pipelineLayout) {
        vkDestroyPipelineLayout(dev,m_pipelineLayout,nullptr);
        m_pipelineLayout = VK_NULL_HANDLE;
    }

    if(m_pipelineCache) {
        vkDestroyPipelineCache(dev,m_pipelineCache,nullptr);
        m_pipelineCache = VK_NULL_HANDLE;
    }

    if(m_descSetLayout) {
        vkDestroyDescriptorSetLayout(dev,m_descSetLayout,nullptr);
        m_descSetLayout = VK_NULL_HANDLE;
    }

    if(m_descPool) {
        vkDestroyDescriptorPool(dev,m_descPool,nullptr);
        m_descPool = VK_NULL_HANDLE;
    }

    delete m_mem;
    m_mem = nullptr;
}


namespace {;

float computeCameraDistance(float radius, float fovY, float aspect)
{
    float distanceVertical = radius / std::tan(fovY * 0.5f);
    float fovX = 2.0f * std::atan(std::tan(fovY * 0.5f) * aspect);
    float distanceHorizontal = radius / std::tan(fovX * 0.5f);
    return std::max(distanceVertical, distanceHorizontal) * 1.1f;
}


glm::mat4 calcViewMatrix(
    const glm::vec3& worldMin,
    const glm::vec3& worldMax,
    float aspect)
{
    glm::vec3 size = worldMax - worldMin;
    float radius = glm::length(size) * 0.5f;
    glm::vec3 center = (worldMin + worldMax) * 0.5f;
    float distance = computeCameraDistance(radius, glm::radians(45.0), aspect);
    glm::vec3 cameraPos = center + glm::vec3(0.0f, 0.0f, distance);
    return glm::lookAt(cameraPos, center, glm::vec3(0.0f,1.0f,0.0f));
}
}


void VulkanWindowRenderer::startNextFrame()
{
    if(m_beUpdateModel) {
        this->updateModel(m_model);
        m_beUpdateModel = false;
    }

    VkDevice dev = m_window->device();
    VkCommandBuffer cmdBuf = m_window->currentCommandBuffer();
    const QSize sz = m_window->swapChainImageSize();

    VkClearColorValue clearColor = {0, 0, 0, 1};
    VkClearValue clearValues[3] = {};
    clearValues[0].color = clearValues[2].color = clearColor;
    clearValues[1].depthStencil = VkClearDepthStencilValue{
        .depth = 1,
        .stencil = 0
    };

    VkRenderPassBeginInfo rpBeginInfo {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .renderPass = m_window->defaultRenderPass(),
        .framebuffer = m_window->currentFramebuffer(),
        .renderArea = VkRect2D {
            .extent = VkExtent2D {
                .width = static_cast<uint32_t>(sz.width()),
                .height = static_cast<uint32_t>(sz.height())
            },
        },
        .clearValueCount = static_cast<uint32_t>(m_window->sampleCountFlagBits() > VK_SAMPLE_COUNT_1_BIT ? 3 : 2),
        .pClearValues = clearValues,
    };
    vkCmdBeginRenderPass(cmdBuf,&rpBeginInfo,VK_SUBPASS_CONTENTS_INLINE);

    // Передаем вершинному шейдеру матрицу MVP.
    {
        const QSize sz = m_window->swapChainImageSize();
        float aspect = sz.width() / static_cast<float>(sz.height());
        glm::mat4 viewMatrix = calcViewMatrix(m_modelMin, m_modelMax, aspect);
        viewMatrix = glm::rotate(
            viewMatrix,
            glm::radians(m_rotation),
            glm::vec3(0.0f, 1.0f, 0.0f)
        );
        glm::mat4 mvp = m_projectionMatrix * viewMatrix;

        auto map = m_mem->access();
        std::byte* p = map.data(
            m_uniformBufInfo[m_window->currentFrame()].offset,
            UNIFORM_DATA_SIZE
        );
        memcpy(
            p,
            glm::value_ptr(mvp),
            16 * sizeof(float)
        );
    }

    // Not exactly a real animation system, just advance on every frame for now.
    m_rotation += 1.0f;

    vkCmdBindPipeline(cmdBuf,VK_PIPELINE_BIND_POINT_GRAPHICS,m_pipeline);
    vkCmdBindDescriptorSets(cmdBuf,VK_PIPELINE_BIND_POINT_GRAPHICS,m_pipelineLayout,0,1,
        &m_descSet[m_window->currentFrame()],0,nullptr);
    VkDeviceSize vbOffset = 0;
    VkBuffer buf = m_mem->buffer();
    vkCmdBindVertexBuffers(cmdBuf, 0, 1, &buf, &vbOffset);

    vkCmdBindIndexBuffer(
        cmdBuf,
        m_mem->buffer(),
        m_indexOffset,
        VK_INDEX_TYPE_UINT32
    );

    VkViewport viewport {
        .x = 0,
        .y = 0,
        .width = static_cast<float>(sz.width()),
        .height = static_cast<float>(sz.height()),
        .minDepth = 0,
        .maxDepth = 1,
    };
    vkCmdSetViewport(cmdBuf, 0, 1, &viewport);

    VkRect2D scissor {
        .offset = VkOffset2D {
            .x = 0,
            .y = 0
        },
        .extent = VkExtent2D {
            .width = static_cast<uint32_t>(viewport.width),
            .height = static_cast<uint32_t>(viewport.height),
        }
    };
    vkCmdSetScissor(cmdBuf, 0, 1, &scissor);

    vkCmdDrawIndexed(cmdBuf, m_indexCount, 1, 0, 0, 0);

    vkCmdEndRenderPass(cmdBuf);

    m_window->frameReady();
    m_window->requestUpdate();
}


void VulkanWindowRenderer::setModel(CTriMesh::Ptr mesh)
{
    m_model = mesh;
    m_beUpdateModel = true;
}


void VulkanWindowRenderer::updateModel(CTriMesh::Ptr mesh)
{
    // Note that the vertex data and the projection matrix assume OpenGL. With
    // Vulkan Y is negated in clip space and the near/far plane is at 0/1 instead
    // of -1/1. These will be corrected for by an extra transformation when
    // calculating the modelview-projection matrix.

    VkDevice dev = m_window->device();

    // Prepare the vertex and uniform data. The vertex data will never
    // change so one buffer is sufficient regardless of the value of
    // QVulkanWindow::CONCURRENT_FRAME_COUNT. Uniform data is changing per
    // frame however so active frames have to have a dedicated copy.

    // Use just one memory allocation and one buffer. We will then specify the
    // appropriate offsets for uniform buffers in the VkDescriptorBufferInfo.
    // Have to watch out for
    // VkPhysicalDeviceLimits::minUniformBufferOffsetAlignment, though.

    // The uniform buffer is not strictly required in this example, we could
    // have used push constants as well since our single matrix (64 bytes) fits
    // into the spec mandated minimum limit of 128 bytes. However, once that
    // limit is not sufficient, the per-frame buffers, as shown below, will
    // become necessary.

    const int concurrentFrameCount = m_window->concurrentFrameCount();
    const VkPhysicalDeviceLimits *pdevLimits = &m_window->physicalDeviceProperties()->limits;
    const VkDeviceSize uniAlign = pdevLimits->minUniformBufferOffsetAlignment;
    qDebug("uniform buffer offset alignment is %u",(uint)uniAlign);
    // В памяти, связанной с буфером, выровненные по `uniAlign`
    // данные размещаются следующим образом:
    // * вершинные данные;
    // * индексы;
    // * uniform-переменные для каждого фрейма.
    size_t nodesCount = mesh->NbNodes();
    size_t cellsCount = mesh->NbCells();
    const VkDeviceSize vertexAllocSize =
        aligned(3 * sizeof(float) * nodesCount, uniAlign);
    const VkDeviceSize indexAllocSize =
        aligned(3 * sizeof(uint32_t) * cellsCount, uniAlign);
    const VkDeviceSize uniformAllocSize = aligned(UNIFORM_DATA_SIZE, uniAlign);
    m_indexOffset = vertexAllocSize;
    m_indexCount = 3 * cellsCount;
    m_mem->resize(
          vertexAllocSize
        + indexAllocSize
        + concurrentFrameCount * uniformAllocSize
    );
    VkBufferCreateInfo bufInfo {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size = vertexAllocSize
              + indexAllocSize
              + concurrentFrameCount * uniformAllocSize,
        .usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT
               | VK_BUFFER_USAGE_INDEX_BUFFER_BIT
               | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT
    };

    {
        auto map = m_mem->access();
        std::byte* data = map.data();

        // Копируем в память координаты вершин.
        float* xyzData = reinterpret_cast<float*>(data);
        for(size_t i = 0; i < nodesCount; ++i) {
            gp_Pnt pt = mesh->GetNode(i);
            *xyzData = static_cast<float>(pt.X()); ++xyzData;
            *xyzData = static_cast<float>(pt.Y()); ++xyzData;
            *xyzData = static_cast<float>(pt.Z()); ++xyzData;
        }

        uint32_t* ijkData = reinterpret_cast<uint32_t*>(data + vertexAllocSize);
        for(size_t i = 0; i < cellsCount; ++i) {
            const auto& tr = mesh->GetCell(i);
            *ijkData = static_cast<uint32_t>(tr.na); ++ijkData;
            *ijkData = static_cast<uint32_t>(tr.nb); ++ijkData;
            *ijkData = static_cast<uint32_t>(tr.nc); ++ijkData;
        }

        QMatrix4x4 ident;
        memset(m_uniformBufInfo, 0, sizeof(m_uniformBufInfo));
        VkDeviceSize uniformStartOffset = vertexAllocSize + indexAllocSize;
        for(int i = 0; i < concurrentFrameCount; ++i) {
            const VkDeviceSize offset = uniformStartOffset + i * uniformAllocSize;
            memcpy(data + offset, ident.constData(), 16 * sizeof(float));
            m_uniformBufInfo[i].buffer = m_mem->buffer();
            m_uniformBufInfo[i].offset = offset;
            m_uniformBufInfo[i].range = uniformAllocSize;
        }
    }

    // Освобождаем пул от ранее выделенных наборов дескрипторов.
    vkResetDescriptorPool(
        dev,
        m_descPool,
        0
    );

    // Создаем наборы дескрипторов и связываем их с памятью буфера.
    for(int i = 0; i < concurrentFrameCount; ++i) {
        VkDescriptorSetAllocateInfo descSetAllocInfo {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
            .descriptorPool = m_descPool,
            .descriptorSetCount = 1,
            .pSetLayouts = &m_descSetLayout
        };
        VkResult r = vkAllocateDescriptorSets(
            dev,
            &descSetAllocInfo,
            &m_descSet[i]
        );
        if(r != VK_SUCCESS) {
            auto msg = std::format(
                "Failed to allocate descriptor set: {}",
                static_cast<int>(r)
            );
            throw std::runtime_error(msg);
        }

        VkWriteDescriptorSet descWrite {
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet = m_descSet[i],
            .descriptorCount = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .pBufferInfo = &m_uniformBufInfo[i],
        };
        vkUpdateDescriptorSets(
            dev,
            1,
            &descWrite,
            0,
            nullptr
        );
    }

    m_modelMin = glm::vec3(+FLT_MAX, +FLT_MAX, +FLT_MAX);
    m_modelMax = glm::vec3(-FLT_MAX, -FLT_MAX, -FLT_MAX);
    for(size_t i = 0; i < mesh->NbNodes(); ++i) {
        gp_Pnt pt = mesh->GetNode(i);
        m_modelMin.x = std::min(m_modelMin.x, static_cast<float>(pt.X()));
        m_modelMin.y = std::min(m_modelMin.y, static_cast<float>(pt.Y()));
        m_modelMin.z = std::min(m_modelMin.z, static_cast<float>(pt.Z()));
        m_modelMax.x = std::max(m_modelMax.x, static_cast<float>(pt.X()));
        m_modelMax.y = std::max(m_modelMax.y, static_cast<float>(pt.Y()));
        m_modelMax.z = std::max(m_modelMax.z, static_cast<float>(pt.Z()));
    }
}
