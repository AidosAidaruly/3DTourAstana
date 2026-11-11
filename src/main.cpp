#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "Camera.h"
#include "Scene.h"

#include <algorithm>
#include <array>
#include <exception>
#include <filesystem>
#include <iostream>

namespace
{
    struct LandmarkViewpoint
    {
        int key;
        Vec3 eye;
        Vec3 target;
        bool wasPressed = false;
    };
}

int main(int argc, char** argv)
{
    glfwSetErrorCallback([](int code, const char* message)
    {
        std::cerr << "GLFW " << code << ": " << message << std::endl;
    });

    if (!glfwInit())
    {
        return 1;
    }

    // Request an OpenGL 3.3 Core context before creating the window.
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(
        1280, 720, "Astana 3D Tour | WASD + mouse | ESC to close", nullptr, nullptr
    );

    if (!window)
    {
        glfwTerminate();
        return 1;
    }

    // GLAD must load OpenGL functions after the context becomes current.
    glfwMakeContextCurrent(window);

    if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress)) ||
        !GLAD_GL_VERSION_3_3)
    {
        std::cerr << "Failed to load OpenGL 3.3" << std::endl;
        glfwDestroyWindow(window);
        glfwTerminate();
        return 1;
    }

    Camera camera;
    constexpr Vec3 botanicalGarden = BotanicalGarden::center;
    constexpr Vec3 expo = ExpoDistrict::center;
    std::array<LandmarkViewpoint, 2> viewpoints{{
        // 6: Just inside the east entrance, looking along the garden axis.
        {GLFW_KEY_6, {botanicalGarden.x + 101.0f, 3.0f, botanicalGarden.z},
         botanicalGarden},
        // 7: East processional route, facing Nur Alem and its main public lobby.
        {GLFW_KEY_7, {expo.x + 130.0f, 3.0f, expo.z},
         {expo.x, 42.0f, expo.z}}
    }};

    glfwSetWindowUserPointer(window, &camera);

    glfwSwapInterval(1);
    glfwSetCursorPosCallback(window, [](GLFWwindow* callbackWindow, double x, double y)
    {
        auto* activeCamera = static_cast<Camera*>(glfwGetWindowUserPointer(callbackWindow));
        activeCamera->mouseLook(x, y);
    });

    glfwSetWindowFocusCallback(window, [](GLFWwindow* callbackWindow, int focused)
    {
        if (focused)
        {
            auto* activeCamera = static_cast<Camera*>(glfwGetWindowUserPointer(callbackWindow));
            activeCamera->resetMouse();
        }
    });
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    if (glfwRawMouseMotionSupported())
    {
        glfwSetInputMode(window, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
    }

    glEnable(GL_DEPTH_TEST);

    int result = 0;

    try
    {
        auto shaders =
            std::filesystem::absolute(argc > 0 ? argv[0] : "Astana3DTour.exe").parent_path() /
            "shaders";

        // Scene resources are destroyed before the window and OpenGL context.
        Scene scene(shaders);
        double previous = glfwGetTime();

        while (!glfwWindowShouldClose(window))
        {
            double now = glfwGetTime();
            float dt = static_cast<float>(std::min(now - previous, 0.1));
            previous = now;

            glfwPollEvents();

            bool focused = glfwGetWindowAttrib(window, GLFW_FOCUSED) != 0;

            for (LandmarkViewpoint& viewpoint : viewpoints)
            {
                bool pressed = glfwGetKey(window, viewpoint.key) == GLFW_PRESS;

                if (focused && pressed && !viewpoint.wasPressed)
                {
                    camera.setViewpoint(viewpoint.eye, viewpoint.target);
                }

                viewpoint.wasPressed = pressed;
            }

            if (focused)
            {
                camera.processInput(window, dt);
            }

            int width = 0, height = 0;
            glfwGetFramebufferSize(window, &width, &height);

            if (width == 0 || height == 0)
            {
                glfwWaitEventsTimeout(0.05);
                continue;
            }

            glViewport(0, 0, width, height);
            glClearColor(0.65f, 0.80f, 0.92f, 1);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            Mat4 view = camera.viewMatrix(),
                 projection = projectionMatrix(
                     static_cast<float>(width) / static_cast<float>(height)
                 );

            scene.render(view, projection, static_cast<float>(now));
            glfwSwapBuffers(window);
        }
    }
    catch (const std::exception& error)
    {
        std::cerr << error.what() << std::endl;
        result = 1;
    }

    glfwDestroyWindow(window);
    glfwTerminate();

    return result;
}
