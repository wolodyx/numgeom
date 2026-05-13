#include "applicationinner.h"

#include "numgeom/iteratorimpl.hpp"

#include "applicationstate.h"

Application::Inner::Inner(State* impl) {
  impl_ = impl;
}

Application::Inner::~Inner() {}

bool Application::Inner::HasLogo() const { return !!impl_->logo; }

const Logo& Application::Inner::GetLogo() const { return impl_->logo; }

bool Application::Inner::HasScreenTexts() const {
  return !impl_->screen_text_objects_.empty();
}

Iterator<const ScreenText*> Application::Inner::GetScreenTextObjects() const {
  auto it = new IteratorImpl_StdIterator<std::list<ScreenText*>::const_iterator>(
      impl_->screen_text_objects_.begin(),
      impl_->screen_text_objects_.end());
  return Iterator<const ScreenText*>(it);
}

glm::mat4 Application::Inner::GetViewMatrix() const {
  return impl_->camera.GetViewMatrix();
}

glm::mat4 Application::Inner::GetProjectionMatrix() const {
  return impl_->camera.GetProjectionMatrix(impl_->scene.GetBoundBox());
}
