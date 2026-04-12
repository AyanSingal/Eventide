#pragma once

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <GLFW/glfw3.h>

class Camera{
public:

    Camera();

    glm::mat4 getViewMatrix();
    glm::mat4 getProjectionMatrix(float aspectRatio);

    void processKeyboard(GLFWwindow *window, float deltaTime);
    void processMouse(float xOffset, float yOffset);

    glm::vec3 position;

private:
    glm::vec3 front;
    glm::vec3 up;

    float yaw;
    float pitch;
    float fov;
    float speed;
    float sensitivity;
    float nearPlane;
    float farPlane;

    void updateFront();
};