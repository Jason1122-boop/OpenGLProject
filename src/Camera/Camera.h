#pragma once
#include <glm/glm.hpp>
#include <GLFW/glfw3.h>

class Camera
{
public:
    glm::vec3 pos;
    glm::vec3 front;
    glm::vec3 up;

    float yaw;
    float pitch;

    Camera();

    void processKeyboard(GLFWwindow* window);
    void processMouse(double xpos, double ypos);
};