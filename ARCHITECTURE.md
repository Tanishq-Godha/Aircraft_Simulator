# Architecture Overview

This project is a flight simulator built with OpenGL. The codebase is organized into modular directories separated by their domains of responsibility.

## Directory Structure

*   **`bin/`**: Contains compiled executables and required dynamic-link libraries (DLLs).
*   **`core/`**: Central initialization, state management, mathematical utilities, and user input mapping.
*   **`flight/`**: The core flying mechanics, including the camera systems, jet models and animations, and rigorous flight physics calculations.
*   **`graphics/`**: Abstractions required for visual rendering, such as shader loading, shadow mapping systems, and loading `.glb`/`.obj` 3D assets using Assimp.
*   **`world/`**: Everything related to the environment surrounding the jet - from sky weather profiles and dynamic atmosphere colors to voxel-based terrain/city generation.
*   **`ui/`**: User interface components including the in-cockpit Heads-Up Display (HUD) and complex menus (plane selection, map selection).
*   **`planes/`**: 3D assets (`.glb` files) of external community planes.
*   **`shaders/`**: GLSL shaders (Vertex, Fragment) required for graphics.
*   **`tests/`**: Contains experimental or debugging code (e.g., input connection test).
*   **`docs/`**: Feature documentations and task tracking docs.

## Entry Point
*   **`main.cpp`**: Contains the OpenGL `display()` and `mainLoop()` routines. Responsible for initializing components and driving the main physics and rendering loop loops at a steady dt.
*   **`Makefile`**: Compiles the source files from their respective sub-directories into `bin/voxel_flight.exe`.