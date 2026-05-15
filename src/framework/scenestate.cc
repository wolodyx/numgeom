#include "scenestate.h"

#include "numgeom/screentext.h"

Scene::State::~State() {
  for (auto o : screen_text_objects_)
    delete o;
}
