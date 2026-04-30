#ifndef NUMGEOM_FRAMEWORK_CAMERA_H
#define NUMGEOM_FRAMEWORK_CAMERA_H

#include <functional>
#include <optional>

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
  glm::vec3 GetPivotPoint() const;
  void SetPivotPoint(const glm::vec3&);

  void Translate(const glm::vec3& v);

  //! Moves the camera along the screen coordinate system.
  //! The offset vector is set in the number of pixels.
  void Translate(const glm::ivec2& v);

  void RotateAroundPivot(const glm::vec2& screenOffset);

  //! Exponential zoom to the pivot point of focus
  //! k > 0 - close to, k < 0 - move away from the pivot point.
  void Zoom(float k);

  void FitBox(const AlignedBoundBox&);

  void SetBoundBoxFunction(std::function<AlignedBoundBox()>);
  void SetViewportSizeFunction(std::function<std::tuple<uint32_t,uint32_t>()>);

 private:
  AlignedBoundBox GetSceneBox() const;
  float GetAspect() const;
  float ComputeCameraDistance(float radius) const;

 private:
  static const float fov_y_;
  glm::vec3 eye_;
  glm::vec3 forward_;
  glm::vec3 up_;
  std::optional<glm::vec3> pivot_point_;
  //! \name Requesting functions.
  //! \{
  std::function<std::tuple<uint32_t,uint32_t>()> get_viewport_size_function_;
  std::function<AlignedBoundBox()> get_boundbox_function_;
  //! \}
};

#endif  // !NUMGEOM_FRAMEWORK_CAMERA_H
