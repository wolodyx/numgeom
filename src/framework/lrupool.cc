#include "numgeom/lrupool.h"

#include <cassert>
#include <limits>

LruPool::Consumer::Consumer(PoolPtr pool) : lru_pool_(pool.get()) {
  assert(pool != nullptr);
}

LruPool::Consumer::~Consumer() {
  GetPool()->Free(this);
}

void LruPool::Consumer::Use() {
  lru_pool_->Use(this);
}

LruPool* LruPool::Consumer::GetPool() const {
  return lru_pool_;
}

LruPool::Resource::Resource(LruPool* pool): pool_(pool) {}

LruPool::Resource::~Resource() {}

const LruPool* LruPool::Resource::GetPool() const { return pool_; }

LruPool::LruPool(size_t num_resources) {
  resources_.resize(num_resources, nullptr);
  this->Initialize();
}

LruPool::~LruPool() {
}

void LruPool::Destroy(LruPool* pool) {
  for (auto res : pool->resources_) {
    if (res && res->consumer_)
      res->consumer_->ConfiscateResource();
  }
  pool->Finalize();
  delete pool;
}

size_t LruPool::GetResourcesCount() const {
  return resources_.size();
}

size_t LruPool::GetFreeResourcesCount() const {
  size_t count = 0;
  for (auto* res : resources_) {
    if (!res || !res->consumer_)
      ++count;
  }
  return count;
}

bool LruPool::Allocate(Consumer* consumer) {
  if (!consumer || consumer->GetPool() != this)
    return false;

  ++frame_counter_;

  if (auto* existing = FindByConsumer(consumer)) {
    existing->last_used_frame_ = frame_counter_;
    return true;
  }

  if (auto* free_res = FindFree()) {
    free_res->consumer_ = consumer;
    free_res->last_used_frame_ = frame_counter_;
    Initialize(free_res);
    consumer->SetResource(free_res);
    return true;
  }

  auto* lru = FindLeastRecentlyUsed();
  assert(lru != nullptr && lru->consumer_ != nullptr);

  lru->consumer_->ConfiscateResource();
  Finalize(lru);
  lru->consumer_ = consumer;
  lru->last_used_frame_ = frame_counter_;
  Initialize(lru);
  consumer->SetResource(lru);
  return true;
}

void LruPool::Use(Consumer* consumer) {
  if (!consumer || consumer->GetPool() != this) {
    return;
  }
  auto* res = FindByConsumer(consumer);
  if (res != nullptr) {
    res->last_used_frame_ = frame_counter_;
  }
}

bool LruPool::Free(Consumer* consumer) {
  if (!consumer || consumer->GetPool() != this) {
    return false;
  }
  auto* res = FindByConsumer(consumer);
  if (res == nullptr)
    return false;
  Finalize(res);
  res->consumer_ = nullptr;
  res->last_used_frame_ = 0;
  return true;
}

LruPool::Resource* LruPool::FindByConsumer(Consumer* consumer) {
  for (auto* res : resources_) {
    if (res != nullptr && res->consumer_ == consumer) {
      return res;
    }
  }
  return nullptr;
}

LruPool::Resource* LruPool::FindFree() {
  for (auto it = resources_.begin(); it != resources_.end(); ++it) {
    Resource* res = (*it);
    if (res == nullptr) {
      (*it) = this->AllocateResourceObject();
      return (*it);
    }
    if (res->consumer_ == nullptr)
      return res;
  }
  return nullptr;
}

LruPool::Resource* LruPool::FindLeastRecentlyUsed() {
  Resource* lru = nullptr;
  uint64_t min_frame = std::numeric_limits<uint64_t>::max();
  for (auto* res : resources_) {
    if (res->consumer_ != nullptr && res->last_used_frame_ < min_frame) {
      min_frame = res->last_used_frame_;
      lru = res;
    }
  }
  return lru;
}
