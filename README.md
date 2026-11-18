# Astana3DTour — 3D Tour to Astana

## Project Overview

**Official project title:** 3D Tour to Astana

**Team members:** Aidos Aidaruly and Nurislam Temirgaly.

Astana3DTour is a Computer Graphics Fundamentals project being developed as an interactive 3D tour of selected landmarks in Astana, Kazakhstan.

The current version is an interactive city walkthrough and fly-over with procedural landmarks, developed districts, landscaping and an optimized daylight renderer. It is a tour, with no missions, score, fail state or game-over screen.

## Technologies

- C++17
- OpenGL 3.3 Core
- GLFW for window creation, OpenGL context creation, and input
- GLAD for loading OpenGL functions
- GLSL for vertex and fragment shaders
- Visual Studio 2022 with the MSVC v143 toolset

## Current Features

- Walking, free flight, mouse look, sprinting and landmark teleports.
- Procedural Astana landmarks, residential districts, connected roads, gardens and the Esil riverfront.
- Daylight, local sun shadows, cinematic/anime sky selection, water animation, color grading and FXAA.
- Static meshes, instancing, distance-based LOD, culling and cached rendering resources.
- Resizable help/status overlays, animation pause and synchronized interactive sunlight.

The world scale is 1 OpenGL unit = 1 meter. Baiterek is centered at the origin. The current startup viewpoint faces Khan Shatyr; key **1** visits Baiterek.

## Project Structure

| File or directory | Responsibility |
| --- | --- |
| `Astana3DTour.sln` | Native Visual Studio solution used to build and run the application. |
| `Astana3DTour.vcxproj` / `.vcxproj.filters` | Executable project configuration and Solution Explorer organization. |
| `src/main.cpp` | GLFW and OpenGL initialization, window creation, input callbacks, main loop, rendering coordination, and application cleanup. |
| `src/Camera.h` / `Camera.cpp` | Camera position and orientation, WASD movement, mouse look, and the view matrix. |
| `src/Math3D.h` / `Math3D.cpp` | Vector operations, model transformations (translation, Y-axis rotation, and scaling), and perspective projection. |
| `src/Shader.h` / `Shader.cpp` | Shader file loading, compilation, program linking, error reporting, program use, and uniform lookup. |
| `src/Mesh.h` / `Mesh.cpp` | Reusable indexed geometry, VAO/VBO ownership, drawing, and resource cleanup. |
| `src/Scene.h` / `Scene.cpp` | City integration, rendering coordination, lighting and effect state. |
| `src/TourControls.h`, `TourOverlay.cpp/.h`, `ProjectInfo.h` | Help/status panels, animation clock, sun input and editable submission metadata. |
| `shaders/` | Shared GLSL surface/material shaders; specialized rendering modules also contain GLSL. |
| `include/GLFW/`, `include/glad/`, `include/KHR/` | Required third-party GLFW, GLAD, and Khronos headers. |
| `src/glad.c` | Generated GLAD OpenGL loader implementation. |
| `lib/glfw3.lib` | Existing GLFW library used by the native Debug x64 project. |

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
| **Left Shift** | Hold to sprint at 15 m/s; during fast flight, hold for turbo flight at 50 m/s. |
| **F** | Toggle free flight; returning to walking smoothly descends to the fixed 3 m eye height. Press F again to cancel the descent. |
| **G** | In flight, toggle normal 5 m/s / fast 25 m/s travel. The fast-flight preference is remembered between flights. |
| **Space / Left Ctrl** | Move up / down in free flight. |
| **F1 / H** | Toggle the on-screen help panel; camera controls remain active. |
| **F2** | Hide/show all overlays for an unobstructed scene. |
| **P** | Pause/resume water and fountain animation, keeping navigation and FPS measurement active. |
| **[ / ]** | Rotate sun azimuth by -/+15 degrees per press; hold does not repeat. |
| **R** | Restore the approved default sunlight direction. |
| **F6** | Toggle local real-time sunlight shadows (enabled initially). Holding the key toggles only once. |
| **F7** | Toggle final tone mapping/color balance (enabled initially), independently of FXAA. |
| **F8** | Toggle FXAA edge anti-aliasing (enabled initially). Both F7 and F8 OFF restore the original G7B output. |
| **F9** | Switch between the default cinematic sky and optional anime sky. |
| **1 / 2 / 3** | Teleport to Baiterek / Talan Towers / Khan Shatyr. |
| **5** | Teleport to Ak Orda. |
| **6** | Teleport to the Botanical Garden's east entrance, facing its center. |
| **7** | Teleport to EXPO, facing Nur Alem and its east entrance. |
| **8** | Teleport to MEGA Silk Way, facing its main entrance. |
| **9** | Teleport to the Esil western promenade / Atyrau Bridge approach, facing across the river. |
| **ESC** | Exit the application. |

Normal movement is 5 m/s, sprint is 15 m/s, fast flight is 25 m/s, and turbo flight is 50 m/s. Movement is frame-time based and combined horizontal/vertical input is normalized. Walking stays at a 3 m eye height above the ground after the smooth return from flight. On Atyrau Bridge it follows the raised deck at the same eye height; the existing flat-ground behavior elsewhere is unchanged. Mouse look is unchanged. The window title retains FPS and average frame time and shows the movement mode, speed and shadow toggle state, updated about once per second.

G6 uses one local 2048² sunlight shadow map. See [Graphics G6A](docs/GraphicsG6A.md) for the initial scope, [Graphics G6B](docs/GraphicsG6B.md) for filtering/stability, and [Graphics G6C](docs/GraphicsG6C.md) for nearby landmark and vegetation casters. F6 toggles the whole shadow pass.

[Graphics G7C](docs/GraphicsG7C.md) adds a mild, global final-image adjustment. [Graphics G8A](docs/GraphicsG8A.md) integrates FXAA into that same composite, independently controlled by F8. Both F7 and F8 OFF bypass the scene buffer and composite. The title shows F6, F7 and F8 states.

The Esil River / Atyrau Bridge district occupies the **positive-X Baiterek side**, away from EXPO and MEGA. It includes recessed water and sloped banks, two stone promenades, the curved Atyrau Bridge with its 818-panel lattice canopy, riverside planting, urban furniture, four viewing terraces, garden paths and flowerbeds. Key 9 visits the western bridge approach. See [the district documentation](docs/EsilRiverDistrict.md) for coordinates, geometry and verification details.

## Submission Readiness

See [the interactive requirements verification](docs/FinalRequirements.md) for the implemented controls, diagnostics and validation. Official project identification is maintained in `src/ProjectInfo.h` and displayed in the HUD and help overlay. The exact course topic sheet has not been reviewed; topic-specific technique compliance remains unverified.
