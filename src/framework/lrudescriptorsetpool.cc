#include "lrudescriptorsetpool.h"

#include <array>
#include <cassert>

LruDescriptorSetPool::LruDescriptorSetPool(
    VkDevice device,
    VkDescriptorSetLayout desc_set_layout,
    size_t num_resources) : LruPool(num_resources) {
  device_ = device;
  desc_set_layout_ = desc_set_layout;
  this->Initialize();
}

LruDescriptorSetPool::~LruDescriptorSetPool() {}

void LruDescriptorSetPool::Initialize() {
  size_t ds_count = GetResourcesCount();
  std::array<VkDescriptorPoolSize, 1> desc_pool_sizes = {
      VkDescriptorPoolSize {
        .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        .descriptorCount = static_cast<uint32_t>(ds_count)
      },
  };
  VkDescriptorPoolCreateInfo ci_descpool {
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
      .flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT,
      .maxSets = static_cast<uint32_t>(ds_count),
      .poolSizeCount = static_cast<uint32_t>(desc_pool_sizes.size()),
      .pPoolSizes = desc_pool_sizes.data()
  };
  VkResult r = vkCreateDescriptorPool(device_, &ci_descpool, nullptr,
                                      &desc_pool_);
  assert(r == VK_SUCCESS);
}

void LruDescriptorSetPool::Finalize() {
  vkDestroyDescriptorPool(device_, desc_pool_, nullptr);
}

LruDescriptorSetPool::Resource* LruDescriptorSetPool::AllocateResourceObject() {
  return new Resource(this);
}

void LruDescriptorSetPool::Initialize(LruPool::Resource* base_res) {
  auto res = dynamic_cast<Resource*>(base_res);
  VkResult r;
  VkDescriptorSetAllocateInfo descSetAllocInfo {
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
      .descriptorPool = desc_pool_,
      .descriptorSetCount = 1,
      .pSetLayouts = &desc_set_layout_
  };
  r = vkAllocateDescriptorSets(device_, &descSetAllocInfo, &res->desc_set_);
  assert(r == VK_SUCCESS);
}

void LruDescriptorSetPool::Finalize(LruPool::Resource* base_res) {
  auto res = dynamic_cast<Resource*>(base_res);
  VkResult r;
  r = vkFreeDescriptorSets(device_, desc_pool_, 1, &res->desc_set_);
  assert(r == VK_SUCCESS);
}
