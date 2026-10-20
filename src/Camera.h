#pragma once

#include "Math3D.h"

struct GLFWwindow;

class Camera
{
public:
    void processInput(GLFWwindow* window, float dt);
    void mouseLook(double x, double y);
    void resetMouse();
    Mat4 viewMatrix() const;

private:
    Vec3 forward() const;

    // Start closer to the plaza, looking slightly downward.
    Vec3 position{0, 3, 55};
    float yaw = -90, pitch = -5;
    double lastX = 0, lastY = 0;
    bool firstMouse = true;
};
