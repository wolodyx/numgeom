#include "applicationstate.h"

Application::State::~State() {
  delete inner_interface_;
  delete gpuManager;
}
