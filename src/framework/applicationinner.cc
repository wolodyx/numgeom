#include "applicationinner.h"

#include "applicationstate.h"

Application::Inner::Inner(State* pimpl) {
  pimpl_ = pimpl;
}

Application::Inner::~Inner() {}

bool Application::Inner::HasLogo() const { return !!pimpl_->logo; }

const Logo& Application::Inner::GetLogo() const { return pimpl_->logo; }
