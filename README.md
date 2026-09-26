# ICP OpenGL Project

Real-time 3D application using **OpenGL 4.6 Core Profile**, GLSL shaders, GLFW, GLEW, GLM, ImGui, OpenCV and JSON configuration.

The project was created for the assignment **Create a multiplatform real-time 3D application**.

## Origin and contributions

This course project started from a school-provided code skeleton; some source files still carry an `author: JJ` header from that starting point. The commits in this repository document my later work on resource loading, camera controls and transformations, lighting, multiple scene objects, collisions, transparency, particles, and documentation. The author header does not describe authorship of every later feature.


---

## Implemented features

### Essentials

- OpenGL **4.6 Core Profile**
- GLSL shader-based rendering
- OpenGL debug context and debug callback
- JSON configuration file
- FPS display
- VSync control
- MSAA antialiasing
- Fullscreen/windowed switching with restore of previous window size and position
- Keyboard input
- Mouse input:
  - mouse movement on both axes
  - mouse wheel FOV zoom
  - mouse buttons
- Multiple independently moving 3D models
- OBJ models loaded from files
- Texture atlas / subtextures
- Additional texture loaded from file
- Lighting model:
  - ambient light
  - directional light
  - multiple point lights
  - spotlight / reflector
  - moving lights
- Custom shader effect:
  - distance fog
- Full alpha transparency:
  - transparent objects are rendered separately after opaque objects
  - transparent objects are sorted from far to near
  - no alpha discard is used
- Collision system:
  - map boundaries
  - player/object collisions
  - sliding along obstacles
  - push-out for moving objects
- Particle effects:
  - collision particle burst
  - trail behind moving object

### Extras

- Particle effects

---

## Requirements

### Required tools

- Visual Studio 2022 or Build Tools for Visual Studio 2022
  - Desktop development with C++
  - MSVC compiler
  - Windows SDK
- CMake 3.28 or newer
- Ninja
- Git
- GPU and driver supporting:
  - OpenGL 4.6 Core Profile
  - OpenGL Direct State Access

### Libraries

The project uses:

- GLFW
- GLEW
- GLM
- nlohmann/json
- ImGui
- OpenCV

All libraries are installed through **vcpkg manifest mode** using `vcpkg.json`.

---

## Tested environment

The project was tested on:

- Windows 10/11
- Visual Studio 2022 / MSVC
- CLion
- CMake
- Ninja
- vcpkg
- NVIDIA GPU with OpenGL 4.6 support

---

## Important platform note

The source code uses multiplatform libraries such as CMake, GLFW, GLEW, GLM, OpenCV and ImGui.

However, the assignment requires **OpenGL 4.6 Core Profile** and **Direct State Access**. Some systems may not expose OpenGL 4.6. For example, macOS usually exposes only older OpenGL versions, so the project may not run there without reducing the OpenGL/GLSL version and replacing unsupported functionality.

---

## Project structure

Expected structure:

```txt
PG2/
├── CMakeLists.txt
├── CMakePresets.json
├── main.cpp
├── app.cpp
├── app.hpp
├── Mesh.hpp
├── Model.hpp
├── Texture.cpp
├── Texture.hpp
├── ShaderProgram.cpp
├── ShaderProgram.hpp
├── OBJloader.cpp
├── OBJloader.hpp
├── gl_utils.cpp
├── gl_utils.h
├── gl_err_callback.cpp
├── gl_err_callback.h
├── glfw_callbacks.cpp
├── glfw_callbacks.h
├── assets.hpp
├── non_copyable.hpp
├── vcpkg.json
└── resources/
    ├── config.json
    ├── shaders/
    ├── models/
    ├── textures/
    └── screenshots/
```

The application expects the `resources/` directory to be available relative to the project root.

---

## Windows installation and build

The project uses **vcpkg manifest mode**. Dependencies are declared in `vcpkg.json` and installed from the project root.

### 1. Install required tools

Install:

- Visual Studio 2022 or Build Tools for Visual Studio 2022
- CMake 3.28 or newer
- Ninja
- Git

The project should be built as an **x64** application.

Recommended terminal:

```txt
x64 Native Tools Command Prompt for VS 2022
```

---

### 2. Install vcpkg inside the project directory

From the project directory:

```cmd
cd C:\path\to\project
git clone https://github.com/microsoft/vcpkg.git
cd vcpkg
bootstrap-vcpkg.bat
```

Set `VCPKG_ROOT`:

```cmd
setx VCPKG_ROOT C:\path\to\project\vcpkg
```

After setting the environment variable, restart the terminal or IDE.

---

### 3. Install dependencies

From the project root:

```cmd
cd C:\path\to\project
.\vcpkg\vcpkg.exe install --triplet x64-windows
```

The first installation may take longer because OpenCV is a larger dependency.

---

### 4. Dependency manifest

The project dependencies are declared in `vcpkg.json`:

```json
{
  "dependencies": [
    "glew",
    "glfw3",
    "glm",
    "nlohmann-json",
    {
      "name": "imgui",
      "features": [
        "glfw-binding",
        "opengl3-binding"
      ]
    },
    "opencv4"
  ]
}
```

---

### 5. CMake configuration

The project uses a non-hardcoded OpenCV lookup through vcpkg:

```cmake
find_package(OpenCV CONFIG REQUIRED)
```

Relevant part of `CMakeLists.txt`:

```cmake
cmake_minimum_required(VERSION 3.30)

project(icp VERSION 2025.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_BUILD_TYPE Debug)

find_package(glm           CONFIG REQUIRED)
find_package(GLEW                 REQUIRED)
find_package(glfw3         CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(imgui         CONFIG REQUIRED)
find_package(OpenCV        CONFIG REQUIRED)
```

---

### 6. Configure the project

Open **x64 Native Tools Command Prompt for VS 2022**.

Check selected compiler:

```cmd
where cl
echo %VSCMD_ARG_TGT_ARCH%
```

The output should contain:

```txt
Hostx64\x64
x64
```

Configure from the project root:

```cmd
cd C:\path\to\project

cmake -S . -B build -G Ninja "-DCMAKE_TOOLCHAIN_FILE=C:/path/to/project/vcpkg/scripts/buildsystems/vcpkg.cmake" "-DVCPKG_TARGET_TRIPLET=x64-windows"
```

Replace:

```txt
C:/path/to/project
```

with the actual project path.

Example:

```cmd
cmake -S . -B build -G Ninja "-DCMAKE_TOOLCHAIN_FILE=C:/Users/username/source/repos/PG2/vcpkg/scripts/buildsystems/vcpkg.cmake" "-DVCPKG_TARGET_TRIPLET=x64-windows"
```

---

### 7. Build the project

From the project root:

```cmd
cmake --build build --target icp -j 8
```

---

### 8. Run the application

Run the executable from the **project root directory**, because the application loads files from the `resources/` directory.

Example:

```cmd
.\build\icp.exe
```

If the executable is generated in another folder, run the correct executable path, but keep the working directory as the project root.

---

## Build and run from an IDE

The command-line build above is the reference setup.

When using CLion or another IDE, use these settings:

### CMake toolchain file

```txt
C:/path/to/project/vcpkg/scripts/buildsystems/vcpkg.cmake
```

### CMake option

```txt
-DVCPKG_TARGET_TRIPLET=x64-windows
```

### Working directory

Set the working directory to the project root:

```txt
C:/path/to/project
```

### Target

Build and run target:

```txt
icp
```

---

## Controls

| Key / Input | Action |
|---|---|
| W | Move forward |
| S | Move backward |
| A | Move left |
| D | Move right |
| Space | Move up |
| Left Shift | Move down |
| Mouse movement | Camera look |
| Mouse wheel | Change FOV / zoom |
| Right mouse button | Release cursor |
| Tab | Capture/release cursor |
| Escape | Release cursor first, quit on second press |
| F1 | Show/hide ImGui HUD |
| F2 | Save screenshot |
| F3 | Save screenshot without MSAA |
| F4 | Save screenshot with MSAA |
| F11 | Toggle fullscreen/windowed |
| V | Toggle VSync |
| M | Toggle MSAA |

---

## Configuration file

The application reads:

```txt
resources/config.json
```

Example:

```json
{
  "window": {
    "width": 1280,
    "height": 720,
    "title": "ICP OpenGL Project",
    "vsync": true,
    "msaa_samples": 4,
    "fullscreen": false
  },
  "camera": {
    "speed": 2.5,
    "fov_deg": 60.0,
    "znear": 0.1,
    "zfar": 100.0
  },
  "rendering": {
    "clear_color": [0.08, 0.08, 0.10, 1.0]
  }
}
```

If the config file is missing or invalid, default values from the application are used.

---

## Possible issues

### vcpkg manifest mode error

If vcpkg reports that individual package arguments are not supported, run the install command from the project root:

```cmd
.\vcpkg\vcpkg.exe install --triplet x64-windows
```

Dependencies are read from `vcpkg.json`.

---

### x86 / x64 mismatch

If CMake reports paths containing:

```txt
Hostx86\x86
```

or linker errors such as:

```txt
library machine type 'x64' conflicts with target machine type 'x86'
```

then the project was configured with the wrong compiler architecture.

Open:

```txt
x64 Native Tools Command Prompt for VS 2022
```

Delete the build directory:

```cmd
rmdir /s /q build
```

Then configure again:

```cmd
cmake -S . -B build -G Ninja "-DCMAKE_TOOLCHAIN_FILE=C:/path/to/project/vcpkg/scripts/buildsystems/vcpkg.cmake" "-DVCPKG_TARGET_TRIPLET=x64-windows"
```

---

### OpenCV build takes a long time

OpenCV is installed through vcpkg as `opencv4`.

The first installation can take a long time because vcpkg may need to download and build OpenCV and its dependencies.

---

### OpenCV not found

Check that `opencv4` is listed in `vcpkg.json`.

Then run:

```cmd
.\vcpkg\vcpkg.exe install --triplet x64-windows
```

Delete the build directory and configure again with the vcpkg toolchain file.

---

### Error: `gl.h included before glew.h`

The project defines:

```cmake
GLFW_INCLUDE_NONE
```

This tells GLFW not to include OpenGL headers. GLEW is responsible for OpenGL function loading.

If the error appears again, check the include order in source/header files.

---

### Error: shader files missing

Check that the working directory is the project root and these files exist:

```txt
resources/shaders/point2.vert
resources/shaders/point_or_directional2.frag
```

---

### Error: OBJ file missing

Check that model files are available in:

```txt
resources/models/
```

---

### Error: texture file missing

Check that texture files are available in:

```txt
resources/textures/
```

---

## Repository notes

Recommended files to commit:

```txt
CMakeLists.txt
vcpkg.json
README.md
source files
resources/
```

Build/dependency folders should stay local:

```txt
vcpkg/
vcpkg_installed/
build/
cmake-build-*/
out/
.vs/
.idea/
```
