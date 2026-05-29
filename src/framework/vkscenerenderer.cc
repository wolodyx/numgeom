#include "numgeom/vkscenerenderer.h"

#include <array>
#include <cassert>
#include <format>
#include <map>
#include <mutex>
#include <optional>
#include <set>
#include <vector>

#if defined(VK_USE_PLATFORM_XCB_KHR)
#include "xcb/xcb.h"
#endif
#if defined(_WIN32)
#include "windows.h"
#endif

#include "glm/glm.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "glm/gtx/string_cast.hpp"

#define VMA_IMPLEMENTATION
#include "vk_mem_alloc.h"

#include "boost/log/trivial.hpp"

#include "numgeom/application.h"
#include "numgeom/drawable.h"
#include "numgeom/scene.h"
#include "numgeom/sceneobject.h"
#include "numgeom/screentext.h"

#include "applicationinner.h"
#include "foregroundimage.h"
#include "sceneinner.h"
#include "sceneiterators.h"
#include "screentextds.h"
#include "screentextinner.h"
#include "vkutilities.h"

// Количество изображений в обращении.
#define MAX_NUM_IMAGES 5
// Количество фреймов -- одновременно обрабатываемых GPU изображений.
#define MAX_NUM_FRAMES 2

#ifdef NDEBUG
#undef ENABLE_VALIDATION_LAYERS
#else
#define ENABLE_VALIDATION_LAYERS
#endif

namespace {
;
struct ImageResources {
  //! Изображение для показа.
  VkImage image = VK_NULL_HANDLE;

  //! Вид изображения для показа.
  VkImageView image_view = VK_NULL_HANDLE;

  //! Сэмплированное изображение, создается только при
  //! `VulkanState::sampleCount != VK_SAMPLE_COUNT_1_BIT`.
  VkImage msaa_image = VK_NULL_HANDLE;

  // Вид сэмплированного изображения.
  VkImageView msaa_image_view = VK_NULL_HANDLE;

  //! Фреймбуфер.
  VkFramebuffer frame_buffer = VK_NULL_HANDLE;

  //! https://docs.vulkan.org/guide/latest/swapchain_semaphore_reuse.html
  VkSemaphore submit_semaphore = VK_NULL_HANDLE;
};

struct FrameResources {
  VkDescriptorSet desc_set = VK_NULL_HANDLE;
  VkCommandBuffer cmd_buf = VK_NULL_HANDLE;
  VkSemaphore acquire_semaphore = VK_NULL_HANDLE;
  VkFence cmd_fence = VK_NULL_HANDLE;
  bool cmd_fence_waitable = false;
};

//! Структура, повторяющая расположение uniform-переменных в вершинном шейдере.
//! Следует учитывать выравнивание по правилам std140 (16 байт).
struct VertexBufferObject {
  glm::mat4 mvp_matrix;
  glm::mat4 mv_matrix;
  glm::vec4 normal_matrix_row0;
  glm::vec4 normal_matrix_row1;
  glm::vec4 normal_matrix_row2;
};

//! Структура, повторяющая расположение uniform-переменных во фрагментном шейдере.
//! Следует учитывать выравнивание по правилам std140 (16 байт).
struct FragmentBufferObject {
  glm::vec4 light_pos;
  glm::vec4 view_pos;
};

//! Состояние vulkan и его объектов.
struct VulkanState {
  VkInstance instance = VK_NULL_HANDLE;
  VkDebugUtilsMessengerEXT debug_messenger;
  VkPhysicalDevice physical_device = VK_NULL_HANDLE;
  VkPhysicalDeviceMemoryProperties memory_properties;
  VkDevice device = VK_NULL_HANDLE;
  VmaAllocator allocator;
  std::optional<uint32_t> graphics_family_index, present_family_index;
  VkQueue queue = VK_NULL_HANDLE;

  VkFormat image_format = VK_FORMAT_UNDEFINED;
  VkColorSpaceKHR color_space = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
  VkFormat depth_stencil_format = VK_FORMAT_D24_UNORM_S8_UINT;
  VkPresentModeKHR present_mode = VK_PRESENT_MODE_FIFO_KHR;
  uint32_t min_image_count = 2;
  VkSampleCountFlagBits sample_count = VK_SAMPLE_COUNT_1_BIT;
  uint32_t frame_count = MAX_NUM_FRAMES;

  VkRenderPass renderpass = VK_NULL_HANDLE;

  VkPipelineLayout pipeline_layout = VK_NULL_HANDLE;
  VkPipeline pipeline = VK_NULL_HANDLE;
  VkPipeline text_pipeline = VK_NULL_HANDLE;
  VkPipeline fgimage_pipeline = VK_NULL_HANDLE;

  VkCommandPool cmd_pool = VK_NULL_HANDLE;
  VkDescriptorSetLayout desc_layout = VK_NULL_HANDLE;
  VkDescriptorPool descriptor_pool = VK_NULL_HANDLE;
};

//! Ресурсы Vulkan для отрисовки изображения на переднем плане сцены.
struct FgImageObjects {
  VkImage logo_image = VK_NULL_HANDLE;
  VmaAllocation alloc_logo_image = VK_NULL_HANDLE;
  VkImageView logo_image_view = VK_NULL_HANDLE;
  VkSampler logo_sampler = VK_NULL_HANDLE;

  bool operator!() const { return logo_image == VK_NULL_HANDLE; }
  void Release(VulkanState*);
  void Render(VkCommandBuffer cmd_buf, VkPipeline image_pipeline,
              VkPipelineLayout pipeline_layout,
              const ForegroundImage& fg_image,
              const VkExtent2D& image_extent) const;
  bool Initialize(const VulkanState*, const ForegroundImage&);
};

//! Ресурсы для отображения текста на переднем плане сцены.
struct FgTextObjects {
  VkBuffer buffer_text_vertex = VK_NULL_HANDLE;
  VmaAllocation alloc_text_vertex = VK_NULL_HANDLE;
  VkBuffer buffer_text_index = VK_NULL_HANDLE;
  VmaAllocation alloc_text_index = VK_NULL_HANDLE;
  uint32_t text_index_count = 0;
  VkImage text_image = VK_NULL_HANDLE;
  VmaAllocation alloc_text_image = VK_NULL_HANDLE;
  VkImageView text_image_view = VK_NULL_HANDLE;
  VkSampler text_sampler = VK_NULL_HANDLE;
  VkBuffer buffer_text_color = VK_NULL_HANDLE;
  VmaAllocation alloc_text_color = VK_NULL_HANDLE;

  bool operator!() const { return buffer_text_vertex == VK_NULL_HANDLE; }
  bool Initialize(VulkanState*, const ScreenText*);
  void Release(VulkanState*);
  void Render(VkCommandBuffer cmd_buf, VkPipeline pipeline) const;
  bool InitQuad(VulkanState*, const VkExtent2D&, const ScreenText*);
};

struct VulkanObjects {
  VulkanObjects(VulkanState* s) {
    vk_state = s;
  }

  VulkanState* vk_state;

  VkSurfaceKHR surface = VK_NULL_HANDLE;
  bool is_external_surface = false;

  std::array<FrameResources, MAX_NUM_FRAMES> frame_res;
  size_t current_frame_index = 0;
  bool stop_rendering = true; //!< Признак остановки рендеринга.

  VkSwapchainKHR swapchain = VK_NULL_HANDLE;
  uint32_t image_count = 0;
  VkExtent2D image_extent;
  std::array<ImageResources, MAX_NUM_IMAGES> image_res;
  VkImage depth_stencil_image = VK_NULL_HANDLE;
  VkDeviceMemory depth_stencil_mem = VK_NULL_HANDLE;
  VkImageView depth_stencil_view = VK_NULL_HANDLE;
  VkDeviceMemory msaa_image_mem = VK_NULL_HANDLE;

  //! Буферы и память под данные для отправки на GPU.
  //! \{
  VkBuffer buffer_vertex = VK_NULL_HANDLE;
  VmaAllocation alloc_vertex = VK_NULL_HANDLE;
  VkBuffer buffer_index = VK_NULL_HANDLE;
  VmaAllocation alloc_index = VK_NULL_HANDLE;
  VkBuffer buffer_normal = VK_NULL_HANDLE;
  VmaAllocation alloc_normal = VK_NULL_HANDLE;
  VkBuffer buffer_color = VK_NULL_HANDLE;
  VmaAllocation alloc_color = VK_NULL_HANDLE;
  VkBuffer buffer_frame = VK_NULL_HANDLE;
  VmaAllocation alloc_frame = VK_NULL_HANDLE;
  //! \}

  uint32_t index_count = 0; //!< Количество примитивов для отрисовки.

  FgImageObjects fg_image_objects;
  const ScreenText* screen_text = nullptr;
  FgTextObjects fg_text_objects;
};

//! Prototypes
//! \{
bool InitInstance(VulkanState*);
//! \}
}  // namespace

struct VkSceneRenderer::Impl {
  Application* app;
#ifdef LINUX
  struct Xcb {
    xcb_connection_t* connection = nullptr;
    xcb_window_t window = 0;
  };
  Xcb xcb;
#endif
  VulkanState vulkanState;
  std::map<Scene*, VulkanObjects*> scenes_vulkan_objects;
};

VkSceneRenderer::VkSceneRenderer(Application* app) {
  assert(app != nullptr);

  impl_ = new Impl {
      .app = app,
  };

  if (!InitInstance(&impl_->vulkanState)) return;
}

namespace
{
void Finalize(VulkanState* state) {
  vkDestroyPipeline(state->device, state->pipeline, nullptr);
  state->pipeline = VK_NULL_HANDLE;
  if (state->fgimage_pipeline != VK_NULL_HANDLE) {
    vkDestroyPipeline(state->device, state->fgimage_pipeline, nullptr);
    state->fgimage_pipeline = VK_NULL_HANDLE;
  }

  vkDestroyPipelineLayout(state->device,state->pipeline_layout,nullptr);
  vkDestroyDescriptorSetLayout(state->device,state->desc_layout,nullptr);
  vkDestroyDescriptorPool(state->device,state->descriptor_pool,nullptr);

  vkDestroyRenderPass(state->device,state->renderpass,nullptr);
  state->renderpass = VK_NULL_HANDLE;

  vkDestroyCommandPool(state->device,state->cmd_pool,nullptr);
  state->cmd_pool = VK_NULL_HANDLE;

  // Destroy text pipeline
  if (state->text_pipeline != VK_NULL_HANDLE) {
    vkDestroyPipeline(state->device, state->text_pipeline, nullptr);
    state->text_pipeline = VK_NULL_HANDLE;
  }

  vmaDestroyAllocator(state->allocator);

  vkDestroyDevice(state->device, nullptr);

  // vkDestroyInstance(state->instance, nullptr);
  // state->instance = VK_NULL_HANDLE;
}
}

VkSceneRenderer::~VkSceneRenderer() {
  ::Finalize(&impl_->vulkanState);
  delete impl_;
}

namespace {
;

VkSampleCountFlagBits GetMaxUsableSampleCount(VkPhysicalDevice device) {
  VkPhysicalDeviceProperties properties;
  vkGetPhysicalDeviceProperties(device, &properties);

  VkSampleCountFlags counts = properties.limits.framebufferColorSampleCounts &
                              properties.limits.framebufferDepthSampleCounts;

  if (counts & VK_SAMPLE_COUNT_64_BIT) return VK_SAMPLE_COUNT_64_BIT;
  if (counts & VK_SAMPLE_COUNT_32_BIT) return VK_SAMPLE_COUNT_32_BIT;
  if (counts & VK_SAMPLE_COUNT_16_BIT) return VK_SAMPLE_COUNT_16_BIT;
  if (counts & VK_SAMPLE_COUNT_8_BIT) return VK_SAMPLE_COUNT_8_BIT;
  if (counts & VK_SAMPLE_COUNT_4_BIT) return VK_SAMPLE_COUNT_4_BIT;
  if (counts & VK_SAMPLE_COUNT_2_BIT) return VK_SAMPLE_COUNT_2_BIT;
  return VK_SAMPLE_COUNT_1_BIT;
}

bool CreateRenderPass(VulkanState* state) {
  bool msaa = (state->sample_count != VK_SAMPLE_COUNT_1_BIT);

  std::vector<VkAttachmentDescription> attachments = {
      {
          // Non-msaa render target or resolve target.
          .format = state->image_format,
          .samples = VK_SAMPLE_COUNT_1_BIT,
          .loadOp = msaa ? VK_ATTACHMENT_LOAD_OP_DONT_CARE
                         : VK_ATTACHMENT_LOAD_OP_CLEAR,
          .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
          .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
          .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
          .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
          .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
      },
      {
          // Depth attachment with multisampling
          .format = state->depth_stencil_format,
          .samples = state->sample_count,
          .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
          .storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
          .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
          .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
          .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
          .finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
      },
      {
          // Multisampled color attachment
          .format = state->image_format,
          .samples = state->sample_count,
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
      .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};
  VkAttachmentReference depthAttachmentRef = {
      .attachment = 1,
      .layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL};
  VkAttachmentReference resolveAttachmentRef = {
      .attachment = 0, .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};

  std::vector<VkSubpassDescription> subpasses = {
      {.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
       .colorAttachmentCount = static_cast<uint32_t>(1),
       .pColorAttachments = &colorAttachmentRef,
       .pResolveAttachments = msaa ? &resolveAttachmentRef : nullptr,
       .pDepthStencilAttachment = &depthAttachmentRef}};

  std::vector<VkSubpassDependency> dependencies = {{
      .srcSubpass = VK_SUBPASS_EXTERNAL,
      .dstSubpass = 0,
      .srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
      .dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
      .srcAccessMask = 0,
      .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
  }};

  VkRenderPassCreateInfo ci_renderpass{
      .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
      .attachmentCount = static_cast<uint32_t>(msaa ? 3 : 2),
      .pAttachments = attachments.data(),
      .subpassCount = static_cast<uint32_t>(subpasses.size()),
      .pSubpasses = subpasses.data(),
      .dependencyCount = static_cast<uint32_t>(dependencies.size()),
      .pDependencies = dependencies.data(),
  };

  VkResult r = vkCreateRenderPass(state->device, &ci_renderpass, nullptr,
                                  &state->renderpass);
  if (r != VK_SUCCESS) {
    BOOST_LOG_TRIVIAL(trace) << std::format(
        "RenderPass object creation error ({}).", VkResultToString(r));
    return false;
  }

  if (state->renderpass == VK_NULL_HANDLE) {
    BOOST_LOG_TRIVIAL(trace) << "Renderpass creation error.";
    return false;
  }
  BOOST_LOG_TRIVIAL(trace) << std::format(
      "Renderpass {} is created.", static_cast<void*>(state->renderpass));
  return true;
}

bool CreateGraphicsPipeline(VulkanState* state) {
  VkResult r;

  // Создаем макеты набора дескрипторов.
  std::vector<VkDescriptorSetLayoutBinding> descriptorSetLayoutBindings{
      VkDescriptorSetLayoutBinding{
          .binding = 0,
          .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
          .descriptorCount = 1,
          .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
      },
      VkDescriptorSetLayoutBinding{
          .binding = 1,
          .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
          .descriptorCount = 1,
          .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
      },
      VkDescriptorSetLayoutBinding{
          .binding = 2,
          .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
          .descriptorCount = 1,
          .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
      },
      // Text texture sampler (binding 3)
      VkDescriptorSetLayoutBinding{
          .binding = 3,
          .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
          .descriptorCount = 1,
          .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
      },
      // Text color uniform buffer (binding 4)
      VkDescriptorSetLayoutBinding{
          .binding = 4,
          .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
          .descriptorCount = 1,
          .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
      },
  };
  VkDescriptorSetLayoutCreateInfo ci_descriptorSetLayout{
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
      .bindingCount = static_cast<uint32_t>(descriptorSetLayoutBindings.size()),
      .pBindings = descriptorSetLayoutBindings.data()};
  r = vkCreateDescriptorSetLayout(state->device, &ci_descriptorSetLayout,
                                  nullptr, &state->desc_layout);
  if (r != VK_SUCCESS) {
    BOOST_LOG_TRIVIAL(trace) << std::format(
        "Descriptor set layout creation error ({}).", VkResultToString(r));
    return false;
  }

  // Создаем макет конвейера.
  std::array<VkDescriptorSetLayout, 1> descriptorSetLayouts{state->desc_layout};

  // Push constant range for foreground image quad parameters
  VkPushConstantRange push_constant_range{
      .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
      .offset = 0,
      .size = 6 * sizeof(float),  // position (2) + size (2) + screenSize (2)
  };

  VkPipelineLayoutCreateInfo ci_pipelineLayout{
      .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
      .setLayoutCount = static_cast<uint32_t>(descriptorSetLayouts.size()),
      .pSetLayouts = descriptorSetLayouts.data(),
      .pushConstantRangeCount = 1,
      .pPushConstantRanges = &push_constant_range,
  };
  r = vkCreatePipelineLayout(state->device, &ci_pipelineLayout, nullptr,
                             &state->pipeline_layout);
  if (r != VK_SUCCESS) {
    BOOST_LOG_TRIVIAL(trace) << std::format(
        "Pipeline layout creation error ({}).", VkResultToString(r));
    return false;
  }

  // Подготавливаем шейдерные модули (вершинный и фрагментный).

  static uint32_t vs_spirv_source[] = {
#include "app.vert.spv.h"
  };
  VkShaderModuleCreateInfo ci_vsModule{
      .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
      .codeSize = sizeof(vs_spirv_source),
      .pCode = (uint32_t*)vs_spirv_source,
  };
  VkShaderModule vsModule = VK_NULL_HANDLE;
  r = vkCreateShaderModule(state->device, &ci_vsModule, nullptr, &vsModule);
  if (r != VK_SUCCESS) {
    BOOST_LOG_TRIVIAL(trace) << std::format(
        "Vertex shader creation error ({}).", VkResultToString(r));
    return false;
  }

  static uint32_t fs_spirv_source[] = {
#include "app.frag.spv.h"
  };
  VkShaderModuleCreateInfo ci_fsModule{
      .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
      .codeSize = sizeof(fs_spirv_source),
      .pCode = (uint32_t*)fs_spirv_source,
  };
  VkShaderModule fsModule = VK_NULL_HANDLE;
  r = vkCreateShaderModule(state->device, &ci_fsModule, nullptr, &fsModule);
  if (r != VK_SUCCESS) {
    BOOST_LOG_TRIVIAL(trace) << std::format(
        "Fragment shader creation error ({}).", VkResultToString(r));
    return false;
  }

  std::array<VkPipelineShaderStageCreateInfo, 2> ci_stages = {
      VkPipelineShaderStageCreateInfo{
          .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
          .stage = VK_SHADER_STAGE_VERTEX_BIT,
          .module = vsModule,
          .pName = "main",
      },
      VkPipelineShaderStageCreateInfo{
          .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
          .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
          .module = fsModule,
          .pName = "main",
      },
  };

  std::vector<VkVertexInputBindingDescription> vertex_binding_descs = {
      VkVertexInputBindingDescription{
        .binding = 0,
        .stride = 3 * sizeof(float),
        .inputRate = VK_VERTEX_INPUT_RATE_VERTEX
      },
      VkVertexInputBindingDescription{
        .binding = 1,
        .stride = 3 * sizeof(float),
        .inputRate = VK_VERTEX_INPUT_RATE_VERTEX
      },
      VkVertexInputBindingDescription{
        .binding = 2,
        .stride = 3 * sizeof(float),
        .inputRate = VK_VERTEX_INPUT_RATE_VERTEX
      },
  };
  std::vector<VkVertexInputAttributeDescription> vertex_attr_descs = {
      VkVertexInputAttributeDescription{
        .location = 0,
        .binding = 0,
        .format = VK_FORMAT_R32G32B32_SFLOAT,
        .offset = 0
      },
      VkVertexInputAttributeDescription{
        .location = 1,
        .binding = 1,
        .format = VK_FORMAT_R32G32B32_SFLOAT,
        .offset = 0
      },
      VkVertexInputAttributeDescription{
        .location = 2,
        .binding = 2,
        .format = VK_FORMAT_R32G32B32_SFLOAT,
        .offset = 0
      },
  };
  VkPipelineVertexInputStateCreateInfo ci_vertexInputState{
      .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
      .vertexBindingDescriptionCount = static_cast<uint32_t>(vertex_binding_descs.size()),
      .pVertexBindingDescriptions = vertex_binding_descs.data(),
      .vertexAttributeDescriptionCount = static_cast<uint32_t>(vertex_attr_descs.size()),
      .pVertexAttributeDescriptions = vertex_attr_descs.data()
  };

  VkPipelineInputAssemblyStateCreateInfo ci_inputAssemblyState{
      .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
      .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
      .primitiveRestartEnable = false,
  };

  VkPipelineViewportStateCreateInfo ci_viewportState{
      .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
      .viewportCount = 1,
      .scissorCount = 1,
  };

  VkPipelineRasterizationStateCreateInfo ci_rasterizationState{
      .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
      .rasterizerDiscardEnable = false,
      .polygonMode = VK_POLYGON_MODE_FILL,
      .cullMode = VK_CULL_MODE_BACK_BIT,
      .frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
      .lineWidth = 1.0f,
  };

  VkPipelineMultisampleStateCreateInfo ci_multisampleState{
      .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
      .rasterizationSamples = state->sample_count,
  };

  VkPipelineDepthStencilStateCreateInfo ci_depthStencilState{
      .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
      .depthTestEnable = VK_TRUE,
      .depthWriteEnable = VK_TRUE,
      .depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL,
  };

  VkPipelineColorBlendAttachmentState attachmentStates[] = {
      {.colorWriteMask = VK_COLOR_COMPONENT_A_BIT | VK_COLOR_COMPONENT_R_BIT |
                         VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT},
  };
  VkPipelineColorBlendStateCreateInfo ci_colorBlendState{
      .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
      .attachmentCount = 1,
      .pAttachments = attachmentStates};

  VkDynamicState dynamicStates[] = {
      VK_DYNAMIC_STATE_VIEWPORT,
      VK_DYNAMIC_STATE_SCISSOR,
  };
  VkPipelineDynamicStateCreateInfo ci_pipelineDynamicState = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
      .dynamicStateCount = 2,
      .pDynamicStates = dynamicStates,
  };

  VkGraphicsPipelineCreateInfo ci_pipelines[] = {{
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
      .layout = state->pipeline_layout,
      .renderPass = state->renderpass,
      .subpass = 0,
  }};
  VkPipeline pipelines[] = {VK_NULL_HANDLE};
  r = vkCreateGraphicsPipelines(state->device, VK_NULL_HANDLE, 1, ci_pipelines,
                                nullptr, pipelines);
  if (r != VK_SUCCESS) {
    BOOST_LOG_TRIVIAL(trace) << std::format(
        "Graphics pipeline creation error ({}).", VkResultToString(r));
    return false;
  }
  state->pipeline = pipelines[0];

  if (state->pipeline == VK_NULL_HANDLE) {
    BOOST_LOG_TRIVIAL(trace) << "Graphics pipeline creation error.";
    return false;
  }
  BOOST_LOG_TRIVIAL(trace) << std::format("Graphics pipeline {} is created.",
                                          static_cast<void*>(state->pipeline));

  vkDestroyShaderModule(state->device, vsModule, nullptr);
  vkDestroyShaderModule(state->device, fsModule, nullptr);

  return true;
}

bool CreateFgImagePipeline(VulkanState* state) {
  VkResult r;

  // Подготавливаем шейдерные модули для логотипа.
  static uint32_t vs_spirv_source[] = {
#include "fgimage.vert.spv.h"
  };
  VkShaderModuleCreateInfo ci_vs_module{
      .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
      .codeSize = sizeof(vs_spirv_source),
      .pCode = (uint32_t*)vs_spirv_source,
  };
  VkShaderModule vs_module = VK_NULL_HANDLE;
  r = vkCreateShaderModule(state->device, &ci_vs_module, nullptr, &vs_module);
  if (r != VK_SUCCESS) {
    BOOST_LOG_TRIVIAL(trace) << std::format(
        "Foreground image vertex shader creation error ({}).", VkResultToString(r));
    return false;
  }

  static uint32_t fs_spirv_source[] = {
#include "fgimage.frag.spv.h"
  };
  VkShaderModuleCreateInfo ci_fs_module{
      .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
      .codeSize = sizeof(fs_spirv_source),
      .pCode = (uint32_t*)fs_spirv_source,
  };
  VkShaderModule fs_module = VK_NULL_HANDLE;
  r = vkCreateShaderModule(state->device, &ci_fs_module, nullptr, &fs_module);
  if (r != VK_SUCCESS) {
    BOOST_LOG_TRIVIAL(trace) << std::format(
        "Foreground image fragment shader creation error ({}).", VkResultToString(r));
    vkDestroyShaderModule(state->device, vs_module, nullptr);
    return false;
  }

  std::array<VkPipelineShaderStageCreateInfo, 2> ci_stages = {
      VkPipelineShaderStageCreateInfo{
          .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
          .stage = VK_SHADER_STAGE_VERTEX_BIT,
          .module = vs_module,
          .pName = "main",
      },
      VkPipelineShaderStageCreateInfo{
          .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
          .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
          .module = fs_module,
          .pName = "main",
      },
  };

  // No vertex buffers -- quad data is generated in the vertex shader
  VkPipelineVertexInputStateCreateInfo ci_vertex_input_state {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
  };

  VkPipelineInputAssemblyStateCreateInfo ci_input_assembly_state{
      .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
      .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
      .primitiveRestartEnable = false,
  };

  VkPipelineViewportStateCreateInfo ci_viewport_state{
      .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
      .viewportCount = 1,
      .scissorCount = 1,
  };

  VkPipelineRasterizationStateCreateInfo ci_rasterization_state{
      .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
      .rasterizerDiscardEnable = false,
      .polygonMode = VK_POLYGON_MODE_FILL,
      .cullMode = VK_CULL_MODE_NONE, // no culling for 2D quad
      .frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
      .lineWidth = 1.0f,
  };

  VkPipelineMultisampleStateCreateInfo ci_multisample_state{
      .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
      .rasterizationSamples = state->sample_count,
  };

  // Disable depth test for overlay
  VkPipelineDepthStencilStateCreateInfo ci_depth_stencil_state{
      .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
      .depthTestEnable = VK_FALSE,
      .depthWriteEnable = VK_FALSE,
      .depthCompareOp = VK_COMPARE_OP_ALWAYS,
  };

  // Alpha blending
  VkPipelineColorBlendAttachmentState attachment_state = {
      .blendEnable = VK_TRUE,
      .srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
      .dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
      .colorBlendOp = VK_BLEND_OP_ADD,
      .srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
      .dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
      .alphaBlendOp = VK_BLEND_OP_ADD,
      .colorWriteMask = VK_COLOR_COMPONENT_A_BIT | VK_COLOR_COMPONENT_R_BIT |
                        VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT,
  };
  VkPipelineColorBlendStateCreateInfo ci_color_blend_state{
      .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
      .logicOpEnable = VK_FALSE,
      .attachmentCount = 1,
      .pAttachments = &attachment_state,
  };

  VkDynamicState dynamic_states[] = {
      VK_DYNAMIC_STATE_VIEWPORT,
      VK_DYNAMIC_STATE_SCISSOR,
  };
  VkPipelineDynamicStateCreateInfo ci_pipeline_dynamic_state = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
      .dynamicStateCount = 2,
      .pDynamicStates = dynamic_states,
  };

  VkGraphicsPipelineCreateInfo ci_pipelines[] = {{
      .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
      .stageCount = static_cast<uint32_t>(ci_stages.size()),
      .pStages = ci_stages.data(),
      .pVertexInputState = &ci_vertex_input_state,
      .pInputAssemblyState = &ci_input_assembly_state,
      .pViewportState = &ci_viewport_state,
      .pRasterizationState = &ci_rasterization_state,
      .pMultisampleState = &ci_multisample_state,
      .pDepthStencilState = &ci_depth_stencil_state,
      .pColorBlendState = &ci_color_blend_state,
      .pDynamicState = &ci_pipeline_dynamic_state,
      .layout = state->pipeline_layout,
      .renderPass = state->renderpass,
      .subpass = 0,
  }};
  VkPipeline pipelines[] = {VK_NULL_HANDLE};
  r = vkCreateGraphicsPipelines(state->device, VK_NULL_HANDLE, 1, ci_pipelines,
                                nullptr, pipelines);
  if (r != VK_SUCCESS) {
    BOOST_LOG_TRIVIAL(trace) << std::format(
        "Foreground image graphics pipeline creation error ({}).", VkResultToString(r));
    vkDestroyShaderModule(state->device, vs_module, nullptr);
    vkDestroyShaderModule(state->device, fs_module, nullptr);
    return false;
  }
  state->fgimage_pipeline = pipelines[0];

  vkDestroyShaderModule(state->device, vs_module, nullptr);
  vkDestroyShaderModule(state->device, fs_module, nullptr);

  if (state->fgimage_pipeline == VK_NULL_HANDLE) {
    BOOST_LOG_TRIVIAL(trace) << "Foreground image graphics pipeline creation error.";
    return false;
  }
  BOOST_LOG_TRIVIAL(trace) << std::format("Foreground image graphics pipeline {} is created.",
                                          static_cast<void*>(state->fgimage_pipeline));
  return true;
}

bool CreateTextPipeline(VulkanState* state) {
  VkResult r;

  // Подготавливаем шейдерные модули для текста.
  static uint32_t vs_spirv_source[] = {
#include "text.vert.spv.h"
  };
  VkShaderModuleCreateInfo ci_vs_module{
      .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
      .codeSize = sizeof(vs_spirv_source),
      .pCode = (uint32_t*)vs_spirv_source,
  };
  VkShaderModule vs_module = VK_NULL_HANDLE;
  r = vkCreateShaderModule(state->device, &ci_vs_module, nullptr, &vs_module);
  if (r != VK_SUCCESS) {
    BOOST_LOG_TRIVIAL(trace) << std::format(
        "Text vertex shader creation error ({}).", VkResultToString(r));
    return false;
  }

  static uint32_t fs_spirv_source[] = {
#include "text.frag.spv.h"
  };
  VkShaderModuleCreateInfo ci_fs_module{
      .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
      .codeSize = sizeof(fs_spirv_source),
      .pCode = (uint32_t*)fs_spirv_source,
  };
  VkShaderModule fs_module = VK_NULL_HANDLE;
  r = vkCreateShaderModule(state->device, &ci_fs_module, nullptr, &fs_module);
  if (r != VK_SUCCESS) {
    BOOST_LOG_TRIVIAL(trace) << std::format(
        "Text fragment shader creation error ({}).", VkResultToString(r));
    vkDestroyShaderModule(state->device, vs_module, nullptr);
    return false;
  }

  std::array<VkPipelineShaderStageCreateInfo, 2> ci_stages = {
      VkPipelineShaderStageCreateInfo{
          .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
          .stage = VK_SHADER_STAGE_VERTEX_BIT,
          .module = vs_module,
          .pName = "main",
      },
      VkPipelineShaderStageCreateInfo{
          .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
          .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
          .module = fs_module,
          .pName = "main",
      },
  };

  // Vertex input for text quad: position (vec2) and uv (vec2).
  std::vector<VkVertexInputBindingDescription> vertex_binding_descs = {
      VkVertexInputBindingDescription{
        .binding = 0,
        .stride = 4 * sizeof(float), // position + uv
        .inputRate = VK_VERTEX_INPUT_RATE_VERTEX
      },
  };
  std::vector<VkVertexInputAttributeDescription> vertex_attr_descs = {
      VkVertexInputAttributeDescription{
        .location = 0,
        .binding = 0,
        .format = VK_FORMAT_R32G32_SFLOAT,
        .offset = 0
      },
      VkVertexInputAttributeDescription{
        .location = 1,
        .binding = 0,
        .format = VK_FORMAT_R32G32_SFLOAT,
        .offset = 2 * sizeof(float)
      },
  };
  VkPipelineVertexInputStateCreateInfo ci_vertex_input_state{
      .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
      .vertexBindingDescriptionCount = static_cast<uint32_t>(vertex_binding_descs.size()),
      .pVertexBindingDescriptions = vertex_binding_descs.data(),
      .vertexAttributeDescriptionCount = static_cast<uint32_t>(vertex_attr_descs.size()),
      .pVertexAttributeDescriptions = vertex_attr_descs.data()
  };

  VkPipelineInputAssemblyStateCreateInfo ci_input_assembly_state{
      .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
      .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
      .primitiveRestartEnable = false,
  };

  VkPipelineViewportStateCreateInfo ci_viewport_state{
      .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
      .viewportCount = 1,
      .scissorCount = 1,
  };

  VkPipelineRasterizationStateCreateInfo ci_rasterization_state{
      .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
      .rasterizerDiscardEnable = false,
      .polygonMode = VK_POLYGON_MODE_FILL,
      .cullMode = VK_CULL_MODE_NONE, // no culling for 2D text
      .frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
      .lineWidth = 1.0f,
  };

  VkPipelineMultisampleStateCreateInfo ci_multisample_state{
      .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
      .rasterizationSamples = state->sample_count,
  };

  // Disable depth test for overlay
  VkPipelineDepthStencilStateCreateInfo ci_depth_stencil_state{
      .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
      .depthTestEnable = VK_FALSE,
      .depthWriteEnable = VK_FALSE,
      .depthCompareOp = VK_COMPARE_OP_ALWAYS,
  };

  // Alpha blending for text.
  VkPipelineColorBlendAttachmentState attachment_state = {
      .blendEnable = VK_TRUE,
      .srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
      .dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
      .colorBlendOp = VK_BLEND_OP_ADD,
      .srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
      .dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
      .alphaBlendOp = VK_BLEND_OP_ADD,
      .colorWriteMask = VK_COLOR_COMPONENT_A_BIT | VK_COLOR_COMPONENT_R_BIT |
                        VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT,
  };
  VkPipelineColorBlendStateCreateInfo ci_color_blend_state{
      .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
      .logicOpEnable = VK_FALSE,
      .attachmentCount = 1,
      .pAttachments = &attachment_state,
  };

  VkDynamicState dynamic_states[] = {
      VK_DYNAMIC_STATE_VIEWPORT,
      VK_DYNAMIC_STATE_SCISSOR,
  };
  VkPipelineDynamicStateCreateInfo ci_pipeline_dynamic_state = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
      .dynamicStateCount = 2,
      .pDynamicStates = dynamic_states,
  };

  VkGraphicsPipelineCreateInfo ci_pipelines[] = {{
      .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
      .stageCount = static_cast<uint32_t>(ci_stages.size()),
      .pStages = ci_stages.data(),
      .pVertexInputState = &ci_vertex_input_state,
      .pInputAssemblyState = &ci_input_assembly_state,
      .pViewportState = &ci_viewport_state,
      .pRasterizationState = &ci_rasterization_state,
      .pMultisampleState = &ci_multisample_state,
      .pDepthStencilState = &ci_depth_stencil_state,
      .pColorBlendState = &ci_color_blend_state,
      .pDynamicState = &ci_pipeline_dynamic_state,
      .layout = state->pipeline_layout,
      .renderPass = state->renderpass,
      .subpass = 0,
  }};
  VkPipeline pipelines[] = {VK_NULL_HANDLE};
  r = vkCreateGraphicsPipelines(state->device, VK_NULL_HANDLE, 1, ci_pipelines,
                                nullptr, pipelines);
  if (r != VK_SUCCESS) {
    BOOST_LOG_TRIVIAL(trace) << std::format(
        "Text graphics pipeline creation error ({}).", VkResultToString(r));
    vkDestroyShaderModule(state->device, vs_module, nullptr);
    vkDestroyShaderModule(state->device, fs_module, nullptr);
    return false;
  }
  state->text_pipeline = pipelines[0];

  if (state->text_pipeline == VK_NULL_HANDLE) {
    BOOST_LOG_TRIVIAL(trace) << "Text graphics pipeline creation error.";
    vkDestroyShaderModule(state->device, vs_module, nullptr);
    vkDestroyShaderModule(state->device, fs_module, nullptr);
    return false;
  }
  BOOST_LOG_TRIVIAL(trace) << std::format("Text graphics pipeline {} is created.",
                                          static_cast<void*>(state->text_pipeline));

  vkDestroyShaderModule(state->device, vs_module, nullptr);
  vkDestroyShaderModule(state->device, fs_module, nullptr);

  return true;
}

bool InitImage(VulkanObjects* vk_objects, ImageResources* resources) {
  BOOST_LOG_TRIVIAL(trace) << "Create image resources ...";

  VulkanState* state = vk_objects->vk_state;

  VkResult r;

  VkImageViewCreateInfo ci_imageView{
      .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
      .image = resources->image,
      .viewType = VK_IMAGE_VIEW_TYPE_2D,
      .format = state->image_format,
      .components =
          {
              .r = VK_COMPONENT_SWIZZLE_R,
              .g = VK_COMPONENT_SWIZZLE_G,
              .b = VK_COMPONENT_SWIZZLE_B,
              .a = VK_COMPONENT_SWIZZLE_A,
          },
      .subresourceRange =
          {
              .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
              .baseMipLevel = 0,
              .levelCount = 1,
              .baseArrayLayer = 0,
              .layerCount = 1,
          },
  };
  r = vkCreateImageView(state->device, &ci_imageView, nullptr,
                        &resources->image_view);
  assert(r == VK_SUCCESS);

  bool msaa = (state->sample_count != VK_SAMPLE_COUNT_1_BIT);

  VkImageView attachments[3];
  attachments[0] = resources->image_view;
  attachments[1] = vk_objects->depth_stencil_view;
  attachments[2] = resources->msaa_image_view;

  VkFramebufferCreateInfo ci_framebuffer{
      .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
      .renderPass = state->renderpass,
      .attachmentCount = static_cast<uint32_t>(msaa ? 3 : 2),
      .pAttachments = attachments,
      .width = vk_objects->image_extent.width,
      .height = vk_objects->image_extent.height,
      .layers = 1};
  r = vkCreateFramebuffer(state->device, &ci_framebuffer, nullptr,
                          &resources->frame_buffer);
  assert(r == VK_SUCCESS);

  VkSemaphoreCreateInfo semaphoreCreateInfo{
      .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
  };
  r = vkCreateSemaphore(state->device, &semaphoreCreateInfo, nullptr,
                        &resources->submit_semaphore);
  assert(r == VK_SUCCESS);

  return true;
}

bool InitFrame(VulkanState* state, FrameResources* resources) {
  BOOST_LOG_TRIVIAL(trace) << "Create frame resources ...";

  VkResult r;

  VkFenceCreateInfo ci_fence{.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
                             .flags = VK_FENCE_CREATE_SIGNALED_BIT};
  r = vkCreateFence(state->device, &ci_fence, nullptr, &resources->cmd_fence);
  assert(r == VK_SUCCESS);
  resources->cmd_fence_waitable = true;

  VkCommandBufferAllocateInfo commandBufferAllocateInfo{
      .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
      .commandPool = state->cmd_pool,
      .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
      .commandBufferCount = 1,
  };
  r = vkAllocateCommandBuffers(state->device, &commandBufferAllocateInfo,
                               &resources->cmd_buf);
  assert(r == VK_SUCCESS);

  VkSemaphoreCreateInfo semaphoreCreateInfo{
      .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
  };
  r = vkCreateSemaphore(state->device, &semaphoreCreateInfo, nullptr,
                        &resources->acquire_semaphore);
  assert(r == VK_SUCCESS);

  VkDescriptorSetAllocateInfo descSetAllocInfo{
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
      .descriptorPool = state->descriptor_pool,
      .descriptorSetCount = 1,
      .pSetLayouts = &state->desc_layout};
  r = vkAllocateDescriptorSets(state->device, &descSetAllocInfo,
                               &resources->desc_set);
  assert(r == VK_SUCCESS);

  return true;
}

void Finalize(VulkanState* state, ImageResources* resources) {
  BOOST_LOG_TRIVIAL(trace) << "Finalize image resources ...";

  vkDestroyFramebuffer(state->device, resources->frame_buffer, nullptr);
  resources->frame_buffer = VK_NULL_HANDLE;

  vkDestroyImageView(state->device, resources->image_view, nullptr);
  resources->image_view = VK_NULL_HANDLE;

  vkDestroyImageView(state->device, resources->msaa_image_view, nullptr);
  resources->msaa_image_view = VK_NULL_HANDLE;

  vkDestroyImage(state->device, resources->msaa_image, nullptr);
  resources->msaa_image = VK_NULL_HANDLE;

  vkDestroySemaphore(state->device, resources->submit_semaphore, nullptr);
  resources->submit_semaphore = VK_NULL_HANDLE;
}

void Finalize(VulkanState* state, FrameResources* frame) {
  BOOST_LOG_TRIVIAL(trace) << "Finalize frame resources ...";

  vkFreeCommandBuffers(state->device, state->cmd_pool, 1, &frame->cmd_buf);

  vkDestroyFence(state->device, frame->cmd_fence, nullptr);
  frame->cmd_fence = VK_NULL_HANDLE;
  frame->cmd_fence_waitable = false;

  vkDestroySemaphore(state->device, frame->acquire_semaphore, nullptr);
  frame->acquire_semaphore = VK_NULL_HANDLE;
}

struct SwapChainSupportDetails {
  VkSurfaceCapabilitiesKHR capabilities;
  std::vector<VkSurfaceFormatKHR> formats;
  std::vector<VkPresentModeKHR> present_modes;
};

SwapChainSupportDetails QuerySwapChainSupport(VkPhysicalDevice device,
                                              VkSurfaceKHR surface) {
  SwapChainSupportDetails details;

  vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface,
                                            &details.capabilities);

  uint32_t formatCount;
  vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);

  if (formatCount != 0) {
    details.formats.resize(formatCount);
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount,
                                         details.formats.data());
  }

  uint32_t presentModeCount;
  vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount,
                                            nullptr);

  if (presentModeCount != 0) {
    details.present_modes.resize(presentModeCount);
    vkGetPhysicalDeviceSurfacePresentModesKHR(
        device, surface, &presentModeCount, details.present_modes.data());
  }

  return details;
}

uint32_t ChooseTransientImageMemType(VulkanState* state, VkImage img,
                                     uint32_t startIndex) {
  VkPhysicalDeviceMemoryProperties props;
  vkGetPhysicalDeviceMemoryProperties(state->physical_device, &props);

  VkMemoryRequirements memReq;
  vkGetImageMemoryRequirements(state->device, img, &memReq);
  if (memReq.memoryTypeBits == 0) return -1;

  uint32_t memTypeIndex = static_cast<uint32_t>(-1);
  const VkMemoryType* memType = props.memoryTypes;
  bool foundDevLocal = false;
  for (uint32_t i = startIndex; i < props.memoryTypeCount; ++i) {
    if (memReq.memoryTypeBits & (1 << i)) {
      if (memType[i].propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) {
        if (!foundDevLocal) {
          foundDevLocal = true;
          memTypeIndex = i;
        }
        if (memType[i].propertyFlags &
            VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT) {
          memTypeIndex = i;
          break;
        }
      }
    }
  }

  return memTypeIndex;
}

bool CreateTransientImage(VulkanObjects* vk_objects, VkFormat format,
                          VkImageUsageFlags usage,
                          VkImageAspectFlags aspectMask, VkImage* images,
                          VkDeviceMemory* mem, VkImageView* views, int count) {
  VulkanState* state = vk_objects->vk_state;

  VkMemoryRequirements memReq;
  VkResult r;

  assert(count > 0);
  for (int i = 0; i < count; ++i) {
    VkImageCreateInfo ci_image{
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .imageType = VK_IMAGE_TYPE_2D,
        .format = format,
        .extent = VkExtent3D{.width = vk_objects->image_extent.width,
                             .height = vk_objects->image_extent.height,
                             .depth = 1},
        .mipLevels = 1,
        .arrayLayers = 1,
        .samples = state->sample_count,
        .tiling = VK_IMAGE_TILING_OPTIMAL,
        .usage = usage | VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT,
    };

    r = vkCreateImage(state->device, &ci_image, nullptr, images + i);
    if (r != VK_SUCCESS) {
      BOOST_LOG_TRIVIAL(error)
          << std::format("Failed to create image: {}", VkResultToString(r));
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
    memInfo.memoryTypeIndex =
        ChooseTransientImageMemType(state, images[0], startIndex);
    if (memInfo.memoryTypeIndex == uint32_t(-1)) {
      BOOST_LOG_TRIVIAL(error) << "No suitable memory type found";
      return false;
    }
    startIndex = memInfo.memoryTypeIndex + 1;
    BOOST_LOG_TRIVIAL(trace) << std::format(
        "Allocating {} bytes for transient image (memtype {})",
        uint32_t(memInfo.allocationSize), memInfo.memoryTypeIndex);
    r = vkAllocateMemory(state->device, &memInfo, nullptr, mem);
    if (r != VK_SUCCESS && r != VK_ERROR_OUT_OF_DEVICE_MEMORY) {
      BOOST_LOG_TRIVIAL(error) << std::format(
          "Failed to allocate image memory: {}", VkResultToString(r));
      return false;
    }
  } while (r != VK_SUCCESS);

  VkDeviceSize ofs = 0;
  for (int i = 0; i < count; ++i) {
    r = vkBindImageMemory(state->device, images[i], *mem, ofs);
    if (r != VK_SUCCESS) {
      BOOST_LOG_TRIVIAL(error) << std::format("Failed to bind image memory: {}",
                                              VkResultToString(r));
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
    imgViewInfo.subresourceRange.levelCount =
        imgViewInfo.subresourceRange.layerCount = 1;

    r = vkCreateImageView(state->device, &imgViewInfo, nullptr, views + i);
    if (r != VK_SUCCESS) {
      BOOST_LOG_TRIVIAL(error) << std::format("Failed to create image view: {}",
                                              VkResultToString(r));
      return false;
    }
  }

  return true;
}

void ReleaseSwapchain(VulkanObjects* vk_objects) {
  VulkanState* state = vk_objects->vk_state;
  vkDestroySwapchainKHR(state->device, vk_objects->swapchain, nullptr);

  for (int i = 0; i < vk_objects->image_count; ++i) {
    ImageResources* image = &vk_objects->image_res[i];
    Finalize(state, image);
  }

  vkFreeMemory(state->device,vk_objects->msaa_image_mem, nullptr);

  vkDestroyImageView(state->device, vk_objects->depth_stencil_view, nullptr);
  vkDestroyImage(state->device, vk_objects->depth_stencil_image, nullptr);
  vkFreeMemory(state->device, vk_objects->depth_stencil_mem, nullptr);
}

bool RecreateSwapchain(VulkanObjects* vk_objects, VkExtent2D extent) {
  // Защита от вырождения окна в нулевой размер.
  vk_objects->image_extent = extent;
  if (extent.width == 0 || extent.height == 0) {
    BOOST_LOG_TRIVIAL(debug) << "Degenerated Window";
    return true;
  }

  VulkanState* state = vk_objects->vk_state;

  VkResult r;

  r = vkDeviceWaitIdle(state->device);
  if (r != VK_SUCCESS) {
    BOOST_LOG_TRIVIAL(fatal) << std::format("vkDeviceWaitIdle returned {}",
                                            VkResultToString(r));
    std::abort();
  }

  VkSurfaceCapabilitiesKHR surfaceCaps;
  r = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(state->physical_device,
                                                vk_objects->surface,
                                                &surfaceCaps);
  assert(r == VK_SUCCESS);

  // Выбираем размер изображений.
  vk_objects->image_extent = extent;
  assert(surfaceCaps.currentExtent.width == static_cast<uint32_t>(-1) ||
         surfaceCaps.currentExtent.width == extent.width &&
             surfaceCaps.currentExtent.height == extent.height);

  VkSurfaceTransformFlagBitsKHR preTransform =
      (surfaceCaps.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR)
          ? VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR
          : surfaceCaps.currentTransform;

  assert(state->graphics_family_index == state->present_family_index);
  std::array<uint32_t, 1> queueFamilyIndices = {
      state->graphics_family_index.value()};

  VkSwapchainKHR oldSwapchain = vk_objects->swapchain;

  VkSwapchainCreateInfoKHR ci_swapchain{
      .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
      .surface = vk_objects->surface,
      .minImageCount = state->min_image_count,
      .imageFormat = state->image_format,
      .imageColorSpace = state->color_space,
      .imageExtent = vk_objects->image_extent,
      .imageArrayLayers = 1,
      .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
      .imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
      .queueFamilyIndexCount = static_cast<uint32_t>(queueFamilyIndices.size()),
      .pQueueFamilyIndices = queueFamilyIndices.data(),
      .preTransform = preTransform,
      .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
      .presentMode = state->present_mode,
      .oldSwapchain = oldSwapchain};
  VkSwapchainKHR newSwapchain;
  r = vkCreateSwapchainKHR(state->device, &ci_swapchain, nullptr,
                           &newSwapchain);
  if (r != VK_SUCCESS) {
    BOOST_LOG_TRIVIAL(trace) << std::format(
        "Swapchain object creation error ({}).", VkResultToString(r));
    return false;
  }
  BOOST_LOG_TRIVIAL(trace) << std::format(
      "Success of create swapchain object ({}).",
      static_cast<void*>(newSwapchain));

  if (oldSwapchain != VK_NULL_HANDLE) {
    ReleaseSwapchain(vk_objects);
  }

  vk_objects->swapchain = newSwapchain;

  r = vkGetSwapchainImagesKHR(state->device,vk_objects->swapchain,
                              &vk_objects->image_count, nullptr);
  if (r != VK_SUCCESS) return false;
  if (vk_objects->image_count == 0) return false;
  BOOST_LOG_TRIVIAL(trace) << std::format("Count of images in swapchain is {}.",
                                          vk_objects->image_count);

  std::vector<VkImage> swapchainImages(vk_objects->image_count);
  r = vkGetSwapchainImagesKHR(state->device, vk_objects->swapchain,
                              &vk_objects->image_count, swapchainImages.data());
  if (r != VK_SUCCESS) return false;
  if (vk_objects->image_count > MAX_NUM_IMAGES) return false;

  // Создаем изображение буфера глубины.
  CreateTransientImage(vk_objects, state->depth_stencil_format,
                       VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
                       VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT,
                       &vk_objects->depth_stencil_image, &vk_objects->depth_stencil_mem,
                       &vk_objects->depth_stencil_view, 1);

  std::vector<VkImage> msaaImages(vk_objects->image_count, VK_NULL_HANDLE);
  std::vector<VkImageView> msaaViews(vk_objects->image_count, VK_NULL_HANDLE);
  bool msaa = (state->sample_count != VK_SAMPLE_COUNT_1_BIT);
  if (msaa) {
    CreateTransientImage(
        vk_objects, state->image_format, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        VK_IMAGE_ASPECT_COLOR_BIT, msaaImages.data(), &vk_objects->msaa_image_mem,
        msaaViews.data(), vk_objects->image_count);
  }
  for (size_t i = 0; i < vk_objects->image_count; ++i) {
    ImageResources& resources = vk_objects->image_res[i];
    resources.image = swapchainImages[i];
    resources.msaa_image = msaaImages[i];
    resources.msaa_image_view = msaaViews[i];
    InitImage(vk_objects, &resources);
  }

  if (vk_objects->screen_text) {
    vk_objects->fg_text_objects.InitQuad(state, extent, vk_objects->screen_text);
  }

  return true;
}

void FgImageObjects::Render(VkCommandBuffer cmd_buf,
                            VkPipeline image_pipeline,
                            VkPipelineLayout pipeline_layout,
                            const ForegroundImage& fg_image,
                            const VkExtent2D& image_extent) const {
  vkCmdBindPipeline(cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS, image_pipeline);
  struct PushConstants {
    float pos_x, pos_y;
    float size_x, size_y;
    float screen_w, screen_h;
  };
  PushConstants pc = {
      .pos_x = static_cast<float>(fg_image.position.x),
      .pos_y = static_cast<float>(fg_image.position.y),
      .size_x = static_cast<float>(fg_image.width),
      .size_y = static_cast<float>(fg_image.height),
      .screen_w = static_cast<float>(image_extent.width),
      .screen_h = static_cast<float>(image_extent.height),
  };
  vkCmdPushConstants(cmd_buf, pipeline_layout,
                     VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(pc), &pc);
  // Draw 6 vertices as 2 triangles.
  vkCmdDraw(cmd_buf, 6, 1, 0, 0);
}

void Render(Scene* scene, VulkanObjects* vk_objects, ImageResources& image,
            FrameResources& frame) {
  VulkanState* state = vk_objects->vk_state;

  // Пересоздаем командный буфер.
  if (frame.cmd_buf != VK_NULL_HANDLE) {
    vkFreeCommandBuffers(state->device, state->cmd_pool, 1, &frame.cmd_buf);
    frame.cmd_buf = VK_NULL_HANDLE;
  }
  VkCommandBufferAllocateInfo ci_cmdBuf{
      .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
      .commandPool = state->cmd_pool,
      .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
      .commandBufferCount = 1};
  vkAllocateCommandBuffers(state->device, &ci_cmdBuf, &frame.cmd_buf);

  // Begin command buffer.
  VkCommandBufferBeginInfo commandBufferBeginInfo{
      .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
  };
  vkBeginCommandBuffer(frame.cmd_buf, &commandBufferBeginInfo);

  const VkClearColorValue lavender{
      .float32 = {230 / 256.0f, 230 / 256.0f, 250 / 256.0f}};

  // Begin renderpass command.
  VkClearValue clearValues[] = {
      {.color = lavender}, {.depthStencil = {1.0f, 0}}, {.color = lavender}};
  VkRenderPassBeginInfo bi_renderpass{
      .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
      .renderPass = state->renderpass,
      .framebuffer = image.frame_buffer,
      .renderArea = {.offset = {0, 0}, .extent = vk_objects->image_extent},
      .clearValueCount = 3,
      .pClearValues = clearValues};
  vkCmdBeginRenderPass(frame.cmd_buf, &bi_renderpass,
                       VK_SUBPASS_CONTENTS_INLINE);

  // Дальнейшие команды будут вызваны, только если сцена не пуста.
  if (vk_objects->buffer_vertex) {
    vkCmdBindPipeline(frame.cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS,
                      state->pipeline);

    vkCmdBindDescriptorSets(frame.cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS,
                            state->pipeline_layout, 0, 1, &frame.desc_set, 0,
                            nullptr);

    // Bind vertex buffers.
    std::vector<VkBuffer> buffers = {
        vk_objects->buffer_vertex,
        vk_objects->buffer_normal,
        vk_objects->buffer_color
    };
    VkDeviceSize offsets[] = {0, 0, 0};
    vkCmdBindVertexBuffers(frame.cmd_buf, 0, buffers.size(), buffers.data(), offsets);

    // Bind index buffers.
    vkCmdBindIndexBuffer(frame.cmd_buf,vk_objects->buffer_index, 0,
                         VK_INDEX_TYPE_UINT32);

    const VkViewport viewport = {
        .x = 0,
        .y = 0,
        .width = static_cast<float>(vk_objects->image_extent.width),
        .height = static_cast<float>(vk_objects->image_extent.height),
        .minDepth = 0,
        .maxDepth = 1,
    };
    vkCmdSetViewport(frame.cmd_buf, 0, 1, &viewport);

    const VkRect2D scissor = {
        .offset = {0, 0},
        .extent = vk_objects->image_extent,
    };
    vkCmdSetScissor(frame.cmd_buf, 0, 1, &scissor);

    vkCmdDrawIndexed(frame.cmd_buf, vk_objects->index_count, 1, 0, 0, 0);
  }

  // Draw foreground image if available
  if (!!vk_objects->fg_image_objects) {
    auto scene_inner_if = scene->GetInnerInterface();
    const ForegroundImage& fg_image = scene_inner_if->GetFgImage();
    vk_objects->fg_image_objects.Render(frame.cmd_buf, state->fgimage_pipeline,
                                        state->pipeline_layout, fg_image,
                                        vk_objects->image_extent);
  }

  // Draw text overlay if available
  if (!!vk_objects->fg_text_objects) {
    vk_objects->fg_text_objects.Render(frame.cmd_buf, state->text_pipeline);
  }

  vkCmdEndRenderPass(frame.cmd_buf);

  vkEndCommandBuffer(frame.cmd_buf);

  VkPipelineStageFlags waitDstStageMask[] = {
      VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
  };
  VkSubmitInfo submitInfo{.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
                          .waitSemaphoreCount = static_cast<uint32_t>(1),
                          .pWaitSemaphores = &frame.acquire_semaphore,
                          .pWaitDstStageMask = waitDstStageMask,
                          .commandBufferCount = 1,
                          .pCommandBuffers = &frame.cmd_buf,
                          .signalSemaphoreCount = static_cast<uint32_t>(1),
                          .pSignalSemaphores = &image.submit_semaphore};
  assert(!frame.cmd_fence_waitable);
  vkQueueSubmit(state->queue, 1, &submitInfo, frame.cmd_fence);
  frame.cmd_fence_waitable = true;
}

VKAPI_ATTR VkBool32 VKAPI_CALL
DebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
              VkDebugUtilsMessageTypeFlagsEXT messageType,
              const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
              void* pUserData) {
  auto outMessage =
      std::format("Validation Layer: {}", pCallbackData->pMessage);
  if((messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT) != 0)
    BOOST_LOG_TRIVIAL(debug) << outMessage;
  else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT)
    BOOST_LOG_TRIVIAL(info) << outMessage;
  else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
    BOOST_LOG_TRIVIAL(warning) << outMessage;
  else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
    BOOST_LOG_TRIVIAL(error) << outMessage;
  else
    BOOST_LOG_TRIVIAL(trace) << outMessage;

  return VK_FALSE;
}

bool CheckExtensions(const std::vector<const char*>& extensionNames) {
  BOOST_LOG_TRIVIAL(trace) << "Check Extensions";

  VkResult r;

  uint32_t count = 0;
  r = vkEnumerateInstanceExtensionProperties(nullptr, &count, nullptr);
  assert(r == VK_SUCCESS);
  if (count == 0) return false;
  std::vector<VkExtensionProperties> props(count);
  r = vkEnumerateInstanceExtensionProperties(nullptr, &count, props.data());
  assert(r == VK_SUCCESS);
  bool haveExtensions = true;
  for (const char* name : extensionNames) {
    bool has = false;
    for (const VkExtensionProperties& prop : props) {
      if (strcmp(prop.extensionName, name) == 0) {
        has = true;
        break;
      }
    }
    BOOST_LOG_TRIVIAL(trace) << std::format(" * extension {} {}", name,
                                            has ? "exists" : "doesn't exist");
    if (!has) haveExtensions = false;
  }
  return haveExtensions;
}

//! Создаем экземпляр vulkan.
bool InitInstance(VulkanState* state) {
  VkResult r;

  r = volkInitialize();
  if (r != VK_SUCCESS) {
    BOOST_LOG_TRIVIAL(trace) << std::format(
        "Volk initialization error ({}).", VkResultToString(r));
    return false;
  }

  std::vector<const char*> extensionNames{
      VK_KHR_SURFACE_EXTENSION_NAME,
#if defined(VK_USE_PLATFORM_XCB_KHR)
      VK_KHR_XCB_SURFACE_EXTENSION_NAME,
#endif
#if defined(_WIN32)
      VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
#endif
#ifdef ENABLE_VALIDATION_LAYERS
      VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
#endif
  };

  if (!CheckExtensions(extensionNames)) return false;

  VkApplicationInfo applicationInfo{
      .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
      .pApplicationName = "app-qt",
      .applicationVersion = VK_MAKE_VERSION(0, 0, 1),
      .pEngineName = "numgeom",
      .engineVersion = VK_MAKE_VERSION(0, 0, 1),
      .apiVersion = VK_API_VERSION_1_2,
  };
  VkInstanceCreateInfo ci_instance{
      .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
      .pApplicationInfo = &applicationInfo,
      .enabledExtensionCount = static_cast<uint32_t>(extensionNames.size()),
      .ppEnabledExtensionNames = extensionNames.data()};

#ifdef ENABLE_VALIDATION_LAYERS
  std::vector<const char*> layersNames{
      "VK_LAYER_KHRONOS_validation",
  };
  ci_instance.enabledLayerCount = static_cast<uint32_t>(layersNames.size());
  ci_instance.ppEnabledLayerNames = layersNames.data();
  VkDebugUtilsMessengerCreateInfoEXT ci_debugUtilsMessenger{
      .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
      .messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                         VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                         VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
      .messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                     VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                     VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
      .pfnUserCallback = DebugCallback,
  };
  ci_instance.pNext = &ci_debugUtilsMessenger;
#endif

  r = vkCreateInstance(&ci_instance, nullptr, &state->instance);
  if (r != VK_SUCCESS) {
    BOOST_LOG_TRIVIAL(trace) << std::format(
        "Vulkan instance creation error ({}).", VkResultToString(r));
    return false;
  }
  BOOST_LOG_TRIVIAL(trace) << std::format(
      "Success of create vulkan instance ({})",
      static_cast<void*>(state->instance));

  // Load instance-level Vulkan functions
  volkLoadInstance(state->instance);

#ifdef ENABLE_VALIDATION_LAYERS
  r = vkCreateDebugUtilsMessengerEXT(state->instance, &ci_debugUtilsMessenger, nullptr,
           &state->debug_messenger);
  if (r != VK_SUCCESS) {
    BOOST_LOG_TRIVIAL(trace) << std::format(
        "Error of creating debug utils messenger ({}).", VkResultToString(r));
    return false;
  }
#endif

  return true;
}

void FindQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface,
                       std::optional<uint32_t>& graphicsFamily,
                       std::optional<uint32_t>& presentFamily) {
  uint32_t queueFamilyCount = 0;
  vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

  std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
  vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount,
                                           queueFamilies.data());

  int i = 0;
  for (uint32_t i = 0; i < queueFamilyCount; ++i) {
    const auto& queueFamily = queueFamilies[i];
    if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) graphicsFamily = i;

    VkBool32 presentSupport = false;
    vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);

    if (presentSupport) presentFamily = i;

    if (graphicsFamily.has_value() && presentFamily.has_value()) return;
  }
}

bool CheckDeviceExtensionSupport(
    VkPhysicalDevice device,
    const std::vector<const char*>& requiredExtensionNames) {
  uint32_t extensionCount;
  vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount,
                                       nullptr);

  std::vector<VkExtensionProperties> availableExtensions(extensionCount);
  vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount,
                                       availableExtensions.data());

  std::set<std::string> requiredExtensionNames2(requiredExtensionNames.begin(),
                                                requiredExtensionNames.end());

  for (const auto& extension : availableExtensions)
    requiredExtensionNames2.erase(extension.extensionName);

  return requiredExtensionNames2.empty();
}

//! Оценка пригодности физического устройства для целей проекта.
bool IsDeviceSuitable(VkPhysicalDevice device, VkSurfaceKHR surface,
                      const std::vector<const char*>& requiredExtensionNames) {
  // Проверка присутствия в устройстве очередей рендеринга и показа.
  std::optional<uint32_t> graphicsFamily, presentFamily;
  FindQueueFamilies(device, surface, graphicsFamily, presentFamily);
  if (!graphicsFamily.has_value() || !presentFamily.has_value()) return false;

  // Проверка поддержки устройством заявленных проектом расширений.
  bool extensionsSupported =
      CheckDeviceExtensionSupport(device, requiredExtensionNames);
  if (!extensionsSupported) return false;

  // Проверка возможности создания swap-chain'а.
  SwapChainSupportDetails swapChainSupport =
      QuerySwapChainSupport(device, surface);
  if (swapChainSupport.formats.empty() ||
      swapChainSupport.present_modes.empty() ||
    (swapChainSupport.capabilities.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR) == 0) {
    return false;
  }

  VkPhysicalDeviceFeatures supportedFeatures;
  vkGetPhysicalDeviceFeatures(device, &supportedFeatures);
  if (!supportedFeatures.samplerAnisotropy) return false;

  return true;
}

bool SelectPhysicalDevice(VulkanState* state,
                          VkSurfaceKHR surface,
                          const std::vector<const char*>& extensionNames) {
  uint32_t devicesCount = 0;
  VkResult r =
      vkEnumeratePhysicalDevices(state->instance, &devicesCount, nullptr);
  if (r != VK_SUCCESS) return false;
  BOOST_LOG_TRIVIAL(trace) << std::format("Count of physical devices is {}.",
                                          devicesCount);
  if (devicesCount == 0) {
    BOOST_LOG_TRIVIAL(trace) << "Failed to find GPUs with Vulkan support.";
    return false;
  }

  std::vector<VkPhysicalDevice> devices(devicesCount);
  r = vkEnumeratePhysicalDevices(state->instance, &devicesCount,
                                 devices.data());
  if (r != VK_SUCCESS) return false;

  for (VkPhysicalDevice device : devices) {
    if (IsDeviceSuitable(device, surface, extensionNames)) {
      state->physical_device = device;
      break;
    }
  }
  if (state->physical_device == VK_NULL_HANDLE) {
    BOOST_LOG_TRIVIAL(trace) << "Couldn't find a suitable physical device.";
    return false;
  }
  BOOST_LOG_TRIVIAL(trace) << std::format(
      "Physical device #{} is selected.",
      static_cast<void*>(state->physical_device));

  // Получаем свойства памяти физического устройства.
  vkGetPhysicalDeviceMemoryProperties(state->physical_device,
                                      &state->memory_properties);

  return true;
}

bool CreateLogicalDevice(VulkanState* state, VkSurfaceKHR surface) {
  // Выбираем семейство очередей.
  FindQueueFamilies(state->physical_device, surface,
                    state->graphics_family_index, state->present_family_index);
  if (!state->graphics_family_index.has_value()) {
    BOOST_LOG_TRIVIAL(trace) << "Graphics family not found";
    return false;
  }
  if (!state->present_family_index.has_value()) {
    BOOST_LOG_TRIVIAL(trace) << "Present family not found";
    return false;
  }

  //\warning Следует разобрать случай, когда очередь рендеринга и показа не
  //совпадают.
  if (state->graphics_family_index != state->present_family_index) return false;

  // Создаем логическое устройство.
  std::vector<VkDeviceQueueCreateInfo> ci_queues;
  std::set<uint32_t> queueFamilies = {state->graphics_family_index.value(),
                                      state->present_family_index.value()};
  float queuePriorities[] = {1.0f};
  for (uint32_t queueFamily : queueFamilies) {
    VkDeviceQueueCreateInfo ci_queue{
        .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        .queueFamilyIndex = queueFamily,
        .queueCount = 1,
        .pQueuePriorities = queuePriorities,
    };
    ci_queues.push_back(ci_queue);
  }

  std::vector<const char*> deviceExtensionNames = {
      VK_KHR_SWAPCHAIN_EXTENSION_NAME};

  VkDeviceCreateInfo ci_device{
      .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
      .queueCreateInfoCount = static_cast<uint32_t>(ci_queues.size()),
      .pQueueCreateInfos = ci_queues.data(),
      .enabledExtensionCount =
          static_cast<uint32_t>(deviceExtensionNames.size()),
      .ppEnabledExtensionNames = deviceExtensionNames.data(),
      .pEnabledFeatures = nullptr};
  VkResult r = vkCreateDevice(state->physical_device, &ci_device, nullptr,
                              &state->device);
  if (r != VK_SUCCESS) {
    BOOST_LOG_TRIVIAL(trace) << std::format(
        "Logical device creation error ({}).", VkResultToString(r));
    return false;
  }
  BOOST_LOG_TRIVIAL(trace) << std::format("Logical device {} is created",
                                          static_cast<void*>(state->device));

  volkLoadDevice(state->device);

  // Создаем vma-аллокатор.
  VmaAllocatorCreateInfo ci_allocator {
      .flags = VMA_ALLOCATOR_CREATE_EXT_MEMORY_BUDGET_BIT,
      .physicalDevice = state->physical_device,
      .device = state->device,
      .instance = state->instance,
      .vulkanApiVersion = VK_API_VERSION_1_2,
  };
  VmaVulkanFunctions vulkanFunctions = {};
  vmaImportVulkanFunctionsFromVolk(&ci_allocator, &vulkanFunctions);
  ci_allocator.pVulkanFunctions = &vulkanFunctions;
  r = vmaCreateAllocator(&ci_allocator, &state->allocator);
  if (r != VK_SUCCESS) {
    BOOST_LOG_TRIVIAL(trace) << std::format(
        "VMA allocator creation error ({}).", VkResultToString(r));
    return false;
  }

  VkDeviceQueueInfo2 queueInfo{
      .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_INFO_2,
      .queueFamilyIndex = state->graphics_family_index.value(),
      .queueIndex = 0,
  };
  vkGetDeviceQueue2(state->device, &queueInfo, &state->queue);
  BOOST_LOG_TRIVIAL(trace) << std::format("Queue {} is selected.",
                                          static_cast<void*>(state->queue));
  return true;
}

#ifdef LINUX
bool CreateSurface(VulkanState* state, VkSceneRenderer::Impl::Xcb* xcb) {
  int screenNum = 0;
  xcb_connection_t* connection = xcb_connect(nullptr, &screenNum);
  if (xcb_connection_has_error(connection)) {
    xcb_disconnect(connection);
    return false;
  }
  const xcb_setup_t* setup = xcb_get_setup(connection);
  int screenCount = xcb_setup_roots_length(setup);
  xcb_screen_iterator_t screen_iterator = xcb_setup_roots_iterator(setup);
  for (int i = 0; i < screenNum; ++i) xcb_screen_next(&screen_iterator);

  xcb_screen_t* _screen = screen_iterator.data;
  xcb_flush(connection);

  xcb->connection = connection;
  BOOST_LOG_TRIVIAL(trace) << std::format("Xcb connection {} and window {}",
                                          static_cast<void*>(xcb->connection),
                                          xcb->window);

  VkXcbSurfaceCreateInfoKHR ci_surface{
      .sType = VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR,
      .connection = connection,
      .window = xcb->window};
  VkResult r = vkCreateXcbSurfaceKHR(state->instance, &ci_surface, nullptr,
                                     &state->surface);
  if (r != VK_SUCCESS) {
    BOOST_LOG_TRIVIAL(trace)
        << std::format("Xcb surface creation error ({}).", VkResultToString(r));
    return false;
  }
  BOOST_LOG_TRIVIAL(trace) << std::format(
      "Success of create vulkan surface ({})",
      static_cast<void*>(state->surface));

  return true;
}
#endif

VkFormat SelectDepthStencilFormat(VkPhysicalDevice device) {
  const VkFormat dsFormatCandidates[] = {VK_FORMAT_D24_UNORM_S8_UINT,
                                         VK_FORMAT_D32_SFLOAT_S8_UINT,
                                         VK_FORMAT_D16_UNORM_S8_UINT};

  for (int idx = 0; idx < sizeof(dsFormatCandidates) / sizeof(VkFormat);
       ++idx) {
    VkFormat format = dsFormatCandidates[idx];
    VkFormatProperties prop;
    vkGetPhysicalDeviceFormatProperties(device, format, &prop);
    if (prop.optimalTilingFeatures &
        VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT)
      return format;
  }

  return VK_FORMAT_UNDEFINED;
}

bool ChooseSwapchainSettings(VulkanState* state, VkSurfaceKHR surface) {
  SwapChainSupportDetails swapchainSupport =
      QuerySwapChainSupport(state->physical_device, surface);

  state->image_format = VK_FORMAT_UNDEFINED;

  VkFormat format = VK_FORMAT_UNDEFINED;
  VkColorSpaceKHR colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
  for (const auto& f : swapchainSupport.formats) {
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

  state->image_format = format;
  state->color_space = colorSpace;
  if (state->image_format == VK_FORMAT_UNDEFINED) {
    BOOST_LOG_TRIVIAL(trace) << "Image format undefined.";
    return false;
  }

  state->depth_stencil_format = SelectDepthStencilFormat(state->physical_device);
  if (state->depth_stencil_format == VK_FORMAT_UNDEFINED)
    BOOST_LOG_TRIVIAL(error)
        << "Failed to find an optimal depth-stencil format";

  BOOST_LOG_TRIVIAL(trace) << std::format(
      "Color format: {} Depth-stencil format: {}",
      VkFormatToString(state->image_format),
      VkFormatToString(state->depth_stencil_format));

  // Выбираем режим показа.
  for (auto pm : swapchainSupport.present_modes) {
    if (pm == VK_PRESENT_MODE_MAILBOX_KHR) {
      state->present_mode = VK_PRESENT_MODE_MAILBOX_KHR;
      break;
    }
  }

  state->min_image_count = 2;
  if (state->min_image_count < swapchainSupport.capabilities.minImageCount) {
    if (swapchainSupport.capabilities.minImageCount > MAX_NUM_IMAGES) {
      BOOST_LOG_TRIVIAL(error) << std::format(
          "VkSurfaceCapabilitiesKHR.minImageCount is too large (is: {}, max: "
          "{})",
          swapchainSupport.capabilities.minImageCount, MAX_NUM_IMAGES);
      return false;
    }
    state->min_image_count = swapchainSupport.capabilities.minImageCount;
  }
  if (swapchainSupport.capabilities.maxImageCount > 0 &&
      state->min_image_count > swapchainSupport.capabilities.maxImageCount) {
    state->min_image_count = swapchainSupport.capabilities.maxImageCount;
  }

  return true;
}

bool FgImageObjects::Initialize(const VulkanState* state,
                                const ForegroundImage& fg_image) {
  BOOST_LOG_TRIVIAL(trace) << "Creating logo resources...";
  if (fg_image.IsEmpty()) {
    BOOST_LOG_TRIVIAL(error) << "Foreground image is empty";
    return false;
  }

  VkResult r = VK_SUCCESS;

  // Create staging buffer
  VkBuffer staging_buffer = VK_NULL_HANDLE;
  VmaAllocation staging_allocation = VK_NULL_HANDLE;
  VkBufferCreateInfo bufferInfo = {
      .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
      .size = static_cast<VkDeviceSize>(fg_image.pixels.size()),
      .usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
  };
  VmaAllocationCreateInfo alloc_ci = {
      .flags = VMA_ALLOCATION_CREATE_MAPPED_BIT,
      .usage = VMA_MEMORY_USAGE_CPU_TO_GPU,
  };
  r = vmaCreateBuffer(state->allocator, &bufferInfo, &alloc_ci,
                      &staging_buffer, &staging_allocation, nullptr);
  if (r != VK_SUCCESS) {
    BOOST_LOG_TRIVIAL(error) << "Failed to create staging buffer for logo: "
                             << VkResultToString(r);
    return false;
  }

  // Copy pixel data
  void* mapped_data = nullptr;
  vmaMapMemory(state->allocator, staging_allocation, &mapped_data);
  memcpy(mapped_data, fg_image.pixels.data(), fg_image.pixels.size());
  vmaUnmapMemory(state->allocator, staging_allocation);

  // Create image
  VkImageCreateInfo image_ci = {
      .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
      .imageType = VK_IMAGE_TYPE_2D,
      .format = VK_FORMAT_R8G8B8A8_UNORM,
      .extent = {static_cast<uint32_t>(fg_image.width),
                 static_cast<uint32_t>(fg_image.height), 1},
      .mipLevels = 1,
      .arrayLayers = 1,
      .samples = VK_SAMPLE_COUNT_1_BIT,
      .tiling = VK_IMAGE_TILING_OPTIMAL,
      .usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
      .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
      .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
  };
  VmaAllocationCreateInfo image_alloc_ci = {
      .flags = 0,
      .usage = VMA_MEMORY_USAGE_GPU_ONLY,
  };
  r = vmaCreateImage(state->allocator, &image_ci, &image_alloc_ci,
                     &logo_image, &alloc_logo_image, nullptr);
  if (r != VK_SUCCESS) {
    BOOST_LOG_TRIVIAL(error) << "Failed to create logo image: "
                             << VkResultToString(r);
    vmaDestroyBuffer(state->allocator, staging_buffer, staging_allocation);
    return false;
  }

  // Create temporary command pool and buffer
  VkCommandPool cmd_pool = VK_NULL_HANDLE;
  VkCommandPoolCreateInfo pool_ci = {
      .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
      .flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT,
      .queueFamilyIndex = state->graphics_family_index.value(),
  };
  r = vkCreateCommandPool(state->device, &pool_ci, nullptr, &cmd_pool);
  if (r != VK_SUCCESS) {
    BOOST_LOG_TRIVIAL(error) << "Failed to create command pool for logo: "
                             << VkResultToString(r);
    vmaDestroyImage(state->allocator, logo_image, alloc_logo_image);
    vmaDestroyBuffer(state->allocator, staging_buffer, staging_allocation);
    return false;
  }

  VkCommandBuffer cmd_buf = VK_NULL_HANDLE;
  VkCommandBufferAllocateInfo alloc_buf_info = {
      .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
      .commandPool = cmd_pool,
      .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
      .commandBufferCount = 1,
  };
  r = vkAllocateCommandBuffers(state->device, &alloc_buf_info, &cmd_buf);
  if (r != VK_SUCCESS) {
    BOOST_LOG_TRIVIAL(error) << "Failed to allocate command buffer for logo: "
                             << VkResultToString(r);
    vkDestroyCommandPool(state->device, cmd_pool, nullptr);
    vmaDestroyImage(state->allocator, logo_image, alloc_logo_image);
    vmaDestroyBuffer(state->allocator, staging_buffer, staging_allocation);
    return false;
  }

  // Begin command buffer
  VkCommandBufferBeginInfo begin_info = {
      .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
      .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
  };
  vkBeginCommandBuffer(cmd_buf, &begin_info);

  // Transition image layout to transfer destination
  VkImageMemoryBarrier barrier = {
      .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
      .srcAccessMask = 0,
      .dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
      .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
      .newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
      .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
      .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
      .image = logo_image,
      .subresourceRange = {
          .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
          .baseMipLevel = 0,
          .levelCount = 1,
          .baseArrayLayer = 0,
          .layerCount = 1,
      },
  };
  vkCmdPipelineBarrier(cmd_buf,
                       VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                       VK_PIPELINE_STAGE_TRANSFER_BIT,
                       0,
                       0, nullptr,
                       0, nullptr,
                       1, &barrier);

  // Copy buffer to image
  VkBufferImageCopy region = {
      .bufferOffset = 0,
      .bufferRowLength = 0,
      .bufferImageHeight = 0,
      .imageSubresource = {
          .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
          .mipLevel = 0,
          .baseArrayLayer = 0,
          .layerCount = 1,
      },
      .imageOffset = {0, 0, 0},
      .imageExtent = {static_cast<uint32_t>(fg_image.width),
                      static_cast<uint32_t>(fg_image.height), 1},
  };
  vkCmdCopyBufferToImage(cmd_buf, staging_buffer, logo_image,
                         VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

  // Transition image layout to shader read-only optimal
  barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
  barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
  barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
  barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  vkCmdPipelineBarrier(cmd_buf,
                       VK_PIPELINE_STAGE_TRANSFER_BIT,
                       VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                       0,
                       0, nullptr,
                       0, nullptr,
                       1, &barrier);

  vkEndCommandBuffer(cmd_buf);

  // Submit command buffer
  VkSubmitInfo submit_info = {
      .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
      .commandBufferCount = 1,
      .pCommandBuffers = &cmd_buf,
  };
  r = vkQueueSubmit(state->queue, 1, &submit_info, VK_NULL_HANDLE);
  if (r != VK_SUCCESS) {
    BOOST_LOG_TRIVIAL(error) << "Failed to submit logo copy commands: "
                             << VkResultToString(r);
    vkFreeCommandBuffers(state->device, cmd_pool, 1, &cmd_buf);
    vkDestroyCommandPool(state->device, cmd_pool, nullptr);
    vmaDestroyImage(state->allocator, logo_image, alloc_logo_image);
    vmaDestroyBuffer(state->allocator, staging_buffer, staging_allocation);
    return false;
  }

  // Wait for queue to finish
  r = vkQueueWaitIdle(state->queue);
  if (r != VK_SUCCESS) {
    BOOST_LOG_TRIVIAL(error) << "Failed to wait for logo copy: "
                             << VkResultToString(r);
    vkFreeCommandBuffers(state->device, cmd_pool, 1, &cmd_buf);
    vkDestroyCommandPool(state->device, cmd_pool, nullptr);
    vmaDestroyImage(state->allocator, logo_image, alloc_logo_image);
    vmaDestroyBuffer(state->allocator, staging_buffer, staging_allocation);
    return false;
  }

  // Cleanup temporary command pool and staging buffer
  vkFreeCommandBuffers(state->device, cmd_pool, 1, &cmd_buf);
  vkDestroyCommandPool(state->device, cmd_pool, nullptr);
  vmaDestroyBuffer(state->allocator, staging_buffer, staging_allocation);

  // Create image view
  VkImageViewCreateInfo view_info = {
      .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
      .image = logo_image,
      .viewType = VK_IMAGE_VIEW_TYPE_2D,
      .format = VK_FORMAT_R8G8B8A8_UNORM,
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
  r = vkCreateImageView(state->device, &view_info, nullptr, &logo_image_view);
  if (r != VK_SUCCESS) {
    BOOST_LOG_TRIVIAL(error) << "Failed to create logo image view: "
                             << VkResultToString(r);
    vmaDestroyImage(state->allocator, logo_image, alloc_logo_image);
    return false;
  }

  // Create sampler
  VkSamplerCreateInfo sampler_ci = {
      .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
      .magFilter = VK_FILTER_LINEAR,
      .minFilter = VK_FILTER_LINEAR,
      .mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
      .addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
      .addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
      .addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
      .mipLodBias = 0.0f,
      .anisotropyEnable = VK_FALSE,
      .maxAnisotropy = 1.0f,
      .compareEnable = VK_FALSE,
      .compareOp = VK_COMPARE_OP_ALWAYS,
      .minLod = 0.0f,
      .maxLod = 0.0f,
      .borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK,
      .unnormalizedCoordinates = VK_FALSE,
  };
  r = vkCreateSampler(state->device, &sampler_ci, nullptr, &logo_sampler);
  if (r != VK_SUCCESS) {
    BOOST_LOG_TRIVIAL(error) << "Failed to create logo sampler: "
                             << VkResultToString(r);
    vkDestroyImageView(state->device, logo_image_view, nullptr);
    vmaDestroyImage(state->allocator, logo_image, alloc_logo_image);
    return false;
  }

  BOOST_LOG_TRIVIAL(trace) << "Logo resources created successfully";
  return true;
}

bool FgTextObjects::Initialize(VulkanState* state, const ScreenText* text) {
  BOOST_LOG_TRIVIAL(trace) << "Creating text resources...";
  assert(text != nullptr);
  auto text_internal_if = text->GetInnerInterface();

  VkResult r = VK_SUCCESS;

  // Create staging buffer
  VkBuffer staging_buffer = VK_NULL_HANDLE;
  VmaAllocation staging_allocation = VK_NULL_HANDLE;
  VkBufferCreateInfo bufferInfo = {
      .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
      .size = static_cast<VkDeviceSize>(text_internal_if->GetAtlasPixels().size()),
      .usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
  };
  VmaAllocationCreateInfo alloc_ci = {
      .flags = VMA_ALLOCATION_CREATE_MAPPED_BIT,
      .usage = VMA_MEMORY_USAGE_CPU_TO_GPU,
  };
  r = vmaCreateBuffer(state->allocator, &bufferInfo, &alloc_ci,
                      &staging_buffer, &staging_allocation, nullptr);
  if (r != VK_SUCCESS) {
    BOOST_LOG_TRIVIAL(error) << "Failed to create staging buffer for text: "
                             << VkResultToString(r);
    return false;
  }

  // Copy pixel data
  void* mapped_data = nullptr;
  vmaMapMemory(state->allocator, staging_allocation, &mapped_data);
  memcpy(mapped_data, text_internal_if->GetAtlasPixels().data(),
         text_internal_if->GetAtlasPixels().size());
  vmaUnmapMemory(state->allocator, staging_allocation);

  // Create image - text atlas is single-channel (R8)
  auto atlas_size = text_internal_if->GetAtlasSize();
  VkImageCreateInfo image_ci = {
      .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
      .imageType = VK_IMAGE_TYPE_2D,
      .format = VK_FORMAT_R8_UNORM,
      .extent = {static_cast<uint32_t>(atlas_size.x),
                 static_cast<uint32_t>(atlas_size.y), 1},
      .mipLevels = 1,
      .arrayLayers = 1,
      .samples = VK_SAMPLE_COUNT_1_BIT,
      .tiling = VK_IMAGE_TILING_OPTIMAL,
      .usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
      .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
      .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
  };
  VmaAllocationCreateInfo image_alloc_ci = {
      .flags = 0,
      .usage = VMA_MEMORY_USAGE_GPU_ONLY,
  };
  r = vmaCreateImage(state->allocator, &image_ci, &image_alloc_ci,
                     &text_image, &alloc_text_image, nullptr);
  if (r != VK_SUCCESS) {
    BOOST_LOG_TRIVIAL(error) << "Failed to create text image: "
                             << VkResultToString(r);
    vmaDestroyBuffer(state->allocator, staging_buffer, staging_allocation);
    return false;
  }

  // Create temporary command pool and buffer
  VkCommandPool cmd_pool = VK_NULL_HANDLE;
  VkCommandPoolCreateInfo pool_ci = {
      .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
      .flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT,
      .queueFamilyIndex = state->graphics_family_index.value(),
  };
  r = vkCreateCommandPool(state->device, &pool_ci, nullptr, &cmd_pool);
  if (r != VK_SUCCESS) {
    BOOST_LOG_TRIVIAL(error) << "Failed to create command pool for text: "
                             << VkResultToString(r);
    vmaDestroyImage(state->allocator, text_image, alloc_text_image);
    vmaDestroyBuffer(state->allocator, staging_buffer, staging_allocation);
    return false;
  }

  VkCommandBuffer cmd_buf = VK_NULL_HANDLE;
  VkCommandBufferAllocateInfo alloc_buf_info = {
      .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
      .commandPool = cmd_pool,
      .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
      .commandBufferCount = 1,
  };
  r = vkAllocateCommandBuffers(state->device, &alloc_buf_info, &cmd_buf);
  if (r != VK_SUCCESS) {
    BOOST_LOG_TRIVIAL(error) << "Failed to allocate command buffer for text: "
                             << VkResultToString(r);
    vkDestroyCommandPool(state->device, cmd_pool, nullptr);
    vmaDestroyImage(state->allocator, text_image, alloc_text_image);
    vmaDestroyBuffer(state->allocator, staging_buffer, staging_allocation);
    return false;
  }

  // Begin command buffer
  VkCommandBufferBeginInfo begin_info = {
      .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
      .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
  };
  vkBeginCommandBuffer(cmd_buf, &begin_info);

  // Transition image layout for copy
  VkImageMemoryBarrier barrier = {
      .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
      .srcAccessMask = 0,
      .dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
      .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
      .newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
      .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
      .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
      .image = text_image,
      .subresourceRange = {
          .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
          .baseMipLevel = 0,
          .levelCount = 1,
          .baseArrayLayer = 0,
          .layerCount = 1,
      },
  };
  vkCmdPipelineBarrier(cmd_buf, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                       VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0,
                       nullptr, 1, &barrier);

  // Copy buffer to image
  VkBufferImageCopy region = {
      .bufferOffset = 0,
      .bufferRowLength = 0,
      .bufferImageHeight = 0,
      .imageSubresource = {
          .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
          .mipLevel = 0,
          .baseArrayLayer = 0,
          .layerCount = 1,
      },
      .imageOffset = {0, 0, 0},
      .imageExtent = {static_cast<uint32_t>(atlas_size.x),
                      static_cast<uint32_t>(atlas_size.y), 1},
  };
  vkCmdCopyBufferToImage(cmd_buf, staging_buffer, text_image,
                         VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

  // Transition image layout for shader reading
  barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
  barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
  barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
  barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  vkCmdPipelineBarrier(cmd_buf, VK_PIPELINE_STAGE_TRANSFER_BIT,
                       VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr,
                       0, nullptr, 1, &barrier);

  vkEndCommandBuffer(cmd_buf);

  // Submit command buffer
  VkSubmitInfo submit_info = {
      .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
      .commandBufferCount = 1,
      .pCommandBuffers = &cmd_buf,
  };
  r = vkQueueSubmit(state->queue, 1, &submit_info, VK_NULL_HANDLE);
  if (r != VK_SUCCESS) {
    BOOST_LOG_TRIVIAL(error) << "Failed to submit text copy commands: "
                             << VkResultToString(r);
    vkFreeCommandBuffers(state->device, cmd_pool, 1, &cmd_buf);
    vkDestroyCommandPool(state->device, cmd_pool, nullptr);
    vmaDestroyImage(state->allocator, text_image, alloc_text_image);
    vmaDestroyBuffer(state->allocator, staging_buffer, staging_allocation);
    return false;
  }

  // Wait for queue to finish
  r = vkQueueWaitIdle(state->queue);
  if (r != VK_SUCCESS) {
    BOOST_LOG_TRIVIAL(error) << "Failed to wait for text copy: "
                             << VkResultToString(r);
    vkFreeCommandBuffers(state->device, cmd_pool, 1, &cmd_buf);
    vkDestroyCommandPool(state->device, cmd_pool, nullptr);
    vmaDestroyImage(state->allocator, text_image, alloc_text_image);
    vmaDestroyBuffer(state->allocator, staging_buffer, staging_allocation);
    return false;
  }

  // Cleanup temporary command pool and staging buffer
  vkFreeCommandBuffers(state->device, cmd_pool, 1, &cmd_buf);
  vkDestroyCommandPool(state->device, cmd_pool, nullptr);
  vmaDestroyBuffer(state->allocator, staging_buffer, staging_allocation);

  // Create image view
  VkImageViewCreateInfo view_info = {
      .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
      .image = text_image,
      .viewType = VK_IMAGE_VIEW_TYPE_2D,
      .format = VK_FORMAT_R8_UNORM,
      .components = {
          .r = VK_COMPONENT_SWIZZLE_R,
          .g = VK_COMPONENT_SWIZZLE_R,
          .b = VK_COMPONENT_SWIZZLE_R,
          .a = VK_COMPONENT_SWIZZLE_ONE,
      },
      .subresourceRange = {
          .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
          .baseMipLevel = 0,
          .levelCount = 1,
          .baseArrayLayer = 0,
          .layerCount = 1,
      },
  };
  r = vkCreateImageView(state->device, &view_info, nullptr, &text_image_view);
  if (r != VK_SUCCESS) {
    BOOST_LOG_TRIVIAL(error) << "Failed to create text image view: "
                             << VkResultToString(r);
    vmaDestroyImage(state->allocator, text_image, alloc_text_image);
    return false;
  }

  // Create sampler
  VkSamplerCreateInfo sampler_ci = {
      .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
      .magFilter = VK_FILTER_LINEAR,
      .minFilter = VK_FILTER_LINEAR,
      .mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
      .addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
      .addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
      .addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
      .mipLodBias = 0.0f,
      .anisotropyEnable = VK_FALSE,
      .maxAnisotropy = 1.0f,
      .compareEnable = VK_FALSE,
      .compareOp = VK_COMPARE_OP_ALWAYS,
      .minLod = 0.0f,
      .maxLod = 0.0f,
      .borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK,
      .unnormalizedCoordinates = VK_FALSE,
  };
  r = vkCreateSampler(state->device, &sampler_ci, nullptr, &text_sampler);
  if (r != VK_SUCCESS) {
    BOOST_LOG_TRIVIAL(error) << "Failed to create text sampler: "
                             << VkResultToString(r);
    vkDestroyImageView(state->device, text_image_view, nullptr);
    vmaDestroyImage(state->allocator, text_image, alloc_text_image);
    return false;
  }

  // Create color uniform buffer
  VkBufferCreateInfo color_buffer_ci = {
      .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
      .size = sizeof(glm::vec4),
      .usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
  };
  VmaAllocationCreateInfo color_alloc_ci = {
      .flags = VMA_ALLOCATION_CREATE_MAPPED_BIT,
      .usage = VMA_MEMORY_USAGE_CPU_TO_GPU,
  };
  r = vmaCreateBuffer(state->allocator, &color_buffer_ci, &color_alloc_ci,
                      &buffer_text_color, &alloc_text_color, nullptr);
  if (r != VK_SUCCESS) {
    BOOST_LOG_TRIVIAL(error) << "Failed to create text color uniform buffer: "
                             << VkResultToString(r);
    vkDestroySampler(state->device, text_sampler, nullptr);
    vkDestroyImageView(state->device, text_image_view, nullptr);
    vmaDestroyImage(state->allocator, text_image, alloc_text_image);
    return false;
  }

  // Update color buffer with initial color
  void* color_mapped = nullptr;
  vmaMapMemory(state->allocator, alloc_text_color, &color_mapped);
  glm::vec4 color = text_internal_if->GetColor();
  memcpy(color_mapped, &color, sizeof(glm::vec4));
  vmaUnmapMemory(state->allocator, alloc_text_color);

  BOOST_LOG_TRIVIAL(trace) << "Text resources created successfully";
  return true;
}

void FgTextObjects::Render(VkCommandBuffer cmd_buf, VkPipeline pipeline) const {
  vkCmdBindPipeline(cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
  // Descriptor set already bound (same as scene, includes text texture and color)
  // Bind text vertex buffer (binding 0)
  VkDeviceSize offsets[] = {0};
  vkCmdBindVertexBuffers(cmd_buf, 0, 1, &buffer_text_vertex, offsets);
  // Bind text index buffer
  vkCmdBindIndexBuffer(cmd_buf, buffer_text_index, 0,
                        VK_INDEX_TYPE_UINT16);
  // Viewport and scissor already set (same as scene)
  vkCmdDrawIndexed(cmd_buf, text_index_count, 1, 0, 0, 0);
}

bool FgTextObjects::InitQuad(VulkanState* state, const VkExtent2D& image_extent,
                             const ScreenText* text) {
  BOOST_LOG_TRIVIAL(trace) << "Creating text quad buffers...";

  if (image_extent.width == 0 || image_extent.height == 0) {
    BOOST_LOG_TRIVIAL(warning) << "Invalid image extent, skipping text quad buffer creation";
    return false;
  }

  if (!text) {
    BOOST_LOG_TRIVIAL(warning) << "Text is empty, skipping quad buffer creation";
    return false;
  }
  auto text_inner_if = text->GetInnerInterface();

  // Convert screen coordinates to NDC
  float screen_width = static_cast<float>(image_extent.width);
  float screen_height = static_cast<float>(image_extent.height);

  // Vertex data: position (x,y) and UV (u,v)
  struct Vertex {
    float x, y;
    float u, v;
  };

  // Index data for two triangles per character
  std::vector<Vertex> vertices;
  std::vector<uint16_t> indices;

  // Starting position in screen coordinates
  glm::ivec2 position = text_inner_if->GetPosition();
  float cursor_x = static_cast<float>(position.x);
  float cursor_y = static_cast<float>(position.y);

  const std::string& text_str = text_inner_if->GetText();
  uint32_t vertex_offset = 0;

  for (size_t i = 0; i < text_str.size(); ++i) {
    char c = text_str[i];
    uint32_t codepoint = static_cast<uint32_t>(c);

    const GlyphInfo* glyph = text_inner_if->GetGlyphInfo(codepoint);
    if (!glyph)
      continue; //< Skip unknown characters

    // Calculate quad position in screen coordinates
    float x_pos = cursor_x + glyph->bearing.x;
    float y_pos = cursor_y - glyph->bearing.y;  // Y is down in screen coordinates

    float w = static_cast<float>(glyph->size.x);
    float h = static_cast<float>(glyph->size.y);

    // Convert to NDC
    float left = (x_pos * 2.0f / screen_width) - 1.0f;
    float right = ((x_pos + w) * 2.0f / screen_width) - 1.0f;
    float top = (y_pos * 2.0f / screen_height) - 1.0f;
    float bottom = ((y_pos + h) * 2.0f / screen_height) - 1.0f;

    // UV coordinates from atlas
    float u_min = glyph->uv_min.x;
    float u_max = glyph->uv_max.x;
    float v_min = glyph->uv_min.y;
    float v_max = glyph->uv_max.y;

    // Add vertices for this quad
    vertices.push_back({left,  top,    u_min, v_min});
    vertices.push_back({right, top,    u_max, v_min});
    vertices.push_back({right, bottom, u_max, v_max});
    vertices.push_back({left,  bottom, u_min, v_max});

    // Add indices for two triangles
    indices.push_back(vertex_offset + 0);
    indices.push_back(vertex_offset + 1);
    indices.push_back(vertex_offset + 2);
    indices.push_back(vertex_offset + 0);
    indices.push_back(vertex_offset + 2);
    indices.push_back(vertex_offset + 3);

    vertex_offset += 4;

    // Advance cursor
    cursor_x += glyph->advance / 64.0f;  // advance is in 1/64 pixels
  }

  if (vertices.empty()) {
    BOOST_LOG_TRIVIAL(warning) << "No vertices generated for text";
    return false;
  }

  // Destroy existing buffers if any
  if (buffer_text_vertex != VK_NULL_HANDLE) {
    vmaDestroyBuffer(state->allocator, buffer_text_vertex, alloc_text_vertex);
    buffer_text_vertex = VK_NULL_HANDLE;
  }
  if (buffer_text_index != VK_NULL_HANDLE) {
    vmaDestroyBuffer(state->allocator, buffer_text_index, alloc_text_index);
    buffer_text_index = VK_NULL_HANDLE;
  }

  // Create vertex buffer
  VkBufferCreateInfo vertexBufferInfo = {
      .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
      .size = vertices.size() * sizeof(Vertex),
      .usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
  };
  VmaAllocationCreateInfo vertexAllocInfo = {
      .usage = VMA_MEMORY_USAGE_CPU_TO_GPU,
  };
  VkResult r = vmaCreateBuffer(state->allocator, &vertexBufferInfo, &vertexAllocInfo,
                               &buffer_text_vertex, &alloc_text_vertex, nullptr);
  if (r != VK_SUCCESS) {
    BOOST_LOG_TRIVIAL(error) << "Failed to create text vertex buffer: "
                             << VkResultToString(r);
    return false;
  }

  // Copy vertex data
  void* mapped_data = nullptr;
  vmaMapMemory(state->allocator, alloc_text_vertex, &mapped_data);
  memcpy(mapped_data, vertices.data(), vertices.size() * sizeof(Vertex));
  vmaUnmapMemory(state->allocator, alloc_text_vertex);

  // Create index buffer
  VkBufferCreateInfo indexBufferInfo = {
      .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
      .size = indices.size() * sizeof(uint16_t),
      .usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
  };
  VmaAllocationCreateInfo indexAllocInfo = {
      .usage = VMA_MEMORY_USAGE_CPU_TO_GPU,
  };
  r = vmaCreateBuffer(state->allocator, &indexBufferInfo, &indexAllocInfo,
                      &buffer_text_index, &alloc_text_index, nullptr);
  if (r != VK_SUCCESS) {
    BOOST_LOG_TRIVIAL(error) << "Failed to create text index buffer: "
                             << VkResultToString(r);
    vmaDestroyBuffer(state->allocator, buffer_text_vertex, alloc_text_vertex);
    buffer_text_vertex = VK_NULL_HANDLE;
    return false;
  }

  // Copy index data
  vmaMapMemory(state->allocator, alloc_text_index, &mapped_data);
  memcpy(mapped_data, indices.data(), indices.size() * sizeof(uint16_t));
  vmaUnmapMemory(state->allocator, alloc_text_index);

  text_index_count = static_cast<uint32_t>(indices.size());
  BOOST_LOG_TRIVIAL(trace) << "Text quad buffers created, index count = " << text_index_count;
  return true;
}

void FgTextObjects::Release(VulkanState* state) {
  if (buffer_text_vertex != VK_NULL_HANDLE) {
    vmaDestroyBuffer(state->allocator, buffer_text_vertex, alloc_text_vertex);
    buffer_text_vertex = VK_NULL_HANDLE;
  }
  if (buffer_text_index != VK_NULL_HANDLE) {
    vmaDestroyBuffer(state->allocator, buffer_text_index, alloc_text_index);
    buffer_text_index = VK_NULL_HANDLE;
  }

  // Cleanup text resources
  if (text_sampler != VK_NULL_HANDLE) {
    vkDestroySampler(state->device, text_sampler, nullptr);
    text_sampler = VK_NULL_HANDLE;
  }
  if (text_image_view != VK_NULL_HANDLE) {
    vkDestroyImageView(state->device, text_image_view, nullptr);
    text_image_view = VK_NULL_HANDLE;
  }
  if (text_image != VK_NULL_HANDLE) {
    vmaDestroyImage(state->allocator, text_image, alloc_text_image);
    text_image = VK_NULL_HANDLE;
    alloc_text_image = VK_NULL_HANDLE;
  }
  if (buffer_text_color != VK_NULL_HANDLE) {
    vmaDestroyBuffer(state->allocator, buffer_text_color, alloc_text_color);
    buffer_text_color = VK_NULL_HANDLE;
  }
}

bool InitializeVulkanState(VulkanState* state, VkSurfaceKHR surface) {
  if (surface == VK_NULL_HANDLE)
    return false;

  if (state->renderpass != VK_NULL_HANDLE)
    return true;

  VkResult r = VK_SUCCESS;

  std::vector<const char*> deviceExtensionNames{
      VK_KHR_SWAPCHAIN_EXTENSION_NAME,
  };

  // Выбираем физическое устройство, если еще не выбрано.
  if (state->physical_device == VK_NULL_HANDLE &&
      !SelectPhysicalDevice(state, surface, deviceExtensionNames))
    return false;

  state->sample_count = GetMaxUsableSampleCount(state->physical_device);
  BOOST_LOG_TRIVIAL(trace) << std::format(
      "Selected MSAA sample count: {}",
      static_cast<size_t>(state->sample_count));

  // Создаем логическое устройство и извлекаем очередь.
  if (state->device == VK_NULL_HANDLE && !CreateLogicalDevice(state,surface))
    return false;

  // Выбираем настройки, необходимые для последующего создания swapchain.
  if (!ChooseSwapchainSettings(state,surface)) return false;

  // Создаем объект renderpass.
  if (!CreateRenderPass(state)) return false;

  if (!CreateGraphicsPipeline(state)) return false;
  if (!CreateFgImagePipeline(state)) return false;
  if (!CreateTextPipeline(state)) return false;

  VkCommandPoolCreateInfo ci_commandpool{
      .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
      .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
      .queueFamilyIndex = 0,  //< \warning
  };
  vkCreateCommandPool(state->device, &ci_commandpool, nullptr, &state->cmd_pool);
  return true;
}

VulkanObjects* InitializeVulkanObjects(VulkanState* vk_state,
                                       VkSurfaceKHR surface,
                                       Scene* scene) {
  // Доинициализируем глобальное состояние Vulkan,
  // потому что для этого была нужна поверхность Vulkan.
  if (!InitializeVulkanState(vk_state,surface))
    return nullptr;

  VulkanObjects vk_objects(vk_state);
  vk_objects.is_external_surface = true;
  vk_objects.surface = surface;

  // Создаем объект swapchain.
  glm::uvec2 screen_size = scene->GetScreenSize();
  if (!RecreateSwapchain(&vk_objects,VkExtent2D{screen_size.x, screen_size.y}))
    return nullptr;

  VkResult r;

  // Создаем объект пула дескрипторов.
  // Need descriptors for:
  // - Uniform buffers: binding 0 (vertex), binding 1 (fragment), binding 4 (text color)
  // - Combined image samplers: binding 2 (logo), binding 3 (text)
  std::array<VkDescriptorPoolSize, 2> descPoolSizes = {
      VkDescriptorPoolSize{
        .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .descriptorCount = vk_state->frame_count * 3},  // 3 uniform buffers per frame
      VkDescriptorPoolSize{
        .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        .descriptorCount = vk_state->frame_count * 2},  // 2 image samplers per frame
  };
  VkDescriptorPoolCreateInfo ci_descpool{
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
      .maxSets = vk_state->frame_count,
      .poolSizeCount = static_cast<uint32_t>(descPoolSizes.size()),
      .pPoolSizes = descPoolSizes.data()};
  r = vkCreateDescriptorPool(vk_state->device, &ci_descpool, nullptr,
                             &vk_state->descriptor_pool);

  for (uint32_t i = 0; i < vk_state->frame_count; ++i)
    InitFrame(vk_state, &vk_objects.frame_res[i]);

  // Память под данные фреймов.
  VkDeviceSize vbo_size = Aligned(sizeof(VertexBufferObject), 256);
  VkDeviceSize fbo_size = Aligned(sizeof(FragmentBufferObject), 256);
  size_t frame_memory_size = vk_state->frame_count * (vbo_size + fbo_size);
  VkBufferCreateInfo ci_buffer {
      .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
      .size = frame_memory_size,
      .usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT
  };
  VmaAllocationCreateInfo ci_allocation {
      .usage = VMA_MEMORY_USAGE_CPU_TO_GPU
  };
  vmaCreateBuffer(vk_state->allocator, &ci_buffer, &ci_allocation,
                  &vk_objects.buffer_frame, &vk_objects.alloc_frame, nullptr);

  vk_objects.stop_rendering = false;

  return new VulkanObjects{vk_objects};
}

}  // namespace

VkInstance VkSceneRenderer::GetInstance() const {
  return impl_->vulkanState.instance;
}

bool VkSceneRenderer::Initialize(Scene* scene, VkSurfaceKHR surface) {
  VulkanObjects* vk_objects =
      InitializeVulkanObjects(&impl_->vulkanState, surface, scene);
  if (!vk_objects)
    return false;
  impl_->scenes_vulkan_objects.insert(std::make_pair(scene,vk_objects));
  return true;
}

namespace {
void UpdateScene(VulkanObjects* vk_objects, const Scene* scene) {
  VulkanState* state = vk_objects->vk_state;
  if (vk_objects->buffer_vertex != VK_NULL_HANDLE) {
    vmaDestroyBuffer(state->allocator,vk_objects->buffer_vertex,vk_objects->alloc_vertex);
    vmaDestroyBuffer(state->allocator,vk_objects->buffer_normal,vk_objects->alloc_normal);
    vmaDestroyBuffer(state->allocator,vk_objects->buffer_color,vk_objects->alloc_color);
    vmaDestroyBuffer(state->allocator,vk_objects->buffer_index,vk_objects->alloc_index);
    vk_objects->buffer_vertex = VK_NULL_HANDLE;
    vk_objects->buffer_normal = VK_NULL_HANDLE;
    vk_objects->buffer_color = VK_NULL_HANDLE;
    vk_objects->buffer_index = VK_NULL_HANDLE;
  }

  size_t n_verts = 0, n_cells = 0;
  GetElementsCount(scene, n_verts, n_cells);

  VkDeviceSize vertex_buffer_size = Aligned(6 * n_verts * sizeof(float), 256);
  VkDeviceSize normal_buffer_size = Aligned(3 * n_cells * sizeof(float), 256);
  VkDeviceSize color_buffer_size = Aligned(3 * n_verts * sizeof(float), 256);
  VkDeviceSize index_buffer_size = Aligned(3 * n_cells * sizeof(uint32_t), 256);

  if(vertex_buffer_size == 0)
    return;

  {
    // Выделяем память под буфер вершин.
    VkBufferCreateInfo ci_buffer {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size = vertex_buffer_size,
        .usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT
    };
    VmaAllocationCreateInfo ci_allocation {
        .usage = VMA_MEMORY_USAGE_CPU_TO_GPU
    };
    VkResult r = vmaCreateBuffer(state->allocator,
                                 &ci_buffer, &ci_allocation,
                                 &vk_objects->buffer_vertex, &vk_objects->alloc_vertex,
                                 nullptr);
    // Копируем вершины.
    void* mapped_data = nullptr;
    vmaMapMemory(state->allocator, vk_objects->alloc_vertex, &mapped_data);
    float* data = reinterpret_cast<float*>(mapped_data);
    for (glm::vec3 pt : GetVertexIterator(scene)) {
      *data++ = pt.x;
      *data++ = pt.y;
      *data++ = pt.z;
    }
    vmaUnmapMemory(state->allocator, vk_objects->alloc_vertex);
  }

  {
    // Выделяем память под буфер с нормалями.
    VkBufferCreateInfo ci_buffer {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size = normal_buffer_size,
        .usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT
    };
    VmaAllocationCreateInfo ci_allocation {
        .usage = VMA_MEMORY_USAGE_CPU_TO_GPU
    };
    vmaCreateBuffer(state->allocator,
                    &ci_buffer, &ci_allocation, &vk_objects->buffer_normal,
                    &vk_objects->alloc_normal, nullptr);
    // Копируем нормали.
    void* mapped_data = nullptr;
    vmaMapMemory(state->allocator, vk_objects->alloc_normal, &mapped_data);
    float* data = reinterpret_cast<float*>(mapped_data);
    for (glm::vec3 n : GetNormalIterator(scene)) {
      *data++ = n.x;
      *data++ = n.y;
      *data++ = n.z;
    }
    vmaUnmapMemory(state->allocator, vk_objects->alloc_normal);
  }

  {
    // Выделяем память под буфер с цветами.
    VkBufferCreateInfo ci_buffer {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size = color_buffer_size,
        .usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT
    };
    VmaAllocationCreateInfo ci_allocation {
        .usage = VMA_MEMORY_USAGE_CPU_TO_GPU
    };
    vmaCreateBuffer(state->allocator,
                    &ci_buffer, &ci_allocation, &vk_objects->buffer_color,
                    &vk_objects->alloc_color, nullptr);
    // Копируем цвета.
    void* mapped_data = nullptr;
    vmaMapMemory(state->allocator, vk_objects->alloc_color, &mapped_data);
    float* data = reinterpret_cast<float*>(mapped_data);
    for (glm::vec3 c : GetColorIterator(scene)) {
      *data++ = c.x;
      *data++ = c.y;
      *data++ = c.z;
    }
    vmaUnmapMemory(state->allocator, vk_objects->alloc_color);
  }

  {
    // Выделяем память под индексный буфер.
    VkBufferCreateInfo ci_buffer {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size = index_buffer_size,
        .usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT
    };
    VmaAllocationCreateInfo ci_allocation {
        .usage = VMA_MEMORY_USAGE_CPU_TO_GPU
    };
    vmaCreateBuffer(state->allocator,
                    &ci_buffer, &ci_allocation, &vk_objects->buffer_index,
                    &vk_objects->alloc_index, nullptr);

    // Копируем индексный буфер.
    void* mapped_data = nullptr;
    vmaMapMemory(state->allocator, vk_objects->alloc_index, &mapped_data);
    uint32_t* data = reinterpret_cast<uint32_t*>(mapped_data);
    for (glm::u32vec3 tr : GetTriaIterator(scene)) {
      *data++ = tr.x;
      *data++ = tr.y;
      *data++ = tr.z;
    }
    vmaUnmapMemory(state->allocator, vk_objects->alloc_index);
  }

  vk_objects->index_count = 3 * n_cells;

  // Create foreground image resources.
  vk_objects->fg_image_objects.Release(state);
  auto scene_inner_if = scene->GetInnerInterface();
  if (scene_inner_if->HasFgImage()) {
    const ForegroundImage& fg_image = scene_inner_if->GetFgImage();
    if (!vk_objects->fg_image_objects.Initialize(state,fg_image)) {
      BOOST_LOG_TRIVIAL(warning) << "Failed to create foreground image resources, continuing without image";
    }
  }

  vk_objects->fg_text_objects.Release(state);
  vk_objects->screen_text = nullptr;
  if (scene_inner_if->HasScreenTexts()) {
    const ScreenText* text = *(scene_inner_if->GetScreenTextObjects());
    vk_objects->screen_text = text;
    if (!vk_objects->fg_text_objects.Initialize(state,text)) {
      BOOST_LOG_TRIVIAL(warning) << "Failed to create foreground text resources, continuing without text";
    }
  }
}

void UpdateDescriptorSets(Scene* scene, VulkanObjects* vk_objects) {
  const VulkanState* state = vk_objects->vk_state;
  FrameResources& frame = vk_objects->frame_res[vk_objects->current_frame_index];

  glm::mat4 model_view_matrix = scene->GetViewMatrix();
  glm::mat3 normal_matrix = glm::mat3(1.0); //glm::transpose(glm::inverse(glm::mat3(model_view_matrix)));
  VertexBufferObject vbo{
    .mvp_matrix = scene->GetProjectionMatrix() * model_view_matrix,
    .mv_matrix = model_view_matrix,
    .normal_matrix_row0 = glm::vec4(normal_matrix[0], 0.0f),
    .normal_matrix_row1 = glm::vec4(normal_matrix[1], 0.0f),
    .normal_matrix_row2 = glm::vec4(normal_matrix[2], 0.0f)
  };
  // Адаптивное размещение источника освещения на основе размера сцены
  AlignedBoundBox box = scene->GetBoundBox();
  glm::vec3 cameraPos = scene->CameraPosition();
  glm::vec3 sceneCenter = box.GetCenter();
  glm::vec3 sceneSize = box.GetSize();
  float sceneRadius = glm::length(sceneSize) * 0.5f;
  float lightDistance = sceneRadius * 2.0f;
  glm::vec3 lightOffset = glm::normalize(glm::vec3(1.0f, 1.0f, -0.5f)) * lightDistance;
  glm::vec3 lightPos = cameraPos + lightOffset;
  FragmentBufferObject fbo{
    .light_pos = glm::vec4(lightPos, 0.0f),
    .view_pos = glm::vec4(cameraPos, 0.0f)
  };

  VkDeviceSize vbo_size = Aligned(sizeof(vbo), 256);
  VkDeviceSize fbo_size = Aligned(sizeof(fbo), 256);
  VkDeviceSize frame_data_size = vbo_size + fbo_size;
  VkDeviceSize frame_offset = vk_objects->current_frame_index * frame_data_size;

  // Осуществляем прямую запись данных в разделяемую память CPU и GPU.
  void* mapped_data = nullptr;
  vmaMapMemory(state->allocator, vk_objects->alloc_frame, &mapped_data);
  std::byte* data = reinterpret_cast<std::byte*>(mapped_data) + frame_offset;
  std::memcpy(data, &vbo, sizeof(vbo));
  std::memcpy(data + vbo_size, &fbo, sizeof(fbo));
  vmaUnmapMemory(state->allocator, vk_objects->alloc_frame);

  std::vector<VkDescriptorBufferInfo> bufferInfo{
      VkDescriptorBufferInfo{
        .buffer = vk_objects->buffer_frame,
        .offset = frame_offset,
        .range = sizeof(vbo)
      },
      VkDescriptorBufferInfo{
        .buffer = vk_objects->buffer_frame,
        .offset = frame_offset + vbo_size,
        .range = sizeof(fbo)
      },
  };
  std::vector<VkWriteDescriptorSet> descWrites{
      VkWriteDescriptorSet{
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .dstSet = frame.desc_set,
        .dstBinding = 0,
        .descriptorCount = 1,
        .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .pBufferInfo = &bufferInfo[0],
      },
      VkWriteDescriptorSet{
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .dstSet = frame.desc_set,
        .dstBinding = 1,
        .descriptorCount = 1,
        .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .pBufferInfo = &bufferInfo[1],
      },
  };
  // Add combined image sampler for logo if available
  if (vk_objects->fg_image_objects.logo_image_view != VK_NULL_HANDLE && vk_objects->fg_image_objects.logo_sampler != VK_NULL_HANDLE) {
    VkDescriptorImageInfo imageInfo{
        .sampler = vk_objects->fg_image_objects.logo_sampler,
        .imageView = vk_objects->fg_image_objects.logo_image_view,
        .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
    };
    descWrites.push_back(VkWriteDescriptorSet{
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .dstSet = frame.desc_set,
        .dstBinding = 2,
        .descriptorCount = 1,
        .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        .pImageInfo = &imageInfo,
    });
  }

  // Add combined image sampler for text if available
  if (vk_objects->fg_text_objects.text_image_view != VK_NULL_HANDLE && vk_objects->fg_text_objects.text_sampler != VK_NULL_HANDLE) {
    VkDescriptorImageInfo imageInfo{
        .sampler = vk_objects->fg_text_objects.text_sampler,
        .imageView = vk_objects->fg_text_objects.text_image_view,
        .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
    };
    descWrites.push_back(VkWriteDescriptorSet{
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .dstSet = frame.desc_set,
        .dstBinding = 3,
        .descriptorCount = 1,
        .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        .pImageInfo = &imageInfo,
    });
  }

  // Add uniform buffer for text color if available
  if (vk_objects->fg_text_objects.buffer_text_color != VK_NULL_HANDLE) {
    VkDescriptorBufferInfo bufferInfo{
        .buffer = vk_objects->fg_text_objects.buffer_text_color,
        .offset = 0,
        .range = sizeof(glm::vec4),
    };
    descWrites.push_back(VkWriteDescriptorSet{
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .dstSet = frame.desc_set,
        .dstBinding = 4,
        .descriptorCount = 1,
        .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .pBufferInfo = &bufferInfo,
    });
  }

  vkUpdateDescriptorSets(state->device,
                         static_cast<uint32_t>(descWrites.size()),
                         descWrites.data(), 0, nullptr);
}

void UpdateDataForFrame(Application* app, Scene* scene, VulkanObjects* vk_objects) {
  VulkanState* state = vk_objects->vk_state;
  FrameResources& frame = vk_objects->frame_res[vk_objects->current_frame_index];

  // Если следует обновить сцену, то ожидаем завершения заданий в очередях
  // и блокируем рендеринг до обновления вершинных и индексных буферов.
  {
    static std::mutex mtx;
    std::lock_guard<std::mutex> lock(mtx);
    if (scene->HasChanges()) {
      vk_objects->stop_rendering = true;
      vkQueueWaitIdle(state->queue);
      UpdateScene(vk_objects, scene);
      scene->ClearChanges();
      vk_objects->stop_rendering = false;
    }
  }
  UpdateDescriptorSets(scene, vk_objects);
}
}  // namespace

bool VkSceneRenderer::Update(Scene* scene) {
  if (!scene) return false;
  auto it = impl_->scenes_vulkan_objects.find(scene);
  if (it == impl_->scenes_vulkan_objects.end())
    return false;
  VulkanObjects* vk_objects = it->second;
  VulkanState* state = vk_objects->vk_state;
  if (vk_objects->stop_rendering) return false;

  auto screen_size = scene->GetScreenSize();
  if (screen_size.x == 0 || screen_size.y == 0)
    return false;
  VkExtent2D currentExtent(screen_size.x, screen_size.y);
  if (vk_objects->image_extent.width != currentExtent.width ||
      vk_objects->image_extent.height != currentExtent.height) {
    if (!RecreateSwapchain(vk_objects,currentExtent)) return false;
  }

  FrameResources& frame = vk_objects->frame_res[vk_objects->current_frame_index];

  // Ожидаем освобождения последнего фрейма.
  if (frame.cmd_fence_waitable) {
    vkWaitForFences(state->device, 1, &frame.cmd_fence, VK_TRUE, UINT64_MAX);
    vkResetFences(state->device, 1, &frame.cmd_fence);
    frame.cmd_fence_waitable = false;
  }

  UpdateDataForFrame(impl_->app, scene, vk_objects);

  while (true) {
    uint32_t index;
    VkResult r =
        vkAcquireNextImageKHR(state->device, vk_objects->swapchain, UINT64_MAX,
                              frame.acquire_semaphore, VK_NULL_HANDLE, &index);
    if (r == VK_SUBOPTIMAL_KHR || r == VK_ERROR_OUT_OF_DATE_KHR) {
      auto screen_size = scene->GetScreenSize();
      if (screen_size.x == 0 || screen_size.y == 0)
        return false;
      VkExtent2D extent(screen_size.x,screen_size.y);
      RecreateSwapchain(vk_objects, extent);
      continue;
    }
    if (r == VK_NOT_READY || r == VK_TIMEOUT) continue;
    if (r != VK_SUCCESS) return false;

    // Рендеринг основной сцены.
    assert(index <= MAX_NUM_IMAGES);
    ImageResources& image = vk_objects->image_res[index];
    Render(scene, vk_objects, image, frame);

    // Отправка полученного изображения на презентацию.
    VkPresentInfoKHR presentInfoKHR{
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &image.submit_semaphore,
        .swapchainCount = 1,
        .pSwapchains = &vk_objects->swapchain,
        .pImageIndices = &index,
    };
    r = vkQueuePresentKHR(state->queue, &presentInfoKHR);
    if (r != VK_SUCCESS) return false;

    break;
  }

  vk_objects->current_frame_index = (vk_objects->current_frame_index + 1) % MAX_NUM_FRAMES;
  return true;
}

namespace {
void FgImageObjects::Release(VulkanState* state) {
  // Cleanup logo resources
  if (logo_sampler != VK_NULL_HANDLE) {
    vkDestroySampler(state->device, logo_sampler, nullptr);
    logo_sampler = VK_NULL_HANDLE;
  }
  if (logo_image_view != VK_NULL_HANDLE) {
    vkDestroyImageView(state->device, logo_image_view, nullptr);
    logo_image_view = VK_NULL_HANDLE;
  }
  if (logo_image != VK_NULL_HANDLE) {
    vmaDestroyImage(state->allocator, logo_image,
                    alloc_logo_image);
    logo_image = VK_NULL_HANDLE;
    alloc_logo_image = VK_NULL_HANDLE;
  }
}
}

void VkSceneRenderer::Finalize(Scene* scene) {
  if (!scene)
    return;
  auto it = impl_->scenes_vulkan_objects.find(scene);
  if (it == impl_->scenes_vulkan_objects.end())
    return;
  VulkanObjects* vk_objects = it->second;
  impl_->scenes_vulkan_objects.erase(it);

  VulkanState* state = vk_objects->vk_state;

  vkDeviceWaitIdle(state->device);

  ReleaseSwapchain(vk_objects);

  for (int i = 0; i < state->frame_count; ++i) {
    FrameResources* frame = &vk_objects->frame_res[i];
    ::Finalize(state, frame);
  }

  vmaDestroyBuffer(state->allocator, vk_objects->buffer_vertex, vk_objects->alloc_vertex);
  vmaDestroyBuffer(state->allocator, vk_objects->buffer_normal, vk_objects->alloc_normal);
  vmaDestroyBuffer(state->allocator, vk_objects->buffer_color, vk_objects->alloc_color);
  vmaDestroyBuffer(state->allocator, vk_objects->buffer_index, vk_objects->alloc_index);
  vmaDestroyBuffer(state->allocator, vk_objects->buffer_frame, vk_objects->alloc_frame);

  // Cleanup fgimage quad buffers
  vk_objects->fg_image_objects.Release(state);
  vk_objects->fg_text_objects.Release(state);

  if (!vk_objects->is_external_surface)
    vkDestroySurfaceKHR(state->instance, vk_objects->surface, nullptr);
  vk_objects->surface = VK_NULL_HANDLE;
}
