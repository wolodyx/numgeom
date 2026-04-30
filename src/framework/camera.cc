#include "camera.h"

#include "glm/gtc/matrix_transform.hpp"

#include "numgeom/alignedboundbox.h"

const float Camera::fov_y_ = glm::radians(45.0f);

AlignedBoundBox Camera::GetSceneBox() const {
  if (!get_boundbox_function_)
    return AlignedBoundBox();
  return get_boundbox_function_();
}

float Camera::GetAspect() const {
  if (!get_viewport_size_function_)
    return 1.0f;
  int width, height;
  std::tie(width, height) = get_viewport_size_function_();
  return width / static_cast<float>(height);
}

float Camera::ComputeCameraDistance(float radius) const {
  float aspect = this->GetAspect();
  float distance_vertical = radius / std::tan(fov_y_ * 0.5f);
  float fov_x = 2.0f * std::atan(std::tan(fov_y_ * 0.5f) * aspect);
  float distance_horizontal = radius / std::tan(fov_x * 0.5f);
  return std::max(distance_vertical, distance_horizontal) * 1.1f;
}

Camera::Camera() {
  eye_ = glm::vec3(0.0f, 0.0f, 0.0f);
  forward_ = glm::vec3(0.0f, 1.0f, 0.0f);
  up_ = glm::vec3(0.0f, 0.0f, 1.0f);
}

Camera::Camera(const glm::vec3& pos, const glm::vec3& dir, const glm::vec3& up)
    : eye_(pos), forward_(dir), up_(up) {}

glm::mat4 Camera::GetViewMatrix() const {
  const glm::vec3 f = glm::normalize(forward_);
  const glm::vec3 s = glm::normalize(glm::cross(f, up_));
  const glm::vec3 u = glm::normalize(glm::cross(f, s));
  const float tx = glm::dot(s, eye_);
  const float ty = glm::dot(u, eye_);
  const float tz = glm::dot(f, eye_);
  glm::mat4 m(glm::vec4(s.x, u.x, f.x, 0.0f),
              glm::vec4(s.y, u.y, f.y, 0.0f),
              glm::vec4(s.z, u.z, f.z, 0.0f),
              glm::vec4(-tx, -ty, -tz, 1.0));
  return m;
}

glm::mat4 Camera::GetProjectionMatrix(const AlignedBoundBox& box) const {
  glm::vec3 center = box.GetCenter();
  glm::vec3 size = box.GetSize();
  float radius = glm::length(size) * 0.5f;
  float distance = glm::length(eye_ - center);
  float z_near = std::max(0.001f, distance - radius * 2.0f);
  float z_far = distance + radius * 2.0f;
  float aspect = this->GetAspect();
  const float tan_half_fov_y = std::tan(fov_y_ / 2.0f);
  glm::mat4 m(0.0f);
  m[0][0] = 1.0f / (aspect * tan_half_fov_y);
  m[1][1] = 1.0f / tan_half_fov_y;
  m[2][2] = z_far / (z_far - z_near);
  m[2][3] = 1.0f;
  m[3][2] = -(z_far * z_near) / (z_far - z_near);
  return m;
}

glm::vec3 Camera::GetPosition() const { return eye_; }

glm::vec3 Camera::GetPivotPoint() const {
  return pivot_point_.value_or(glm::vec3(0.0f));
}

void Camera::SetPivotPoint(const glm::vec3& pivot_point) {
  pivot_point_ = pivot_point;
}

void Camera::Translate(const glm::vec3& v) { eye_ += v; }

void Camera::Translate(const glm::ivec2& pixels_offset) {
  // Оси экрана в мировой системе координат.
  glm::vec3 x_screen_axis = glm::normalize(glm::cross(forward_, up_));
  glm::vec3 y_screen_axis = glm::normalize(glm::cross(x_screen_axis, forward_));
  // Преобразуем пиксельное смещение в экранное.
  // Учитываем, что ось Y окна не совпадает с оригинальной системой координат.
  glm::vec2 screen_offset(static_cast<float>(-pixels_offset.x),
                          static_cast<float>(+pixels_offset.y));
  const AlignedBoundBox box = this->GetSceneBox();
  int vp_width, vp_height;
  std::tie(vp_width, vp_height) = get_viewport_size_function_();
  if (!box.IsEmpty()) {
    auto box_size = box.GetSize();
    screen_offset.x *= box_size.x / static_cast<float>(vp_width);
    screen_offset.y *= box_size.y / static_cast<float>(vp_height);
  }
  glm::vec3 global_offset = screen_offset.x * x_screen_axis +
                            screen_offset.y * y_screen_axis;
  eye_ += global_offset;
}

void Camera::Zoom(float k) {
  const float sensitivity = 0.2f;
  const float min_factor = 0.01f;
  const float max_factor = 0.5f;
  const float min_distance = 0.01f;
  glm::vec3 pivot_point = this->GetPivotPoint();
  glm::vec3 to_focus = pivot_point - eye_;
  float current_distance = glm::length(to_focus);
  if (current_distance < 0.001f) {
    eye_ += (k * forward_);
    return;
  }
  glm::vec3 direction = to_focus / current_distance;
  float factor = sensitivity * std::abs(k);
  factor = glm::clamp(factor, min_factor, max_factor);
  if (k < 0) factor = -factor;
  float new_distance = current_distance * (1.0f - factor);
  if (new_distance < min_distance)
    new_distance = min_distance;
  eye_ = pivot_point - direction * new_distance;
}

void Camera::FitBox(const AlignedBoundBox& box) {
  glm::vec3 center = box.GetCenter();
  glm::vec3 size = box.GetSize();
  float radius = glm::length(size) * 0.5f;
  float distance = ComputeCameraDistance(radius);
  eye_ = center - forward_ * distance;
  up_ = glm::normalize(up_);
}

void Camera::SetBoundBoxFunction(std::function<AlignedBoundBox()> func) {
  get_boundbox_function_ = func;
  if (get_boundbox_function_ && !pivot_point_.has_value()) {
    AlignedBoundBox box = get_boundbox_function_();
    if (!box.IsEmpty())
      pivot_point_ = box.GetCenter();
  }
}

void Camera::SetViewportSizeFunction(
    std::function<std::tuple<uint32_t,uint32_t>()> func) {
  get_viewport_size_function_ = func;
}

void Camera::RotateAroundPivot(const glm::vec2& screen_offset) {
  static const float base_sensitivity = 0.01f;
  static const float min_pitch = -glm::half_pi<float>() + 0.01f;
  static const float max_pitch = glm::half_pi<float>() - 0.01f;
  glm::vec3 pivot_point = this->GetPivotPoint();

  // Вычисляем вектор от точки опоры до камеры
  glm::vec3 vec_to_camera = eye_ - pivot_point;

  // Создаем локальную систему координат
  glm::vec3 right = glm::normalize(glm::cross(forward_, up_));
  glm::vec3 up = up_;

  // Применяем повороты
  float pitch = -screen_offset.y * base_sensitivity;
  float yaw = -screen_offset.x * base_sensitivity;

  // Ограничиваем pitch для предотвращения переворота
  pitch = glm::clamp(pitch, min_pitch, max_pitch);

  // Поворот вокруг right axis (pitch)
  glm::mat4 pitch_rot = glm::rotate(glm::mat4(1.0f), pitch, right);
  vec_to_camera = glm::vec3(pitch_rot * glm::vec4(vec_to_camera, 0.0f));
  up = glm::vec3(pitch_rot * glm::vec4(up, 0.0f));

  // Поворот вокруг up axis (yaw)
  glm::mat4 yawRot = glm::rotate(glm::mat4(1.0f), yaw, up);
  vec_to_camera = glm::vec3(yawRot * glm::vec4(vec_to_camera, 0.0f));

  // Обновляем позицию и направление
  eye_ = pivot_point + vec_to_camera;
  forward_ = glm::normalize(pivot_point - eye_);
  up_ = glm::normalize(up);
}
