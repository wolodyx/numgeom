#ifndef NUMGEOM_FRAMEWORK_CAMERA_H
#define NUMGEOM_FRAMEWORK_CAMERA_H

#include <functional>

#include "glm/glm.hpp"

class AlignedBoundBox;

/** \class Camera
An auxiliary class for controlling camera parameters.
*/
class Camera {
 public:
  Camera();

  Camera(const glm::vec3& pos, const glm::vec3& dir, const glm::vec3& up);

  glm::mat4 GetViewMatrix() const;

  glm::mat4 GetProjectionMatrix(const AlignedBoundBox&) const;

  glm::vec3 GetPosition() const;

  void Translate(const glm::vec3& v);

  //! Смещение камеры вдоль экранных координат.
  void Translate(const glm::vec2& v);

  void RotateAroundPivot(const glm::vec3& pivotPoint,
                         const glm::vec2& screenOffset);

  void Zoom(float k);

  void FitBox(const AlignedBoundBox&);

  void SetAspectFunction(std::function<float()>);

 private:
  float ComputeCameraDistance(float radius) const;

 private:
  static const float fov_y_;
  glm::vec3 eye_;
  glm::vec3 forward_;
  glm::vec3 up_;
  //! Функция, запрашивающая соотношение сторон экрана в сцену.
  std::function<float()> aspect_function_;
};

#endif  // !NUMGEOM_FRAMEWORK_CAMERA_H
