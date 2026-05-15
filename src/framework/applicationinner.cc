#include "applicationinner.h"

#include "applicationstate.h"

Application::Inner::Inner(State* impl) {
  impl_ = impl;
}

Application::Inner::~Inner() {}
