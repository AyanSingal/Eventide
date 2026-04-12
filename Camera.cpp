#include "Camera.h"

Camera::Camera()
    : position(2.0f, 2.0f, 2.0f),
      up(0.0f, 1.0f, 0.0f),
      yaw(-135.0f),
      pitch(-35.0f),
      fov(45.0f),
      speed(2.5f),
      sensitivity(0.1f),
      nearPlane(0.1f),
      farPlane(10.0f)
{
    updateFront();
}

void Camera::updateFront()
{
    glm::vec3 direction;
    direction.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    direction.y = sin(glm::radians(pitch));
    direction.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    front = glm::normalize(direction);
}

glm::mat4 Camera::getViewMatrix()
{
    return glm::lookAt(position, position + front, up);
}

glm::mat4 Camera::getProjectionMatrix(float aspectRatio)
{
    glm::mat4 proj = glm::perspective(glm::radians(fov), aspectRatio, nearPlane, farPlane);
    proj[1][1] *= -1;  
    return proj;
}

void Camera::processKeyboard(GLFWwindow* window, float deltaTime)
{
    float velocity = speed * deltaTime;
    
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        position += front * velocity;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        position -= front * velocity;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        position -= glm::normalize(glm::cross(front, up)) * velocity;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        position += glm::normalize(glm::cross(front, up)) * velocity;
    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
        position += up * velocity;
    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
        position -= up * velocity;
}

void Camera::processMouse(float xOffset, float yOffset)
{
    yaw += xOffset * sensitivity;
    pitch += yOffset * sensitivity;
    
    if (pitch > 89.0f) pitch = 89.0f;
    if (pitch < -89.0f) pitch = -89.0f;
    
    updateFront();
}