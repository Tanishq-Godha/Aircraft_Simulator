# Core Module (System Initialization & State)

The `core/` directory acts as the central nervous system of the simulator. It initializes the application, stores globally accessible states, manages hardware I/O mapping, and provides basic mathematical utilities used across all other submodules.

## Detailed File Breakdown

### 1. `globals.cpp` / `globals.h`
This acts as the single source of truth for the simulator's state machine. Because the project spans multiple deeply disconnected systems (physics, rendering, UI, weather), they synchronize via `external` variable links defined here.
*   **Flight States**: Variables like positional data (`planeX`, `planeY`, `planeZ`), velocity vectors (`vX`, `vY`, `vZ`), and rotational configurations (`pitch`, `yaw`, `roll`). 
*   **Engine & Mechanism States**: Tracks `throttle`, `currentSpeed`, `gearDeployed`, `suspension` depth, `wheelRotation` RPM, and `crashTimer`.
*   **World Flags**: Holds the master `deltaTime` (vital for framerate-independent physics), `gameTime` for continuous day/night rotations, `weatherMode`, and overarching `gameState` (0 = Menu, 1 = App).

### 2. `init.cpp` / `init.h`
This file is executed immediately when the OpenGL context bounds to the display window. 
*   **Pipeline Setup**: Enacts base configurations for the fixed-function OpenGL pipeline: enabling `GL_DEPTH_TEST`, depth buffer routines, normal homogenization (`GL_NORMALIZE`), and alpha transparency blending equations.
*   **Fog Integration**: Registers the linear fog distances (`GL_FOG_START`, `GL_FOG_END`) rendering distant geometries seamlessly into the skybox limits. 
*   **Pointers & Overrides**: Sets up fundamental drawing structures like native `GLUquadric` memory allocations (the underlying shapes used to render the procedural geometries). Hooks the `ShadowSystem` initial cascades early within the startup trace.

### 3. `input.cpp` / `input.h`
Handles continuous hardware polling from the user to convert raw inputs into logical airplane state modifications.
*   **Keyboard Callback Hooks**: Intercepts `glutKeyboardFunc` & `glutSpecialFunc`. Stores inputs in dual boolean arrays (`keys[256]` & `specialKeys[256]`) to decouple press animations from sequential polling rates.
*   **Direct In-Game Macros**: Maps keys like `P` to reset engine pointers, `L` to activate the Auto-Land trigger checks, `V` for camera perspectives, `G` for landing gear operations, and `W/A/S/D` matrices mapped sequentially inside the active run-loop.
*   **Gamepad Telemetry**: Leverages the `XInput` Microsoft APIs for Xbox controllers. Handles controller mapping such as analog trigger ranges (`0 - 255`) decoded smoothly into continuous `throttle` changes, interprets controller thumbstick dead-zones to interpolate joystick offsets to `pitchTarget` & `targetRoll`, and maps face-buttons to specific game states.

### 4. `math_utils.h`
A strict, universally included inline header optimizing heavy mathematical load formulas running inside sequential loops.
*   **Constants**: Hard variable scopes for `PI` (3.14159265f) and simulated relative `GRAVITY` indexes mapping real-world drag down to the OpenGL scale. 
*   **Interpolation**: Features quick scalar line equations (`lerp`, `mixf`) frequently requested by camera dampening hooks and suspension regressions.
*   **Clamps & Radians**: Safe guarders preventing variable overflows (`clampf`) utilized predominantly in airspeed checks (`maxSpeed`), and fast macro trigonometric conversions (`degToRad`) essential for mapping rotational models into transformation matrices gracefully.