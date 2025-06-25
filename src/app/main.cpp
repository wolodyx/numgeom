/*
#include <QApplication>
#include <QCoreApplication>
#include <QCommandLineParser>

#include "mainwindow.h"

int main(int argc, char** argv)
{
    //Q_INIT_RESOURCE(application);
#ifdef Q_OS_ANDROID
    QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
#endif

    QApplication app(argc, argv);
    QCoreApplication::setOrganizationName("numgeom");
    QCoreApplication::setApplicationName("NumGeom App");
    QCoreApplication::setApplicationVersion("0.0.1");
    QCommandLineParser parser;
    parser.setApplicationDescription(QCoreApplication::applicationName());
    parser.addHelpOption();
    parser.addVersionOption();
    parser.addPositionalArgument("file", "The file to open.");
    parser.process(app);

    MainWindow mainWin;
    mainWin.resize(900, 600);
    mainWin.show();
    return app.exec();
}
*/

#include <cstring>
#include <iostream>
#include <set>

#include <QApplication>
#include <QGuiApplication>
#include <QWidget>
#include <QVBoxLayout>
#include <QLabel>
#include <QTimer>
#include <QByteArray>
#include <qpa/qplatformnativeinterface.h>

#include <vulkan/vulkan.h>
#include <vulkan/vulkan_wayland.h>


#define VK_CHECK_RESULT(f) { \
    VkResult res = (f); \
    if (res != VK_SUCCESS) { \
        qFatal("Vulkan error %d at %s:%d", res, __FILE__, __LINE__); \
    } \
}


const std::vector<const char*> validationLayers = {
    //"VK_LAYER_KHRONOS_validation"
    //"VK_LAYER_MESA_device_select",
    "VK_LAYER_MESA_overlay",
    //"VK_LAYER_INTEL_nullhw"
};

#ifdef NDEBUG
    const bool enableValidationLayers = false;
#else
    const bool enableValidationLayers = true;
#endif

VkInstance CreateInstance();
VkSurfaceKHR CreateSurface(VkInstance, QWidget*);
VkSurfaceKHR CreateWaylandSurface(VkInstance instance, QWindow* window);
VkPhysicalDevice SelectPhysicalDevice(
    VkInstance instance,
    VkSurfaceKHR surface
);
VkDevice CreateLogicalDevice(VkPhysicalDevice);


bool checkValidationLayerSupport()
{
    uint32_t layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

    std::vector<VkLayerProperties> availableLayers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());
    // std::cout << "Available layers:" << std::endl;
    // for(auto layer : availableLayers)
    //     std::cout << layer.layerName << std::endl;

    for(const char* layerName : validationLayers)
    {
        bool layerFound = false;

        for(const auto& layerProperties : availableLayers)
        {
            if(strcmp(layerName, layerProperties.layerName) == 0)
            {
                layerFound = true;
                break;
            }
        }

        if(!layerFound)
            return false;
    }

    return true;
}


class VulkanWidget : public QWidget {
public:
    VulkanWidget(QWidget* parent = nullptr) : QWidget(parent) {
        setAttribute(Qt::WA_NativeWindow);
        setAttribute(Qt::WA_PaintOnScreen);
        setMinimumSize(640, 480);

        // Таймер для обновления рендеринга
        QTimer* timer = new QTimer(this);
        connect(timer, &QTimer::timeout, this, [this]() { update(); });
        timer->start(16); // ~60 FPS
    }

    ~VulkanWidget() {
        cleanupVulkan();
    }

protected:
    void showEvent(QShowEvent* event) override {
        QWidget::showEvent(event);
        if (!m_initialized) {
            initVulkan();
            m_initialized = true;
        }
    }

    void paintEvent(QPaintEvent* event) override {
        if (m_initialized) {
            renderFrame();
        }
    }

private:
    bool m_initialized = false;
    VkInstance m_instance = VK_NULL_HANDLE;
    VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;
    VkDevice m_device = VK_NULL_HANDLE;
    VkQueue m_queue = VK_NULL_HANDLE;
    VkSurfaceKHR m_surface = VK_NULL_HANDLE;
    VkSwapchainKHR m_swapchain = VK_NULL_HANDLE;
    VkRenderPass m_renderPass = VK_NULL_HANDLE;
    VkPipelineLayout m_pipelineLayout = VK_NULL_HANDLE;
    VkPipeline m_graphicsPipeline = VK_NULL_HANDLE;
    std::vector<VkFramebuffer> m_swapchainFramebuffers;
    VkCommandPool m_commandPool = VK_NULL_HANDLE;
    std::vector<VkCommandBuffer> m_commandBuffers;
    VkSemaphore m_imageAvailableSemaphore = VK_NULL_HANDLE;
    VkSemaphore m_renderFinishedSemaphore = VK_NULL_HANDLE;
    VkFence m_inFlightFence = VK_NULL_HANDLE;

    void initVulkan() {
        m_instance = CreateInstance();
        //m_surface = CreateSurface(m_instance, this);
        m_surface = CreateWaylandSurface(m_instance, this->windowHandle());
        m_physicalDevice = SelectPhysicalDevice(m_instance, m_surface);
        m_device = CreateLogicalDevice(m_physicalDevice);
        vkGetDeviceQueue(m_device, 0, 0, &m_queue);
        createSwapchain();
        createRenderPass();
        createGraphicsPipeline();
        createFramebuffers();
        createCommandPool();
        createCommandBuffers();
        createSyncObjects();
    }

    void cleanupVulkan() {
        vkDeviceWaitIdle(m_device);

        if (m_inFlightFence != VK_NULL_HANDLE) {
            vkDestroyFence(m_device, m_inFlightFence, nullptr);
        }
        if (m_renderFinishedSemaphore != VK_NULL_HANDLE) {
            vkDestroySemaphore(m_device, m_renderFinishedSemaphore, nullptr);
        }
        if (m_imageAvailableSemaphore != VK_NULL_HANDLE) {
            vkDestroySemaphore(m_device, m_imageAvailableSemaphore, nullptr);
        }
        if (m_commandPool != VK_NULL_HANDLE) {
            vkDestroyCommandPool(m_device, m_commandPool, nullptr);
        }
        for (auto framebuffer : m_swapchainFramebuffers) {
            vkDestroyFramebuffer(m_device, framebuffer, nullptr);
        }
        if (m_graphicsPipeline != VK_NULL_HANDLE) {
            vkDestroyPipeline(m_device, m_graphicsPipeline, nullptr);
        }
        if (m_pipelineLayout != VK_NULL_HANDLE) {
            vkDestroyPipelineLayout(m_device, m_pipelineLayout, nullptr);
        }
        if (m_renderPass != VK_NULL_HANDLE) {
            vkDestroyRenderPass(m_device, m_renderPass, nullptr);
        }
        if (m_swapchain != VK_NULL_HANDLE) {
            vkDestroySwapchainKHR(m_device, m_swapchain, nullptr);
        }
        if (m_device != VK_NULL_HANDLE) {
            vkDestroyDevice(m_device, nullptr);
        }
        if (m_surface != VK_NULL_HANDLE) {
            vkDestroySurfaceKHR(m_instance, m_surface, nullptr);
        }
        if (m_instance != VK_NULL_HANDLE) {
            vkDestroyInstance(m_instance, nullptr);
        }
    }


    void createSwapchain() {
        // Упрощенная реализация - в реальном приложении нужно учитывать форматы, режимы представления и т.д.
        VkSwapchainCreateInfoKHR createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        createInfo.surface = m_surface;
        createInfo.minImageCount = 2; // Двойная буферизация
        createInfo.imageFormat = VK_FORMAT_B8G8R8A8_UNORM;
        createInfo.imageColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
        createInfo.imageExtent = { (uint32_t)width(), (uint32_t)height() };
        createInfo.imageArrayLayers = 1;
        createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        createInfo.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
        createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        createInfo.presentMode = VK_PRESENT_MODE_FIFO_KHR;
        createInfo.clipped = VK_TRUE;

        VK_CHECK_RESULT(vkCreateSwapchainKHR(m_device, &createInfo, nullptr, &m_swapchain));
    }

    void createRenderPass() {
        VkAttachmentDescription colorAttachment = {};
        colorAttachment.format = VK_FORMAT_B8G8R8A8_UNORM;
        colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
        colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

        VkAttachmentReference colorAttachmentRef = {};
        colorAttachmentRef.attachment = 0;
        colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkSubpassDescription subpass = {};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments = &colorAttachmentRef;

        VkRenderPassCreateInfo renderPassInfo = {};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassInfo.attachmentCount = 1;
        renderPassInfo.pAttachments = &colorAttachment;
        renderPassInfo.subpassCount = 1;
        renderPassInfo.pSubpasses = &subpass;

        VK_CHECK_RESULT(vkCreateRenderPass(m_device, &renderPassInfo, nullptr, &m_renderPass));
    }

    void createGraphicsPipeline()
    {
        const uint32_t triangle_vert_spv[] =
        {
            #include "triangle.vert.spv.h"
        };

        const uint32_t triangle_frag_spv[] =
        {
            #include "triangle.frag.spv.h"
        };

        VkShaderModule vShaderModule = createShaderModule(triangle_vert_spv, sizeof(triangle_vert_spv));
        VkShaderModule fShaderModule = createShaderModule(triangle_frag_spv, sizeof(triangle_frag_spv));

        VkPipelineShaderStageCreateInfo shaderStages[] =
        {
            VkPipelineShaderStageCreateInfo
            {
                .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                .stage = VK_SHADER_STAGE_VERTEX_BIT,
                .module = vShaderModule,
                .pName = "main"
            },
            VkPipelineShaderStageCreateInfo
            {
                .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
                .module = fShaderModule,
                .pName = "main"
            }
        };

        VkVertexInputBindingDescription vertexBindingDescriptions[] =
        {
            {
                .binding = 0,
                .stride = 3 * sizeof(float),
                .inputRate = VK_VERTEX_INPUT_RATE_VERTEX
            },
            {
                .binding = 1,
                .stride = 3 * sizeof(float),
                .inputRate = VK_VERTEX_INPUT_RATE_VERTEX
            },
            {
                .binding = 2,
                .stride = 3 * sizeof(float),
                .inputRate = VK_VERTEX_INPUT_RATE_VERTEX
            }
        };

        VkVertexInputAttributeDescription vertexAttributeDescriptions[] =
        {
            {
                .location = 0,
                .binding = 0,
                .format = VK_FORMAT_R32G32B32_SFLOAT,
                .offset = 0
            },
            {
                .location = 1,
                .binding = 1,
                .format = VK_FORMAT_R32G32B32_SFLOAT,
                .offset = 0
            },
            {
                .location = 2,
                .binding = 2,
                .format = VK_FORMAT_R32G32B32_SFLOAT,
                .offset = 0
            }
        };

        VkPipelineVertexInputStateCreateInfo vertexInputInfo = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .vertexBindingDescriptionCount = 3,
            .pVertexBindingDescriptions = vertexBindingDescriptions,
            .vertexAttributeDescriptionCount = 3,
            .pVertexAttributeDescriptions = vertexAttributeDescriptions
        };

        VkPipelineInputAssemblyStateCreateInfo inputAssembly = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
            .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP,
            .primitiveRestartEnable = VK_FALSE
        };

        VkViewport viewport =
        {
            .x = 0.0f,
            .y = 0.0f,
            .width = (float)width(),
            .height = (float)height(),
            .minDepth = 0.0f,
            .maxDepth = 1.0f
        };

        VkRect2D scissor =
        {
            .offset = {0, 0},
            .extent = {(uint32_t)width(), (uint32_t)height()}
        };

        VkPipelineViewportStateCreateInfo viewportState =
        {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
            .viewportCount = 1,
            .pViewports = &viewport,
            .scissorCount = 1,
            .pScissors = &scissor
        };

        VkPipelineRasterizationStateCreateInfo rasterizer =
        {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
            .depthClampEnable = VK_FALSE,
            .rasterizerDiscardEnable = VK_FALSE,
            .polygonMode = VK_POLYGON_MODE_FILL,
            .cullMode = VK_CULL_MODE_BACK_BIT,
            .frontFace = VK_FRONT_FACE_CLOCKWISE,
            .depthBiasEnable = VK_FALSE,
            .lineWidth = 1.0f,
        };

        VkPipelineMultisampleStateCreateInfo multisampling =
        {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
            .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
            .sampleShadingEnable = VK_FALSE,
        };

        VkPipelineDepthStencilStateCreateInfo depthStencil =
        {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
        };

        VkPipelineColorBlendAttachmentState colorBlendAttachment = {
            .blendEnable = VK_FALSE,
            .colorWriteMask = VK_COLOR_COMPONENT_R_BIT
                            | VK_COLOR_COMPONENT_G_BIT
                            | VK_COLOR_COMPONENT_B_BIT
                            | VK_COLOR_COMPONENT_A_BIT,

        };

        VkPipelineColorBlendStateCreateInfo colorBlending = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
            .logicOpEnable = VK_FALSE,
            .attachmentCount = 1,
            .pAttachments = &colorBlendAttachment,
        };

        VkDynamicState dynamicStates[] =
        {
            VK_DYNAMIC_STATE_VIEWPORT,
            VK_DYNAMIC_STATE_SCISSOR,
        };

        VkPipelineDynamicStateCreateInfo dynamic =
        {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
            .dynamicStateCount = 2,
            .pDynamicStates = dynamicStates
        };

        VkDescriptorSetLayoutBinding bindings[] = {
            {
                .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                .descriptorCount = 1,
                .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
                .pImmutableSamplers = NULL
            }
        };

        VkDescriptorSetLayoutCreateInfo setLayoutCreateInfo =
        {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
            .bindingCount = 1,
            .pBindings = bindings
        };

        VkDescriptorSetLayout setLayout;
        vkCreateDescriptorSetLayout(
            m_device,
            &setLayoutCreateInfo,
            nullptr,
            &setLayout
        );

        VkPipelineLayoutCreateInfo pipelineLayoutInfo = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
            .setLayoutCount = 1,
            .pSetLayouts = &setLayout
        };

        VK_CHECK_RESULT(vkCreatePipelineLayout(m_device, &pipelineLayoutInfo, nullptr, &m_pipelineLayout));

        VkGraphicsPipelineCreateInfo pipelineInfo = {
            .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .stageCount = 2,
            .pStages = shaderStages,
            .pVertexInputState = &vertexInputInfo,
            .pInputAssemblyState = &inputAssembly,
            .pTessellationState = nullptr,
            .pViewportState = &viewportState,
            .pRasterizationState = &rasterizer,
            .pMultisampleState = &multisampling,
            .pDepthStencilState = &depthStencil,
            .pColorBlendState = &colorBlending,
            .pDynamicState = &dynamic,
            .layout = m_pipelineLayout,
            .renderPass = m_renderPass,
            .subpass = 0,
            .basePipelineHandle = VkPipeline{0},
            .basePipelineIndex = 0
        };

        VK_CHECK_RESULT(vkCreateGraphicsPipelines(
            m_device,
            VK_NULL_HANDLE,
            1,
            &pipelineInfo,
            nullptr,
            &m_graphicsPipeline));

        vkDestroyShaderModule(m_device, fShaderModule, nullptr);
        vkDestroyShaderModule(m_device, vShaderModule, nullptr);
    }

    VkShaderModule createShaderModule(const uint32_t* code, size_t size) {
        VkShaderModuleCreateInfo createInfo =
        {
            .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
            .codeSize = size,
            .pCode = code
        };

        VkShaderModule shaderModule;
        VK_CHECK_RESULT(vkCreateShaderModule(m_device, &createInfo, nullptr, &shaderModule));
        return shaderModule;
    }

    void createFramebuffers() {
        uint32_t imageCount = 0;
        vkGetSwapchainImagesKHR(m_device, m_swapchain, &imageCount, nullptr);
        std::vector<VkImage> swapchainImages(imageCount);
        vkGetSwapchainImagesKHR(m_device, m_swapchain, &imageCount, swapchainImages.data());

        m_swapchainFramebuffers.resize(imageCount);
        for (size_t i = 0; i < imageCount; i++) {
            VkImageViewCreateInfo createInfo = {};
            createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            createInfo.image = swapchainImages[i];
            createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
            createInfo.format = VK_FORMAT_B8G8R8A8_UNORM;
            createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            createInfo.subresourceRange.baseMipLevel = 0;
            createInfo.subresourceRange.levelCount = 1;
            createInfo.subresourceRange.baseArrayLayer = 0;
            createInfo.subresourceRange.layerCount = 1;

            VkImageView imageView;
            VK_CHECK_RESULT(vkCreateImageView(m_device, &createInfo, nullptr, &imageView));

            VkFramebufferCreateInfo framebufferInfo = {};
            framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            framebufferInfo.renderPass = m_renderPass;
            framebufferInfo.attachmentCount = 1;
            framebufferInfo.pAttachments = &imageView;
            framebufferInfo.width = width();
            framebufferInfo.height = height();
            framebufferInfo.layers = 1;

            VK_CHECK_RESULT(vkCreateFramebuffer(m_device, &framebufferInfo, nullptr, &m_swapchainFramebuffers[i]));
        }
    }

    void createCommandPool() {
        VkCommandPoolCreateInfo poolInfo = {};
        poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        poolInfo.queueFamilyIndex = 0; // Упрощенно - предполагаем, что семейство 0 поддерживает графику
        poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

        VK_CHECK_RESULT(vkCreateCommandPool(m_device, &poolInfo, nullptr, &m_commandPool));
    }

    void createCommandBuffers() {
        m_commandBuffers.resize(m_swapchainFramebuffers.size());

        VkCommandBufferAllocateInfo allocInfo = {};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.commandPool = m_commandPool;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandBufferCount = (uint32_t)m_commandBuffers.size();

        VK_CHECK_RESULT(vkAllocateCommandBuffers(m_device, &allocInfo, m_commandBuffers.data()));

        for (size_t i = 0; i < m_commandBuffers.size(); i++) {
            VkCommandBufferBeginInfo beginInfo = {};
            beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

            VK_CHECK_RESULT(vkBeginCommandBuffer(m_commandBuffers[i], &beginInfo));

            VkRenderPassBeginInfo renderPassInfo = {};
            renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
            renderPassInfo.renderPass = m_renderPass;
            renderPassInfo.framebuffer = m_swapchainFramebuffers[i];
            renderPassInfo.renderArea.offset = {0, 0};
            renderPassInfo.renderArea.extent = {(uint32_t)width(), (uint32_t)height()};

            VkClearValue clearColor = {{{0.0f, 0.0f, 0.8f, 1.0f}}};
            renderPassInfo.clearValueCount = 1;
            renderPassInfo.pClearValues = &clearColor;

            vkCmdBeginRenderPass(m_commandBuffers[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
            vkCmdBindPipeline(m_commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, m_graphicsPipeline);
            vkCmdDraw(m_commandBuffers[i], 3, 1, 0, 0); // Рисуем треугольник
            vkCmdEndRenderPass(m_commandBuffers[i]);

            VK_CHECK_RESULT(vkEndCommandBuffer(m_commandBuffers[i]));
        }
    }

    void createSyncObjects() {
        VkSemaphoreCreateInfo semaphoreInfo = {};
        semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        VkFenceCreateInfo fenceInfo = {};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

        VK_CHECK_RESULT(vkCreateSemaphore(m_device, &semaphoreInfo, nullptr, &m_imageAvailableSemaphore));
        VK_CHECK_RESULT(vkCreateSemaphore(m_device, &semaphoreInfo, nullptr, &m_renderFinishedSemaphore));
        VK_CHECK_RESULT(vkCreateFence(m_device, &fenceInfo, nullptr, &m_inFlightFence));
    }

    void renderFrame() {
        vkWaitForFences(m_device, 1, &m_inFlightFence, VK_TRUE, UINT64_MAX);
        vkResetFences(m_device, 1, &m_inFlightFence);

        uint32_t imageIndex;
        vkAcquireNextImageKHR(m_device, m_swapchain, UINT64_MAX, m_imageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex);

        VkSubmitInfo submitInfo = {};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

        VkSemaphore waitSemaphores[] = {m_imageAvailableSemaphore};
        VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = waitSemaphores;
        submitInfo.pWaitDstStageMask = waitStages;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &m_commandBuffers[imageIndex];

        VkSemaphore signalSemaphores[] = {m_renderFinishedSemaphore};
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = signalSemaphores;

        VK_CHECK_RESULT(vkQueueSubmit(m_queue, 1, &submitInfo, m_inFlightFence));

        VkPresentInfoKHR presentInfo = {};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = signalSemaphores;

        VkSwapchainKHR swapChains[] = {m_swapchain};
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = swapChains;
        presentInfo.pImageIndices = &imageIndex;

        vkQueuePresentKHR(m_queue, &presentInfo);
    }
};

int main(int argc, char *argv[]) {
#ifdef VK_USE_PLATFORM_WAYLAND_KHR
    qputenv("QT_QPA_PLATFORM", "wayland");
#endif
    QApplication app(argc, argv);

    VulkanWidget widget;
    widget.show();

    return app.exec();
}



VkInstance CreateInstance()
{
    if(enableValidationLayers && !checkValidationLayerSupport())
        throw std::runtime_error("validation layers requested, but not available!");

    uint32_t apiVersion;
    vkEnumerateInstanceVersion(&apiVersion);
    std::cout << "apiVersion = " << apiVersion << std::endl;

    VkApplicationInfo appInfo = {};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "Vulkan Triangle";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "No Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_0;

    const char* extensions[] = {
        VK_KHR_SURFACE_EXTENSION_NAME,
#ifdef VK_USE_PLATFORM_XCB_KHR
        VK_KHR_XCB_SURFACE_EXTENSION_NAME,
#elif VK_USE_PLATFORM_WAYLAND_KHR
        VK_KHR_WAYLAND_SURFACE_EXTENSION_NAME,
#elif defined(VK_USE_PLATFORM_WIN32_KHR)
        VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
#endif
        VK_EXT_DEBUG_UTILS_EXTENSION_NAME // Для отладки (опционально)
    };

    VkInstanceCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;
    createInfo.enabledExtensionCount = sizeof(extensions) / sizeof(extensions[0]);
    createInfo.ppEnabledExtensionNames = extensions;
    if(enableValidationLayers)
    {
        createInfo.enabledLayerCount = validationLayers.size();
        createInfo.ppEnabledLayerNames = validationLayers.data();
    }

    VkInstance instance;
    VK_CHECK_RESULT(vkCreateInstance(&createInfo, nullptr, &instance));
    return instance;
}


VkSurfaceKHR CreateSurface(VkInstance instance, QWidget* widget) {
    VkSurfaceKHR surface;
#ifdef VK_USE_PLATFORM_XCB_KHR
    VkXcbSurfaceCreateInfoKHR createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR;
    createInfo.connection = x11Info().connection();
    createInfo.window = winId();
    VK_CHECK_RESULT(vkCreateXcbSurfaceKHR(m_instance, &createInfo, nullptr, &surface));
#elif VK_USE_PLATFORM_WAYLAND_KHR
    VkWaylandSurfaceCreateInfoKHR createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_WAYLAND_SURFACE_CREATE_INFO_KHR;
    createInfo.display = (wl_display*)QGuiApplication::platformNativeInterface()->nativeResourceForWindow("display", widget->windowHandle());
    createInfo.surface = (wl_surface*)QGuiApplication::platformNativeInterface()->nativeResourceForWindow("surface", widget->windowHandle());
    VK_CHECK_RESULT(vkCreateWaylandSurfaceKHR(instance, &createInfo, nullptr, &surface));
#elif defined(VK_USE_PLATFORM_WIN32_KHR)
    VkWin32SurfaceCreateInfoKHR createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
    createInfo.hwnd = (HWND)winId();
    createInfo.hinstance = GetModuleHandle(nullptr);
    VK_CHECK_RESULT(vkCreateWin32SurfaceKHR(instance, &createInfo, nullptr, &surface));
#endif
    return surface;
}


VkSurfaceKHR CreateWaylandSurface(VkInstance instance, QWindow* window)
{
    // 1. Получаем Wayland display и surface через Qt API
    auto nativeInterface = QGuiApplication::platformNativeInterface();
    if (!nativeInterface) {
        throw std::runtime_error("Failed to get platform native interface");
    }

    // 2. Получаем Wayland объекты
    struct wl_display* waylandDisplay = static_cast<struct wl_display*>(
        nativeInterface->nativeResourceForWindow("display", window));
    struct wl_surface* waylandSurface = static_cast<struct wl_surface*>(
        nativeInterface->nativeResourceForWindow("surface", window));

    if(!waylandDisplay || !waylandSurface)
        throw std::runtime_error("Failed to get Wayland display/surface");

    // 3. Настраиваем информацию для создания поверхности
    VkWaylandSurfaceCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_WAYLAND_SURFACE_CREATE_INFO_KHR;
    createInfo.display = waylandDisplay;
    createInfo.surface = waylandSurface;

    // 4. Создаем поверхность
    VkSurfaceKHR surface;
    VkResult result = vkCreateWaylandSurfaceKHR(instance, &createInfo, nullptr, &surface);
    if(result != VK_SUCCESS)
        throw std::runtime_error("Failed to create Wayland surface");

    return surface;
}


VkPhysicalDevice SelectPhysicalDevice(VkInstance instance, VkSurfaceKHR surface) {
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
    if (deviceCount == 0) {
        throw std::runtime_error("No Vulkan-compatible GPUs found");
    }

    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

    // Структура для хранения информации о подходящем устройстве
    struct {
        VkPhysicalDevice device = VK_NULL_HANDLE;
        int score = 0;
        uint32_t graphicsFamily = UINT32_MAX;
        uint32_t presentFamily = UINT32_MAX;
    } bestDevice;

    // Оцениваем каждое устройство
    for (const auto& device : devices) {
        int currentScore = 0;
        uint32_t currentGraphicsFamily = UINT32_MAX;
        uint32_t currentPresentFamily = UINT32_MAX;

        // 1. Проверяем свойства устройства
        VkPhysicalDeviceProperties deviceProperties;
        vkGetPhysicalDeviceProperties(device, &deviceProperties);

        // 2. Проверяем возможности устройства
        VkPhysicalDeviceFeatures deviceFeatures;
        vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

        // 3. Проверяем поддержку очередей
        uint32_t queueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);
        std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

        // Ищем семейства очередей для графики и презентации
        for (uint32_t i = 0; i < queueFamilyCount; i++) {
            // Проверяем поддержку графики
            if (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
                currentGraphicsFamily = i;
            }

            // Проверяем поддержку презентации
            VkBool32 presentSupport = false;
            vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);
            if (presentSupport) {
                currentPresentFamily = i;
            }
        }

        // Устройство должно поддерживать как графику, так и презентацию
        if (currentGraphicsFamily == UINT32_MAX || currentPresentFamily == UINT32_MAX) {
            continue; // Пропускаем неподходящие устройства
        }

        // 4. Проверяем поддержку свопчейна
        uint32_t formatCount;
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);
        uint32_t presentModeCount;
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);
        if (formatCount == 0 || presentModeCount == 0) {
            continue; // Устройство не поддерживает свопчейн
        }

        // 5. Проверяем поддержку необходимых расширений
        const std::vector<const char*> requiredExtensions = {
            VK_KHR_SWAPCHAIN_EXTENSION_NAME
        };

        uint32_t extensionCount;
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);
        std::vector<VkExtensionProperties> availableExtensions(extensionCount);
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

        std::set<std::string> requiredExtensionsSet(requiredExtensions.begin(), requiredExtensions.end());
        for (const auto& extension : availableExtensions) {
            requiredExtensionsSet.erase(extension.extensionName);
        }
        if (!requiredExtensionsSet.empty()) {
            continue; // Не все расширения поддерживаются
        }

        // 6. Система оценки устройств
        // Дискретная видеокарта получает больше очков
        if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
            currentScore += 1000;
        }

        // Максимальный размер текстур увеличивает оценку
        currentScore += deviceProperties.limits.maxImageDimension2D;

        // Устройства без geometry shaders получают 0 очков
        if (!deviceFeatures.geometryShader) {
            currentScore = 0;
        }

        // 7. Обновляем лучшее устройство
        if (currentScore > bestDevice.score) {
            bestDevice.device = device;
            bestDevice.score = currentScore;
            bestDevice.graphicsFamily = currentGraphicsFamily;
            bestDevice.presentFamily = currentPresentFamily;
        }
    }

    if (bestDevice.device == VK_NULL_HANDLE) {
        throw std::runtime_error("Failed to find a suitable GPU");
    }

    return bestDevice.device;
}


//! Ищем семейство с поддержкой графики
uint32_t GetGraphicsFamilyIndex(VkPhysicalDevice physicalDevice)
{
    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, nullptr);
    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, queueFamilies.data());

    // Ищем семейство с поддержкой графики
    for(uint32_t i = 0; i < queueFamilyCount; i++)
    {
        if(queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
            return i;
    }
    return UINT32_MAX;
}


bool CheckDeviceExtensionSupport(
    VkPhysicalDevice physicalDevice,
    const std::vector<const char*>& requiredExtensions)
{
    std::set<std::string> availableExtensions;
    {
        uint32_t extensionCount;
        vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, nullptr);
        std::vector<VkExtensionProperties> extensions(extensionCount);
        vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, extensions.data());
        for(const auto& ext : extensions)
            availableExtensions.insert(ext.extensionName);
    }

    for(const char* ext : requiredExtensions)
    {
        if(availableExtensions.find(ext) == availableExtensions.end())
            return false;
    }

    return true;
}


VkDevice CreateLogicalDevice(VkPhysicalDevice physicalDevice)
{
    static float queuePriority = 1.0f;
    std::vector<const char*> deviceExtensions =
    {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };

    if(!CheckDeviceExtensionSupport(physicalDevice,deviceExtensions))
        throw std::runtime_error("Device Extensions don't supported");

    uint32_t graphicsFamily = GetGraphicsFamilyIndex(physicalDevice);
    if (graphicsFamily == UINT32_MAX)
        throw std::runtime_error("No graphics queue family found");
    std::cout << "Physical Device = " << physicalDevice << std::endl;
    std::cout << "Graphics Family Index = " << graphicsFamily << std::endl;

    VkDeviceQueueCreateInfo queueCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .queueFamilyIndex = graphicsFamily,
        .queueCount = 1,
        .pQueuePriorities = &queuePriority
    };

    VkPhysicalDeviceFeatures deviceFeatures = {};

    VkDeviceCreateInfo createInfo = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .queueCreateInfoCount = 1,
        .pQueueCreateInfos = &queueCreateInfo,
        .enabledLayerCount = 0,
        .ppEnabledLayerNames = nullptr,
        .enabledExtensionCount = sizeof(deviceExtensions) / sizeof(deviceExtensions[0]),
        .ppEnabledExtensionNames = deviceExtensions.data(),
        .pEnabledFeatures = &deviceFeatures
    };

    if(enableValidationLayers)
    {
        createInfo.enabledLayerCount = validationLayers.size();
        createInfo.ppEnabledLayerNames = validationLayers.data();
    }

    VkDevice logicalDevice;
    VK_CHECK_RESULT(vkCreateDevice(physicalDevice, &createInfo, nullptr, &logicalDevice));
    return logicalDevice;
}
