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


glm::mat4 Camera::projectionMatrix() const
{
    float aspect = m_aspectFunction();
    return glm::perspective(
        glm::radians(45.0f),
        aspect,
        0.01f,
        100.0f
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
    glm::vec3 size = maxPoint - minPoint;
    float radius = glm::length(size) * 0.5f;
    glm::vec3 center = (minPoint + maxPoint) * 0.5f;
    float distance = computeCameraDistance(radius);
    m_direction = glm::vec3(0.0f, 0.0f, distance);
    m_position = center - m_direction;
    m_up = glm::vec3(0.0f, 1.0f, 0.0f);
}


void Camera::setAspectFunction(std::function<float()> func)
{
    m_aspectFunction = func;
}
