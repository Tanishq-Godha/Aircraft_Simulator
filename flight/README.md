# Flight Module (Aerodynamics & Aircraft Rendering)

The `flight/` directory coordinates the core interactivity of playing the flight simulator. From applying thrust and lift drag algorithms inside the physics loop, to rendering the external jet visuals and directing the virtual camera perspectives.

## Detailed File Breakdown

### 1. `physics.cpp` / `physics.h`
The flight dynamics engine runs the heavy lifting of evaluating global simulation offsets continuously over the master `deltaTime`. 
*   **Aerodynamic Solvers**: Calculates the `sinkRate`, dynamically reads the `throttle`, and applies varying forces like speed bleed due to aggressive pitch sweeps or un-retracted landing gear drag values. Detects complex behaviors like `isStalling` via threshold constraints on Forward-Z calculations vs upward lift.
*   **Collision Detectors**: Interfaces tightly with `terrain.h` natively reading `getVoxelHeight()` directly underneath the global aircraft vectors checking for imminent impacts.
*   **Landing Evaluator**: Tracks whether the impending ground hit maps on an active `isRunway()`, dynamically checking `gearAnimation` and `currentSpeed` variables. Successfully separates events into Safe Touchdowns, Hard Touchdowns (triggering suspension bounce feedback), or Belly Landings / Complete Explosions scaling `cameraTrauma` directly.
*   **Afterburner & Control Systems**: Maps the physics array integrating the Autopilot (`autopilotAlt` ceiling capture logic) and executing automatic safe descent procedures if `autoLandOn` is activated under valid safe landing zones.

### 2. `jet.cpp` / `jet.h`
Maintains the internal rendering hierarchy and custom animations applied strictly to the user’s aircraft.
*   **Procedural Geometry Builder**: Instead of exclusively relying on external assets, natively hosts an advanced fallback procedural mesh construction looping together structured `glutSolidCube`, `gluCylinder`, and `gluDisk` routines stacked precisely on an OpenGL transformation matrix, creating cockpits, wings, and animated engines natively in C++.
*   **Control Surface Modifiers**: Reads the relative physics inputs targeting `aileronAngle`, `elevatorAngle`, and `rudderAngle` translating them directly to rotation coordinates for visual feedback animating the airplane control surfaces correctly along hinges (`drawHingedSurface`).
*   **Gear Mechanism**: Operates specific logic opening the belly `GearBayCavity`, dropping the struts continuously on the `gearAnimation` curve, deploying wheels mapping suspension offsets and continuous rolling RPM based on simulated ground speeds. 
*   **Particles & Crashes**: Hosts the `drawExplosionAndSparks()` mapping physics limits over fragments spanning outward recursively on crash triggers creating a 3D explosion effect.

### 3. `camera.cpp` / `camera.h`
Takes the spatial coordinate variables from the airplane and transforms them into valid View Matrices (`gluLookAt`). 
*   **Follow System**: Implements a robust lagging "chase camera". Uses simulated mathematical dampening, interpolation, and spring formulas dragging the camera dynamically behind the jet. The camera slightly pans out or trails further behind to emphasize intense velocity pushes.
*   **Matrix Orientations**: Calculates Forward, Up, and Right basis vectors to bind internal cameras (Cockpit Mode 1) rigidly to the interior vertices, guaranteeing the `pitch` & `roll` rotate precisely with the craft frame.
*   **Shaker Feedback**: Implements algorithmic noise logic attached to `cameraTrauma`, mapping raw vibrational camera shakes across vectors corresponding exactly to crash speeds, runway drag, or explosive scenarios.