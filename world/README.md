# World Module (Terrain, Atmospherics & Clouds)

The `world/` module defines the external environment that the aircraft operates within. It spans procedural ground chunking algorithms over infinitely scrolling spaces, weather simulations dictating global visual constants, and skybox volumetrics utilizing pseudo-random noise fields.

## Detailed File Breakdown

### 1. `sky.cpp` / `sky.h`
Handles complex sky layering techniques, multi-layered cloud rendering, and specific transitional weather profiles governing the game visual state recursively.
*   **Procedural Cloud Volumes**: Incorporates low-overhead Cellular (Worley) distance noise calculations generating the primary cauliflower aesthetic of clouds natively without textures. Also couples basic multi-octave hashing over grids.
*   **Volumetric Puff Algorithms**: Employs `drawCloudPuff()` looping billboarding primitives (spheres stretched against X/Z vectors with manipulated normals mapping gradients). Translates them dynamically using `drawSky()`, generating overlapping layers scaling over distance matching internal transparency fields based strictly upon atmospheric altitude constraints.
*   **Weather Profiler**: Returns rigorous `WeatherProfile` data struct variations based on simple internal parameters (like "Cloudy", "Clear", "Storm"). Configures parameters for external dependencies (`windZBase`, `haze`, `alpha`, `sunBoost`, `fogStart`, `minPuffs`).

### 2. `atmosphere.cpp` / `atmosphere.h`
Computes light vectors and background rendering formulas based strictly on procedural time calculations determining physical day-night mechanics over the world rendering boundaries.
*   **Sun & Day/Night Tracking**: Uses `gameTime` mathematically iterating from 0 to 24 calculating rotational `elevation` and explicit directional outputs returning the active sun position cleanly (`getSunDirection`), while toggling automatically to starfields and moonlighting states during night cycles.
*   **Interpolated Sky Vectors**: Calls `setupSkyClearColor()`, continuously shifting RGB floats interpolating specific gradients mapping colors from morning blues through sunset oranges cleanly over delta-time constants natively calling `glClearColor`.
*   **Environmental Fog Integration**: Extends `setupAtmosphericFog()` mapping densities corresponding to global height arrays smoothly fading 3D horizons seamlessly simulating real-time haze, removing brutal mesh clipping artifacts on distant bounds correctly matching the internal active `WeatherProfile`.

### 3. `terrain.cpp` / `terrain.h`
Generates the interactive topological ground features continuously beneath the moving camera and aircraft vectors mapping physical responses against objects rendered physically onto the active view.
*   **Noise Mechanics & Grids**: Leverages a custom Perlin-style noise permutation system chunking coordinates natively determining base land heights ensuring `getVoxelHeight()` matches perfectly physically under the procedural mapping visually mapped dynamically ahead in `drawVoxelTerrain()`.
*   **Airport Integration**: Handles strict boundary limits via specific checks (`isRunway`, `isTaxiway`, `isServiceRoad`) carving completely flat planes across designated mathematical boundaries guaranteeing perfect landings for the physics detector overriding default height permutation loops securely with distinct base geometries.
*   **City & Skyscraper Layouts**: Defines internal routines chunking `doesBuildingRootSpawn()` plotting building generation matrices using internal seeds. Evaluates sizes via `drawBuilding()` creating custom colored and randomized structural skyscraper sizes dynamically. Includes specialized procedural landmarks (like `TwinTowers`) generated specifically across active flight coordinate vectors via explicit user mapping.