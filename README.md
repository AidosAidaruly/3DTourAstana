# Astana3DTour — 3D Tour to Astana

## Project Overview

Astana3DTour is a Computer Graphics Fundamentals project being developed as an interactive 3D tour of selected landmarks in Astana, Kazakhstan.

The current version is a Week 6 prototype. It provides the rendering, camera, and scene foundations for future landmarks; it is not a completed city or landmark tour.

## Technologies

- C++17
- OpenGL 3.3 Core
- GLFW for window creation, OpenGL context creation, and input
- GLAD for loading OpenGL functions
- GLSL for vertex and fragment shaders
- Visual Studio 2022 with the MSVC v143 toolset

## Current Features

- Interactive WASD movement and mouse-look camera controls.
- A 400 × 400 ground plane with a procedural checkerboard pattern generated in the fragment shader.
- A 100 × 60 light-gray central plaza, centered at the world origin, with its top at `y = 0.05` to avoid z-fighting.
- Model, view, and perspective projection transformations.
- Depth testing for correct surface visibility.
- Modular C++ code for camera controls, mathematics, shaders, meshes, and scene rendering.

The world scale is 1 OpenGL unit = 1 meter. The origin `(0, 0, 0)` is reserved as the reference position for the future Baiterek monument. The camera starts at `(0, 3, 55)` with yaw `-90°` and pitch `-5°`.

## Project Structure

| File or directory | Responsibility |
| --- | --- |
| `Astana3DTour.sln` | Native Visual Studio solution used to build and run the application. |
| `Astana3DTour.vcxproj` / `.vcxproj.filters` | Executable project configuration and Solution Explorer organization. |
| `src/main.cpp` | GLFW and OpenGL initialization, window creation, input callbacks, main loop, rendering coordination, and application cleanup. |
| `src/Camera.h` / `Camera.cpp` | Camera position and orientation, WASD movement, mouse look, and the view matrix. |
| `src/Math3D.h` / `Math3D.cpp` | Vector operations, model transformations (translation, Y-axis rotation, and scaling), and perspective projection. |
| `src/Shader.h` / `Shader.cpp` | Shader file loading, compilation, program linking, error reporting, program use, and uniform lookup. |
| `src/Mesh.h` / `Mesh.cpp` | Reusable ground and cube geometry, VAO/VBO ownership, drawing, and resource cleanup. |
| `src/Scene.h` / `Scene.cpp` | Ground and plaza placement, model matrices, colors, and scene rendering. |
| `shaders/` | GLSL vertex and fragment shaders for the checkerboard ground and solid-color plaza. |
| `include/GLFW/`, `include/glad/`, `include/KHR/` | Required third-party GLFW, GLAD, and Khronos headers. |
| `src/glad.c` | Generated GLAD OpenGL loader implementation. |
| `lib/glfw3.lib` | Existing GLFW library used by the native Debug x64 project. |

The reusable cube geometry currently forms the thin plaza slab. No separate test cube is visible in the scene.

The existing CMake files are retained, but their source list has not been updated for the modular architecture. Use the native Visual Studio solution for the current build workflow.

## Build and Run

### Requirements

- Windows with Visual Studio 2022.
- The **Desktop development with C++** workload, including the **MSVC v143 x64/x86 build tools** and a **Windows SDK**.
- A graphics driver supporting **OpenGL 3.3 Core**.

The required GLFW library, GLAD source, and GLFW/GLAD/Khronos headers are included in the repository. No additional math library or dependency download is needed.

The supplied GLFW library requires the Debug DLL runtime (`/MDd`). The native project is configured for **Debug x64** with this runtime. A Release build would require a compatible GLFW library and a corresponding project configuration.

### Visual Studio workflow

1. Open **`Astana3DTour.sln` in the repository root** using Visual Studio 2022.
2. Select **Debug** as the solution configuration and **x64** as the platform.
3. If necessary, right-click **Astana3DTour** in Solution Explorer and choose **Set as Startup Project**.
4. Select **Build > Build Solution** (`Ctrl+Shift+B`).
5. Press **F5** to launch with the debugger, or **Ctrl+F5** to launch without debugging.

The executable is generated at `bin\x64\Debug\Astana3DTour.exe`. The project copies `shaders/` next to the executable after building and sets the debugger working directory to the executable output directory. Rebuild after editing shaders so the runtime copies are refreshed.

## Controls

| Input | Action |
| --- | --- |
| **W / S** | Move forward / backward parallel to the ground. |
| **A / D** | Strafe left / right. |
| **Mouse** | Rotate the camera using the captured cursor. |
| **ESC** | Exit the application. |

Movement speed is 5 world units per second. Mouse look limits the pitch to prevent the camera from flipping.

## Planned Features

The following features are planned and are not implemented in the current prototype:

- A simplified Baiterek monument at the world origin.
- Surrounding buildings and pedestrian paths.
- Talan Towers and Khan Shatyr.
- Textures and lighting.
- Hierarchical modeling and procedural modeling of scene objects.

## Development Status

The Week 6 prototype currently implements the interactive camera, checkerboard ground, central gray plaza, transformation matrices, depth testing, and modular rendering architecture.

Landmarks, buildings, pedestrian paths, image textures, lighting, and hierarchical or procedural city models remain future development tasks. Development proceeds incrementally, with each stage kept small enough to build, inspect, and explain as part of the Computer Graphics Fundamentals project.
