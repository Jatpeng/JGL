# Game Workspace

This folder is the standalone runtime workspace for Python-driven JGL scenes.

Layout:

- `script/`: Python scripts and bootstrap helpers.
- `engine/<Config>/`: built engine binaries copied by CMake.
- `Assets/`: runtime assets copied by CMake.
- `Shaders/`: runtime shaders copied by CMake.

Build and sync the workspace:

Windows:

```powershell
cmake -S . -B build -DJGL_ENABLE_PYTHON=ON
cmake --build build --config Release
```

macOS:

```bash
brew install cmake python@3.13 glfw glew assimp
cmake -S . -B build -DJGL_ENABLE_PYTHON=ON -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

Launch:

Windows:

```powershell
.\Game\run_three_cubes.bat
.\Game\run_script.bat three_cubes.py
```

macOS:

```bash
bash ./Game/run_three_cubes.sh
bash ./Game/run_script.sh three_cubes.py
```

The launcher prefers Python 3.13 because the current `pyjgl` build targets CPython 3.13.
On macOS, CMake expects GLFW, GLEW, and Assimp to come from system packages.
