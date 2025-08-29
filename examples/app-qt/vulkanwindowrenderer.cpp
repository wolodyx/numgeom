#include "vulkanwindowrenderer.h"

#include <cassert>


VulkanWindowRenderer::VulkanWindowRenderer(QVulkanWindow *w)
    : m_window(w)
{
}


void VulkanWindowRenderer::initResources()
{
    qDebug("initResources");
}


void VulkanWindowRenderer::initSwapChainResources()
{
    qDebug("initSwapChainResources");
}


void VulkanWindowRenderer::releaseSwapChainResources()
{
    qDebug("releaseSwapChainResources");
}


void VulkanWindowRenderer::releaseResources()
{
    qDebug("releaseResources");
}


void VulkanWindowRenderer::startNextFrame()
{
    m_green += 0.005f;
    if (m_green > 1.0f)
        m_green = 0.0f;

    VkClearValue clearValues[2];
    memset(clearValues, 0, sizeof(clearValues));
    clearValues[0].color = VkClearColorValue{{ 0.0f, m_green, 0.0f, 1.0f }};
    clearValues[1].depthStencil = VkClearDepthStencilValue{ 1.0f, 0 };

    VkInstance instance = m_window->vulkanInstance()->vkInstance();
    const QSize sz = m_window->swapChainImageSize();
    VkRenderPassBeginInfo rpBeginInfo {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .renderPass = m_window->defaultRenderPass(),
        .framebuffer = m_window->currentFramebuffer(),
        .renderArea = VkRect2D {
            .extent = VkExtent2D {
                .width = static_cast<uint32_t>(sz.width()),
                .height = static_cast<uint32_t>(sz.height())
            }
        },
        .clearValueCount = 2,
        .pClearValues = clearValues,
    };
    VkCommandBuffer cmdBuf = m_window->currentCommandBuffer();
    vkCmdBeginRenderPass(cmdBuf, &rpBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

    // Do nothing else. We will just clear to green, changing the component on
    // every invocation. This also helps verifying the rate to which the thread
    // is throttled to. (The elapsed time between startNextFrame calls should
    // typically be around 16 ms. Note that rendering is 2 frames ahead of what
    // is displayed.)

    vkCmdEndRenderPass(cmdBuf);

    m_window->frameReady();
    m_window->requestUpdate(); // render continuously, throttled by the presentation rate
}
