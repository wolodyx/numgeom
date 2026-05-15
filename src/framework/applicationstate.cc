#include "applicationstate.h"

Application::State::State() {
}

Application::State::~State() {
  delete inner_interface_;
  delete renderer;
  for (auto [name, scene] : scenes_)
    delete scene;
}
