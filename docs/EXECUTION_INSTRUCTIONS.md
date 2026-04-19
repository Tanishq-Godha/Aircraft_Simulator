# Execution Instructions

This project uses `make` mapped through the MSYS2 / MinGW-w64 toolchain to manage build targets. Because the dependencies have been moved to logically grouped folders, you must use the root directory `Makefile` properly.

## 1. Compilation

To compile all the source code correctly, ensure you open an MSYS2 MinGW-w64 shell in the root of this project (where the Makefile is located). 

From your standard Windows PowerShell or Command Prompt, run the following command to invoke MSYS2's build system in the current directory:

```powershell
C:\msys64\msys2_shell.cmd -mingw64 -no-start -defterm -here -c make
```
*Note: This command will read the `Makefile`, compile all necessary files via `g++`, gather the required external libraries (`freeglut`, `opengl32`, `glu32`, and `assimp`), and place the newly compiled binary executable inside the `bin/` subfolder.*

You can also run `make clean` using the exact same argument structure to clear out stale object `.o` files:
```powershell
C:\msys64\msys2_shell.cmd -mingw64 -no-start -defterm -here -c "make clean"
```

## 2. Execution

Because assets such as models (`planes/*.glb`) and graphic pipelines (`shaders/*`) are stored relative to the project root, **you must execute the binary directly from the project root**, NOT from inside the `bin/` folder.

To start the simulator, simply type:

```powershell
.\bin\voxel_flight.exe
```

This ensures the executable can successfully locate and load your shaders and assets smoothly.