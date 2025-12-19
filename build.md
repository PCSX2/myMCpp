# Building myMCpp

## Prerequisites

### General
*   **C++ Compiler**: C++20 compatible (MSVC, Clang).
*   **CMake**: Version 3.16 or newer.
*   **Qt6**: Core, Gui, and Widgets modules.

### Windows
*   **Visual Studio 2022** (Desktop development with C++).
*   **Vulkan SDK**: For Vulkan headers and loader.
*   **Qt6**: Install via the Qt Online Installer (select MSVC 2022 64-bit). 
<!-- I want to build Qt6 from source using our deps script cmake I just haven't got that far yet. -->

### macOS
<!-- TODO -->

### Linux 
<!-- only tested on Fedora you'll need to figure out the package names for your distribution yourself Kam :p -->
*   **Development Tools**: `cmake, clang, ninja`.
*   **Qt6**: `qt6-qtbase-devel qt6-qtbase-private-devel`.
*   **Vulkan**: `libvulkan-dev`, `vulkan-headers`.
*   **X11**: `libx11-dev`, `libXrandr-dev`.
*   **Wayland**: `wayland-devel/wayland-scanner`.
*   **Xinerama**: `libXinerama-devel`.
*   **libxcursor**: `libXcursor-devel`.
*   **libXi**: `libXi-devel`.
*   **glslc**: `glslc`.

## Build Instructions

### Windows (Visual Studio)

1.  **Clone the repository**:
    ```powershell
    git clone https://github.com/SternXD/myMCpp.git
    cd myMCpp
    ```

2.  **Configure with CMake**:
    You can use the Visual Studio "Open Folder" feature, or command line:
    ```powershell
    mkdir build
    cd build
    cmake .. -DCMAKE_PREFIX_PATH="C:/Qt/6.10.1/msvc2022_64"
    ```
    *Replace the path with your actual Qt installation path.*

3.  **Build**:
    ```powershell
    cmake --build . --config Release --parallel
    ```

4.  **Run**:
    The executable will be in `build/bin/Release/myMCpp.exe`.
    *Note: The build process automatically deploys the Qt DLLs.*

### macOS

<!-- TODO -->

### Linux

1.  **Clone the repository**:
    ```bash
    git clone https://github.com/SternXD/myMCpp.git
    cd myMCpp
    ```

2.  **Build**:
    ```bash
    mkdir build && cd build
    ```
    ```bash
    cmake .. \
    -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_C_COMPILER=clang \
    -DCMAKE_CXX_COMPILER=clang++
    ```
    
    ```bash
    cmake --build . --parallel --config Release
    ```

3.  **Run**:
    Executable at `build/bin/myMCpp`.

## Troubleshooting

*   **CMake can't find Qt6**: Make sure `CMAKE_PREFIX_PATH` points to the `lib/cmake` parent directory (the compiler specific folder, e.g., `msvc2022_64` or `gcc_64`).
*   **Vulkan headers missing**: Make sure the Vulkan SDK is installed. The specific renderer used (Vulkan/OpenGL/Metal) is chosen at runtime, but all backends are currently compiled.
