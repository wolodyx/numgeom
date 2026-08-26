#include "numgeom/appcontext.h"

#include "numgeom/application.h"
#include "numgeom/vkscenerenderer.h"

#include "vkutilities.h"

bool AppContext::SetSampleCount(SampleCount count) {
  if (count == SampleCount::Bits_None)
    return false;
  Application* app = Application::Get();
  auto renderer = app->GetRenderer();
  VkSampleCountFlagBits max_count = renderer->GetMaxUsableSampleCount();
  VkSampleCountFlagBits desired_count = ToVkSampleCountFlagBits(count);
  bool in_the_range = static_cast<int>(desired_count) <= static_cast<int>(max_count);
  if (!in_the_range)
    return false;
  VkSampleCountFlagBits next_count = (in_the_range ? desired_count : max_count);
  return renderer->SetSampleCount(next_count);
}

SampleCount AppContext::GetSampleCount() const {
  Application* app = Application::Get();
  auto renderer = app->GetRenderer();
  return ToSampleCount(renderer->GetSampleCount());
}

SelectionMode AppContext::GetSelectionMode() const {
  return selection_mode_;
}

void AppContext::SetSelectionMode(SelectionMode selection_mode) {
  selection_mode_ = selection_mode;
}

void AppContext::SetContextMenuFunction(
    std::function<void(Workbench*,int,int)> func) {
  cm_function_ = func;
}

std::function<void(Workbench*,int,int)> AppContext::GetContextMenuFunction() {
  return cm_function_;
}
