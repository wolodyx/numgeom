#include "applicationstate.h"

Application::State::~State() {
  delete inner_interface_;
  delete gpuManager;
  for (auto o : screen_text_objects_)
    delete o;
}
