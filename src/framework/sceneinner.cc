#include "sceneinner.h"

#include "numgeom/iteratorimpl.hpp"

#include "scenestate.h"

Scene::Inner::Inner(State* impl) {
  impl_ = impl;
}

Scene::Inner::~Inner() {}

bool Scene::Inner::HasFgImage() const { return !!impl_->fg_image_; }

const FgImage& Scene::Inner::GetFgImage() const { return impl_->fg_image_; }

bool Scene::Inner::HasScreenTexts() const {
  return !impl_->screen_text_objects_.empty();
}

Iterator<const ScreenText*> Scene::Inner::GetScreenTextObjects() const {
  auto it = new IteratorImpl_StdIterator<std::list<ScreenText*>::const_iterator>(
      impl_->screen_text_objects_.begin(),
      impl_->screen_text_objects_.end());
  return Iterator<const ScreenText*>(it);
}
