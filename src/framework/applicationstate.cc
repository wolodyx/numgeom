#include "applicationstate.h"

Application::State::State() {
  scenes_.push_back(new Scene());
  scenes_.push_back(new Scene());
}

Application::State::~State() {
  delete inner_interface_;
  delete renderer;
  for (auto o : screen_text_objects_)
    delete o;
  for (Scene* s : scenes_)
    delete s;
}
