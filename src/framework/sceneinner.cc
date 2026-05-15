#include "sceneinner.h"

#include "numgeom/iteratorimpl.hpp"

#include "scenestate.h"

Scene::Inner::Inner(State* impl) {
  impl_ = impl;
}

Scene::Inner::~Inner() {}

bool Scene::Inner::HasLogo() const { return !!impl_->logo; }

const Logo& Scene::Inner::GetLogo() const { return impl_->logo; }

bool Scene::Inner::HasScreenTexts() const {
  return !impl_->screen_text_objects_.empty();
}

Iterator<const ScreenText*> Scene::Inner::GetScreenTextObjects() const {
  auto it = new IteratorImpl_StdIterator<std::list<ScreenText*>::const_iterator>(
      impl_->screen_text_objects_.begin(),
      impl_->screen_text_objects_.end());
  return Iterator<const ScreenText*>(it);
}
