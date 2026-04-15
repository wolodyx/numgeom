#include "numgeom/drawable.h"

#include <cassert>

Drawable::Drawable(SceneObject* parent) {
  assert(parent != nullptr);
  parent_ = parent;
}

Drawable::~Drawable() {
}

Drawable0::Drawable0(SceneObject* parent) : Drawable(parent) {
}

Drawable0::~Drawable0() {
}

Drawable0* Drawable0::Cast(Drawable* d) {
  return dynamic_cast<Drawable0*>(d);
}

const Drawable0* Cast(const Drawable* d) {
  return dynamic_cast<const Drawable0*>(d);
}

Drawable1::Drawable1(SceneObject* parent) : Drawable(parent) {
}

Drawable1::~Drawable1() {
}

Drawable1* Drawable1::Cast(Drawable* d) {
  return dynamic_cast<Drawable1*>(d);
}

const Drawable1* Drawable1::Cast(const Drawable* d) {
  return dynamic_cast<const Drawable1*>(d);
}

Drawable2::Drawable2(SceneObject* parent) : Drawable(parent) {
}

Drawable2::~Drawable2() {
}

Drawable2* Drawable2::Cast(Drawable* d) {
  return dynamic_cast<Drawable2*>(d);
}

const Drawable2* Drawable2::Cast(const Drawable* d) {
  return dynamic_cast<const Drawable2*>(d);
}
