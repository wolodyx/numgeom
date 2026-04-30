#include "camera.h"

#include "glm/gtc/matrix_transform.hpp"

#include "numgeom/alignedboundbox.h"

const float Camera::fov_y_ = glm::radians(45.0f);

float Camera::ComputeCameraDistance(float radius) const {
  float aspect = aspect_function_();
  float distance_vertical = radius / std::tan(fov_y_ * 0.5f);
  float fov_x = 2.0f * std::atan(std::tan(fov_y_ * 0.5f) * aspect);
  float distance_horizontal = radius / std::tan(fov_x * 0.5f);
  return std::max(distance_vertical, distance_horizontal) * 1.1f;
}

Camera::Camera() {
  aspect_function_ = []() { return 1.0f; };
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
  float aspect = aspect_function_();
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

void Camera::Translate(const glm::vec3& v) { eye_ += v; }

void Camera::Translate(const glm::vec2& screen_offset) {
  static const float base_speed = 0.1f;
  static const float sensitivity = 0.3f;

  // Коррекция для разных соотношений сторон: замедляем
  // * горизонтальное движение для широких экранов;
  // * вертикальное движение для высоких экранов.
  glm::vec2 scaled_offset = screen_offset * sensitivity;
  glm::vec2 corrected_offset = scaled_offset;
  float aspect = aspect_function_();
  if (aspect > 1.0f)
    corrected_offset.x /= aspect;
  else
    corrected_offset.y *= aspect;

  glm::vec3 x_screen_axis = glm::normalize(glm::cross(forward_, up_));
  glm::vec3 y_screen_axis = glm::normalize(glm::cross(x_screen_axis, forward_));
  glm::vec3 offset =
      corrected_offset.x * x_screen_axis + corrected_offset.y * y_screen_axis;
  eye_ += (offset * base_speed);
}

void Camera::Zoom(float k) { eye_ += (k * forward_); }

void Camera::FitBox(const AlignedBoundBox& box) {
  glm::vec3 center = box.GetCenter();
  glm::vec3 size = box.GetSize();
  float radius = glm::length(size) * 0.5f;
  float distance = ComputeCameraDistance(radius);
  eye_ = center - forward_ * distance;
  up_ = glm::normalize(up_);
}

void Camera::SetAspectFunction(std::function<float()> func) {
  aspect_function_ = func;
}

void Camera::RotateAroundPivot(const glm::vec3& pivot_point,
                               const glm::vec2& screen_offset) {
  static const float base_sensitivity = 0.01f;
  static const float min_pitch = -glm::half_pi<float>() + 0.01f;
  static const float max_pitch = glm::half_pi<float>() - 0.01f;

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
