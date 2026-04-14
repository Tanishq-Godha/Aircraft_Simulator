# Aircraft Models Folder

Place your `.glb` (glTF Binary) files in this directory to make them available as selectable aircraft in the simulator.

## How to Add Custom Planes:

1. Export or find a 3D model in `.glb` format (or `.fbx` - both are supported)
2. Place the file directly in this `planes/` folder
3. Start the simulator and go to "Select Plane" from the main menu
4. Choose your custom aircraft

## Example:

```
planes/
  ├── fighter.glb
  ├── passenger.glb
  └── experimental.glb
```

## Important Notes:

- The model will be automatically scaled to match the default aircraft size (~80 game units)
- Models should ideally be centered at origin with Y-axis pointing up
- Embedded textures in the GLB file will be loaded automatically
- The first time loading a large model may take a few seconds

## Supported Formats:

- **.glb** (glTF Binary) - Recommended
- **.fbx** (Autodesk FBX) - Also supported

## Model Naming:

- Use descriptive names (preferable with no spaces, or use underscores)
- Examples: `f16_fighter.glb`, `classic_dc3.glb`, `spaceshuttle.glb`
