#include "numgeom/drawable.h"

#include <cassert>

Drawable::Drawable(SceneObject* parent) {
  assert(parent != nullptr);
  parent_ = parent;
  is_visible_ = true;
  is_pickable_ = true;
  is_selected_ = false;
  is_highlighted_ = false;
  static uint32_t max_id_ = 1;
  id_ = max_id_++;
}

Drawable::~Drawable() {
}

AlignedBoundBox Drawable::GetBoundBox() const {
  AlignedBoundBox box;
  for (auto pt : this->GetVertices())
    box.Expand(pt);
  return box;
}

void Drawable::SetColor(const glm::vec3& color) {
  color_ = color;
}

void Drawable::SetColor(float r, float g, float b) {
  color_ = glm::vec3(r,g,b);
}

void Drawable::SetColor(int r,int g,int b) {
  assert(r > 0 && r < 256);
  assert(g > 0 && g < 256);
  assert(b > 0 && b < 256);
  color_ = glm::vec3(r/255.0,g/255.0,b/255.0);
}

glm::vec3 Drawable::GetColor() const {
  return color_;
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

bool Drawable::IsVisible() const { return is_visible_; }

void Drawable::SetVisibility(bool visible) { is_visible_ = visible; }

bool Drawable::IsPickable() const { return is_visible_ && is_pickable_; }

void Drawable::DisablePicking() { is_pickable_ = false; }

void Drawable::EnablePicking() { is_pickable_ = true; }

bool Drawable::IsSelected() const { return is_selected_; }

void Drawable::Select() { is_selected_ = true; }

void Drawable::Deselect() { is_selected_ = false; }

bool Drawable::IsHighlighted() const { return is_highlighted_; }

void Drawable::Highlight(bool on) { is_highlighted_ = on; }

uint32_t Drawable::GetId() const { return id_; }
