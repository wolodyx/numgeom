#include "gtest/gtest.h"

#include "numgeom/lrupool.h"

class Resource : public LruPool::Resource {
 public:
  Resource(LruPool* pool) : LruPool::Resource(pool) {}
  virtual ~Resource() {}
  int data_ = -1;
};

class LruTestPool : public LruPool {
 public:
  LruTestPool(size_t num_resources) : LruPool(num_resources) {
    for (size_t r = 1; r <= num_resources; ++r)
      resources_.insert(r);
  }
  virtual ~LruTestPool() {
    if (resources_.size() != GetResourcesCount())
      std::abort();
  }
 private:
  Resource* AllocateResourceObject() override {
    return new ::Resource(this);
  }
  void Initialize(LruPool::Resource* base_res) override {
    auto res = dynamic_cast<::Resource*>(base_res);
    ASSERT_FALSE(resources_.empty());
    res->data_ = *resources_.begin();
    resources_.erase(resources_.begin());
  }
  void Finalize(LruPool::Resource* base_res) override {
    auto res = dynamic_cast<::Resource*>(base_res);
    auto itb = resources_.insert(res->data_);
    ASSERT_TRUE(itb.second);
  }
  std::set<int> resources_;
};

class Consumer : public LruPool::Consumer {
 public:
  Consumer(LruPool::PoolPtr pool) : LruPool::Consumer(pool) {}
  virtual ~Consumer() {}
  bool ConfiscateResource() override { resource_ = -1; return true; }
  void SetResource(LruPool::Resource* res) override {
    resource_ = dynamic_cast<::Resource*>(res)->data_;
  }
  int resource_ = -1;
};

TEST(LruPool, Use3ResourcesWith10Consumers) {
  LruPool::PoolPtr pool = LruPool::Make<LruTestPool>(3);
  ASSERT_EQ(pool->GetResourcesCount(), 3);
  ASSERT_EQ(pool->GetFreeResourcesCount(), 3);

  std::vector<Consumer*> consumers(10);
  for (int i = 0; i < 10; ++i)
    consumers[i] = new Consumer(pool);

  for (int i = 0; i < 10; ++i) {
    Consumer* consumer = consumers[i];
    ASSERT_TRUE(pool->Allocate(consumer));
    ASSERT_NE(consumer->resource_, -1);
    if (i >= 3) {
      ASSERT_EQ(pool->GetFreeResourcesCount(), 0);
    }
  }

  for (int i = 0; i < 10; ++i)
    delete consumers[i];

  ASSERT_EQ(pool->GetFreeResourcesCount(), 3);
}
