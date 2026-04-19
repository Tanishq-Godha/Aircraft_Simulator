# Aircraft Model Loading System - Setup Guide

## Overview

Your flight simulator now supports loading custom aircraft models from `.glb` files! This allows you to replace the default procedural jet with 3D models from external files.

## Prerequisites

Make sure you have the following libraries installed:

1. **Assimp** (Asset Importer Library) - for loading GLB/FBX models
   - Windows: Download from https://github.com/assimp/assimp/releases
   - Or use MinGW package manager: `pacman -S mingw-w64-x86_64-assimp`

2. **FreeGLUT** (already installed in your setup)

## Installation Steps

### 1. Install Assimp (MinGW-w64)

```bash
# If using MinGW package manager
pacman -S mingw-w64-x86_64-assimp mingw-w64-x86_64-assimp-docs
```

Or compile from source:
- Download from: https://github.com/assimp/assimp
- Follow build instructions for Windows/MinGW

### 2. Update Compilation

The Makefile has already been updated to link `-lassimp`. Just recompile:

```bash
make clean
make
```

### 3. Add Custom Plane Models

1. Create or download a `.glb` file (3D model in glTF Binary format)
2. Place it in the `planes/` folder in your project directory
3. Repeat for as many planes as you want

Example:
```
curr/
├── voxel_flight.exe
└── planes/
    ├── f16.glb
    ├── passenger_jet.glb
    └── helicopter.glb
```

## Usage

### In the Simulator

1. Run the game
2. From the main menu, click **"Select Plane"**
3. Choose between:
   - **DEFAULT** - Original procedural jet
   - **Your custom planes** - Listed by filename (without .glb)
4. Click to select and return to menu
5. Start the game - your selected plane will load!

### Model Requirements

- **Format**: `.glb` (glTF Binary) or `.fbx` (Autodesk FBX)
- **Size**: Model will be auto-scaled to ~80 game units (same as default jet)
- **Orientation**: Y-axis should point up; model should be roughly centered at origin
- **Textures**: Embedded textures in the GLB file are loaded automatically

## Where to Find Models

- **TurboSquid**: https://www.turbosquid.com/
- **Sketchfab**: https://sketchfab.com/ (filter for glTF/GLB)
- **CGTrader**: https://www.cgtrader.com/
- **Free Models**: Search for "Free 3D aircraft GLB" on Sketchfab

## Tips for Best Results

1. **Model Size**: Try to keep models under 1MB for faster loading
2. **Vertex Count**: Keep below 100k vertices for smooth performance
3. **Textures**: Larger than 2k resolution textures may impact performance
4. **Materials**: Simple materials work better than complex shader networks
5. **File Size**: Use texture compression (UASTC or ETC2 in GLB tools)

## Troubleshooting

### Model doesn't load
- Check console output for error messages
- Verify file is valid GLB/FBX format
- Ensure `planes/` folder exists

### Model appears very small/large
- The auto-scaling should handle most cases
- Verify model is roughly 1-10 units in size in original software

### Textures not showing
- Ensure textures are embedded in the GLB file
- If using separate texture files, bake them into the model

### Performance issues
- Reduce polygon count
- Lower texture resolution
- Ensure model is optimized before export

## Code Changes Summary

### New Files
- `model_loader.h` / `model_loader.cpp` - Core model loading system
- `stb_image.h` - Texture loading support
- `planes/` - Folder for custom aircraft

### Modified Files
- `jet.h` / `jet.cpp` - Added drawJet() wrapper, model loading functions
- `menu.h` / `menu.cpp` - Added plane selection menu and interface
- `main.cpp` - Integrated plane selection state (state 4)
- `globals.h` / `globals.cpp` - Added selectedPlane variable
- `Makefile` - Added Assimp and stb_image linking

### Key Functions
- `loadSelectedPlaneModel(filename)` - Load a plane from planes/ folder
- `drawJet()` - Render either loaded model or default procedural jet
- `getAvailablePlanes(list)` - Scan planes/ folder for available models
- `unloadJetModel()` - Clean up loaded model resources

## Performance Impact

- **Startup**: No impact (models load on-demand when selected)
- **Runtime**: Depends on model complexity; default jet is still performant
- **Memory**: Varies with model size; typically 5-50MB depending on model

## Future Enhancements

Possible future improvements:
- Model preview/thumbnail in menu
- Multiple damage variants of same model
- Custom cockpit views per model
- Model-specific animations
- AI traffic with custom models

Enjoy flying your custom aircraft!
