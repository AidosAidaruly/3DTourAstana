#include "Camera.h"

#include <GLFW/glfw3.h>

#include <algorithm>
#include <cmath>

void Camera::setViewpoint(Vec3 eye, Vec3 target)
{
    Vec3 direction = target - eye;
    float horizontalDistance = std::sqrt(
        direction.x * direction.x + direction.z * direction.z
    );

    position = eye;
    yaw = std::atan2(direction.z, direction.x) * 180.0f / pi;
    pitch = std::clamp(
        std::atan2(direction.y, horizontalDistance) * 180.0f / pi,
        -89.0f, 89.0f
    );
    resetMouse();
}

Vec3 Camera::forward() const
{
    // Convert yaw and pitch from degrees to radians.
    float y = yaw * pi / 180, p = pitch * pi / 180;

    return {
        std::cos(y) * std::cos(p),
        std::sin(p),
        std::sin(y) * std::cos(p)
    };
}

void Camera::mouseLook(double x, double y)
{
    // Reset the mouse reference to avoid a jump when focus is regained.
    if (firstMouse)
    {
        lastX = x;
        lastY = y;
        firstMouse = false;
    }

    yaw += static_cast<float>(x - lastX) * 0.1f;
    pitch = std::clamp(
        pitch + static_cast<float>(lastY - y) * 0.1f,
        -89.0f,
        89.0f
    );

    lastX = x;
    lastY = y;
}

void Camera::resetMouse()
{
    firstMouse = true;
}

Mat4 Camera::viewMatrix() const
{
    // Build the camera basis and translate the world relative to the camera.
    Vec3 f = forward(), s = normalize(cross(f, {0, 1, 0})), u = cross(s, f);

    return {
        s.x, u.x, -f.x, 0,
        s.y, u.y, -f.y, 0,
        s.z, u.z, -f.z, 0,
        -dot(s, position), -dot(u, position), dot(f, position), 1
    };
}

void Camera::processInput(GLFWwindow* window, float dt)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
    {
        glfwSetWindowShouldClose(window, GLFW_TRUE);
    }

    bool fPressed = glfwGetKey(window, GLFW_KEY_F) == GLFW_PRESS;

    if (fPressed && !fWasPressed)
    {
        freeFlight = !freeFlight;

        if (!freeFlight)
        {
            position.y = walkingEyeHeight;
        }
    }

    fWasPressed = fPressed;

    // Both modes use yaw for horizontal travel; pitch affects looking only.
    Vec3 f = forward();
    f.y = 0;
    f = normalize(f);

    Vec3 right = normalize(cross(f, {0, 1, 0})), movement{0, 0, 0};

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
    {
        movement = movement + f;
    }

    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
    {
        movement = movement - f;
    }

    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
    {
        movement = movement + right;
    }

    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
    {
        movement = movement - right;
    }

    if (freeFlight)
    {
        if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
        {
            movement.y += 1.0f;
        }

        if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS)
        {
            movement.y -= 1.0f;
        }
    }

    float speed = glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS
        ? 15.0f : 5.0f;

    // Normalize combined input so diagonal movement is not faster.
    if (dot(movement, movement) > 0)
    {
        position = position + normalize(movement) * (speed * dt);
    }
}
