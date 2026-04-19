# Aircraft Simulator 🛫

A comprehensive C++ and OpenGL-based flight simulator featuring custom shaders, physics, atmospheric rendering, and full gamepad support.

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![C++](https://img.shields.io/badge/C++-17-blue.svg)](https://isocpp.org/)
[![OpenGL](https://img.shields.io/badge/OpenGL-3.3+-green.svg)](https://www.opengl.org/)

---

## 📸 Gallery

<p align="center">
  <img src="docs/screenshots/gameplay1_placeholder.png" width="48%" alt="Gameplay image 1 showing the aircraft over terrain">
  <img src="docs/screenshots/gameplay2_placeholder.png" width="48%" alt="Gameplay image 2 showing the HUD and cockpit view">
</p>

> **Note:** *(Replace the image paths above with actual screenshot links after capturing them in-game!)*

---

## ✨ Features

- **Advanced Flight Physics**: Realistic pitch, yaw, roll, and throttle mechanics. Features dynamic landing sequences, including crash detection if you attempt a belly landing with your gear up.
- **XInput Gamepad Support**: Plug-and-play support for Xbox controllers, featuring smooth analog stick handling and deadzone management.
- **Custom Shader Pipeline**: Hybrid rendering combining legacy fixed-function operations with modern vertex and fragment shaders for depth, atmospheric scattering, and lighting.
- **Dynamic Terrain & Sky**: Procedurally generated environments, skyboxes, and atmospheric fog integrating with the time of day.
- **Assimp Model Loading**: Support for loading complex 3D aircraft models seamlessly into the graphics pipeline.
- **Modular Architecture**: Clean, scalable, decoupled codebase segregating physics, rendering, UI, and world rules.

---

## 📂 Project Structure

Following a recent architecture overhaul, the codebase is divided into clear functional domains:

```text
├── bin/          # Compiled executables and required DLLs
├── core/         # Core engine loops, math utilities, and state globals
├── docs/         # Deep-dive architecture and execution documentation
├── flight/       # Flight physics logic, Jet rendering, and Camera handling
├── graphics/     # Shader compilation, model loaders (Assimp), and shadows
├── planes/       # 3D Jet models and configuration text files
├── shaders/      # GLSL Vertex and Fragment shaders
├── tests/        # Internal modular tests (e.g., XInput testing)
├── ui/           # Heads-Up Display (HUD) and Main Menu rendering
└── world/        # Environment elements: Atmosphere, Sky, and Terrain
```
*(For a deeper dive into the system logic, refer to `ARCHITECTURE.md` and `docs/EXECUTION_INSTRUCTIONS.md`)*

---

## 🛠️ Prerequisites & Building

This project is configured to be built on Windows using the **MSYS2 (MinGW-w64)** environment.

### Dependencies
- **GCC / G++**: C++ compiler
- **Make**: Build automation
- **FreeGLUT**: Window and context management
- **Assimp**: 3D model importing
- **XInput**: Gamepad controller API (Windows native)

### Build Instructions

1. Open your **MSYS2 MinGW 64-bit** terminal.
2. Navigate to the project directory:
   ```bash
   cd /c/path/to/your/Aircraft_Simulator/curr
   ```
3. Compile the project using explicitly configured Makefiles:
   ```bash
   make clean
   make
   ```
4. Run the generated executable:
   ```bash
   ./bin/voxel_flight.exe
   ```

*(Alternatively, you can build directly via the integrated `Makefile` using your preferred IDE task runner).*

---

## 🎮 Controls

### Keyboard
| Action | Key |
| :--- | :--- |
| **Throttle Up / Down** | `W` / `S` |
| **Pitch Up / Down** | `Up Arrow` / `Down Arrow` |
| **Roll Left / Right** | `Left Arrow` / `Right Arrow` |
| **Yaw Left / Right** | `A` / `D` |
| **Toggle Landing Gear** | `G` |
| **Change Camera View** | `C` |
| **Pause/Menu** | `Esc` |

### Gamepad (Xbox Controller)
| Action | Controller Input |
| :--- | :--- |
| **Throttle Up / Down** | `Right Trigger (RT)` / `Left Trigger (LT)` |
| **Pitch & Roll** | `Right Stick (Y-axis)` & `Left Stick (X-axis)` |
| **Yaw Left / Right** | `Left Bumper (LB)` / `Right Bumper (RB)` |
| **Toggle Landing Gear** | `D-Pad Down` |
| **Change Camera View** | `Y Button` |
| **Pause/Menu** | `Start Button` |

*(Refer to the in-game Controls menu for a complete and up-to-date mapping).*

---

## 🤝 Contributing

Contributions, issues, and feature requests are welcome! 
Feel free to check [issues page](https://github.com/Tanishq-Godha/Aircraft_Simulator/issues).

1. Fork the Project
2. Create your Feature Branch (`git checkout -b feature/AmazingFeature`)
3. Commit your Changes (`git commit -m 'Add some AmazingFeature'`)
4. Push to the Branch (`git push origin feature/AmazingFeature`)
5. Open a Pull Request

---

## 📝 License

Distributed under the MIT License. See `LICENSE` for more information.
