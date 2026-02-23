#include "numgeom/gpumanager.h"

#include <array>
#include <cassert>
#include <format>
#include <mutex>
#include <optional>
#include <set>
#include <vector>

#if defined(_WIN32)
#  define USE_PLATFORM_WIN32_KHR
#elif defined(__linux__)
#  define USE_PLATFORM_XCB_KHR
#endif

#define VK_PROTOTYPES
#include "vulkan/vulkan.h"

#if defined(USE_PLATFORM_XCB_KHR)
#  include "xcb/xcb.h"
#  include "vulkan/vulkan_xcb.h"
#endif
#if defined(_WIN32)
#  include "windows.h"
#  include "vulkan/vulkan_win32.h"
#endif

#include "glm/glm.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "glm/gtx/string_cast.hpp"

#include "boost/log/trivial.hpp"

#include "numgeom/application.h"
#include "numgeom/gpumemory.h"

#include "meshchunk.h"
#include "vkutilities.h"

// Количество изображений в обращении.
#define MAX_NUM_IMAGES 5
// Количество фреймов -- одновременно обрабатываемых GPU изображений.
#define MAX_NUM_FRAMES 2

#ifdef NDEBUG
#  undef ENABLE_VALIDATION_LAYERS
#else
#  define ENABLE_VALIDATION_LAYERS
#endif


namespace
{;
struct ImageResources
{
    //! Изображение для показа.
    VkImage image = VK_NULL_HANDLE;

    //! Вид изображения для показа.
    VkImageView imageView = VK_NULL_HANDLE;

    //! Сэмплированное изображение, создается только при
    //! `VulkanState::sampleCount != VK_SAMPLE_COUNT_1_BIT`.
    VkImage msaaImage = VK_NULL_HANDLE;

    // Вид сэмплированного изображения.
    VkImageView msaaImageView = VK_NULL_HANDLE;

    //! Фреймбуфер.
    VkFramebuffer framebuffer = VK_NULL_HANDLE;
};


struct FrameResources
{
    VkDescriptorSet descSet = VK_NULL_HANDLE;
    VkCommandBuffer cmdBuf = VK_NULL_HANDLE;
    VkSemaphore imageSemaphore = VK_NULL_HANDLE;
    VkSemaphore drawSemaphore = VK_NULL_HANDLE;
    VkFence cmdFence = VK_NULL_HANDLE;
    bool cmdFenceWaitable = false;
};


//! Состояние vulkan и его объектов.
struct VulkanState
{
    //! Признак остановки рендеринга.
    bool stopRendering = true;

    VkInstance instance = VK_NULL_HANDLE;

    VkDebugUtilsMessengerEXT debugMessenger;

    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;

    VkPhysicalDeviceMemoryProperties memoryProperties;

    VkDevice device = VK_NULL_HANDLE;

    std::optional<uint32_t> graphicsFamilyIndex, presentFamilyIndex;

    VkQueue queue = VK_NULL_HANDLE;

    VkSwapchainKHR swapchain = VK_NULL_HANDLE;

    VkSurfaceKHR surface = VK_NULL_HANDLE;
    bool externalSurface = false;

    VkFormat imageFormat = VK_FORMAT_UNDEFINED;

    VkColorSpaceKHR colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;

    VkFormat depthStencilFormat = VK_FORMAT_D24_UNORM_S8_UINT;

    VkPresentModeKHR presentMode = VK_PRESENT_MODE_FIFO_KHR;

    uint32_t minImageCount = 2;

    VkSampleCountFlagBits sampleCount = VK_SAMPLE_COUNT_1_BIT;

    VkRenderPass renderpass = VK_NULL_HANDLE;

    VkPipeline pipeline = VK_NULL_HANDLE;

    VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;

    VkCommandPool cmdPool = VK_NULL_HANDLE;

    uint32_t imageCount = 0;

    uint32_t frameCount = MAX_NUM_FRAMES;

    VkExtent2D imageExtent;

    std::array<ImageResources,MAX_NUM_IMAGES> imageRes;

    std::array<FrameResources,MAX_NUM_FRAMES> frameRes;

    //! Изображение буфера глубины.
    VkImage depthStencilImage = VK_NULL_HANDLE;
    VkDeviceMemory depthStencilMem = VK_NULL_HANDLE;
    VkImageView depthStencilView = VK_NULL_HANDLE;

    VkDescriptorPool descriptorPool = VK_NULL_HANDLE;

    VkDescriptorSetLayout descLayout = VK_NULL_HANDLE;

    GpuMemory* memory = nullptr;

    GpuMemory* frameMemory = nullptr;

    MeshChunk* mesh = nullptr;

    //! Память устройства, в котором содержатся изображения в
    //! `ImageResources::msaaImage`.
    VkDeviceMemory msaaImageMem = VK_NULL_HANDLE;

    size_t currentFrameIndex = 0;
};
}


enum VulkanInitState
{
    Begin = 0,
    Instance,
    Surface,
    PhysicalDevice,
    LogicalDevice,
    Swapchain,
    Renderpass,
    Pipeline,
    End
};


struct GpuManager::Impl
{
    Application* app;
#ifdef LINUX
    struct Xcb
    {
        xcb_connection_t* connection = nullptr;
        xcb_window_t window = 0;
    };
    Xcb xcb;
#endif
    VulkanState vulkanState;
    std::function<std::tuple<uint32_t,uint32_t>()> getImageExtent;
};


GpuManager::GpuManager(Application* app)
{
    assert(app != nullptr);

    m_pimpl = new Impl {
        .app = app,
    };
}


GpuManager::~GpuManager()
{
    delete m_pimpl;
}


void GpuManager::setImageExtentFunction(std::function<std::tuple<uint32_t,uint32_t>()> func)
{
    m_pimpl->getImageExtent = func;
}


namespace
{;

VkSampleCountFlagBits getMaxUsableSampleCount(VkPhysicalDevice device)
{
    VkPhysicalDeviceProperties properties;
    vkGetPhysicalDeviceProperties(device, &properties);

    VkSampleCountFlags counts =
          properties.limits.framebufferColorSampleCounts
        & properties.limits.framebufferDepthSampleCounts;

    if(counts & VK_SAMPLE_COUNT_64_BIT) return VK_SAMPLE_COUNT_64_BIT;
    if(counts & VK_SAMPLE_COUNT_32_BIT) return VK_SAMPLE_COUNT_32_BIT;
    if(counts & VK_SAMPLE_COUNT_16_BIT) return VK_SAMPLE_COUNT_16_BIT;
    if(counts & VK_SAMPLE_COUNT_8_BIT)  return VK_SAMPLE_COUNT_8_BIT;
    if(counts & VK_SAMPLE_COUNT_4_BIT)  return VK_SAMPLE_COUNT_4_BIT;
    if(counts & VK_SAMPLE_COUNT_2_BIT)  return VK_SAMPLE_COUNT_2_BIT;
    return VK_SAMPLE_COUNT_1_BIT;
}


bool createRenderPass(VulkanState* state)
{
    bool msaa = (state->sampleCount != VK_SAMPLE_COUNT_1_BIT);

    std::vector<VkAttachmentDescription> attachments = {
        { // Non-msaa render target or resolve target.
            .format = state->imageFormat,
            .samples = VK_SAMPLE_COUNT_1_BIT,
            .loadOp = msaa ? VK_ATTACHMENT_LOAD_OP_DONT_CARE : VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
            .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
            .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
            .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
        },
        { // Depth attachment with multisampling
            .format = state->depthStencilFormat,
            .samples = state->sampleCount,
            .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
            .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
            .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
            .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
        },
        { // Multisampled color attachment
            .format = state->imageFormat,
            .samples = state->sampleCount,
            .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
            .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
            .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
            .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        },
    };

    VkAttachmentReference colorAttachmentRef = {
        .attachment = static_cast<uint32_t>(msaa ? 2 : 0),
        .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
    };
    VkAttachmentReference depthAttachmentRef = {
        .attachment = 1,
        .layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
    };
    VkAttachmentReference resolveAttachmentRef = {
        .attachment = 0,
        .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
    };

    std::vector<VkSubpassDescription> subpasses = {
        {
            .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
            .colorAttachmentCount = static_cast<uint32_t>(1),
            .pColorAttachments = &colorAttachmentRef,
            .pResolveAttachments = msaa ? &resolveAttachmentRef : nullptr,
            .pDepthStencilAttachment = &depthAttachmentRef
        }
    };

    std::vector<VkSubpassDependency> dependencies = {
        {
            .srcSubpass = VK_SUBPASS_EXTERNAL,
            .dstSubpass = 0,
            .srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            .dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            .srcAccessMask = 0,
            .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
        }
    };

    VkRenderPassCreateInfo ci_renderpass {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        .attachmentCount = static_cast<uint32_t>(msaa ? 3 : 2),
        .pAttachments = attachments.data(),
        .subpassCount = static_cast<uint32_t>(subpasses.size()),
        .pSubpasses = subpasses.data(),
        .dependencyCount = static_cast<uint32_t>(dependencies.size()),
        .pDependencies = dependencies.data(),
    };

    VkResult r = vkCreateRenderPass(
        state->device,
        &ci_renderpass,
        nullptr,
        &state->renderpass
    );
    if(r != VK_SUCCESS) {
        BOOST_LOG_TRIVIAL(trace) <<
            std::format(
                "RenderPass object creation error ({}).",
                VkResultToString(r)
            );
        return false;
    }

    if(state->renderpass == VK_NULL_HANDLE) {
        BOOST_LOG_TRIVIAL(trace) << "Renderpass creation error.";
        return false;
    }
    BOOST_LOG_TRIVIAL(trace) <<
        std::format(
            "Renderpass {} is created.",
            static_cast<void*>(&state->renderpass)
        );
    return true;
}


bool createGraphicsPipeline(VulkanState* state)
{
    VkResult r;

    // Создаем макеты набора дескрипторов.
    std::array<VkDescriptorSetLayoutBinding,1> descriptorSetLayoutBindings {
        VkDescriptorSetLayoutBinding {
            .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
        }
    };
    VkDescriptorSetLayoutCreateInfo ci_descriptorSetLayout {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .bindingCount = static_cast<uint32_t>(descriptorSetLayoutBindings.size()),
        .pBindings = descriptorSetLayoutBindings.data()
    };
    r = vkCreateDescriptorSetLayout(state->device, &ci_descriptorSetLayout, nullptr, &state->descLayout);
    if(r != VK_SUCCESS) {
        BOOST_LOG_TRIVIAL(trace) <<
            std::format(
                "Descriptor set layout creation error ({}).",
                VkResultToString(r)
            );
        return false;
    }

    // Создаем макет конвейера.
    std::array<VkDescriptorSetLayout,1> descriptorSetLayouts {
        state->descLayout
    };
    VkPipelineLayoutCreateInfo ci_pipelineLayout {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .setLayoutCount = static_cast<uint32_t>(descriptorSetLayouts.size()),
        .pSetLayouts = descriptorSetLayouts.data(),
    };
    r = vkCreatePipelineLayout(state->device, &ci_pipelineLayout, nullptr, &state->pipelineLayout);
    if(r != VK_SUCCESS) {
        BOOST_LOG_TRIVIAL(trace) <<
            std::format(
                "Pipeline layout creation error ({}).",
                VkResultToString(r)
            );
        return false;
    }

    // Подготавливаем шейдерные модули (вершинный и фрагментный).

    static uint32_t vs_spirv_source[] = {
#       include "app.vert.spv.h"
    };
    VkShaderModuleCreateInfo ci_vsModule {
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .codeSize = sizeof(vs_spirv_source),
        .pCode = (uint32_t *)vs_spirv_source,
    };
    VkShaderModule vsModule = VK_NULL_HANDLE;
    r = vkCreateShaderModule(state->device, &ci_vsModule, nullptr, &vsModule);
    if(r != VK_SUCCESS) {
        BOOST_LOG_TRIVIAL(trace) <<
            std::format(
                "Vertex shader creation error ({}).",
                VkResultToString(r)
            );
        return false;
    }

    static uint32_t fs_spirv_source[] = {
#       include "app.frag.spv.h"
    };
    VkShaderModuleCreateInfo ci_fsModule {
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .codeSize = sizeof(fs_spirv_source),
        .pCode = (uint32_t *)fs_spirv_source,
    };
    VkShaderModule fsModule = VK_NULL_HANDLE;
    r = vkCreateShaderModule(state->device, &ci_fsModule, nullptr, &fsModule);
    if(r != VK_SUCCESS) {
        BOOST_LOG_TRIVIAL(trace) <<
            std::format(
                "Fragment shader creation error ({}).",
                VkResultToString(r)
            );
        return false;
    }

    std::array<VkPipelineShaderStageCreateInfo,2> ci_stages = {
        VkPipelineShaderStageCreateInfo {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .stage = VK_SHADER_STAGE_VERTEX_BIT,
            .module = vsModule,
            .pName = "main",
        },
        VkPipelineShaderStageCreateInfo {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
            .module = fsModule,
            .pName = "main",
        },
    };

    VkVertexInputBindingDescription vertexBindingDescriptions[] = {
        {
            .binding = 0,
            .stride = 3 * sizeof(float),
            .inputRate = VK_VERTEX_INPUT_RATE_VERTEX
        },
    };
    VkVertexInputAttributeDescription vertexAttributeDescriptions[] = {
        {
            .location = 0,
            .binding = 0,
            .format = VK_FORMAT_R32G32B32_SFLOAT,
            .offset = 0
        },
    };
    VkPipelineVertexInputStateCreateInfo ci_vertexInputState {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .vertexBindingDescriptionCount = 1,
        .pVertexBindingDescriptions = vertexBindingDescriptions,
        .vertexAttributeDescriptionCount = 1,
        .pVertexAttributeDescriptions = vertexAttributeDescriptions
    };

    VkPipelineInputAssemblyStateCreateInfo ci_inputAssemblyState {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
        .primitiveRestartEnable = false,
    };

    VkPipelineViewportStateCreateInfo ci_viewportState {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .viewportCount = 1,
        .scissorCount = 1,
    };

    VkPipelineRasterizationStateCreateInfo ci_rasterizationState {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .rasterizerDiscardEnable = false,
        .polygonMode = VK_POLYGON_MODE_FILL,
        .cullMode = VK_CULL_MODE_BACK_BIT,
        .frontFace = VK_FRONT_FACE_CLOCKWISE,
        .lineWidth = 1.0f,
    };

    VkPipelineMultisampleStateCreateInfo ci_multisampleState {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .rasterizationSamples = state->sampleCount,
    };

    VkPipelineDepthStencilStateCreateInfo ci_depthStencilState {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO
    };

    VkPipelineColorBlendAttachmentState attachmentStates[] = {
        {
            .colorWriteMask = VK_COLOR_COMPONENT_A_BIT |
                              VK_COLOR_COMPONENT_R_BIT |
                              VK_COLOR_COMPONENT_G_BIT |
                              VK_COLOR_COMPONENT_B_BIT
        },
    };
    VkPipelineColorBlendStateCreateInfo ci_colorBlendState {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .attachmentCount = 1,
        .pAttachments = attachmentStates
    };

    VkDynamicState dynamicStates[] = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR,
    };
    VkPipelineDynamicStateCreateInfo ci_pipelineDynamicState = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
        .dynamicStateCount = 2,
        .pDynamicStates = dynamicStates,
    };

    VkGraphicsPipelineCreateInfo ci_pipelines[] = {
        {
            .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
            .stageCount = static_cast<uint32_t>(ci_stages.size()),
            .pStages = ci_stages.data(),
            .pVertexInputState = &ci_vertexInputState,
            .pInputAssemblyState = &ci_inputAssemblyState,
            .pViewportState = &ci_viewportState,
            .pRasterizationState = &ci_rasterizationState,
            .pMultisampleState = &ci_multisampleState,
            .pDepthStencilState = &ci_depthStencilState,
            .pColorBlendState = &ci_colorBlendState,
            .pDynamicState = &ci_pipelineDynamicState,
            .layout = state->pipelineLayout,
            .renderPass = state->renderpass,
            .subpass = 0,
        }
    };
    VkPipeline pipelines[] = {
        VK_NULL_HANDLE
    };
    r = vkCreateGraphicsPipelines(state->device, VK_NULL_HANDLE, 1, ci_pipelines, nullptr, pipelines);
    if(r != VK_SUCCESS) {
        BOOST_LOG_TRIVIAL(trace) <<
            std::format(
                "Graphics pipeline creation error ({}).",
                VkResultToString(r)
            );
        return false;
    }
    state->pipeline = pipelines[0];

    if(state->pipeline == VK_NULL_HANDLE) {
        BOOST_LOG_TRIVIAL(trace) << "Graphics pipeline creation error.";
        return false;
    }
    BOOST_LOG_TRIVIAL(trace) <<
        std::format(
            "Graphics pipeline {} is created.",
            static_cast<void*>(&state->pipeline)
        );
    return true;
}


bool initImage(VulkanState* state, ImageResources* resources)
{
    BOOST_LOG_TRIVIAL(trace) << "Create image resources ...";

    VkImageViewCreateInfo ci_imageView {
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .image = resources->image,
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .format = state->imageFormat,
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
        state->device,
        &ci_imageView,
        nullptr,
        &resources->imageView
    );

    bool msaa = (state->sampleCount != VK_SAMPLE_COUNT_1_BIT);

    VkImageView attachments[3];
    attachments[0] = resources->imageView;
    attachments[1] = state->depthStencilView;
    attachments[2] = resources->msaaImageView;

    VkFramebufferCreateInfo ci_framebuffer {
        .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
        .renderPass = state->renderpass,
        .attachmentCount = static_cast<uint32_t>(msaa ? 3 : 2),
        .pAttachments = attachments,
        .width = state->imageExtent.width,
        .height = state->imageExtent.height,
        .layers = 1
    };
    vkCreateFramebuffer(
        state->device,
        &ci_framebuffer,
        nullptr,
        &resources->framebuffer
    );

    return true;
}


bool initFrame(VulkanState* state, FrameResources* resources)
{
    BOOST_LOG_TRIVIAL(trace) << "Create frame resources ...";

    VkResult r;

    VkFenceCreateInfo ci_fence {
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        .flags = VK_FENCE_CREATE_SIGNALED_BIT
    };
    r = vkCreateFence(
        state->device,
        &ci_fence,
        nullptr,
        &resources->cmdFence
    );
    assert(r == VK_SUCCESS);
    resources->cmdFenceWaitable = true;

    VkCommandBufferAllocateInfo commandBufferAllocateInfo {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool = state->cmdPool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = 1,
    };
    r = vkAllocateCommandBuffers(
        state->device,
        &commandBufferAllocateInfo,
        &resources->cmdBuf
    );
    assert(r == VK_SUCCESS);

    VkSemaphoreCreateInfo semaphoreCreateInfo {
       .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
    };
    r = vkCreateSemaphore(
        state->device,
        &semaphoreCreateInfo,
        nullptr,
        &resources->imageSemaphore
    );
    assert(r == VK_SUCCESS);
    r = vkCreateSemaphore(
        state->device,
        &semaphoreCreateInfo,
        nullptr,
        &resources->drawSemaphore
    );
    assert(r == VK_SUCCESS);

    VkDescriptorSetAllocateInfo descSetAllocInfo {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .descriptorPool = state->descriptorPool,
        .descriptorSetCount = 1,
        .pSetLayouts = &state->descLayout
    };
    r = vkAllocateDescriptorSets(
        state->device,
        &descSetAllocInfo,
        &resources->descSet
    );
    assert(r == VK_SUCCESS);

    return true;
}


void finalizeResources(VulkanState* state, ImageResources* resources)
{
    BOOST_LOG_TRIVIAL(trace) << "Finalize image resources ...";

    vkDestroyFramebuffer(state->device, resources->framebuffer, nullptr);

    vkDestroyImageView(state->device, resources->imageView, nullptr);
    vkDestroyImageView(state->device, resources->msaaImageView, nullptr);
    vkDestroyImage(state->device, resources->msaaImage, nullptr);
}


void finalizeResources(VulkanState* state, FrameResources* frame)
{
    BOOST_LOG_TRIVIAL(trace) << "Finalize frame resources ...";

    vkFreeCommandBuffers(state->device, state->cmdPool, 1, &frame->cmdBuf);

    vkDestroyFence(state->device, frame->cmdFence, nullptr);
    frame->cmdFenceWaitable = false;

    vkDestroySemaphore(state->device, frame->imageSemaphore, nullptr);
    vkDestroySemaphore(state->device, frame->drawSemaphore, nullptr);
}


struct SwapChainSupportDetails
{
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};


SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device, VkSurfaceKHR surface)
{
    SwapChainSupportDetails details;

    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);

    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);

    if (formatCount != 0) {
        details.formats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.formats.data());
    }

    uint32_t presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);

    if (presentModeCount != 0) {
        details.presentModes.resize(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, details.presentModes.data());
    }

    return details;
}


uint32_t chooseTransientImageMemType(
    VulkanState* state,
    VkImage img,
    uint32_t startIndex)
{
    VkPhysicalDeviceMemoryProperties props;
    vkGetPhysicalDeviceMemoryProperties(state->physicalDevice, &props);

    VkMemoryRequirements memReq;
    vkGetImageMemoryRequirements(state->device, img, &memReq);
    if(memReq.memoryTypeBits == 0)
        return -1;

    uint32_t memTypeIndex = static_cast<uint32_t>(-1);
    const VkMemoryType* memType = props.memoryTypes;
    bool foundDevLocal = false;
    for(uint32_t i = startIndex; i < props.memoryTypeCount; ++i) {
        if(memReq.memoryTypeBits & (1 << i)) {
            if(memType[i].propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) {
                if(!foundDevLocal) {
                    foundDevLocal = true;
                    memTypeIndex = i;
                }
                if(memType[i].propertyFlags & VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT) {
                    memTypeIndex = i;
                    break;
                }
            }
        }
    }

    return memTypeIndex;
}


bool createTransientImage(
    VulkanState* state,
    VkFormat format,
    VkImageUsageFlags usage,
    VkImageAspectFlags aspectMask,
    VkImage* images,
    VkDeviceMemory* mem,
    VkImageView* views,
    int count)
{
    VkMemoryRequirements memReq;
    VkResult r;

    assert(count > 0);
    for(int i = 0; i < count; ++i) {
        VkImageCreateInfo ci_image {
            .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
            .imageType = VK_IMAGE_TYPE_2D,
            .format = format,
            .extent = VkExtent3D {
                .width = state->imageExtent.width,
                .height = state->imageExtent.height,
                .depth = 1
            },
            .mipLevels = 1,
            .arrayLayers = 1,
            .samples = state->sampleCount,
            .tiling = VK_IMAGE_TILING_OPTIMAL,
            .usage = usage | VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT,
        };

        r = vkCreateImage(state->device, &ci_image, nullptr, images + i);
        if(r != VK_SUCCESS) {
            BOOST_LOG_TRIVIAL(error) <<
                std::format(
                    "Failed to create image: {}",
                    VkResultToString(r)
                );
            return false;
        }
        vkGetImageMemoryRequirements(state->device, images[i], &memReq);
    }

    VkMemoryAllocateInfo memInfo;
    memset(&memInfo, 0, sizeof(memInfo));
    memInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    memInfo.allocationSize = Aligned(memReq.size, memReq.alignment) * count;

    uint32_t startIndex = 0;
    do {
        memInfo.memoryTypeIndex = chooseTransientImageMemType(state, images[0], startIndex);
        if (memInfo.memoryTypeIndex == uint32_t(-1)) {
            BOOST_LOG_TRIVIAL(error) << "No suitable memory type found";
            return false;
        }
        startIndex = memInfo.memoryTypeIndex + 1;
        BOOST_LOG_TRIVIAL(trace) <<
            std::format(
                "Allocating %u bytes for transient image (memtype %u)",
                uint32_t(memInfo.allocationSize),
                memInfo.memoryTypeIndex
            );
        r = vkAllocateMemory(state->device, &memInfo, nullptr, mem);
        if(r != VK_SUCCESS && r != VK_ERROR_OUT_OF_DEVICE_MEMORY) {
            BOOST_LOG_TRIVIAL(error) <<
                std::format(
                    "Failed to allocate image memory: {}",
                    VkResultToString(r)
                );
            return false;
        }
    } while(r != VK_SUCCESS);

    VkDeviceSize ofs = 0;
    for (int i = 0; i < count; ++i) {
        r = vkBindImageMemory(state->device, images[i], *mem, ofs);
        if(r != VK_SUCCESS) {
            BOOST_LOG_TRIVIAL(error) <<
                std::format(
                    "Failed to bind image memory: {}",
                    VkResultToString(r)
                );
            return false;
        }
        ofs += Aligned(memReq.size, memReq.alignment);

        VkImageViewCreateInfo imgViewInfo;
        memset(&imgViewInfo, 0, sizeof(imgViewInfo));
        imgViewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        imgViewInfo.image = images[i];
        imgViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        imgViewInfo.format = format;
        imgViewInfo.components.r = VK_COMPONENT_SWIZZLE_R;
        imgViewInfo.components.g = VK_COMPONENT_SWIZZLE_G;
        imgViewInfo.components.b = VK_COMPONENT_SWIZZLE_B;
        imgViewInfo.components.a = VK_COMPONENT_SWIZZLE_A;
        imgViewInfo.subresourceRange.aspectMask = aspectMask;
        imgViewInfo.subresourceRange.levelCount = imgViewInfo.subresourceRange.layerCount = 1;

        r = vkCreateImageView(state->device, &imgViewInfo, nullptr, views + i);
        if(r != VK_SUCCESS) {
            BOOST_LOG_TRIVIAL(error) <<
                std::format(
                    "Failed to create image view: {}",
                    VkResultToString(r)
                );
            return false;
        }
    }

    return true;
}


void releaseSwapchain(VulkanState* state)
{
    vkDestroySwapchainKHR(state->device, state->swapchain, nullptr);

    for(int i = 0; i < state->frameCount; ++i) {
        FrameResources* frame = &state->frameRes[i];
        finalizeResources(state, frame);
    }

    for(int i = 0; i < state->imageCount; ++i) {
        ImageResources* image = &state->imageRes[i];
        finalizeResources(state, image);
    }

    vkFreeMemory(state->device, state->msaaImageMem, nullptr);

    vkDestroyImageView(state->device, state->depthStencilView, nullptr);
    vkDestroyImage(state->device, state->depthStencilImage, nullptr);
    vkFreeMemory(state->device, state->depthStencilMem, nullptr);
}


bool recreateSwapchain(
    VulkanState* state,
    uint32_t width,
    uint32_t height)
{
    // Защита от вырождения окна в нулевой размер.
    if(width == 0 || height == 0)
        return true;

    VkResult r;

    r = vkDeviceWaitIdle(state->device);
    assert(r == VK_SUCCESS);

    VkSurfaceCapabilitiesKHR surfaceCaps;
    r = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(state->physicalDevice, state->surface, &surfaceCaps);
    assert(r == VK_SUCCESS);

    // Выбираем размер изображений.
    state->imageExtent.width = width;
    state->imageExtent.height = height;
    assert(surfaceCaps.currentExtent.width == static_cast<uint32_t>(-1)
        || surfaceCaps.currentExtent.width == width
        && surfaceCaps.currentExtent.height == height
    );

    VkSurfaceTransformFlagBitsKHR preTransform =
        (surfaceCaps.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR)
            ? VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR
            : surfaceCaps.currentTransform;

    assert(state->graphicsFamilyIndex == state->presentFamilyIndex);
    std::array<uint32_t, 1> queueFamilyIndices = {
        state->graphicsFamilyIndex.value()
    };

    VkSwapchainKHR oldSwapchain = state->swapchain;

    VkSwapchainCreateInfoKHR ci_swapchain {
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .surface = state->surface,
        .minImageCount = state->minImageCount,
        .imageFormat = state->imageFormat,
        .imageColorSpace = state->colorSpace,
        .imageExtent = state->imageExtent,
        .imageArrayLayers = 1,
        .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        .imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount = static_cast<uint32_t>(queueFamilyIndices.size()),
        .pQueueFamilyIndices = queueFamilyIndices.data(),
        .preTransform = preTransform,
        .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        .presentMode = state->presentMode,
        .oldSwapchain = oldSwapchain
    };
    VkSwapchainKHR newSwapchain;
    r = vkCreateSwapchainKHR(
        state->device,
        &ci_swapchain,
        nullptr,
        &newSwapchain
    );
    if(r != VK_SUCCESS) {
        BOOST_LOG_TRIVIAL(trace) <<
            std::format(
                "Swapchain object creation error ({}).",
                VkResultToString(r)
            );
        return false;
    }
    BOOST_LOG_TRIVIAL(trace) <<
        std::format(
            "Swapchain object is ({}).",
            static_cast<void*>(&newSwapchain)
        );

    if(oldSwapchain != VK_NULL_HANDLE) {
        releaseSwapchain(state);
    }

    state->swapchain = newSwapchain;

    r = vkGetSwapchainImagesKHR(
        state->device,
        state->swapchain,
        &state->imageCount,
        nullptr
    );
    if(r != VK_SUCCESS)
        return false;
    if(state->imageCount == 0)
        return false;
    BOOST_LOG_TRIVIAL(trace) <<
        std::format(
            "Count of images in swapchain is {}.",
            state->imageCount
        );

    std::vector<VkImage> swapchainImages(state->imageCount);
    r = vkGetSwapchainImagesKHR(
        state->device,
        state->swapchain,
        &state->imageCount,
        swapchainImages.data()
    );
    if(r != VK_SUCCESS)
        return false;
    if(state->imageCount > MAX_NUM_IMAGES)
        return false;

    // Создаем изображение буфера глубины.
    createTransientImage(
        state,
        state->depthStencilFormat,
        VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
        VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT,
        &state->depthStencilImage,
        &state->depthStencilMem,
        &state->depthStencilView,
        1
    );

    std::vector<VkImage> msaaImages(state->imageCount, VK_NULL_HANDLE);
    std::vector<VkImageView> msaaViews(state->imageCount, VK_NULL_HANDLE);
    bool msaa = (state->sampleCount != VK_SAMPLE_COUNT_1_BIT);
    if(msaa) {
        createTransientImage(
            state,
            state->imageFormat,
            VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
            VK_IMAGE_ASPECT_COLOR_BIT,
            msaaImages.data(),
            &state->msaaImageMem,
            msaaViews.data(),
            state->imageCount
        );
    }
    for(size_t i = 0; i < state->imageCount; ++i) {
        ImageResources& resources = state->imageRes[i];
        resources.image = swapchainImages[i];
        resources.msaaImage = msaaImages[i];
        resources.msaaImageView = msaaViews[i];
        initImage(state, &resources);
    }

    return true;
}


void render(
    VulkanState* state,
    ImageResources& image,
    FrameResources& frame)
{
    // Пересоздаем командный буфер.
    if(frame.cmdBuf != VK_NULL_HANDLE) {
        vkFreeCommandBuffers(state->device, state->cmdPool, 1, &frame.cmdBuf);
        frame.cmdBuf = VK_NULL_HANDLE;
    }
    VkCommandBufferAllocateInfo ci_cmdBuf {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool = state->cmdPool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = 1
    };
    vkAllocateCommandBuffers(state->device, &ci_cmdBuf, &frame.cmdBuf);

    // Begin command buffer.
    VkCommandBufferBeginInfo commandBufferBeginInfo {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
    };
    vkBeginCommandBuffer(
        frame.cmdBuf,
        &commandBufferBeginInfo
    );

    // Begin renderpass command.
    VkClearValue clearValues[] = {
        { .color = { .float32 = { 0.2f, 0.2f, 0.2f, 1.0f } } },
        { .depthStencil = { 1.0f, 0 } },
        { .color = { .float32 = { 0.2f, 0.2f, 0.2f, 1.0f } } }
    };
    VkRenderPassBeginInfo bi_renderpass {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .renderPass = state->renderpass,
        .framebuffer = image.framebuffer,
        .renderArea = {
            .offset = { 0, 0 },
            .extent = state->imageExtent
        },
        .clearValueCount = 3,
        .pClearValues = clearValues
    };
    vkCmdBeginRenderPass(
        frame.cmdBuf,
        &bi_renderpass,
        VK_SUBPASS_CONTENTS_INLINE
    );

    // Дальнейшие команды будут вызваны, только если сцена не пуста.
    if(state->mesh)
    {
        vkCmdBindPipeline(frame.cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS, state->pipeline);

        vkCmdBindDescriptorSets(
            frame.cmdBuf,
            VK_PIPELINE_BIND_POINT_GRAPHICS,
            state->pipelineLayout,
            0,
            1,
            &frame.descSet,
            0,
            nullptr
        );

        // Bind vertex buffers.
        VkBuffer buffers[] = {
            state->memory->buffer()
        };
        VkDeviceSize offsets[] = {
            0
        };
        vkCmdBindVertexBuffers(
            frame.cmdBuf,
            0,
            1,
            buffers,
            offsets
        );

        // Bind index buffers.
        vkCmdBindIndexBuffer(
            frame.cmdBuf,
            state->mesh->buffer(),
            state->mesh->indexOffset(),
            VK_INDEX_TYPE_UINT32
        );

        const VkViewport viewport = {
            .x = 0,
            .y = 0,
            .width = static_cast<float>(state->imageExtent.width),
            .height = static_cast<float>(state->imageExtent.height),
            .minDepth = 0,
            .maxDepth = 1,
        };
        vkCmdSetViewport(frame.cmdBuf, 0, 1, &viewport);

        const VkRect2D scissor = {
            .offset = { 0, 0 },
            .extent = state->imageExtent,
        };
        vkCmdSetScissor(frame.cmdBuf, 0, 1, &scissor);

        vkCmdDrawIndexed(
            frame.cmdBuf,
            state->mesh->indexCount(),
            1,
            0,
            0,
            0
        );
    }

    vkCmdEndRenderPass(frame.cmdBuf);

    vkEndCommandBuffer(frame.cmdBuf);

    VkPipelineStageFlags waitDstStageMask[] = {
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
    };
    VkSubmitInfo submitInfo {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .waitSemaphoreCount = static_cast<uint32_t>(1),
        .pWaitSemaphores = &frame.imageSemaphore,
        .pWaitDstStageMask = waitDstStageMask,
        .commandBufferCount = 1,
        .pCommandBuffers = &frame.cmdBuf,
        .signalSemaphoreCount = static_cast<uint32_t>(1),
        .pSignalSemaphores = &frame.drawSemaphore
    };
    assert(!frame.cmdFenceWaitable);
    vkQueueSubmit(state->queue, 1, &submitInfo, frame.cmdFence);
    //frame.imageSemaphoreWaitable = false;
    frame.cmdFenceWaitable = true;
}


VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData)
{
    auto outMessage = std::format("Validation Layer: {}", pCallbackData->pMessage);
    if(messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT != 0)
        BOOST_LOG_TRIVIAL(debug) << outMessage;
    else if(messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT)
        BOOST_LOG_TRIVIAL(info) << outMessage;
    else if(messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
        BOOST_LOG_TRIVIAL(warning) << outMessage;
    else if(messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
        BOOST_LOG_TRIVIAL(error) << outMessage;
    else
        BOOST_LOG_TRIVIAL(trace) << outMessage;

    return VK_FALSE;
}


//! Создаем экземпляр vulkan.
bool initInstance(VulkanState* state)
{
    std::vector<const char*> extensionNames {
        //VK_KHR_SWAPCHAIN_EXTENSION_NAME,
        VK_KHR_SURFACE_EXTENSION_NAME,
#if defined(USE_PLATFORM_XCB_KHR)
        VK_KHR_XCB_SURFACE_EXTENSION_NAME,
#endif
#if defined(_WIN32)
        VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
#endif
#ifdef ENABLE_VALIDATION_LAYERS
        VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
#endif
    };

    VkApplicationInfo applicationInfo {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pApplicationName = "app-qt",
        .applicationVersion = VK_MAKE_VERSION(0, 0, 1),
        .pEngineName = "numgeom",
        .engineVersion = VK_MAKE_VERSION(0, 0, 1),
        .apiVersion = VK_API_VERSION_1_2,
    };
    VkInstanceCreateInfo ci_instance {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pApplicationInfo = &applicationInfo,
        .enabledExtensionCount = static_cast<uint32_t>(extensionNames.size()),
        .ppEnabledExtensionNames = extensionNames.data()
    };

#ifdef ENABLE_VALIDATION_LAYERS
    std::vector<const char*> layersNames {
        "VK_LAYER_KHRONOS_validation",
    };
    ci_instance.enabledLayerCount = static_cast<uint32_t>(layersNames.size());
    ci_instance.ppEnabledLayerNames = layersNames.data();
    VkDebugUtilsMessengerCreateInfoEXT ci_debugUtilsMessenger {
        .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
        .messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT
                         | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT
                         | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
        .messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT
                     | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT
                     | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
        .pfnUserCallback = debugCallback,
    };
    ci_instance.pNext = &ci_debugUtilsMessenger;
#endif

    VkResult r = vkCreateInstance(&ci_instance, nullptr, &state->instance);
    if(r != VK_SUCCESS) {
        BOOST_LOG_TRIVIAL(trace) <<
            std::format(
                "Vulkan instance creation error ({}).",
                VkResultToString(r)
            );
        return false;
    }
    BOOST_LOG_TRIVIAL(trace) <<
        std::format(
            "Success of create vulkan instance ({})",
            static_cast<void*>(&state->instance)
        );

#ifdef ENABLE_VALIDATION_LAYERS
    auto func = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(
        vkGetInstanceProcAddr(
            state->instance,
            "vkCreateDebugUtilsMessengerEXT")
        );
    if(!func) {
        BOOST_LOG_TRIVIAL(trace) << "Function `vkCreateDebugUtilsMessengerEXT` is not found.";
        return false;
    }
    r = func(
        state->instance,
        &ci_debugUtilsMessenger,
        nullptr,
        &state->debugMessenger
    );
    if(r != VK_SUCCESS) {
        BOOST_LOG_TRIVIAL(trace) <<
            std::format(
                "Error of creating debug utils messenger ({}).",
                VkResultToString(r)
            );
        return false;
    }
#endif

    return true;
}


void findQueueFamilies(
    VkPhysicalDevice device,
    VkSurfaceKHR surface,
    std::optional<uint32_t>& graphicsFamily,
    std::optional<uint32_t>& presentFamily)
{
    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

    int i = 0;
    for(uint32_t i = 0; i < queueFamilyCount; ++i) {
        const auto& queueFamily = queueFamilies[i];
        if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
            graphicsFamily = i;

        VkBool32 presentSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);

        if(presentSupport)
            presentFamily = i;

        if(graphicsFamily.has_value() && presentFamily.has_value())
            return;
    }
}


bool checkDeviceExtensionSupport(
    VkPhysicalDevice device,
    const std::vector<const char*>& requiredExtensionNames)
{
    uint32_t extensionCount;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

    std::vector<VkExtensionProperties> availableExtensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

    std::set<std::string> requiredExtensionNames2(
        requiredExtensionNames.begin(),
        requiredExtensionNames.end()
    );

    for(const auto& extension : availableExtensions)
        requiredExtensionNames2.erase(extension.extensionName);

    return requiredExtensionNames2.empty();
}


//! Оценка пригодности физического устройства для целей проекта.
bool isDeviceSuitable(
    VkPhysicalDevice device,
    VkSurfaceKHR surface,
    const std::vector<const char*>& requiredExtensionNames)
{
    // Проверка присутствия в устройстве очередей рендеринга и показа.
    std::optional<uint32_t> graphicsFamily, presentFamily;
    findQueueFamilies(device, surface, graphicsFamily, presentFamily);
    if(!graphicsFamily.has_value() || !presentFamily.has_value())
        return false;

    // Проверка поддержки устройством заявленных проектом расширений.
    bool extensionsSupported = checkDeviceExtensionSupport(device, requiredExtensionNames);
    if(!extensionsSupported)
        return false;

    // Проверка возможности создания swap-chain'а.
    SwapChainSupportDetails swapChainSupport = querySwapChainSupport(device, surface);
    if(   swapChainSupport.formats.empty()
       || swapChainSupport.presentModes.empty()
       || swapChainSupport.capabilities.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR == 0) {
        return false;
    }

    VkPhysicalDeviceFeatures supportedFeatures;
    vkGetPhysicalDeviceFeatures(device, &supportedFeatures);
    if(!supportedFeatures.samplerAnisotropy)
        return false;

    return true;
}


bool selectPhysicalDevice(
    VulkanState* state,
    const std::vector<const char*>& extensionNames)
{
    uint32_t devicesCount = 0;
    VkResult r = vkEnumeratePhysicalDevices(state->instance, &devicesCount, nullptr);
    if(r != VK_SUCCESS)
        return false;
    BOOST_LOG_TRIVIAL(trace) << std::format("Count of physical devices is {}.", devicesCount);
    if(devicesCount == 0) {
        BOOST_LOG_TRIVIAL(trace) << "Failed to find GPUs with Vulkan support.";
        return false;
    }

    std::vector<VkPhysicalDevice> devices(devicesCount);
    r = vkEnumeratePhysicalDevices(state->instance, &devicesCount, devices.data());
    if(r != VK_SUCCESS)
        return false;

    for(VkPhysicalDevice device : devices) {
        if(isDeviceSuitable(device,state->surface,extensionNames)) {
            state->physicalDevice = device;
            break;
        }
    }
    if(state->physicalDevice == VK_NULL_HANDLE) {
        BOOST_LOG_TRIVIAL(trace) << "Couldn't find a suitable physical device.";
        return false;
    }
    BOOST_LOG_TRIVIAL(trace) <<
        std::format(
            "Physical device #{} is selected.",
            static_cast<void*>(state->physicalDevice)
        );

    // Получаем свойства памяти физического устройства.
    vkGetPhysicalDeviceMemoryProperties(state->physicalDevice, &state->memoryProperties);

    return true;
}


bool createLogicalDevice(VulkanState* state)
{
    // Выбираем семейство очередей.
    findQueueFamilies(
        state->physicalDevice,
        state->surface,
        state->graphicsFamilyIndex,
        state->presentFamilyIndex
    );
    if(!state->graphicsFamilyIndex.has_value()) {
        BOOST_LOG_TRIVIAL(trace) << "Graphics family not found";
        return false;
    }
    if(!state->presentFamilyIndex.has_value()) {
        BOOST_LOG_TRIVIAL(trace) << "Present family not found";
        return false;
    }

    //\warning Следует разобрать случай, когда очередь рендеринга и показа не совпадают.
    if(state->graphicsFamilyIndex != state->presentFamilyIndex)
        return false;

    // Создаем логическое устройство.
    std::vector<VkDeviceQueueCreateInfo> ci_queues;
    std::set<uint32_t> queueFamilies = {
        state->graphicsFamilyIndex.value(),
        state->presentFamilyIndex.value()
    };
    float queuePriorities[] = { 1.0f };
    for(uint32_t queueFamily : queueFamilies) {
        VkDeviceQueueCreateInfo ci_queue {
            .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
            .queueFamilyIndex = queueFamily,
            .queueCount = 1,
            .pQueuePriorities = queuePriorities,
        };
        ci_queues.push_back(ci_queue);
    }

    std::vector<const char*> deviceExtensionNames = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };

    VkDeviceCreateInfo ci_device {
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .queueCreateInfoCount = static_cast<uint32_t>(ci_queues.size()),
        .pQueueCreateInfos = ci_queues.data(),
        .enabledExtensionCount = static_cast<uint32_t>(deviceExtensionNames.size()),
        .ppEnabledExtensionNames = deviceExtensionNames.data(),
        .pEnabledFeatures = nullptr
    };
    VkResult r = vkCreateDevice(
        state->physicalDevice,
        &ci_device,
        nullptr,
        &state->device
    );
    if(r != VK_SUCCESS) {
        BOOST_LOG_TRIVIAL(trace) <<
            std::format(
                "Logical device creation error ({}).",
                VkResultToString(r)
            );
        return false;
    }
    BOOST_LOG_TRIVIAL(trace) <<
        std::format(
            "Logical device {} is created",
            static_cast<void*>(state->device)
        );

    VkDeviceQueueInfo2 queueInfo {
        .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_INFO_2,
        .queueFamilyIndex = state->graphicsFamilyIndex.value(),
        .queueIndex = 0,
    };
    vkGetDeviceQueue2(state->device, &queueInfo, &state->queue);
    BOOST_LOG_TRIVIAL(trace) <<
        std::format(
            "Queue {} is selected.",
            static_cast<void*>(state->queue)
        );
    return true;
}


#ifdef LINUX
bool createSurface(VulkanState* state, GpuManager::Impl::Xcb* xcb)
{
    int screenNum = 0;
    xcb_connection_t* connection = xcb_connect(nullptr, &screenNum);
    if(xcb_connection_has_error(connection)) {
        xcb_disconnect(connection);
        return false;
    }
    const xcb_setup_t* setup = xcb_get_setup(connection);
    int screenCount = xcb_setup_roots_length(setup);
    xcb_screen_iterator_t screen_iterator = xcb_setup_roots_iterator(setup);
    for (int i = 0; i < screenNum; ++i)
        xcb_screen_next(&screen_iterator);

    xcb_screen_t* _screen = screen_iterator.data;
    xcb_flush(connection);

    xcb->connection = connection;
    BOOST_LOG_TRIVIAL(trace) <<
        std::format(
            "Xcb connection {} and window {}",
            static_cast<void*>(xcb->connection),
            xcb->window
        );

    VkXcbSurfaceCreateInfoKHR ci_surface {
        .sType = VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR,
        .connection = connection,
        .window = xcb->window
    };
    VkResult r = vkCreateXcbSurfaceKHR(
        state->instance,
        &ci_surface,
        nullptr,
        &state->surface
    );
    if(r != VK_SUCCESS) {
        BOOST_LOG_TRIVIAL(trace) <<
            std::format(
                "Xcb surface creation error ({}).",
                VkResultToString(r)
            );
        return false;
    }
    BOOST_LOG_TRIVIAL(trace) <<
        std::format(
            "Success of create vulkan surface ({})",
            static_cast<void*>(state->surface)
        );

    return true;
}
#endif


VkFormat selectDepthStencilFormat(VkPhysicalDevice device)
{
    const VkFormat dsFormatCandidates[] = {
        VK_FORMAT_D24_UNORM_S8_UINT,
        VK_FORMAT_D32_SFLOAT_S8_UINT,
        VK_FORMAT_D16_UNORM_S8_UINT
    };

    for(int idx = 0; idx < sizeof(dsFormatCandidates)/sizeof(VkFormat); ++idx) {
        VkFormat format = dsFormatCandidates[idx];
        VkFormatProperties prop;
        vkGetPhysicalDeviceFormatProperties(device, format, &prop);
        if(prop.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT)
            return format;
    }

    return VK_FORMAT_UNDEFINED;
}


bool chooseSwapchainSettings(VulkanState* state)
{
    SwapChainSupportDetails swapchainSupport = querySwapChainSupport(
        state->physicalDevice,
        state->surface
    );

    state->imageFormat = VK_FORMAT_UNDEFINED;

    VkFormat format = VK_FORMAT_UNDEFINED;
    VkColorSpaceKHR colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
    //for(uint32_t i = 0; i < count; i++) {
    for(const auto& f : swapchainSupport.formats) {
        switch (f.format) {
        case VK_FORMAT_R8G8B8A8_SRGB:
        case VK_FORMAT_B8G8R8A8_SRGB:
        case VK_FORMAT_R8G8B8A8_UNORM:
        case VK_FORMAT_B8G8R8A8_UNORM:
            format = f.format;
            colorSpace = f.colorSpace;
            break;
        case VK_FORMAT_R8G8B8_SRGB:
        case VK_FORMAT_B8G8R8_SRGB:
        case VK_FORMAT_R8G8B8_UNORM:
        case VK_FORMAT_R5G6B5_UNORM_PACK16:
        case VK_FORMAT_B5G6R5_UNORM_PACK16:
        default:
            continue;
        }
    }

    state->imageFormat = format;
    state->colorSpace = colorSpace;
    if(state->imageFormat == VK_FORMAT_UNDEFINED) {
        BOOST_LOG_TRIVIAL(trace) << "Image format undefined.";
        return false;
    }

    state->depthStencilFormat = selectDepthStencilFormat(state->physicalDevice);
    if(state->depthStencilFormat == VK_FORMAT_UNDEFINED)
        BOOST_LOG_TRIVIAL(error) << "Failed to find an optimal depth-stencil format";

    BOOST_LOG_TRIVIAL(trace) <<
        std::format(
            "Color format: {} Depth-stencil format: {}",
            VkFormatToString(state->imageFormat),
            VkFormatToString(state->depthStencilFormat)
        );

    // Выбираем режим показа.
    for(auto pm : swapchainSupport.presentModes) {
        if(pm == VK_PRESENT_MODE_MAILBOX_KHR) {
            state->presentMode = VK_PRESENT_MODE_MAILBOX_KHR;
            break;
        }
    }

    state->minImageCount = 2;
    if(state->minImageCount < swapchainSupport.capabilities.minImageCount) {
        if(swapchainSupport.capabilities.minImageCount > MAX_NUM_IMAGES) {
            BOOST_LOG_TRIVIAL(error) <<
                std::format(
                    "VkSurfaceCapabilitiesKHR.minImageCount is too large (is: {}, max: {})",
                    swapchainSupport.capabilities.minImageCount,
                    MAX_NUM_IMAGES
                );
            return false;
        }
        state->minImageCount = swapchainSupport.capabilities.minImageCount;
    }
    if(swapchainSupport.capabilities.maxImageCount > 0
        && state->minImageCount > swapchainSupport.capabilities.maxImageCount) {
       state->minImageCount = swapchainSupport.capabilities.maxImageCount;
    }

    return true;
}


#define CHECK_STATE(var, state) if(var <= state) return true;

bool initialize(GpuManager::Impl* gpm, VulkanInitState s)
{
    BOOST_LOG_TRIVIAL(trace) << "Start gpumanager initialization ...";

    VulkanState* state = &gpm->vulkanState;
    VkResult r = VK_SUCCESS;

    // Выбираем экземпляр vulkan, если еще не выбран.
    if(state->instance == VK_NULL_HANDLE && !initInstance(state))
        return false;
    CHECK_STATE(s, VulkanInitState::Instance);

    // Создаем поверхность vulkan.
    //auto* xcb = &gpm->xcb;
    if(state->surface == VK_NULL_HANDLE /*&& !createSurface(state,xcb)*/)
        return false;
    CHECK_STATE(s, VulkanInitState::Surface);

    std::vector<const char*> deviceExtensionNames {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
    };

    // Выбираем физическое устройство, если еще не выбрано.
    if(state->physicalDevice == VK_NULL_HANDLE && !selectPhysicalDevice(state,deviceExtensionNames))
        return false;
    CHECK_STATE(s, VulkanInitState::PhysicalDevice);

    state->sampleCount = getMaxUsableSampleCount(state->physicalDevice);
    BOOST_LOG_TRIVIAL(trace) <<
        std::format(
            "Selected MSAA sample count: {}",
            static_cast<size_t>(state->sampleCount)
        );

    // Создаем логическое устройство и извлекаем очередь.
    if(state->device == VK_NULL_HANDLE && !createLogicalDevice(state))
        return false;
    CHECK_STATE(s, VulkanInitState::LogicalDevice);

    // Выбираем настройки, необходимые для последующего создания swapchain.
    if(!chooseSwapchainSettings(state))
        return false;

    // Создаем объект renderpass.
    if(!createRenderPass(state))
        return false;
    CHECK_STATE(s, VulkanInitState::Renderpass);

    // Создаем объект swapchain.
    uint32_t width, height;
    std::tie(width, height) = gpm->getImageExtent();
    if(!recreateSwapchain(state,width,height))
        return false;
    CHECK_STATE(s, VulkanInitState::Swapchain);

    if(!createGraphicsPipeline(state))
        return false;
    CHECK_STATE(s, VulkanInitState::Pipeline);

    VkCommandPoolCreateInfo ci_commandpool {
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        .queueFamilyIndex = 0, //< \warning
    };
    vkCreateCommandPool(
        state->device,
        &ci_commandpool,
        nullptr,
        &state->cmdPool
    );

    // Подготавливаем данные для конвейера.
    std::array<VkDescriptorPoolSize,1> descPoolSizes = {
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, state->frameCount},
    };
    VkDescriptorPoolCreateInfo ci_descpool {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .maxSets = state->frameCount,
        .poolSizeCount = static_cast<uint32_t>(descPoolSizes.size()),
        .pPoolSizes = descPoolSizes.data()
    };
    r = vkCreateDescriptorPool(
        state->device,
        &ci_descpool,
        nullptr,
        &state->descriptorPool
    );

    for(uint32_t i = 0; i < state->frameCount; ++i)
        initFrame(state, &state->frameRes[i]);

    // Память под данные фреймов.
    state->frameMemory = new GpuMemory(
        state->physicalDevice,
        state->device,
        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT
    );
    state->frameMemory->resize(state->frameCount * 16 * sizeof(float));

    state->stopRendering = false;

    return true;
}

}


VkInstance GpuManager::instance() const
{
    ::initialize(m_pimpl, VulkanInitState::Instance);
    return m_pimpl->vulkanState.instance;
}


void GpuManager::setSurface(VkSurfaceKHR surface)
{
    VulkanState* state = &m_pimpl->vulkanState;
    state->surface = surface;
    state->externalSurface = true;
}


bool GpuManager::initialize()
{
    return ::initialize(m_pimpl, VulkanInitState::End);
}


namespace
{
void updateScene()
{
}

void updateDataForFrame(Application* app, VulkanState* state)
{
    FrameResources& frame = state->frameRes[state->currentFrameIndex];

    // Если следует обновить сцену, то ожидаем завершения заданий в очередях
    // и блокируем рендеринг до обновления вершинных и индексных буферов.
    static void* s_oldSceneHash = nullptr;
    {
        static std::mutex mtx;
        std::lock_guard<std::mutex> lock(mtx);
        auto scene = app->geometry();
        void* nowSceneHash = scene.get();
        if(s_oldSceneHash != nowSceneHash) {
            state->stopRendering = true;
            vkQueueWaitIdle(state->queue);
            s_oldSceneHash = nowSceneHash;
            if(state->memory) {
                delete state->memory;
                state->memory = nullptr;
                delete state->mesh;
                state->mesh = nullptr;
            }
            state->memory = new GpuMemory(
                state->physicalDevice,
                state->device,
                    VK_BUFFER_USAGE_VERTEX_BUFFER_BIT
                  | VK_BUFFER_USAGE_INDEX_BUFFER_BIT
            );
            state->mesh = new MeshChunk(state->memory, scene);
            state->stopRendering = false;
        }
    }
    auto mapping = state->frameMemory->access();
    float* data = reinterpret_cast<float*>(mapping.data());
    data += (16 * state->currentFrameIndex);

    glm::mat4 mvp = app->getProjectionMatrix() * app->getViewMatrix();
    const float* mptr = glm::value_ptr(mvp);
    for(int j = 0; j < 16; ++j) {
        *data = mptr[j];
        ++data;
    }

    VkDescriptorBufferInfo bufferInfo {
        .buffer = state->frameMemory->buffer(),
        .offset = state->currentFrameIndex * sizeof(float) * 16,
        .range = 16 * sizeof(float)
    };
    VkWriteDescriptorSet descWrite {
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .dstSet = frame.descSet,
        .descriptorCount = 1,
        .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .pBufferInfo = &bufferInfo,
    };
    vkUpdateDescriptorSets(
        state->device,
        1,
        &descWrite,
        0,
        nullptr
    );
}
}


bool GpuManager::update()
{
    VulkanState* state = &m_pimpl->vulkanState;
    if(state->stopRendering)
        return false;

    FrameResources& frame = state->frameRes[state->currentFrameIndex];

    // Ожидаем освобождения последнего фрейма.
    if(frame.cmdFenceWaitable) {
        vkWaitForFences(state->device, 1, &frame.cmdFence, VK_TRUE,UINT64_MAX);
        vkResetFences(state->device, 1, &frame.cmdFence);
        frame.cmdFenceWaitable = false;
    }

    updateDataForFrame(m_pimpl->app, state);

    while(true) {
        uint32_t index;
        // BOOST_LOG_TRIVIAL(trace) << "vkAcquireNextImageKHR ...";
        // BOOST_LOG_TRIVIAL(trace) << "Frame index: " << state->currentFrameIndex;
        VkResult r = vkAcquireNextImageKHR(
            state->device,
            state->swapchain,
            UINT64_MAX,
            frame.imageSemaphore,
            VK_NULL_HANDLE,
            &index
        );
        // BOOST_LOG_TRIVIAL(trace) << std::format("r = {}", VkResultToString(r));
        if(r == VK_SUBOPTIMAL_KHR || r == VK_ERROR_OUT_OF_DATE_KHR) {
            // Размеры окна.
            uint32_t width, height;
            std::tie(width, height) = m_pimpl->getImageExtent();
            recreateSwapchain(state, width, height);
            continue;
        }
        if(r == VK_NOT_READY || r == VK_TIMEOUT)
            continue;
        if(r != VK_SUCCESS)
            return false;

        assert(index <= MAX_NUM_IMAGES);
        ImageResources& image = state->imageRes[index];
        render(state, image, frame);

        VkPresentInfoKHR presentInfoKHR {
            .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
            .waitSemaphoreCount = 1,
            .pWaitSemaphores = &frame.drawSemaphore,
            .swapchainCount = 1,
            .pSwapchains = &state->swapchain,
            .pImageIndices = &index,
            //.pResults = &r,
        };
        r = vkQueuePresentKHR(state->queue, &presentInfoKHR);
        if(r != VK_SUCCESS)
            return false;

        break;
    }

    state->currentFrameIndex = (state->currentFrameIndex + 1) % MAX_NUM_FRAMES;
    return true;
}
