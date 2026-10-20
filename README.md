# Astana3DTour

C++17 / OpenGL 3.3 Core foundation: 1280x720 window, dark background, checker ground plane, depth testing, perspective camera, and separate GLSL shaders. No city objects yet.

## Reused dependencies

Uses workspace-local include/GLFW, lib/glfw3.lib, src/glad.c, include/glad/glad.h and include/KHR/khrplatform.h. GLAD 0.1.36 is generated for OpenGL 3.3 Core. Existing files are not modified. No dependencies are downloaded or installed. All dependencies are preserved inside this project root. No additional math library is needed.

The GLFW archive references MSVCRTD, so this configuration supports MSVC x64 Debug (/MDd) only. Release would require a matching Release GLFW library and CMake configuration changes.

## Build with Visual Studio 2022

Use the existing C++ desktop toolchain, Windows SDK and CMake. From this directory in Developer PowerShell for VS 2022:

```powershell
cmake --preset x64-debug
cmake --build --preset x64-debug
.\build\Debug\Astana3DTour.exe
```

Alternatively open build/Astana3DTour.sln, select Debug / x64, set Astana3DTour as startup project, and build/run. If cmake is not on PATH, use the CMake executable bundled with Visual Studio. CMake copies shaders beside the executable and sets the debugger working directory. Rebuild after editing shaders. Runtime requires a graphics driver supporting OpenGL 3.3.

## Controls

- W/S: walk forward/backward parallel to the ground.
- A/D: strafe left/right. Speed: 5 world units per second.
- Mouse: look around with a captured cursor.
- ESC: close.

Camera starts at (0,3,8), looking slightly downward at a 100x100 ground plane. Resizing updates the viewport and perspective; minimized rendering pauses. GLFW and shader errors are printed to the console.

## Build verification

Configured and clean-built from the workspace root with Visual Studio 2022, MSVC 19.44, Windows SDK 10.0.26100.0, x64 Debug, and C++17. Configuration and build exited successfully. All required dependency files and original project sources/shaders were preserved byte for byte. Shader copies beside the executable were verified. The GLFW archive emits 21 LNK4099 warnings because glfw3.pdb is absent; linking succeeds, but debugging inside GLFW is limited. Interactive rendering and controls have not been manually verified.
