#include "camera.h"

#include "glm/gtc/matrix_transform.hpp"


const float Camera::s_fovY = glm::radians(45.0f);


float Camera::computeCameraDistance(float radius) const
{
    float aspect = m_aspectFunction();
    float distanceVertical = radius / std::tan(s_fovY * 0.5f);
    float fovX = 2.0f * std::atan(std::tan(s_fovY * 0.5f) * aspect);
    float distanceHorizontal = radius / std::tan(fovX * 0.5f);
    return std::max(distanceVertical, distanceHorizontal) * 1.1f;
}


Camera::Camera()
{
    m_aspectFunction = [](){ return 1.0f; };
    m_position = glm::vec3(0.0f, 0.0f, 0.0f);
    m_direction = glm::vec3(0.0f, 0.0f, 1.0f);
    m_up = glm::vec3(0.0f, 1.0f, 0.0f);
}


Camera::Camera(
    const glm::vec3& pos,
    const glm::vec3& dir,
    const glm::vec3& up)
    : m_position(pos), m_direction(dir), m_up(up)
{
}


glm::mat4 Camera::viewMatrix() const
{
    return glm::lookAt(
        m_position,
        m_position + m_direction,
        m_up
    );
}


namespace
{
float distanceBetweenPointAndCube(
    const glm::vec3& pt,
    const glm::vec3& cubeMinPt,
    const glm::vec3& cubeMaxPt)
{
    glm::vec3 closestPoint(
        std::max(cubeMinPt.x, std::min(pt.x, cubeMaxPt.x)),
        std::max(cubeMinPt.y, std::min(pt.y, cubeMaxPt.y)),
        std::max(cubeMinPt.z, std::min(pt.z, cubeMaxPt.z))
    );
    glm::vec3 diff = pt - closestPoint;
    return glm::length(diff);
}
}


glm::mat4 Camera::projectionMatrix(const glm::vec3& minPoint, const glm::vec3& maxPoint) const
{
    return glm::perspective(
        glm::radians(45.0f),
        m_aspectFunction(),
        0.001f,
        1000.0f
    );
}


void Camera::translate(const glm::vec3& v)
{
    m_position += v;
}


void Camera::translate(const glm::vec2& screenOffset)
{
    static const float baseSpeed = 0.1f;
    static const float sensitivity = 0.3f;

    // Коррекция для разных соотношений сторон: замедляем
    // * горизонтальное движение для широких экранов;
    // * вертикальное движение для высоких экранов.
    glm::vec2 scaledOffset = screenOffset * sensitivity;
    glm::vec2 correctedOffset = scaledOffset;
    float aspect = m_aspectFunction();
    if(aspect > 1.0f)
        correctedOffset.x /= aspect;
    else
        correctedOffset.y *= aspect;

    glm::vec3 xScreenAxis = glm::normalize(glm::cross(m_direction, m_up));
    glm::vec3 yScreenAxis = glm::normalize(glm::cross(xScreenAxis, m_direction));
    glm::vec3 offset = correctedOffset.x * xScreenAxis + correctedOffset.y * yScreenAxis;
    m_position += (offset * baseSpeed);
}


void Camera::zoom(float k)
{
    m_position += (k * m_direction);
}


void Camera::fitBox(const glm::vec3& minPoint, const glm::vec3& maxPoint)
{
    glm::vec3 center = (minPoint + maxPoint) * 0.5f;
    glm::vec3 size = maxPoint - minPoint;
    float radius = glm::length(size) * 0.5f;
    float distance = computeCameraDistance(radius);
    m_position = center - m_direction * distance;
    m_up = glm::normalize(m_up);
}


void Camera::setAspectFunction(std::function<float()> func)
{
    m_aspectFunction = func;
}


void Camera::rotateAroundPivot(const glm::vec3& pivotPoint, const glm::vec2& screenOffset)
{
    static const float baseSensitivity = 0.01f;
    static const float minPitch = -glm::half_pi<float>() + 0.01f;
    static const float maxPitch = glm::half_pi<float>() - 0.01f;

    // Вычисляем вектор от точки опоры до камеры
    glm::vec3 vecToCamera = m_position - pivotPoint;

    // Создаем локальную систему координат
    glm::vec3 right = glm::normalize(glm::cross(m_direction, m_up));
    glm::vec3 up = m_up;

    // Применяем повороты
    float pitch = -screenOffset.y * baseSensitivity;
    float yaw = -screenOffset.x * baseSensitivity;

    // Ограничиваем pitch для предотвращения переворота
    pitch = glm::clamp(pitch, minPitch, maxPitch);

    // Поворот вокруг right axis (pitch)
    glm::mat4 pitchRot = glm::rotate(glm::mat4(1.0f), pitch, right);
    vecToCamera = glm::vec3(pitchRot * glm::vec4(vecToCamera, 0.0f));
    up = glm::vec3(pitchRot * glm::vec4(up, 0.0f));

    // Поворот вокруг up axis (yaw)
    glm::mat4 yawRot = glm::rotate(glm::mat4(1.0f), yaw, up);
    vecToCamera = glm::vec3(yawRot * glm::vec4(vecToCamera, 0.0f));

    // Обновляем позицию и направление
    m_position = pivotPoint + vecToCamera;
    m_direction = glm::normalize(pivotPoint - m_position);
    m_up = glm::normalize(up);
}
