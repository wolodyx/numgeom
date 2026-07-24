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
#include "numgeom/fgtext.h"
#include "numgeom/scene.h"
#include "numgeom/sceneobject.h"

#include "fgimage.h"
#include "lrudescriptorsetpool.h"
#include "sceneiterators.h"
#include "vkutilities.h"

static_assert(VK_NULL_HANDLE == nullptr);

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
struct ImageRes {
  //! Изображение для показа.
  VkImage image = VK_NULL_HANDLE;

  //! Вид изображения для показа.
  VkImageView image_view = VK_NULL_HANDLE;

  //! Изображение сцены для первого subpass.
  VkImage scene_color_image = VK_NULL_HANDLE;
  VmaAllocation alloc_scene_color_image = VK_NULL_HANDLE;
  VkImageView scene_color_image_view = VK_NULL_HANDLE;

  //! Изображение идентификаторов объектов для первого subpass.
  VkImage object_id_image = VK_NULL_HANDLE;
  VmaAllocation alloc_object_id_image = VK_NULL_HANDLE;
  VkImageView object_id_image_view = VK_NULL_HANDLE;

  //! Сэмплированное изображение, создается только при
  //! `VulkanGlobalState::sample_count != VK_SAMPLE_COUNT_1_BIT`.
  VkImage msaa_image = VK_NULL_HANDLE;

  VmaAllocation alloc_msaa_image = VK_NULL_HANDLE;

  // Вид сэмплированного изображения.
  VkImageView msaa_image_view = VK_NULL_HANDLE;

  //! Сэмплированное изображение object_id, создается только при
  //! `VulkanState::sampleCount != VK_SAMPLE_COUNT_1_BIT`.
  VkImage msaa_object_id_image = VK_NULL_HANDLE;
  VmaAllocation alloc_msaa_object_id_image = VK_NULL_HANDLE;
  VkImageView msaa_object_id_image_view = VK_NULL_HANDLE;

  //! Фреймбуфер.
  VkFramebuffer frame_buffer = VK_NULL_HANDLE;

  //! Фреймбуфер для композитинга (только цветовой аттачмент, без MSAA/depth).
  VkFramebuffer composite_frame_buffer = VK_NULL_HANDLE;

  //! https://docs.vulkan.org/guide/latest/swapchain_semaphore_reuse.html
  VkSemaphore submit_semaphore = VK_NULL_HANDLE;
};

struct FrameRes {
  VkDescriptorSet graphics_desc_set = VK_NULL_HANDLE;
  VkDescriptorSet selection_desc_set = VK_NULL_HANDLE;
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
struct VulkanGlobalState {
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
  VkFormat depth_stencil_format = VK_FORMAT_UNDEFINED;
  VkPresentModeKHR present_mode = VK_PRESENT_MODE_FIFO_KHR;
  uint32_t min_image_count = 0;
  VkSampleCountFlagBits sample_count = VK_SAMPLE_COUNT_1_BIT;
  uint32_t frame_count = MAX_NUM_FRAMES;

  VkRenderPass graphics_renderpass = VK_NULL_HANDLE;
  VkRenderPass composite_renderpass = VK_NULL_HANDLE;

  VkPipelineLayout graphics_pipeline_layout = VK_NULL_HANDLE;
  VkPipeline graphics_pipeline = VK_NULL_HANDLE;

  VkDescriptorSetLayout selection_desc_layout = VK_NULL_HANDLE;
  VkPipelineLayout selection_pipeline_layout = VK_NULL_HANDLE;
  VkPipeline selection_pipeline = VK_NULL_HANDLE;

  VkPipelineLayout composite_pipeline_layout = VK_NULL_HANDLE;
  VkPipeline composite_pipeline = VK_NULL_HANDLE;

  VkCommandPool cmd_pool = VK_NULL_HANDLE;
  VkDescriptorSetLayout graphics_desc_layout = VK_NULL_HANDLE;
  VkDescriptorSetLayout composite_desc_layout = VK_NULL_HANDLE;
  VkSampler composite_sampler = VK_NULL_HANDLE;

  LruPool::PoolPtr textures_desc_pool;
};

//! Ресурсы Vulkan для представления текстуры.
struct TextureRes : public LruPool::Consumer {
  TextureRes(VulkanGlobalState* vk_global_state)
    : LruPool::Consumer(vk_global_state->textures_desc_pool) {
  }
  bool ConfiscateResource() {
    desc_set = VK_NULL_HANDLE;
    return true;
  }

  void SetResource(LruPool::Resource* base_res) {
    auto res = dynamic_cast<LruDescriptorSetPool::Resource*>(base_res);
    desc_set = res->desc_set_;
  }

  VkImage image = VK_NULL_HANDLE;
  VmaAllocation alloc_image = VK_NULL_HANDLE;
  VkImageView image_view = VK_NULL_HANDLE;
  VkDescriptorSet desc_set = VK_NULL_HANDLE;

  bool operator!() const { return image == VK_NULL_HANDLE; }
  bool Initialize(VulkanGlobalState*, const FgObject*);
  void Release(VulkanGlobalState*);
};

class SceneRes {
 public:
  SceneRes(VulkanGlobalState*, VkSurfaceKHR, Scene*);
  bool RestoreInvalidated();
  void InvalidateSwapchain();
  void Release();
 private:
  bool InitImages();
  void ReleaseImages();
  bool InitFrames();

 public:
  VulkanGlobalState* vk_state;

  VkSurfaceKHR surface = VK_NULL_HANDLE;
  bool is_external_surface = false;

  VkDescriptorPool descriptor_pool = VK_NULL_HANDLE;

  std::array<FrameRes, MAX_NUM_FRAMES> frame_res_array;
  size_t current_frame_index = 0;

  VkSwapchainKHR swapchain = VK_NULL_HANDLE;
  VkSwapchainKHR old_swapchain = VK_NULL_HANDLE;
  uint32_t image_count = 0;
  VkExtent2D image_extent = {
    .width = static_cast<uint32_t>(-1),
    .height = static_cast<uint32_t>(-1)
  };
  uint32_t current_image_index = 0;
  std::array<ImageRes, MAX_NUM_IMAGES> image_res_array;
  VkImage depth_stencil_image = VK_NULL_HANDLE;
  VmaAllocation alloc_depth_stencil = VK_NULL_HANDLE;
  VkImageView depth_stencil_view = VK_NULL_HANDLE;

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
  VkBuffer buffer_object_id = VK_NULL_HANDLE;
  VmaAllocation alloc_object_id = VK_NULL_HANDLE;
  VkBuffer buffer_frame = VK_NULL_HANDLE;
  VmaAllocation alloc_frame = VK_NULL_HANDLE;
  VkBuffer buffer_selection = VK_NULL_HANDLE;
  VmaAllocation alloc_selection = VK_NULL_HANDLE;
  //! \}

  uint32_t index_count = 0; //!< Количество примитивов для отрисовки.
};

struct VulkanObjects {
  VulkanGlobalState global_state;
  std::map<Scene*, SceneRes*> scene_res_array;
  std::map<const FgObject*, TextureRes*> fg_res_array;
  bool RestoreInvalidated(VkSurfaceKHR);
};

//! Prototypes
//! \{
bool InitInstance(VulkanGlobalState*);
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
  VulkanObjects vk_objects; //!< Иерархия Vulkan-объектов.
};

VkSceneRenderer::VkSceneRenderer(Application* app) {
  assert(app != nullptr);

  impl_ = new Impl {
      .app = app,
  };

  if (!InitInstance(&impl_->vk_objects.global_state)) return;
}

namespace
{
void Finalize(VulkanGlobalState* state) {
  state->textures_desc_pool = LruPool::PoolPtr();

  if (state->composite_sampler != VK_NULL_HANDLE) {
    vkDestroySampler(state->device, state->composite_sampler, nullptr);
    state->composite_sampler = VK_NULL_HANDLE;
  }

  vkDestroyPipeline(state->device, state->graphics_pipeline, nullptr);
  state->graphics_pipeline = VK_NULL_HANDLE;
  if (state->selection_pipeline != VK_NULL_HANDLE) {
    vkDestroyPipeline(state->device, state->selection_pipeline, nullptr);
    state->selection_pipeline = VK_NULL_HANDLE;
  }
  if (state->composite_pipeline != VK_NULL_HANDLE) {
    vkDestroyPipeline(state->device, state->composite_pipeline, nullptr);
    state->composite_pipeline = VK_NULL_HANDLE;
  }

  vkDestroyPipelineLayout(state->device, state->graphics_pipeline_layout, nullptr);
  vkDestroyDescriptorSetLayout(state->device, state->graphics_desc_layout, nullptr);
  vkDestroyPipelineLayout(state->device, state->selection_pipeline_layout, nullptr);
  vkDestroyDescriptorSetLayout(state->device, state->selection_desc_layout, nullptr);

  vkDestroyPipelineLayout(state->device, state->composite_pipeline_layout, nullptr);
  vkDestroyDescriptorSetLayout(state->device, state->composite_desc_layout, nullptr);

  vkDestroyRenderPass(state->device, state->graphics_renderpass, nullptr);
  state->graphics_renderpass = VK_NULL_HANDLE;

  if (state->composite_renderpass != VK_NULL_HANDLE) {
    vkDestroyRenderPass(state->device, state->composite_renderpass, nullptr);
    state->composite_renderpass = VK_NULL_HANDLE;
  }

  vkDestroyCommandPool(state->device,state->cmd_pool,nullptr);
  state->cmd_pool = VK_NULL_HANDLE;

  vmaDestroyAllocator(state->allocator);

  vkDestroyDevice(state->device, nullptr);

  // vkDestroyInstance(state->instance, nullptr);
  // state->instance = VK_NULL_HANDLE;
}
}

VkSceneRenderer::~VkSceneRenderer() {
  for (const auto [fg, res] : impl_->vk_objects.fg_res_array) {
    res->Release(&impl_->vk_objects.global_state);
    delete res;
  }
  for (const auto [scene, res] : impl_->vk_objects.scene_res_array) {
    res->Release();
    delete res;
  }
  ::Finalize(&impl_->vk_objects.global_state);
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

bool ChooseSampleCountOfMSAA(VulkanGlobalState* state) {
  if (!IsValidSampleCount(state->sample_count)) {
    state->sample_count = GetMaxUsableSampleCount(state->physical_device);
    BOOST_LOG_TRIVIAL(trace) << std::format(
        "Selected MSAA sample count: {}",
        static_cast<size_t>(state->sample_count));
  }
  return true;
}

bool CreateGraphicsRenderPass(VulkanGlobalState* state) {
  if (state->graphics_renderpass) return true;

  bool msaa = (state->sample_count != VK_SAMPLE_COUNT_1_BIT);

  std::vector<VkAttachmentDescription> attachments = {
      {
          // [0] Swapchain color target used for presentation after the second subpass.
          .format = state->image_format,
          .samples = VK_SAMPLE_COUNT_1_BIT,
          .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
          .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
          .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
          .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
          .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
          .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
      },
      {
          // [1] Depth attachment used by the first subpass.
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
          // [2] Scene color resolve target (single-sampled, used as input in subpass 1).
          .format = state->image_format,
          .samples = VK_SAMPLE_COUNT_1_BIT,
          .loadOp = msaa ? VK_ATTACHMENT_LOAD_OP_DONT_CARE
                         : VK_ATTACHMENT_LOAD_OP_CLEAR,
          .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
          .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
          .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
          .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
          .finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
      },
      {
          // [3] Object-id resolve target (single-sampled, used as input in subpass 1).
          // When MSAA is enabled, this is the resolve target for the MSAA object_id.
          // When MSAA is disabled, this is written directly by subpass 0.
          .format = VK_FORMAT_R32_UINT,
          .samples = VK_SAMPLE_COUNT_1_BIT,
          .loadOp = msaa ? VK_ATTACHMENT_LOAD_OP_DONT_CARE
                         : VK_ATTACHMENT_LOAD_OP_CLEAR,
          .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
          .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
          .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
          .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
          .finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
      },
  };

  // MSAA attachments (only when multisampling is enabled).
  VkAttachmentDescription msaa_color_attachment{};
  VkAttachmentDescription msaa_object_id_attachment{};
  if (msaa) {
    // [4] MSAA color attachment — rendered to by subpass 0, resolved to attachment 2.
    msaa_color_attachment = {
        .format = state->image_format,
        .samples = state->sample_count,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
    };
    attachments.push_back(msaa_color_attachment);
    // [5] MSAA object_id attachment — rendered to by subpass 0, resolved to attachment 3.
    // Required by Vulkan: all color attachments in a subpass must have the same sample count.
    // The resolve is effectively a no-op for R32_UINT (takes sample 0).
    msaa_object_id_attachment = {
        .format = VK_FORMAT_R32_UINT,
        .samples = state->sample_count,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
    };
    attachments.push_back(msaa_object_id_attachment);
  }

  VkAttachmentReference out_color_att_ref = {
      .attachment = 0,
      .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
  };
  VkAttachmentReference depth_att_ref = {
      .attachment = 1,
      .layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
  };
  VkAttachmentReference color_att_ref = {
      .attachment = 2,
      .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
  };
  VkAttachmentReference object_id_att_ref = {
      .attachment = 3,
      .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
  };

  // When MSAA is enabled, subpass 0 renders to MSAA attachments (indices 4 and 5)
  // and resolves to single-sampled attachments (indices 2 and 3).
  // The MSAA object_id resolve is effectively a no-op for R32_UINT (takes sample 0),
  // but is required by Vulkan since all color attachments must have the same sample count.
  VkAttachmentReference msaa_color_att_ref{};
  VkAttachmentReference msaa_object_id_att_ref{};
  std::array<VkAttachmentReference, 2> first_pass_color_refs{};
  std::array<VkAttachmentReference, 2> resolve_att_refs{};
  if (msaa) {
    msaa_color_att_ref = {
        .attachment = 4,
        .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
    };
    msaa_object_id_att_ref = {
        .attachment = 5,
        .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
    };
    first_pass_color_refs = {msaa_color_att_ref, msaa_object_id_att_ref};
    resolve_att_refs = {
        VkAttachmentReference{.attachment = 2, .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL},
        VkAttachmentReference{.attachment = 3, .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL},
    };
  } else {
    first_pass_color_refs = {color_att_ref, object_id_att_ref};
  }

  std::array<VkAttachmentReference, 2> input_att_refs = {{
      {
          .attachment = 2,
          .layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
      },
      {
          .attachment = 3,
          .layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
      },
  }};

  std::array<VkSubpassDescription, 2> subpasses = {{
      {
          .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
          .colorAttachmentCount = static_cast<uint32_t>(first_pass_color_refs.size()),
          .pColorAttachments = first_pass_color_refs.data(),
          .pResolveAttachments = msaa ? resolve_att_refs.data() : nullptr,
          .pDepthStencilAttachment = &depth_att_ref
      },
      {
          .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
          .inputAttachmentCount = static_cast<uint32_t>(input_att_refs.size()),
          .pInputAttachments = input_att_refs.data(),
          .colorAttachmentCount = 1,
          .pColorAttachments = &out_color_att_ref
      },
  }};

  std::vector<VkSubpassDependency> dependencies = {
      {
          .srcSubpass = VK_SUBPASS_EXTERNAL,
          .dstSubpass = 0,
          .srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
          .dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
          .srcAccessMask = 0,
          .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
      },
      {
          .srcSubpass = 0,
          .dstSubpass = 1,
          .srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
          .dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
          .srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
          .dstAccessMask = VK_ACCESS_INPUT_ATTACHMENT_READ_BIT,
      },
  };

  VkRenderPassCreateInfo ci_renderpass{
      .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
      .attachmentCount = static_cast<uint32_t>(attachments.size()),
      .pAttachments = attachments.data(),
      .subpassCount = static_cast<uint32_t>(subpasses.size()),
      .pSubpasses = subpasses.data(),
      .dependencyCount = static_cast<uint32_t>(dependencies.size()),
      .pDependencies = dependencies.data(),
  };

  VkResult r = vkCreateRenderPass(state->device, &ci_renderpass, nullptr,
                                  &state->graphics_renderpass);
  if (r != VK_SUCCESS) {
    BOOST_LOG_TRIVIAL(trace) << std::format(
        "Graphics renderpass object creation error ({}).", VkResultToString(r));
    return false;
  }
  BOOST_LOG_TRIVIAL(trace) <<
      std::format("Graphics renderpass {} is created.",
                  static_cast<void*>(state->graphics_renderpass));
  return true;
}

bool CreateCompositeRenderPass(VulkanGlobalState* state) {
  if (state->composite_renderpass) return true;

  // Single color attachment, no depth/stencil, no multisampling.
  // LoadOp = LOAD to preserve the already-rendered scene.
  VkAttachmentDescription color_attachment = {
      .format = state->image_format,
      .samples = VK_SAMPLE_COUNT_1_BIT,
      .loadOp = VK_ATTACHMENT_LOAD_OP_LOAD,
      .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
      .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
      .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
      .initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
      .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
  };

  VkAttachmentReference color_attachment_ref = {
      .attachment = 0,
      .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
  };

  VkSubpassDescription subpass = {
      .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
      .colorAttachmentCount = 1,
      .pColorAttachments = &color_attachment_ref,
  };

  VkRenderPassCreateInfo ci_renderpass = {
      .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
      .attachmentCount = 1,
      .pAttachments = &color_attachment,
      .subpassCount = 1,
      .pSubpasses = &subpass,
  };

  VkResult r = vkCreateRenderPass(state->device, &ci_renderpass, nullptr,
                                  &state->composite_renderpass);
  if (r != VK_SUCCESS) {
    BOOST_LOG_TRIVIAL(trace) << std::format(
        "Composite renderpass creation error ({}).", VkResultToString(r));
    return false;
  }
  BOOST_LOG_TRIVIAL(trace) << std::format(
      "Composite renderpass {} is created.",
      static_cast<void*>(state->composite_renderpass));
  return true;
}

bool CreateSelectionPipeline(VulkanGlobalState* state) {
  VkResult r;

  if (!state->selection_desc_layout) {
    std::vector<VkDescriptorSetLayoutBinding> desc_set_layout_bindings{
        VkDescriptorSetLayoutBinding{
            .binding = 0,
            .descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
        },
        VkDescriptorSetLayoutBinding{
            .binding = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
        },
        VkDescriptorSetLayoutBinding{
            .binding = 2,
            .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
        },
    };
    VkDescriptorSetLayoutCreateInfo ci_desc_set_layout{
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .bindingCount = static_cast<uint32_t>(desc_set_layout_bindings.size()),
        .pBindings = desc_set_layout_bindings.data()};
    r = vkCreateDescriptorSetLayout(state->device, &ci_desc_set_layout, nullptr,
                                    &state->selection_desc_layout);
    if (r != VK_SUCCESS) {
      BOOST_LOG_TRIVIAL(trace) << std::format(
          "Selection descriptor set layout creation error ({}).",
          VkResultToString(r));
      return false;
    }
  }

  if (!state->selection_pipeline_layout) {
    std::array<VkDescriptorSetLayout, 1> descriptor_set_layouts{
        state->selection_desc_layout};
    VkPipelineLayoutCreateInfo ci_pipeline_layout{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .setLayoutCount = static_cast<uint32_t>(descriptor_set_layouts.size()),
        .pSetLayouts = descriptor_set_layouts.data(),
    };
    r = vkCreatePipelineLayout(state->device, &ci_pipeline_layout, nullptr,
                              &state->selection_pipeline_layout);
    if (r != VK_SUCCESS) {
      BOOST_LOG_TRIVIAL(trace) << std::format(
          "Selection pipeline layout creation error ({}).", VkResultToString(r));
      return false;
    }
  }

  if (state->selection_pipeline) return true;

  static uint32_t vs_spirv_source[] = {
#include "selection.vert.spv.h"
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
        "Selection vertex shader creation error ({}).", VkResultToString(r));
    return false;
  }

  static uint32_t fs_spirv_source[] = {
#include "selection.frag.spv.h"
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
        "Selection fragment shader creation error ({}).", VkResultToString(r));
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

  VkPipelineVertexInputStateCreateInfo ci_vertex_input_state{
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
      .cullMode = VK_CULL_MODE_NONE,
      .frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
      .lineWidth = 1.0f,
  };
  VkPipelineMultisampleStateCreateInfo ci_multisample_state{
      .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
      .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
  };
  VkPipelineDepthStencilStateCreateInfo ci_depth_stencil_state{
      .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
      .depthTestEnable = VK_FALSE,
      .depthWriteEnable = VK_FALSE,
      .depthCompareOp = VK_COMPARE_OP_ALWAYS,
  };
  VkPipelineColorBlendAttachmentState attachment_state = {
      .blendEnable = VK_FALSE,
      .colorWriteMask = VK_COLOR_COMPONENT_A_BIT | VK_COLOR_COMPONENT_R_BIT |
                        VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT,
  };
  VkPipelineColorBlendStateCreateInfo ci_color_blend_state{
      .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
      .logicOpEnable = VK_FALSE,
      .attachmentCount = 1,
      .pAttachments = &attachment_state,
  };
  VkDynamicState dynamic_states[] = {VK_DYNAMIC_STATE_VIEWPORT,
                                     VK_DYNAMIC_STATE_SCISSOR};
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
      .layout = state->selection_pipeline_layout,
      .renderPass = state->graphics_renderpass,
      .subpass = 1,
  }};
  VkPipeline pipelines[] = {VK_NULL_HANDLE};
  r = vkCreateGraphicsPipelines(state->device, VK_NULL_HANDLE, 1, ci_pipelines,
                                nullptr, pipelines);
  vkDestroyShaderModule(state->device, vs_module, nullptr);
  vkDestroyShaderModule(state->device, fs_module, nullptr);
  if (r != VK_SUCCESS) {
    BOOST_LOG_TRIVIAL(trace) << std::format(
        "Selection graphics pipeline creation error ({}).", VkResultToString(r));
    return false;
  }
  state->selection_pipeline = pipelines[0];
  BOOST_LOG_TRIVIAL(trace) << std::format(
      "Selection graphics pipeline {} is created.",
      static_cast<void*>(state->selection_pipeline));
  return true;
}

bool CreateCompositePipeline(VulkanGlobalState* state) {
  VkResult r;

  if (!state->composite_desc_layout) {
    // Создаем макеты набора дескрипторов.
    std::vector<VkDescriptorSetLayoutBinding> desc_set_layout_bindings {
        VkDescriptorSetLayoutBinding{
            .binding = 0,
            .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
        }
    };
    VkDescriptorSetLayoutCreateInfo ci_desc_set_layout {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .bindingCount = static_cast<uint32_t>(desc_set_layout_bindings.size()),
        .pBindings = desc_set_layout_bindings.data()};
    r = vkCreateDescriptorSetLayout(state->device, &ci_desc_set_layout,
                                    nullptr, &state->composite_desc_layout);
    if (r != VK_SUCCESS) {
      BOOST_LOG_TRIVIAL(trace) << std::format(
          "Descriptor set layout creation error ({}).", VkResultToString(r));
      return false;
    }
  }

  // Создаем макет конвейера.
  if (!state->composite_pipeline_layout) {
    std::array<VkDescriptorSetLayout, 1> descriptor_set_layouts {
      state->composite_desc_layout
    };
    // Push constant range for foreground image quad parameters
    VkPushConstantRange push_constant_range {
        .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
        .offset = 0,
        .size = 6 * sizeof(float),  // position (2) + size (2) + screenSize (2)
    };
    VkPipelineLayoutCreateInfo ci_pipeline_layout {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .setLayoutCount = static_cast<uint32_t>(descriptor_set_layouts.size()),
        .pSetLayouts = descriptor_set_layouts.data(),
        .pushConstantRangeCount = 1,
        .pPushConstantRanges = &push_constant_range,
    };
    r = vkCreatePipelineLayout(state->device, &ci_pipeline_layout, nullptr,
                               &state->composite_pipeline_layout);
    if (r != VK_SUCCESS) {
      BOOST_LOG_TRIVIAL(trace) << std::format(
          "Pipeline layout creation error ({}).", VkResultToString(r));
      return false;
    }
  }

  if (state->composite_pipeline) return true;

  // Shader modules are the same as fgimage pipeline.
  static uint32_t vs_spirv_source[] = {
#include "compositing.vert.spv.h"
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
        "Composite vertex shader creation error ({}).", VkResultToString(r));
    return false;
  }

  static uint32_t fs_spirv_source[] = {
#include "compositing.frag.spv.h"
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
        "Composite fragment shader creation error ({}).", VkResultToString(r));
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
      .cullMode = VK_CULL_MODE_NONE,
      .frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
      .lineWidth = 1.0f,
  };

  // Composite pipeline always uses VK_SAMPLE_COUNT_1_BIT
  VkPipelineMultisampleStateCreateInfo ci_multisample_state{
      .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
      .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
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
      .layout = state->composite_pipeline_layout,
      .renderPass = state->composite_renderpass,
      .subpass = 0,
  }};
  VkPipeline pipelines[] = {VK_NULL_HANDLE};
  r = vkCreateGraphicsPipelines(state->device, VK_NULL_HANDLE, 1, ci_pipelines,
                                nullptr, pipelines);
  if (r != VK_SUCCESS) {
    BOOST_LOG_TRIVIAL(trace) << std::format(
        "Composite graphics pipeline creation error ({}).", VkResultToString(r));
    vkDestroyShaderModule(state->device, vs_module, nullptr);
    vkDestroyShaderModule(state->device, fs_module, nullptr);
    return false;
  }
  state->composite_pipeline = pipelines[0];

  vkDestroyShaderModule(state->device, vs_module, nullptr);
  vkDestroyShaderModule(state->device, fs_module, nullptr);

  if (state->composite_pipeline == VK_NULL_HANDLE) {
    BOOST_LOG_TRIVIAL(trace) << "Composite graphics pipeline creation error.";
    return false;
  }
  BOOST_LOG_TRIVIAL(trace) << std::format("Composite graphics pipeline {} is created.",
                                          static_cast<void*>(state->composite_pipeline));
  return true;
}

bool CreateGraphicsPipeline(VulkanGlobalState* state) {
  VkResult r;

  // Создаем макеты набора дескрипторов.
  if (!state->graphics_desc_layout) {
    std::vector<VkDescriptorSetLayoutBinding> desc_set_layout_bindings{
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
    };
    VkDescriptorSetLayoutCreateInfo ci_desc_set_layout {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .bindingCount = static_cast<uint32_t>(desc_set_layout_bindings.size()),
        .pBindings = desc_set_layout_bindings.data()};
    r = vkCreateDescriptorSetLayout(state->device, &ci_desc_set_layout,
                                    nullptr, &state->graphics_desc_layout);
    if (r != VK_SUCCESS) {
      BOOST_LOG_TRIVIAL(trace) << std::format(
          "Descriptor set layout creation error ({}).", VkResultToString(r));
      return false;
    }
  }

  // Создаем макет конвейера.
  if (!state->graphics_pipeline_layout) {
    std::array<VkDescriptorSetLayout, 1> descriptor_set_layouts {
      state->graphics_desc_layout
    };
    VkPipelineLayoutCreateInfo ci_pipelineLayout{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .setLayoutCount = static_cast<uint32_t>(descriptor_set_layouts.size()),
        .pSetLayouts = descriptor_set_layouts.data(),
    };
    r = vkCreatePipelineLayout(state->device, &ci_pipelineLayout, nullptr,
                               &state->graphics_pipeline_layout);
    if (r != VK_SUCCESS) {
      BOOST_LOG_TRIVIAL(trace) << std::format(
          "Pipeline layout creation error ({}).", VkResultToString(r));
      return false;
    }
  }

  // Подготавливаем шейдерные модули (вершинный и фрагментный).

  if (!state->graphics_pipeline) {
    static uint32_t vs_spirv_source[] = {
#     include "app.vert.spv.h"
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
#     include "app.frag.spv.h"
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
        VkVertexInputBindingDescription{
          .binding = 3,
          .stride = sizeof(uint32_t),
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
        VkVertexInputAttributeDescription{
          .location = 3,
          .binding = 3,
          .format = VK_FORMAT_R32_UINT,
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

    std::array<VkPipelineColorBlendAttachmentState, 2> attachmentStates = {{
        {VK_FALSE, VK_BLEND_FACTOR_ONE, VK_BLEND_FACTOR_ZERO, VK_BLEND_OP_ADD,
         VK_BLEND_FACTOR_ONE, VK_BLEND_FACTOR_ZERO, VK_BLEND_OP_ADD,
         VK_COLOR_COMPONENT_A_BIT | VK_COLOR_COMPONENT_R_BIT |
             VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT},
        {VK_FALSE, VK_BLEND_FACTOR_ONE, VK_BLEND_FACTOR_ZERO, VK_BLEND_OP_ADD,
         VK_BLEND_FACTOR_ONE, VK_BLEND_FACTOR_ZERO, VK_BLEND_OP_ADD,
         VK_COLOR_COMPONENT_A_BIT | VK_COLOR_COMPONENT_R_BIT |
             VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT},
    }};
    VkPipelineColorBlendStateCreateInfo ci_colorBlendState{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .attachmentCount = static_cast<uint32_t>(attachmentStates.size()),
        .pAttachments = attachmentStates.data()};

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
        .layout = state->graphics_pipeline_layout,
        .renderPass = state->graphics_renderpass,
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
    state->graphics_pipeline = pipelines[0];

    vkDestroyShaderModule(state->device, vsModule, nullptr);
    vkDestroyShaderModule(state->device, fsModule, nullptr);

    if (state->graphics_pipeline == VK_NULL_HANDLE) {
      BOOST_LOG_TRIVIAL(trace) << "Graphics pipeline creation error.";
      return false;
    }
    BOOST_LOG_TRIVIAL(trace) <<
        std::format("Graphics pipeline {} is created.",
                    static_cast<void*>(state->graphics_pipeline));
  }

  return true;
}

bool SceneRes::InitFrames() {
  VkResult r;

  for (uint32_t i = 0; i < vk_state->frame_count; ++i) {
    FrameRes& frame = frame_res_array[i];
    if (!frame.cmd_fence) {
      BOOST_LOG_TRIVIAL(trace) << "Create frame fence ...";
      VkFenceCreateInfo ci_fence{.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
                                .flags = VK_FENCE_CREATE_SIGNALED_BIT};
      r = vkCreateFence(vk_state->device, &ci_fence, nullptr, &frame.cmd_fence);
      if (r != VK_SUCCESS) return false;
      frame.cmd_fence_waitable = true;
    }
    if (!frame.cmd_buf) {
      BOOST_LOG_TRIVIAL(trace) << "Create frame command buffer ...";
      VkCommandBufferAllocateInfo commandBufferAllocateInfo{
          .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
          .commandPool = vk_state->cmd_pool,
          .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
          .commandBufferCount = 1,
      };
      r = vkAllocateCommandBuffers(vk_state->device, &commandBufferAllocateInfo,
                                  &frame.cmd_buf);
      if (r != VK_SUCCESS) return false;
    }
    if (!frame.acquire_semaphore) {
      BOOST_LOG_TRIVIAL(trace) << "Create frame acquire semaphore ...";
      VkSemaphoreCreateInfo semaphoreCreateInfo{
          .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
      };
      r = vkCreateSemaphore(vk_state->device, &semaphoreCreateInfo, nullptr,
                            &frame.acquire_semaphore);
      if (r != VK_SUCCESS) return false;
    }
    if (!frame.graphics_desc_set) {
      BOOST_LOG_TRIVIAL(trace) << "Allocate frame graphics descriptor set ...";
      VkDescriptorSetAllocateInfo descSetAllocInfo{
          .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
          .descriptorPool = descriptor_pool,
          .descriptorSetCount = 1,
          .pSetLayouts = &vk_state->graphics_desc_layout};
      r = vkAllocateDescriptorSets(vk_state->device, &descSetAllocInfo,
                                  &frame.graphics_desc_set);
      if (r != VK_SUCCESS) return false;
    }

    if (!frame.selection_desc_set) {
      VkDescriptorSetAllocateInfo descSetAllocInfo{
          .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
          .descriptorPool = descriptor_pool,
          .descriptorSetCount = 1,
          .pSetLayouts = &vk_state->selection_desc_layout};
      r = vkAllocateDescriptorSets(vk_state->device, &descSetAllocInfo,
                                  &frame.selection_desc_set);
      if (r != VK_SUCCESS) return false;
    }
  }
  return true;
}

void SceneRes::ReleaseImages() {
  BOOST_LOG_TRIVIAL(trace) << "Finalize image resources ...";

  for (int i = 0; i < image_count; ++i) {
    ImageRes* res = &image_res_array[i];
    if (res->composite_frame_buffer != VK_NULL_HANDLE) {
      vkDestroyFramebuffer(vk_state->device,
                           res->composite_frame_buffer, nullptr);
      res->composite_frame_buffer = VK_NULL_HANDLE;
    }

    if (res->frame_buffer != VK_NULL_HANDLE) {
      vkDestroyFramebuffer(vk_state->device, res->frame_buffer, nullptr);
      res->frame_buffer = VK_NULL_HANDLE;
    }

    if (res->scene_color_image_view != VK_NULL_HANDLE) {
      vkDestroyImageView(vk_state->device, res->scene_color_image_view, nullptr);
      res->scene_color_image_view = VK_NULL_HANDLE;
    }
    if (res->object_id_image_view != VK_NULL_HANDLE) {
      vkDestroyImageView(vk_state->device, res->object_id_image_view, nullptr);
      res->object_id_image_view = VK_NULL_HANDLE;
    }
    if (res->scene_color_image != VK_NULL_HANDLE) {
      vmaDestroyImage(vk_state->allocator, res->scene_color_image,
                      res->alloc_scene_color_image);
      res->scene_color_image = VK_NULL_HANDLE;
      res->alloc_scene_color_image = VK_NULL_HANDLE;
    }
    if (res->object_id_image != VK_NULL_HANDLE) {
      vmaDestroyImage(vk_state->allocator, res->object_id_image,
                      res->alloc_object_id_image);
      res->object_id_image = VK_NULL_HANDLE;
      res->alloc_object_id_image = VK_NULL_HANDLE;
    }

    if (res->image_view != VK_NULL_HANDLE) {
      vkDestroyImageView(vk_state->device, res->image_view, nullptr);
      res->image_view = VK_NULL_HANDLE;
    }

    if (res->msaa_image_view != VK_NULL_HANDLE) {
      vkDestroyImageView(vk_state->device, res->msaa_image_view, nullptr);
      res->msaa_image_view = VK_NULL_HANDLE;
    }

    if (res->msaa_image != VK_NULL_HANDLE) {
      vmaDestroyImage(vk_state->allocator, res->msaa_image, res->alloc_msaa_image);
      res->msaa_image = VK_NULL_HANDLE;
      res->alloc_msaa_image = VK_NULL_HANDLE;
    }

    if (res->msaa_object_id_image_view != VK_NULL_HANDLE) {
      vkDestroyImageView(vk_state->device, res->msaa_object_id_image_view, nullptr);
      res->msaa_object_id_image_view = VK_NULL_HANDLE;
    }

    if (res->msaa_object_id_image != VK_NULL_HANDLE) {
      vmaDestroyImage(vk_state->allocator, res->msaa_object_id_image, res->alloc_msaa_object_id_image);
      res->msaa_object_id_image = VK_NULL_HANDLE;
      res->alloc_msaa_object_id_image = VK_NULL_HANDLE;
    }

    if (res->submit_semaphore != VK_NULL_HANDLE) {
      vkDestroySemaphore(vk_state->device, res->submit_semaphore, nullptr);
      res->submit_semaphore = VK_NULL_HANDLE;
    }
  }
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

bool CreateAttachmentImage(SceneRes* scene_res, VkFormat format,
                           VkImageUsageFlags usage,
                           VkImageAspectFlags aspect_mask, VkImage& image,
                           VmaAllocation* alloc, VkImageView& view) {
  VulkanGlobalState* vk_state = scene_res->vk_state;
  VkDevice device = vk_state->device;

  VkMemoryRequirements mem_req;
  VkResult r;

  VkImageCreateInfo ci_image{
      .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
      .imageType = VK_IMAGE_TYPE_2D,
      .format = format,
      .extent = VkExtent3D{.width = scene_res->image_extent.width,
                            .height = scene_res->image_extent.height,
                            .depth = 1},
      .mipLevels = 1,
      .arrayLayers = 1,
      .samples = VK_SAMPLE_COUNT_1_BIT,
      .tiling = VK_IMAGE_TILING_OPTIMAL,
      .usage = usage,
  };
  r = vkCreateImage(device, &ci_image, nullptr, &image);
  if (r != VK_SUCCESS) {
    BOOST_LOG_TRIVIAL(error)
        << std::format("Failed to create attachment image: {}", VkResultToString(r));
    return false;
  }
  vkGetImageMemoryRequirements(device, image, &mem_req);

  VkMemoryAllocateInfo mem_info = {
      .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
      .allocationSize = Aligned(mem_req.size, mem_req.alignment)
  };
  VmaAllocationCreateInfo ci_alloc{
      .requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
  };
  r = vmaAllocateMemoryForImage(vk_state->allocator, image, &ci_alloc, alloc, nullptr);
  if (r != VK_SUCCESS) {
    BOOST_LOG_TRIVIAL(error) << std::format("Failed to allocate attachment image memory: {}",
                                            VkResultToString(r));
    vkDestroyImage(device, image, nullptr);
    return false;
  }

  r = vmaBindImageMemory(vk_state->allocator, *alloc, image);
  if (r != VK_SUCCESS) {
    BOOST_LOG_TRIVIAL(error) << std::format("Failed to bind attachment image memory: {}",
                                            VkResultToString(r));
    if (*alloc != VK_NULL_HANDLE) {
      vmaDestroyImage(vk_state->allocator, image, *alloc);
      *alloc = VK_NULL_HANDLE;
    } else {
      vkDestroyImage(device, image, nullptr);
    }
    return false;
  }

  VkImageViewCreateInfo ci_image_view = {
      .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
      .image = image,
      .viewType = VK_IMAGE_VIEW_TYPE_2D,
      .format = format,
      .components = {
          .r = VK_COMPONENT_SWIZZLE_R,
          .g = VK_COMPONENT_SWIZZLE_G,
          .b = VK_COMPONENT_SWIZZLE_B,
          .a = VK_COMPONENT_SWIZZLE_A
      },
      .subresourceRange = {
          .aspectMask = aspect_mask,
          .levelCount = 1,
          .layerCount = 1
      }
  };
  r = vkCreateImageView(device, &ci_image_view, nullptr, &view);
  if (r != VK_SUCCESS) {
    BOOST_LOG_TRIVIAL(error) <<
        std::format("Failed to create attachment image view: {}", VkResultToString(r));
    if (*alloc != VK_NULL_HANDLE) {
      vmaDestroyImage(vk_state->allocator, image, *alloc);
      *alloc = VK_NULL_HANDLE;
    } else {
      vkDestroyImage(device, image, nullptr);
    }
    return false;
  }

  return true;
}

bool CreateTransientImage(SceneRes* scene_res, VkFormat format,
                          VkImageUsageFlags usage,
                          VkImageAspectFlags aspect_mask, VkImage& image,
                          VmaAllocation* alloc, VkImageView& view) {
  VulkanGlobalState* vk_state = scene_res->vk_state;
  VkDevice device = vk_state->device;

  VkMemoryRequirements mem_req;
  VkResult r;

  VkImageCreateInfo ci_image{
      .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
      .imageType = VK_IMAGE_TYPE_2D,
      .format = format,
      .extent = VkExtent3D{.width = scene_res->image_extent.width,
                            .height = scene_res->image_extent.height,
                            .depth = 1},
      .mipLevels = 1,
      .arrayLayers = 1,
      .samples = vk_state->sample_count,
      .tiling = VK_IMAGE_TILING_OPTIMAL,
      .usage = usage | VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT,
  };
  r = vkCreateImage(device, &ci_image, nullptr, &image);
  if (r != VK_SUCCESS) {
    BOOST_LOG_TRIVIAL(error)
        << std::format("Failed to create image: {}", VkResultToString(r));
    return false;
  }
  vkGetImageMemoryRequirements(device, image, &mem_req);

  VkMemoryAllocateInfo mem_info = {
      .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
      .allocationSize = Aligned(mem_req.size, mem_req.alignment)
  };

  VmaAllocationCreateInfo ci_alloc {
      .preferredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT |
                        VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT,
  };
  r = vmaAllocateMemoryForImage(vk_state->allocator, image, &ci_alloc, alloc, nullptr);
  if (r != VK_SUCCESS) {
    BOOST_LOG_TRIVIAL(error) << std::format("Failed to allocate image memory: {}",
                                            VkResultToString(r));
    vkDestroyImage(device, image, nullptr);
    return false;
  }

  r = vmaBindImageMemory(vk_state->allocator, *alloc, image);
  if (r != VK_SUCCESS) {
    BOOST_LOG_TRIVIAL(error) << std::format("Failed to bind image memory: {}",
                                            VkResultToString(r));
    if (*alloc != VK_NULL_HANDLE) {
      vmaDestroyImage(vk_state->allocator, image, *alloc);
      *alloc = VK_NULL_HANDLE;
    } else {
      vkDestroyImage(device, image, nullptr);
    }
    return false;
  }

  VkImageViewCreateInfo ci_image_view = {
      .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
      .image = image,
      .viewType = VK_IMAGE_VIEW_TYPE_2D,
      .format = format,
      .components = {
          .r = VK_COMPONENT_SWIZZLE_R,
          .g = VK_COMPONENT_SWIZZLE_G,
          .b = VK_COMPONENT_SWIZZLE_B,
          .a = VK_COMPONENT_SWIZZLE_A
      },
      .subresourceRange = {
          .aspectMask = aspect_mask,
          .levelCount = 1,
          .layerCount = 1
      }
  };
  r = vkCreateImageView(device, &ci_image_view, nullptr, &view);
  if (r != VK_SUCCESS) {
    BOOST_LOG_TRIVIAL(error) <<
        std::format("Failed to create image view: {}", VkResultToString(r));
    if (*alloc != VK_NULL_HANDLE) {
      vmaDestroyImage(vk_state->allocator, image, *alloc);
      *alloc = VK_NULL_HANDLE;
    } else {
      vkDestroyImage(device, image, nullptr);
    }
    return false;
  }

  return true;
}

bool SceneRes::InitImages() {
  BOOST_LOG_TRIVIAL(trace) << "Create image resources ...";

  VkResult r;

  std::vector<VkImage> swapchainImages(image_count);
  r = vkGetSwapchainImagesKHR(vk_state->device, swapchain, &image_count,
                              swapchainImages.data());
  if (r != VK_SUCCESS) return false;
  if (image_count > MAX_NUM_IMAGES) return false;

  bool msaa = (vk_state->sample_count != VK_SAMPLE_COUNT_1_BIT);

  for (size_t i = 0; i < image_count; ++i) {
    ImageRes* image_res = &image_res_array[i];
    image_res->image = swapchainImages[i];
    if (msaa) {
      CreateTransientImage(
          this, vk_state->image_format, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
          VK_IMAGE_ASPECT_COLOR_BIT, image_res->msaa_image,
          &image_res->alloc_msaa_image, image_res->msaa_image_view);
      CreateTransientImage(
          this, VK_FORMAT_R32_UINT, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
          VK_IMAGE_ASPECT_COLOR_BIT, image_res->msaa_object_id_image,
          &image_res->alloc_msaa_object_id_image, image_res->msaa_object_id_image_view);
    }

    VkImageViewCreateInfo ci_imageView{
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .image = image_res->image,
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .format = vk_state->image_format,
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
    r = vkCreateImageView(vk_state->device, &ci_imageView, nullptr,
                          &image_res->image_view);
    assert(r == VK_SUCCESS);

    CreateAttachmentImage(this, vk_state->image_format,
                          VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
                              VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT,
                          VK_IMAGE_ASPECT_COLOR_BIT,
                          image_res->scene_color_image,
                          &image_res->alloc_scene_color_image,
                          image_res->scene_color_image_view);
    CreateAttachmentImage(this, VK_FORMAT_R32_UINT,
                          VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
                              VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT |
                              VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
                          VK_IMAGE_ASPECT_COLOR_BIT,
                          image_res->object_id_image,
                          &image_res->alloc_object_id_image,
                          image_res->object_id_image_view);

    std::vector<VkImageView> attachments;
    attachments.push_back(image_res->image_view);
    attachments.push_back(depth_stencil_view);
    attachments.push_back(image_res->scene_color_image_view);
    attachments.push_back(image_res->object_id_image_view);
    if (msaa) {
      attachments.push_back(image_res->msaa_image_view);
      attachments.push_back(image_res->msaa_object_id_image_view);
    }

    VkFramebufferCreateInfo ci_framebuffer{
        .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
        .renderPass = vk_state->graphics_renderpass,
        .attachmentCount = static_cast<uint32_t>(attachments.size()),
        .pAttachments = attachments.data(),
        .width = image_extent.width,
        .height = image_extent.height,
        .layers = 1};
    r = vkCreateFramebuffer(vk_state->device, &ci_framebuffer, nullptr,
                            &image_res->frame_buffer);
    assert(r == VK_SUCCESS);

    // Create a separate framebuffer for compositing with only the color attachment.
    // This framebuffer is created with composite_renderpass for compatibility.
    VkImageView composite_attachment = image_res->image_view;
    VkFramebufferCreateInfo ci_composite_framebuffer{
        .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
        .renderPass = vk_state->composite_renderpass,
        .attachmentCount = 1,
        .pAttachments = &composite_attachment,
        .width = image_extent.width,
        .height = image_extent.height,
        .layers = 1
    };
    r = vkCreateFramebuffer(vk_state->device, &ci_composite_framebuffer, nullptr,
                            &image_res->composite_frame_buffer);
    assert(r == VK_SUCCESS);

    VkSemaphoreCreateInfo semaphoreCreateInfo{
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
    };
    r = vkCreateSemaphore(vk_state->device, &semaphoreCreateInfo, nullptr,
                          &image_res->submit_semaphore);
    assert(r == VK_SUCCESS);
  }

  return true;
}

void Render(Scene* scene, SceneRes* scene_res,
            ImageRes& image_res, FrameRes& frame,
            const std::map<const FgObject*, TextureRes*>& fg_res_array) {
  VulkanGlobalState* state = scene_res->vk_state;

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

  bool msaa = (state->sample_count != VK_SAMPLE_COUNT_1_BIT);
  // Attachment layout (MSAA):  [0]=swapchain, [1]=depth, [2]=scene_color, [3]=object_id, [4]=msaa_color, [5]=msaa_object_id
  // Attachment layout (no MSAA): [0]=swapchain, [1]=depth, [2]=scene_color, [3]=object_id
  size_t clear_value_count = msaa ? 6 : 4;
  std::vector<VkClearValue> clearValues(clear_value_count);
  clearValues[0].color = lavender;
  clearValues[1].depthStencil = {1.0f, 0};
  clearValues[2].color = lavender;
  clearValues[3].color = {.float32 = {0.0f, 0.0f, 0.0f, 0.0f}};
  if (msaa) {
    clearValues[4].color = lavender;
    clearValues[5].color = {.float32 = {0.0f, 0.0f, 0.0f, 0.0f}};
  }
  VkRenderPassBeginInfo bi_renderpass{
      .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
      .renderPass = state->graphics_renderpass,
      .framebuffer = image_res.frame_buffer,
      .renderArea = {.offset = {0, 0}, .extent = scene_res->image_extent},
      .clearValueCount = static_cast<uint32_t>(clearValues.size()),
      .pClearValues = clearValues.data()};
  vkCmdBeginRenderPass(frame.cmd_buf, &bi_renderpass,
                       VK_SUBPASS_CONTENTS_INLINE);

  const VkViewport viewport = {
      .x = 0,
      .y = 0,
      .width = static_cast<float>(scene_res->image_extent.width),
      .height = static_cast<float>(scene_res->image_extent.height),
      .minDepth = 0,
      .maxDepth = 1,
  };
  const VkRect2D scissor = {
      .offset = {0, 0},
      .extent = scene_res->image_extent,
  };

  // Дальнейшие команды будут вызваны, только если сцена не пуста.
  if (scene_res->buffer_vertex) {
    vkCmdBindPipeline(frame.cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS,
                      state->graphics_pipeline);

    vkCmdBindDescriptorSets(frame.cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS,
                            state->graphics_pipeline_layout, 0, 1,
                            &frame.graphics_desc_set, 0, nullptr);

    // Bind vertex buffers.
    std::vector<VkBuffer> buffers = {
        scene_res->buffer_vertex,
        scene_res->buffer_normal,
        scene_res->buffer_color,
        scene_res->buffer_object_id,
    };
    VkDeviceSize offsets[] = {0, 0, 0, 0};
    vkCmdBindVertexBuffers(frame.cmd_buf, 0, buffers.size(), buffers.data(), offsets);

    // Bind index buffers.
    vkCmdBindIndexBuffer(frame.cmd_buf, scene_res->buffer_index, 0,
                         VK_INDEX_TYPE_UINT32);

    vkCmdSetViewport(frame.cmd_buf, 0, 1, &viewport);
    vkCmdSetScissor(frame.cmd_buf, 0, 1, &scissor);

    vkCmdDrawIndexed(frame.cmd_buf, scene_res->index_count, 1, 0, 0, 0);
  }

  vkCmdNextSubpass(frame.cmd_buf, VK_SUBPASS_CONTENTS_INLINE);

  VkDescriptorImageInfo scene_color_info{
      .imageView = image_res.scene_color_image_view,
      .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
  };
  VkDescriptorImageInfo object_id_info{
      .imageView = image_res.object_id_image_view,
      .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
  };
  VkDescriptorBufferInfo selection_buffer_info{
      .buffer = scene_res->buffer_selection,
      .offset = 0,
      .range = VK_WHOLE_SIZE,
  };
  std::array<VkWriteDescriptorSet, 3> selection_desc_writes = {{
      VkWriteDescriptorSet{
          .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
          .dstSet = frame.selection_desc_set,
          .dstBinding = 0,
          .descriptorCount = 1,
          .descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT,
          .pImageInfo = &scene_color_info,
      },
      VkWriteDescriptorSet{
          .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
          .dstSet = frame.selection_desc_set,
          .dstBinding = 1,
          .descriptorCount = 1,
          .descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT,
          .pImageInfo = &object_id_info,
      },
      VkWriteDescriptorSet{
          .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
          .dstSet = frame.selection_desc_set,
          .dstBinding = 2,
          .descriptorCount = 1,
          .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
          .pBufferInfo = &selection_buffer_info,
      },
  }};
  vkUpdateDescriptorSets(state->device, static_cast<uint32_t>(selection_desc_writes.size()),
                         selection_desc_writes.data(), 0, nullptr);

  vkCmdBindPipeline(frame.cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS,
                    state->selection_pipeline);
  vkCmdBindDescriptorSets(frame.cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS,
                          state->selection_pipeline_layout, 0, 1,
                          &frame.selection_desc_set, 0, nullptr);
  vkCmdSetViewport(frame.cmd_buf, 0, 1, &viewport);
  vkCmdSetScissor(frame.cmd_buf, 0, 1, &scissor);
  vkCmdDraw(frame.cmd_buf, 3, 1, 0, 0);

  vkCmdEndRenderPass(frame.cmd_buf);

  // --- Composite foreground images onto the swapchain image ---
  if (scene->HasFgObjects()) {
    // Transition swapchain image from PRESENT_SRC_KHR to COLOR_ATTACHMENT_OPTIMAL.
    VkImageMemoryBarrier composite_barrier {
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .srcAccessMask = 0,
        .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
        .oldLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
        .newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = image_res.image,
        .subresourceRange = {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1,
        },
    };
    vkCmdPipelineBarrier(frame.cmd_buf,
                         VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                         VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                         0, 0, nullptr, 0, nullptr, 1, &composite_barrier);

    // Begin composite render pass.
    VkRenderPassBeginInfo bi_composite {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .renderPass = state->composite_renderpass,
        .framebuffer = image_res.composite_frame_buffer,
        .renderArea = {
            .offset = {0, 0},
            .extent = scene_res->image_extent
        },
        .clearValueCount = 0,
        .pClearValues = nullptr,
    };
    vkCmdBeginRenderPass(frame.cmd_buf, &bi_composite,
                         VK_SUBPASS_CONTENTS_INLINE);

    // Set viewport and scissor.
    VkViewport composite_viewport{
        .x = 0,
        .y = 0,
        .width = static_cast<float>(scene_res->image_extent.width),
        .height = static_cast<float>(scene_res->image_extent.height),
        .minDepth = 0,
        .maxDepth = 1,
    };
    vkCmdSetViewport(frame.cmd_buf, 0, 1, &composite_viewport);

    VkRect2D composite_scissor{
        .offset = {0, 0},
        .extent = scene_res->image_extent,
    };
    vkCmdSetScissor(frame.cmd_buf, 0, 1, &composite_scissor);

    // Bind the composite pipeline and descriptor set.
    vkCmdBindPipeline(frame.cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS,
                      state->composite_pipeline);

    // Draw each foreground image.
    for (auto* fg : scene->GetFgObjects()) {
      TextureRes* fg_res = fg_res_array.at(fg);

      if (fg_res->desc_set == VK_NULL_HANDLE)
        continue;

      glm::ivec2 screen_pos = scene->GetScreenPosition(fg);

      vkCmdBindDescriptorSets(frame.cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS,
                              state->composite_pipeline_layout, 0, 1,
                              &fg_res->desc_set, 0, nullptr);

      struct PushConstants {
        glm::vec2 position;
        glm::vec2 size;
        glm::vec2 screenSize;
      };
      PushConstants pc {
          .position = glm::vec2(screen_pos),
          .size = glm::vec2(static_cast<float>(fg->GetWidth()),
                            static_cast<float>(fg->GetHeight())),
          .screenSize = glm::vec2(
              static_cast<float>(scene_res->image_extent.width),
              static_cast<float>(scene_res->image_extent.height)),
      };
      vkCmdPushConstants(frame.cmd_buf, state->composite_pipeline_layout,
                         VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(pc), &pc);

      // Draw 6 vertices (two triangles forming a quad).
      vkCmdDraw(frame.cmd_buf, 6, 1, 0, 0);
    }

    vkCmdEndRenderPass(frame.cmd_buf);
  }

  vkEndCommandBuffer(frame.cmd_buf);

  VkPipelineStageFlags waitDstStageMask[] = {
      VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
  };
  VkSubmitInfo submitInfo {
      .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
      .waitSemaphoreCount = static_cast<uint32_t>(1),
      .pWaitSemaphores = &frame.acquire_semaphore,
      .pWaitDstStageMask = waitDstStageMask,
      .commandBufferCount = 1,
      .pCommandBuffers = &frame.cmd_buf,
      .signalSemaphoreCount = static_cast<uint32_t>(1),
      .pSignalSemaphores = &image_res.submit_semaphore
  };
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

bool InitInstance(VulkanGlobalState* state) {
  if (state->instance) return true;

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

bool SelectPhysicalDevice(VulkanGlobalState* state, VkSurfaceKHR surface) {
  if (state->physical_device) return true;

  std::vector<const char*> extensionNames{
      VK_KHR_SWAPCHAIN_EXTENSION_NAME,
  };

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

bool CreateLogicalDevice(VulkanGlobalState* state, VkSurfaceKHR surface) {
  if (state->device) return true;

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
bool CreateSurface(VulkanGlobalState* state, VkSceneRenderer::Impl::Xcb* xcb) {
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

bool ChooseSwapchainSettings(VulkanGlobalState* state, VkSurfaceKHR surface) {
  SwapChainSupportDetails swapchain_support =
      QuerySwapChainSupport(state->physical_device, surface);

  if (state->image_format == VK_FORMAT_UNDEFINED) {
    VkFormat format = VK_FORMAT_UNDEFINED;
    VkColorSpaceKHR color_space = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
    for (const auto& f : swapchain_support.formats) {
      switch (f.format) {
        case VK_FORMAT_R8G8B8A8_SRGB:
        case VK_FORMAT_B8G8R8A8_SRGB:
        case VK_FORMAT_R8G8B8A8_UNORM:
        case VK_FORMAT_B8G8R8A8_UNORM:
          format = f.format;
          color_space = f.colorSpace;
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
    state->color_space = color_space;
    if (state->image_format == VK_FORMAT_UNDEFINED)
      BOOST_LOG_TRIVIAL(trace) << "Image format undefined.";
      return false;
  }

  if (state->depth_stencil_format == VK_FORMAT_UNDEFINED) {
    state->depth_stencil_format = SelectDepthStencilFormat(state->physical_device);
    if (state->depth_stencil_format == VK_FORMAT_UNDEFINED) {
      BOOST_LOG_TRIVIAL(error) << "Failed to find an optimal depth-stencil format";
      return false;
    }
  }

  // Выбираем режим показа.
  if (state->present_mode != VK_PRESENT_MODE_MAILBOX_KHR ) {
    for (auto pm : swapchain_support.present_modes) {
      if (pm == VK_PRESENT_MODE_MAILBOX_KHR) {
        state->present_mode = VK_PRESENT_MODE_MAILBOX_KHR;
        break;
      }
    }
  }

  if (state->min_image_count == 0) {
    state->min_image_count = 2;
    if (state->min_image_count < swapchain_support.capabilities.minImageCount) {
      if (swapchain_support.capabilities.minImageCount > MAX_NUM_IMAGES) {
        BOOST_LOG_TRIVIAL(error) << std::format(
            "VkSurfaceCapabilitiesKHR.minImageCount is too large (is: {}, max: "
            "{})",
            swapchain_support.capabilities.minImageCount, MAX_NUM_IMAGES);
        return false;
      }
      state->min_image_count = swapchain_support.capabilities.minImageCount;
    }
    if (swapchain_support.capabilities.maxImageCount > 0 &&
        state->min_image_count > swapchain_support.capabilities.maxImageCount) {
      state->min_image_count = swapchain_support.capabilities.maxImageCount;
    }
  }

  return true;
}

bool TextureRes::Initialize(VulkanGlobalState* state,
                            const FgObject* fg_object) {
  this->Release(state);

  VkResult r = VK_SUCCESS;

  const PixelBuffer& pixel_buffer = fg_object->GetPixelBuffer();

  // Create staging buffer
  VkBuffer staging_buffer = VK_NULL_HANDLE;
  VmaAllocation staging_allocation = VK_NULL_HANDLE;
  VkBufferCreateInfo bufferInfo = {
      .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
      .size = static_cast<VkDeviceSize>(pixel_buffer.pixels.size()),
      .usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
  };
  VmaAllocationCreateInfo alloc_ci = {
      .flags = VMA_ALLOCATION_CREATE_MAPPED_BIT,
      .usage = VMA_MEMORY_USAGE_CPU_TO_GPU,
  };
  r = vmaCreateBuffer(state->allocator, &bufferInfo, &alloc_ci,
                      &staging_buffer, &staging_allocation, nullptr);
  if (r != VK_SUCCESS) {
    BOOST_LOG_TRIVIAL(error) << "Failed to create staging buffer for texture: "
                             << VkResultToString(r);
    return false;
  }

  // Copy pixel data
  void* mapped_data = nullptr;
  vmaMapMemory(state->allocator, staging_allocation, &mapped_data);
  memcpy(mapped_data, pixel_buffer.pixels.data(), pixel_buffer.pixels.size());
  vmaUnmapMemory(state->allocator, staging_allocation);

  // Create image
  VkImageCreateInfo image_ci = {
      .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
      .imageType = VK_IMAGE_TYPE_2D,
      .format = VK_FORMAT_R8G8B8A8_UNORM,
      .extent = { pixel_buffer.width, pixel_buffer.height, 1 },
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
                     &image, &alloc_image,
                     nullptr);
  if (r != VK_SUCCESS) {
    BOOST_LOG_TRIVIAL(error) << "Failed to create texture image: "
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
    BOOST_LOG_TRIVIAL(error) << "Failed to create command pool for texture: "
                             << VkResultToString(r);
    vmaDestroyImage(state->allocator, image, alloc_image);
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
    BOOST_LOG_TRIVIAL(error) << "Failed to allocate command buffer for texture: "
                             << VkResultToString(r);
    vkDestroyCommandPool(state->device, cmd_pool, nullptr);
    vmaDestroyImage(state->allocator, image, alloc_image);
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
      .image = image,
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
      .imageExtent = { pixel_buffer.width, pixel_buffer.height, 1 },
  };
  vkCmdCopyBufferToImage(cmd_buf, staging_buffer, image,
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
    BOOST_LOG_TRIVIAL(error) << "Failed to submit texture copy commands: "
                             << VkResultToString(r);
    vkFreeCommandBuffers(state->device, cmd_pool, 1, &cmd_buf);
    vkDestroyCommandPool(state->device, cmd_pool, nullptr);
    vmaDestroyImage(state->allocator, image, alloc_image);
    vmaDestroyBuffer(state->allocator, staging_buffer, staging_allocation);
    return false;
  }

  // Wait for queue to finish
  r = vkQueueWaitIdle(state->queue);
  if (r != VK_SUCCESS) {
    BOOST_LOG_TRIVIAL(error) << "Failed to wait for texture copy: "
                             << VkResultToString(r);
    vkFreeCommandBuffers(state->device, cmd_pool, 1, &cmd_buf);
    vkDestroyCommandPool(state->device, cmd_pool, nullptr);
    vmaDestroyImage(state->allocator, image, alloc_image);
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
      .image = image,
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
  r = vkCreateImageView(state->device, &view_info, nullptr, &image_view);
  if (r != VK_SUCCESS) {
    BOOST_LOG_TRIVIAL(error) << "Failed to create texture image view: "
                             << VkResultToString(r);
    vmaDestroyImage(state->allocator, image, alloc_image);
    return false;
  }

  BOOST_LOG_TRIVIAL(trace) << "Texture resources created successfully";
  return true;
}

bool VulkanObjects::RestoreInvalidated(VkSurfaceKHR surface) {
  if (!InitInstance(&global_state)) return false;

  // Аргумент `surface` необходим для выбора физического устройства и семейства
  // очередей. И аргумент может быть недоступен в момент инициализации
  // глобального состояния Vulkan. И при повторном вызове функции surface может
  // прийти.
  if (!global_state.physical_device) {
    if(!surface) return true;
  } else {
    if(surface) {
      //\warning Не понятна логика передачи `surface`,
      // если физическое устройство выбрано.
      //assert(false);
      //return false;
    }
  }

  if (!SelectPhysicalDevice(&global_state,surface)) return false;
  if (!CreateLogicalDevice(&global_state,surface)) return false;
  if (!ChooseSwapchainSettings(&global_state,surface)) return false;
  if (!ChooseSampleCountOfMSAA(&global_state)) return false;
  if (!CreateGraphicsRenderPass(&global_state)) return false;
  if (!CreateGraphicsPipeline(&global_state)) return false;
  if (!CreateSelectionPipeline(&global_state)) return false;
  if (!CreateCompositeRenderPass(&global_state)) return false;
  if (!CreateCompositePipeline(&global_state)) return false;

  if (!global_state.cmd_pool) {
    VkCommandPoolCreateInfo ci_commandpool{
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        .queueFamilyIndex = 0,  //< \warning
    };
    vkCreateCommandPool(global_state.device, &ci_commandpool, nullptr,
                        &global_state.cmd_pool);
  }

  // Create sampler
  if (!global_state.composite_sampler) {
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
    VkResult r = VK_SUCCESS;
    r = vkCreateSampler(global_state.device, &sampler_ci, nullptr,
                        &global_state.composite_sampler);
    assert(r == VK_SUCCESS);
  }

  if (!global_state.textures_desc_pool) {
    global_state.textures_desc_pool = LruPool::Make<LruDescriptorSetPool>(
        global_state.device, global_state.composite_desc_layout, 5);
  }

  return true;
}

SceneRes::SceneRes(VulkanGlobalState* state, VkSurfaceKHR srf, Scene* scene) {
  vk_state = state;
  is_external_surface = true;
  surface = srf;
  RestoreInvalidated();
}

bool SceneRes::RestoreInvalidated() {
  VkResult r;
  VkSurfaceCapabilitiesKHR surface_caps;

  // Выбираем размер изображений.
  if (image_extent.width == static_cast<uint32_t>(-1)) {
    r = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(vk_state->physical_device,
                                                  surface, &surface_caps);
    if (r != VK_SUCCESS)
      return false;
    image_extent = surface_caps.currentExtent;
  }

  if (image_extent.width == static_cast<uint32_t>(-1)
      || image_extent.height == static_cast<uint32_t>(-1))
    return false;

  // Защита от вырождения окна в нулевой размер.
  if (image_extent.width == 0 || image_extent.height == 0) {
    BOOST_LOG_TRIVIAL(debug) << "Degenerated Window";
    return false;
  }

  r = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(vk_state->physical_device,
                                                surface, &surface_caps);
  if (r != VK_SUCCESS)
    return false;
  VkSurfaceTransformFlagBitsKHR preTransform =
      (surface_caps.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR)
          ? VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR
          : surface_caps.currentTransform;

  assert(vk_state->graphics_family_index == vk_state->present_family_index);
  std::array<uint32_t, 1> queueFamilyIndices = {
      vk_state->graphics_family_index.value()
  };

  if (!swapchain) {
    VkSwapchainCreateInfoKHR ci_swapchain{
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .surface = surface,
        .minImageCount = vk_state->min_image_count,
        .imageFormat = vk_state->image_format,
        .imageColorSpace = vk_state->color_space,
        .imageExtent = image_extent,
        .imageArrayLayers = 1,
        .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        .imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount = static_cast<uint32_t>(queueFamilyIndices.size()),
        .pQueueFamilyIndices = queueFamilyIndices.data(),
        .preTransform = preTransform,
        .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        .presentMode = vk_state->present_mode,
        .oldSwapchain = old_swapchain
    };
    r = vkCreateSwapchainKHR(vk_state->device, &ci_swapchain, nullptr, &swapchain);
    if (r != VK_SUCCESS) {
      BOOST_LOG_TRIVIAL(trace) << std::format(
          "Swapchain object creation error ({}).", VkResultToString(r));
      return false;
    }
    BOOST_LOG_TRIVIAL(trace) << std::format(
        "Success of create swapchain object ({}).",
        static_cast<void*>(swapchain));

    if (swapchain && old_swapchain) {
      vkDestroySwapchainKHR(vk_state->device, old_swapchain, nullptr);
      old_swapchain = VK_NULL_HANDLE;
    }

    r = vkGetSwapchainImagesKHR(vk_state->device, swapchain,
                                &image_count, nullptr);
    if (r != VK_SUCCESS || image_count == 0) return false;
    BOOST_LOG_TRIVIAL(trace) << std::format("Count of images in swapchain is {}.",
                                            image_count);

    // Создаем изображение буфера глубины.
    CreateTransientImage(this, vk_state->depth_stencil_format,
                         VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
                         VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT,
                         depth_stencil_image, &alloc_depth_stencil,
                         depth_stencil_view);

    InitImages();
  }

  // Память под данные фреймов.
  if (!buffer_frame) {
    assert(!alloc_frame);
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
                    &buffer_frame, &alloc_frame,
                    nullptr);
  }

  if (!descriptor_pool) {
    // Создаем объект пула дескрипторов.
    std::array<VkDescriptorPoolSize, 3> descPoolSizes = {
        VkDescriptorPoolSize{
          .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
          .descriptorCount = vk_state->frame_count * 3},
        VkDescriptorPoolSize{
          .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
          .descriptorCount = vk_state->frame_count * 2},
        VkDescriptorPoolSize{
          .type = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT,
          .descriptorCount = vk_state->frame_count * 2},
    };
    VkDescriptorPoolCreateInfo ci_descpool {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT,
        .maxSets = vk_state->frame_count * 2,
        .poolSizeCount = static_cast<uint32_t>(descPoolSizes.size()),
        .pPoolSizes = descPoolSizes.data()
    };
    r = vkCreateDescriptorPool(vk_state->device, &ci_descpool, nullptr,
                               &descriptor_pool);
    assert(r == VK_SUCCESS);
  }

  return InitFrames();
}

void SceneRes::InvalidateSwapchain() {
  old_swapchain = std::exchange(swapchain, VK_NULL_HANDLE);
  ReleaseImages();
  if (depth_stencil_view) {
    vkDestroyImageView(vk_state->device, depth_stencil_view, nullptr);
    depth_stencil_view = VK_NULL_HANDLE;
  }
  if (depth_stencil_image) {
    vmaDestroyImage(vk_state->allocator, depth_stencil_image, alloc_depth_stencil);
    depth_stencil_image = VK_NULL_HANDLE;
    alloc_depth_stencil = VK_NULL_HANDLE;
  }
  image_extent = {
    .width = static_cast<uint32_t>(-1),
    .height = static_cast<uint32_t>(-1)
  };

  // The acquire semaphore may still be signaled from a previous
  // vkAcquireNextImageKHR call that was never consumed by vkQueueSubmit
  // (e.g. after VK_SUBOPTIMAL_KHR / VK_ERROR_OUT_OF_DATE_KHR).
  // Destroy and recreate it to clear the signaled state.
  // See https://docs.vulkan.org/guide/latest/swapchain_semaphore_reuse.html
  for (uint32_t i = 0; i < vk_state->frame_count; ++i) {
    FrameRes& frame = frame_res_array[i];
    if (frame.acquire_semaphore != VK_NULL_HANDLE) {
      vkDestroySemaphore(vk_state->device, frame.acquire_semaphore, nullptr);
      frame.acquire_semaphore = VK_NULL_HANDLE;
    }
  }
}

} // namespace

VkInstance VkSceneRenderer::GetInstance() const {
  return impl_->vk_objects.global_state.instance;
}

namespace {
void UpdateScene(const Scene* scene, SceneRes* scene_res) {
  VulkanGlobalState* vk_global_state = scene_res->vk_state;
  auto allocator = vk_global_state->allocator;

  if (scene_res->buffer_vertex != VK_NULL_HANDLE) {
    vmaDestroyBuffer(allocator,scene_res->buffer_vertex,scene_res->alloc_vertex);
    vmaDestroyBuffer(allocator,scene_res->buffer_normal,scene_res->alloc_normal);
    vmaDestroyBuffer(allocator,scene_res->buffer_color,scene_res->alloc_color);
    vmaDestroyBuffer(allocator,scene_res->buffer_object_id,scene_res->alloc_object_id);
    vmaDestroyBuffer(allocator,scene_res->buffer_index,scene_res->alloc_index);
    scene_res->buffer_vertex = VK_NULL_HANDLE;
    scene_res->buffer_normal = VK_NULL_HANDLE;
    scene_res->buffer_color = VK_NULL_HANDLE;
    scene_res->buffer_object_id = VK_NULL_HANDLE;
    scene_res->buffer_index = VK_NULL_HANDLE;
  }

  size_t n_verts = 0, n_cells = 0;
  GetElementsCount(scene, n_verts, n_cells);

  VkDeviceSize vertex_buffer_size = Aligned(6 * n_verts * sizeof(float), 256);
  VkDeviceSize normal_buffer_size = Aligned(3 * n_cells * sizeof(float), 256);
  VkDeviceSize color_buffer_size = Aligned(3 * n_verts * sizeof(float), 256);
  VkDeviceSize object_id_buffer_size = Aligned(3 * n_verts * sizeof(uint32_t), 256);
  VkDeviceSize index_buffer_size = Aligned(3 * n_cells * sizeof(uint32_t), 256);

  if (vertex_buffer_size == 0)
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
    VkResult r = vmaCreateBuffer(allocator,
                                 &ci_buffer, &ci_allocation,
                                 &scene_res->buffer_vertex,
                                 &scene_res->alloc_vertex, nullptr);
    // Копируем вершины.
    void* mapped_data = nullptr;
    vmaMapMemory(allocator, scene_res->alloc_vertex, &mapped_data);
    float* data = reinterpret_cast<float*>(mapped_data);
    for (glm::vec3 pt : GetVertexIterator(scene)) {
      *data++ = pt.x;
      *data++ = pt.y;
      *data++ = pt.z;
    }
    vmaUnmapMemory(allocator, scene_res->alloc_vertex);
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
    vmaCreateBuffer(allocator,
                    &ci_buffer, &ci_allocation, &scene_res->buffer_normal,
                    &scene_res->alloc_normal, nullptr);
    // Копируем нормали.
    void* mapped_data = nullptr;
    vmaMapMemory(allocator, scene_res->alloc_normal, &mapped_data);
    float* data = reinterpret_cast<float*>(mapped_data);
    for (glm::vec3 n : GetNormalIterator(scene)) {
      *data++ = n.x;
      *data++ = n.y;
      *data++ = n.z;
    }
    vmaUnmapMemory(allocator, scene_res->alloc_normal);
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
    vmaCreateBuffer(allocator,
                    &ci_buffer, &ci_allocation, &scene_res->buffer_color,
                    &scene_res->alloc_color, nullptr);
    // Копируем цвета.
    void* mapped_data = nullptr;
    vmaMapMemory(allocator, scene_res->alloc_color, &mapped_data);
    float* data = reinterpret_cast<float*>(mapped_data);
    for (glm::vec3 c : GetColorIterator(scene)) {
      *data++ = c.x;
      *data++ = c.y;
      *data++ = c.z;
    }
    vmaUnmapMemory(allocator, scene_res->alloc_color);
  }

  {
    // Выделяем память под буфер object id.
    VkBufferCreateInfo ci_buffer {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size = object_id_buffer_size,
        .usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT
    };
    VmaAllocationCreateInfo ci_allocation {
        .usage = VMA_MEMORY_USAGE_CPU_TO_GPU
    };
    vmaCreateBuffer(allocator,
                    &ci_buffer, &ci_allocation, &scene_res->buffer_object_id,
                    &scene_res->alloc_object_id, nullptr);
    // Копируем идентификаторы объектов.
    void* mapped_data = nullptr;
    vmaMapMemory(allocator, scene_res->alloc_object_id, &mapped_data);
    uint32_t* data = reinterpret_cast<uint32_t*>(mapped_data);
    for (const SceneObject* o : scene->Objects()) {
      for (Drawable* d : o->Drawables()) {
        if (d->Type() != Drawable::PrimitiveType::Triangles) continue;
        for (size_t i = 0; i < d->GetVertsCount(); ++i) {
          *data++ = d->GetId();
        }
      }
    }
    vmaUnmapMemory(allocator, scene_res->alloc_object_id);
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
    vmaCreateBuffer(allocator, &ci_buffer, &ci_allocation,
                    &scene_res->buffer_index,
                    &scene_res->alloc_index, nullptr);

    // Копируем индексный буфер.
    void* mapped_data = nullptr;
    vmaMapMemory(allocator, scene_res->alloc_index, &mapped_data);
    uint32_t* data = reinterpret_cast<uint32_t*>(mapped_data);
    for (glm::u32vec3 tr : GetTriaIterator(scene)) {
      *data++ = tr.x;
      *data++ = tr.y;
      *data++ = tr.z;
    }
    vmaUnmapMemory(allocator, scene_res->alloc_index);
  }

  scene_res->index_count = 3 * n_cells;
}

void UpdateDescriptorSets(
    Scene* scene, SceneRes* scene_res,
    const std::map<const FgObject*,TextureRes*>& fg_res_array) {
  const VulkanGlobalState* vk_global_state = scene_res->vk_state;
  FrameRes& frame = scene_res->frame_res_array[scene_res->current_frame_index];

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
  VkDeviceSize frame_offset = scene_res->current_frame_index * frame_data_size;

  // Осуществляем прямую запись данных в разделяемую память CPU и GPU.
  void* mapped_data = nullptr;
  vmaMapMemory(vk_global_state->allocator, scene_res->alloc_frame, &mapped_data);
  std::byte* data = reinterpret_cast<std::byte*>(mapped_data) + frame_offset;
  std::memcpy(data, &vbo, sizeof(vbo));
  std::memcpy(data + vbo_size, &fbo, sizeof(fbo));
  vmaUnmapMemory(vk_global_state->allocator, scene_res->alloc_frame);

  std::vector<VkDescriptorBufferInfo> bufferInfo{
      VkDescriptorBufferInfo{
        .buffer = scene_res->buffer_frame,
        .offset = frame_offset,
        .range = sizeof(vbo)
      },
      VkDescriptorBufferInfo{
        .buffer = scene_res->buffer_frame,
        .offset = frame_offset + vbo_size,
        .range = sizeof(fbo)
      },
  };
  // Pack selection data in std140-compliant layout:
  // -- each array element is padded to 16 bytes (vec4 alignment)
  // -- followed by selected_count at offset 64*16 = 1024.
  struct SelectionDataStd140 {
    uint32_t ids[64];  // Each element occupies 16 bytes (only first 4 used)
    uint32_t padding[3 * 64];  // explicit padding to match std140 array stride
    uint32_t selected_count;
  };
  SelectionDataStd140 selection_data{};
  size_t selection_count = 0;
  for (const SceneObject* o : scene->Objects()) {
    for (Drawable* d : o->Drawables()) {
      if (d->IsSelected()) {
        if (selection_count < 64) {
          selection_data.ids[selection_count++] = d->GetId();
        }
      }
    }
  }
  selection_data.selected_count = static_cast<uint32_t>(selection_count);
  VkDeviceSize selection_buffer_size = Aligned(sizeof(SelectionDataStd140), 256);
  if (scene_res->buffer_selection != VK_NULL_HANDLE) {
    vmaDestroyBuffer(vk_global_state->allocator, scene_res->buffer_selection, scene_res->alloc_selection);
  }
  VkBufferCreateInfo ci_selection_buffer{
      .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
      .size = selection_buffer_size,
      .usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
  };
  VmaAllocationCreateInfo ci_selection_alloc{
      .usage = VMA_MEMORY_USAGE_CPU_TO_GPU,
  };
  vmaCreateBuffer(vk_global_state->allocator, &ci_selection_buffer,
                  &ci_selection_alloc, &scene_res->buffer_selection,
                  &scene_res->alloc_selection, nullptr);
  void* selection_mapped = nullptr;
  vmaMapMemory(vk_global_state->allocator, scene_res->alloc_selection,
               &selection_mapped);
  std::memcpy(selection_mapped, &selection_data, sizeof(SelectionDataStd140));
  vmaUnmapMemory(vk_global_state->allocator, scene_res->alloc_selection);
  std::vector<VkWriteDescriptorSet> descWrites{
      VkWriteDescriptorSet{
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .dstSet = frame.graphics_desc_set,
        .dstBinding = 0,
        .descriptorCount = 1,
        .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .pBufferInfo = &bufferInfo[0],
      },
      VkWriteDescriptorSet{
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .dstSet = frame.graphics_desc_set,
        .dstBinding = 1,
        .descriptorCount = 1,
        .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .pBufferInfo = &bufferInfo[1],
      },
  };

  // VkDescriptorImageInfo objects must outlive the vkUpdateDescriptorSets call.
  // Store them in a vector so the pointers in `descWrites` remain valid.
  std::list<VkDescriptorImageInfo> image_infos;
  if (scene->HasFgObjects()) {
    for (FgObject* fg : scene->GetFgObjects()) {
      TextureRes* fg_res = fg_res_array.at(fg);
      if (fg_res->desc_set == VK_NULL_HANDLE) {
        vk_global_state->textures_desc_pool->Allocate(fg_res);
        if (fg_res->desc_set == VK_NULL_HANDLE)
          continue;
      }
      image_infos.push_back(VkDescriptorImageInfo{
          .sampler = vk_global_state->composite_sampler,
          .imageView = fg_res->image_view,
          .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
      });
      descWrites.push_back(VkWriteDescriptorSet{
          .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
          .dstSet = fg_res->desc_set,
          .dstBinding = 0,
          .descriptorCount = 1,
          .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
          .pImageInfo = &image_infos.back(),
      });
    }
  }

  vkUpdateDescriptorSets(vk_global_state->device,
                         static_cast<uint32_t>(descWrites.size()),
                         descWrites.data(), 0, nullptr);
}

bool Synchronize(Application* app, VulkanObjects* vk_objects, Scene* scene) {
  // Обновление данных -- это критическая секция.
  static std::mutex mtx;
  std::lock_guard<std::mutex> lock(mtx);

  auto vk_global_state = &vk_objects->global_state;
  // Доинициализируем глобальное состояние Vulkan,
  // потому что для этого была нужна поверхность Vulkan.
  auto surface = reinterpret_cast<VkSurfaceKHR>(scene->GetVulkanSurface());
  if (!vk_objects->RestoreInvalidated(surface))
    return false;

  // Ожидаем завершения всех заданий в очередях.
  vkQueueWaitIdle(vk_global_state->queue);

  std::list<Scene*> scenes_for_update;
  scenes_for_update.push_back(scene);
  for (Scene* fg_scene : app->GetForegroundScenes(scene))
    scenes_for_update.push_back(fg_scene);

  for (Scene* scene : scenes_for_update) {
    auto it = vk_objects->scene_res_array.find(scene);
    auto scene_res =
        (it != vk_objects->scene_res_array.end() ? it->second : nullptr);
    switch (scene->GetState()) {
    case TrackedObject::State::Clean:
      scene_res->RestoreInvalidated();
      break;
    case TrackedObject::State::New:
      assert(scene_res == nullptr);
      scene_res = new SceneRes(vk_global_state, surface, scene);
      vk_objects->scene_res_array.insert(std::make_pair(scene,scene_res));
      // Проваливаемся в следующую ветку `case` осознанно!
    case TrackedObject::State::Dirty:
      assert(scene_res != nullptr);
      scene_res->RestoreInvalidated();
      UpdateScene(scene, scene_res);
      break;
    case TrackedObject::State::Removed:
      scene_res->Release();
      vk_objects->scene_res_array.erase(it);
      delete scene_res;
      break;
    }
    scene->Sync();
  }
  if (scene->IsDeleted())
    return false;

  // Синхронизируем связку `FgObject -> TextureRes`.
  for (FgObject* fg : scene->GetFgObjects()) {
    auto it = vk_objects->fg_res_array.find(fg);
    auto fg_res = it != vk_objects->fg_res_array.end()
                          ? it->second
                          : nullptr;
    auto state = fg->GetState();
    switch (state) {
    case TrackedObject::State::New:
      assert(fg_res == nullptr);
      fg_res = new TextureRes(vk_global_state);
      fg_res->Initialize(vk_global_state, fg);
      vk_objects->fg_res_array[fg] = fg_res;
      break;
    case TrackedObject::State::Clean:
      break;
    case TrackedObject::State::Dirty:
      fg_res->Initialize(vk_global_state, fg);
      break;
    case TrackedObject::State::Delete:
      fg_res->Release(vk_global_state);
      delete fg_res;
      vk_objects->fg_res_array.erase(it);
      break;
    default:
      std::abort();
    }
    fg->Sync();
  }

  auto scene_res = vk_objects->scene_res_array.at(scene);
  UpdateDescriptorSets(scene, scene_res, vk_objects->fg_res_array);

  return true;
}
}  // namespace

bool VkSceneRenderer::Update(Scene* scene) {
  // Проверяем принадлежность сцены приложению.
  if (!scene || scene != impl_->app->GetScene(scene->GetName()))
    return false;

  assert(!scene->IsDeleted());

  if (!Synchronize(impl_->app,&impl_->vk_objects,scene))
    return false;

  SceneRes* scene_res = impl_->vk_objects.scene_res_array.at(scene);

  auto screen_size = scene->GetScreenSize();
  if (screen_size.x == 0 || screen_size.y == 0)
    return false;

  VulkanGlobalState* state = scene_res->vk_state;
  FrameRes& frame = scene_res->frame_res_array[scene_res->current_frame_index];

  // Ожидаем освобождения последнего фрейма.
  if (frame.cmd_fence_waitable) {
    vkWaitForFences(state->device, 1, &frame.cmd_fence, VK_TRUE, UINT64_MAX);
    vkResetFences(state->device, 1, &frame.cmd_fence);
    frame.cmd_fence_waitable = false;
  }

  while (true) {
    uint32_t index;
    VkResult r;
    r = vkAcquireNextImageKHR(state->device, scene_res->swapchain, UINT64_MAX,
                              frame.acquire_semaphore, VK_NULL_HANDLE, &index);
    if (r == VK_SUBOPTIMAL_KHR || r == VK_ERROR_OUT_OF_DATE_KHR) {
      scene_res->InvalidateSwapchain();
      if (!scene_res->RestoreInvalidated())
        return false;
      continue;
    }
    if (r == VK_NOT_READY || r == VK_TIMEOUT) continue;
    if (r != VK_SUCCESS) return false;

    scene_res->current_image_index = index;

    // Рендеринг основной сцены и наложение текстур переднего плана.
    assert(index <= MAX_NUM_IMAGES);
    ImageRes& image_res = scene_res->image_res_array[index];
    Render(scene, scene_res, image_res, frame, impl_->vk_objects.fg_res_array);

    // Отправка полученного изображения на презентацию.
    VkPresentInfoKHR presentInfoKHR{
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &image_res.submit_semaphore,
        .swapchainCount = 1,
        .pSwapchains = &scene_res->swapchain,
        .pImageIndices = &index,
    };
    r = vkQueuePresentKHR(state->queue, &presentInfoKHR);
    if (r != VK_SUCCESS) return false;

    break;
  }

  scene_res->current_frame_index = (scene_res->current_frame_index + 1) % MAX_NUM_FRAMES;
  return true;
}

namespace {
void TextureRes::Release(VulkanGlobalState* state) {
  if (desc_set != VK_NULL_HANDLE) {
    state->textures_desc_pool->Free(this);
    desc_set = VK_NULL_HANDLE;
  }
  if (image_view != VK_NULL_HANDLE) {
    vkDestroyImageView(state->device, image_view, nullptr);
    image_view = VK_NULL_HANDLE;
  }
  if (image != VK_NULL_HANDLE) {
    vmaDestroyImage(state->allocator, image,
                    alloc_image);
    image = VK_NULL_HANDLE;
    alloc_image = VK_NULL_HANDLE;
  }
}
}

void SceneRes::Release() {
  vkDeviceWaitIdle(vk_state->device);

  if (swapchain != VK_NULL_HANDLE) {
    vkDestroySwapchainKHR(vk_state->device, swapchain, nullptr);
    swapchain = VK_NULL_HANDLE;
  }

  ReleaseImages();

  if (depth_stencil_view != VK_NULL_HANDLE) {
    vkDestroyImageView(vk_state->device, depth_stencil_view, nullptr);
    depth_stencil_view = VK_NULL_HANDLE;
  }
  if (depth_stencil_image != VK_NULL_HANDLE) {
    vmaDestroyImage(vk_state->allocator, depth_stencil_image, alloc_depth_stencil);
    depth_stencil_image = VK_NULL_HANDLE;
    alloc_depth_stencil = VK_NULL_HANDLE;
  }

  // Release frames.
  for (int i = 0; i < vk_state->frame_count; ++i) {
    FrameRes* frame = &frame_res_array[i];

    if (frame->cmd_buf != VK_NULL_HANDLE) {
      vkFreeCommandBuffers(vk_state->device, vk_state->cmd_pool, 1, &frame->cmd_buf);
      frame->cmd_buf = VK_NULL_HANDLE;
    }

    if (frame->cmd_fence != VK_NULL_HANDLE) {
      vkDestroyFence(vk_state->device, frame->cmd_fence, nullptr);
      frame->cmd_fence = VK_NULL_HANDLE;
    }
    frame->cmd_fence_waitable = false;

    if (frame->acquire_semaphore != VK_NULL_HANDLE) {
      vkDestroySemaphore(vk_state->device, frame->acquire_semaphore, nullptr);
      frame->acquire_semaphore = VK_NULL_HANDLE;
    }

    if (frame->graphics_desc_set != VK_NULL_HANDLE) {
      vkFreeDescriptorSets(vk_state->device, descriptor_pool, 1,
                           &frame->graphics_desc_set);
      frame->graphics_desc_set = VK_NULL_HANDLE;
    }

    if (frame->selection_desc_set != VK_NULL_HANDLE) {
      vkFreeDescriptorSets(vk_state->device, descriptor_pool, 1,
                           &frame->selection_desc_set);
      frame->selection_desc_set = VK_NULL_HANDLE;
    }
  }

  if (descriptor_pool != VK_NULL_HANDLE) {
    vkDestroyDescriptorPool(vk_state->device, descriptor_pool, nullptr);
    descriptor_pool = VK_NULL_HANDLE;
  }

  if (buffer_vertex != VK_NULL_HANDLE) {
    vmaDestroyBuffer(vk_state->allocator, buffer_vertex, alloc_vertex);
    buffer_vertex = VK_NULL_HANDLE;
  }
  if (buffer_normal != VK_NULL_HANDLE) {
    vmaDestroyBuffer(vk_state->allocator, buffer_normal, alloc_normal);
    buffer_normal = VK_NULL_HANDLE;
  }
  if (buffer_color != VK_NULL_HANDLE) {
    vmaDestroyBuffer(vk_state->allocator, buffer_color, alloc_color);
    buffer_color = VK_NULL_HANDLE;
  }
  if (buffer_object_id != VK_NULL_HANDLE) {
    vmaDestroyBuffer(vk_state->allocator, buffer_object_id, alloc_object_id);
    buffer_object_id = VK_NULL_HANDLE;
  }
  if (buffer_index != VK_NULL_HANDLE) {
    vmaDestroyBuffer(vk_state->allocator, buffer_index, alloc_index);
    buffer_index = VK_NULL_HANDLE;
  }
  if (buffer_frame != VK_NULL_HANDLE) {
    vmaDestroyBuffer(vk_state->allocator, buffer_frame, alloc_frame);
    buffer_frame = VK_NULL_HANDLE;
  }
  if (buffer_selection != VK_NULL_HANDLE) {
    vmaDestroyBuffer(vk_state->allocator, buffer_selection, alloc_selection);
    buffer_selection = VK_NULL_HANDLE;
  }

  if (!is_external_surface)
    vkDestroySurfaceKHR(vk_state->instance, surface, nullptr);
  surface = VK_NULL_HANDLE;
}

VkSampleCountFlagBits VkSceneRenderer::GetMaxUsableSampleCount() const {
  auto dev = impl_->vk_objects.global_state.physical_device;
  return ::GetMaxUsableSampleCount(dev);
}

bool VkSceneRenderer::SetSampleCount(VkSampleCountFlagBits sample_count) {
  if (!IsValidSampleCount(sample_count))
    return false;
  if (impl_->vk_objects.global_state.sample_count == sample_count)
    return true;

  impl_->vk_objects.global_state.sample_count = sample_count;

  // Invalidate global Vulkan state that depends on sample_count.
  auto& state = impl_->vk_objects.global_state;

  vkQueueWaitIdle(state.queue);

  if (state.graphics_pipeline != VK_NULL_HANDLE) {
    vkDestroyPipeline(state.device, state.graphics_pipeline, nullptr);
    state.graphics_pipeline = VK_NULL_HANDLE;
  }
  if (state.selection_pipeline != VK_NULL_HANDLE) {
    vkDestroyPipeline(state.device, state.selection_pipeline, nullptr);
    state.selection_pipeline = VK_NULL_HANDLE;
  }
  if (state.graphics_renderpass != VK_NULL_HANDLE) {
    vkDestroyRenderPass(state.device, state.graphics_renderpass, nullptr);
    state.graphics_renderpass = VK_NULL_HANDLE;
  }
  for (auto& [scene, scene_res] : impl_->vk_objects.scene_res_array) {
    scene_res->InvalidateSwapchain();
  }

  return true;
}

VkSampleCountFlagBits VkSceneRenderer::GetSampleCount() const {
  return impl_->vk_objects.global_state.sample_count;
}

uint32_t VkSceneRenderer::GetObjectId(Scene* scene, int x, int y) const {
  if (!scene)
    return 0;

  auto it = impl_->vk_objects.scene_res_array.find(scene);
  if (it == impl_->vk_objects.scene_res_array.end())
    return 0;

  SceneRes* scene_res = it->second;
  VulkanGlobalState* state = scene_res->vk_state;

  // Wait for all pending work to complete before reading back.
  vkQueueWaitIdle(state->queue);

  uint32_t idx = scene_res->current_image_index;
  if (idx >= scene_res->image_count)
    return 0;

  ImageRes& image_res = scene_res->image_res_array[idx];
  if (image_res.object_id_image == VK_NULL_HANDLE)
    return 0;

  VkExtent2D extent = scene_res->image_extent;
  if (x < 0 || static_cast<uint32_t>(x) >= extent.width ||
      y < 0 || static_cast<uint32_t>(y) >= extent.height)
    return 0;

  VkDeviceSize pixel_size = sizeof(uint32_t);
  VkDeviceSize buffer_size = pixel_size;

  // Create a staging buffer for readback.
  VkBuffer staging_buffer = VK_NULL_HANDLE;
  VmaAllocation staging_alloc = VK_NULL_HANDLE;
  VkBufferCreateInfo buf_ci{
      .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
      .size = buffer_size,
      .usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT,
  };
  VmaAllocationCreateInfo alloc_ci{
      .flags = VMA_ALLOCATION_CREATE_MAPPED_BIT,
      .usage = VMA_MEMORY_USAGE_CPU_TO_GPU,
  };
  VkResult r = vmaCreateBuffer(state->allocator, &buf_ci, &alloc_ci,
                               &staging_buffer, &staging_alloc, nullptr);
  if (r != VK_SUCCESS)
    return 0;

  // Create a temporary command buffer for the copy.
  VkCommandPool pool = VK_NULL_HANDLE;
  VkCommandPoolCreateInfo pool_ci{
      .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
      .flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT,
      .queueFamilyIndex = state->graphics_family_index.value(),
  };
  r = vkCreateCommandPool(state->device, &pool_ci, nullptr, &pool);
  if (r != VK_SUCCESS) {
    vmaDestroyBuffer(state->allocator, staging_buffer, staging_alloc);
    return 0;
  }

  VkCommandBuffer cmd_buf = VK_NULL_HANDLE;
  VkCommandBufferAllocateInfo alloc_buf{
      .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
      .commandPool = pool,
      .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
      .commandBufferCount = 1,
  };
  r = vkAllocateCommandBuffers(state->device, &alloc_buf, &cmd_buf);
  if (r != VK_SUCCESS) {
    vkDestroyCommandPool(state->device, pool, nullptr);
    vmaDestroyBuffer(state->allocator, staging_buffer, staging_alloc);
    return 0;
  }

  VkCommandBufferBeginInfo begin_info{
      .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
      .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
  };
  vkBeginCommandBuffer(cmd_buf, &begin_info);

  // Transition object_id_image from SHADER_READ_ONLY_OPTIMAL to TRANSFER_SRC_OPTIMAL.
  VkImageMemoryBarrier barrier{
      .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
      .srcAccessMask = VK_ACCESS_SHADER_READ_BIT,
      .dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT,
      .oldLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
      .newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
      .image = image_res.object_id_image,
      .subresourceRange = {
          .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
          .levelCount = 1,
          .layerCount = 1,
      },
  };
  vkCmdPipelineBarrier(cmd_buf,
                       VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                       VK_PIPELINE_STAGE_TRANSFER_BIT,
                       0, 0, nullptr, 0, nullptr, 1, &barrier);

  // Copy a single pixel from (x, y) to the staging buffer.
  VkBufferImageCopy region{
      .imageSubresource = {
          .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
          .layerCount = 1,
      },
      .imageOffset = {static_cast<int32_t>(x), static_cast<int32_t>(y), 0},
      .imageExtent = {1, 1, 1},
  };
  vkCmdCopyImageToBuffer(cmd_buf, image_res.object_id_image,
                         VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                         staging_buffer, 1, &region);

  // Transition object_id_image back to SHADER_READ_ONLY_OPTIMAL.
  barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
  barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
  barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
  barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  vkCmdPipelineBarrier(cmd_buf,
                       VK_PIPELINE_STAGE_TRANSFER_BIT,
                       VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                       0, 0, nullptr, 0, nullptr, 1, &barrier);

  vkEndCommandBuffer(cmd_buf);

  VkSubmitInfo submit_info{
      .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
      .commandBufferCount = 1,
      .pCommandBuffers = &cmd_buf,
  };
  r = vkQueueSubmit(state->queue, 1, &submit_info, VK_NULL_HANDLE);
  if (r != VK_SUCCESS) {
    vkFreeCommandBuffers(state->device, pool, 1, &cmd_buf);
    vkDestroyCommandPool(state->device, pool, nullptr);
    vmaDestroyBuffer(state->allocator, staging_buffer, staging_alloc);
    return 0;
  }

  r = vkQueueWaitIdle(state->queue);
  if (r != VK_SUCCESS) {
    vkFreeCommandBuffers(state->device, pool, 1, &cmd_buf);
    vkDestroyCommandPool(state->device, pool, nullptr);
    vmaDestroyBuffer(state->allocator, staging_buffer, staging_alloc);
    return 0;
  }

  // Read back the pixel value.
  void* mapped_data = nullptr;
  vmaMapMemory(state->allocator, staging_alloc, &mapped_data);
  uint32_t object_id = *reinterpret_cast<uint32_t*>(mapped_data);
  vmaUnmapMemory(state->allocator, staging_alloc);

  // Cleanup.
  vkFreeCommandBuffers(state->device, pool, 1, &cmd_buf);
  vkDestroyCommandPool(state->device, pool, nullptr);
  vmaDestroyBuffer(state->allocator, staging_buffer, staging_alloc);

  return object_id;
}
