#ifndef NUMGEOM_FRAMEWORK_LRUDESCRIPTORSETPOOL_H
#define NUMGEOM_FRAMEWORK_LRUDESCRIPTORSETPOOL_H

#include "volk.h"

#include "numgeom/lrupool.h"

class LruDescriptorSetPool : public LruPool {
 public:
  class Resource : public LruPool::Resource {
   public:
    Resource(LruPool* pool) : LruPool::Resource(pool) {}
    VkDescriptorSet desc_set_ = VK_NULL_HANDLE;
  };

  LruDescriptorSetPool(VkDevice device, VkDescriptorSetLayout desc_set_layout,
                       size_t num_resources);
  void Initialize() override;
  void Finalize() override;
  Resource* AllocateResourceObject() override;
  void Initialize(LruPool::Resource* base_res) override;
  void Finalize(LruPool::Resource* base_res) override;

 private:
  virtual ~LruDescriptorSetPool();

 private:
  VkDevice device_;
  VkDescriptorPool desc_pool_;
  VkDescriptorSetLayout desc_set_layout_;
};
#endif // !NUMGEOM_FRAMEWORK_LRUDESCRIPTORSETPOOL_H
