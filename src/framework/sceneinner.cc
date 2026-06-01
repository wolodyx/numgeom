#include "sceneinner.h"

#include "numgeom/iteratorimpl.hpp"

#include "scenestate.h"

Scene::Inner::Inner(State* impl) {
  impl_ = impl;
}

Scene::Inner::~Inner() {}

bool Scene::Inner::HasFgImages() const {
  return !impl_->fg_image_positions.empty();
}

Iterator<const FgImage*> Scene::Inner::GetFgImages() const {
  auto it = new IteratorImpl_StdMapKey<const FgImage*,glm::ivec2>(
      impl_->fg_image_positions.begin(),
      impl_->fg_image_positions.end());
  return Iterator<const FgImage*>(it);
}

bool Scene::Inner::HasScreenTexts() const {
  return !impl_->screen_text_positions.empty();
}

Iterator<const ScreenText*> Scene::Inner::GetScreenTextObjects() const {
  auto it = new IteratorImpl_StdMapKey<const ScreenText*,glm::ivec2>(
      impl_->screen_text_positions.begin(),
      impl_->screen_text_positions.end());
  return Iterator<const ScreenText*>(it);
}

glm::ivec2 Scene::Inner::GetScreenPosition(const FgImage* fg_image) const {
  auto it = impl_->fg_image_positions.find(fg_image);
  if (it == impl_->fg_image_positions.end())
    return glm::ivec2{};
  return it->second;
}

glm::ivec2 Scene::Inner::GetScreenPosition(const ScreenText* screen_text) const {
  auto it = impl_->screen_text_positions.find(screen_text);
  if (it == impl_->screen_text_positions.end())
    return glm::ivec2{};
  return it->second;
}

void Scene::Inner::AddFgImage(const FgImage* fg_image,
                              const glm::ivec2& screen_position) {
  impl_->fg_image_positions[fg_image] = screen_position;
  impl_->has_changes_ = true;
}
