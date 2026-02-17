#ifndef NUMGEOM_FRAMEWORK_CAMERA_H
#define NUMGEOM_FRAMEWORK_CAMERA_H

#include <functional>

#include "glm/glm.hpp"


/** \class Camera
Вспомогательный класс для управления параметрами камеры.
*/
class Camera
{
public:
    static const float s_fovY;
    glm::vec3 m_position;
    glm::vec3 m_direction;
    glm::vec3 m_up;

public:

    Camera();

    Camera(const glm::vec3& pos, const glm::vec3& dir, const glm::vec3& up);

    //! Матрица вида.
    glm::mat4 viewMatrix() const;

    //! Матрица проекции.
    glm::mat4 projectionMatrix() const;

    void translate(const glm::vec3& v);

    //! Перемещение камеры вдоль экранных координат.
    void translate(const glm::vec2& v);

    void zoom(float k);

    void fitBox(const glm::vec3& minPoint, const glm::vec3& maxPoint);

    void setAspectFunction(std::function<float()>);

private:
    float computeCameraDistance(float radius) const;

private:
    //! Функция, запрашивающая соотношение сторон экрана в сцену.
    std::function<float()> m_aspectFunction;
};

#endif // !NUMGEOM_FRAMEWORK_CAMERA_H
