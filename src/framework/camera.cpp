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


void Camera::translate(const glm::vec2& v)
{
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
