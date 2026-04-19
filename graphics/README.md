# Graphics Module (Pipelines & Render Abstracts)

This `graphics/` directory encapsulates lower-level rendering features. Instead of cluttering the gameplay loops with raw unmanaged OpenGL pointers, it extracts the heavy integrations of `.glb` meshes, real-time cascaded shadows, and shader processing pipelines safely into distinct components.

## Detailed File Breakdown

### 1. `model_loader.cpp` / `model_loader.h`
Integrates the Open Asset Importer Library (Assimp) bridging the external `.glb`, `.gltf`, or `.obj` assets logically into structured OpenGL draw calls.
*   **Scene Parsing**: Maps directly onto Assimp’s hierarchical Node tree (`aiNode` to `aiMesh`), iteratively processing indices, vectors (`mVertices`, `mNormals`), and materials (`mMaterials`).
*   **Scale Restoration**: Features recursive functions `calculateNodeBounds()` to generate absolute Min/Max vertices determining a bounding box scale dynamically ensuring externally downloaded aircraft match perfectly against the game world sizes natively.
*   **Embedded Texture Extraction**: Interfaces raw binary memory using `stb_image.h`, translating `aiTexture` blobs into directly compatible OpenGL active Texture IDs (`loadEmbeddedTexture`), effectively bypassing manual image files by reading directly from self-contained `.glb` structures.
*   **Hierarchical Render**: Generates dynamic VBO equivalents via immediate mode buffers `glBegin()` rendering faces seamlessly using global `LoadedModel` states efficiently tracking texture bounds.

### 2. `shader_loader.cpp` / `shader_loader.h`
Upgrades the traditional Fixed-Function graphics by binding modern GPU Shading units seamlessly across OS architectures.
*   **IO Streams**: Reads physical `.vert` and `.frag` scripts from the `shaders/` directory mapping strings accurately to GPU queues.
*   **Windows Extensions**: Dynamically loads complex `PFNGL` function pointers directly via `wglGetProcAddress`, bypassing the necessity manually including gigantic wrapper libraries (like GLEW/GLAD) achieving extremely lightweight linkage for core commands like `glCreateShader` and `glCompileShader`.
*   **Error Handler Pipeline**: Catches verbose native `glGetShaderInfoLog` strings pumping GPU compile failures or link errors robustly back into `stdout` for fast graphical debugging.

### 3. `shadow_system.cpp` / `shadow_system.h`
Introduces an internal multi-pass directional cascaded Framebuffer Object (`FBO`) technique, generating soft, volumetric-style shading across the game meshes without heavy raytracing.
*   **FBO Initializer**: Sets up standard OpenGL framebuffer extensions mapped exactly onto heavily allocated texture structures (`GL_TEXTURE_2D, GL_DEPTH_COMPONENT24`). Checks explicitly if the bound map is physically complete.
*   **Depth Pass Mechanism**: Calculates raw `gluOrtho2D` cascade planes mapping them onto a precise location around `planeX / planeY`. Binds an initial viewport, rendering the full scene layout from the directional angle of the virtual sun.
*   **Bias and Main Pass Matrix**: Solves floating-point precision shadow acne via internal matrix manipulations. Multiplies the active `CameraMatrix` coordinates against the previously solved depth buffer and interpolates the depth test in the main `main_shaded.frag` file.

### 4. `stb_image.h`
A lightweight, single-header image decoder library.
*   **Purpose**: Provides standard routines for loading uncompressed memory vectors mapped out from the `model_loader.cpp` (JPEG/PNG decoding) natively skipping over the complexities of manually extracting file headers and compressed pixel rows from OS implementations.