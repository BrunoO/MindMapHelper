# MindMap Helper

Cross-platform **C++17** mind-map UI prototype built with **Dear ImGui**, **GLFW 3**, and **OpenGL 3**. Dependencies are pulled on first configure via **CMake FetchContent** (GLFW 3.4, ImGui v1.91.9).

## Requirements

- **CMake** 3.16 or newer  
- A **C++17** compiler (Apple Clang, GCC, or MSVC)  
- **OpenGL** development libraries for your OS  
- Network access on the first **CMake configure** (to fetch GLFW and ImGui)

---

## macOS

**Recommended:** build and run tests from the repository entrypoint:

```bash
./scripts/build_tests_macos.sh
```

Run the application:

```bash
./build/MindMapHelper
```

**Manual** configure and build (with tests):

```bash
cmake -S . -B build -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DBM_ENABLE_TESTS=ON
cmake --build build
ctest --test-dir build --output-on-failure
./build/MindMapHelper
```

---

## Linux

Install a toolchain, CMake, OpenGL headers, and typical X11/Wayland development packages so GLFW can link (exact package names depend on your distribution).

**Debian / Ubuntu** (illustrative):

```bash
sudo apt-get update
sudo apt-get install -y build-essential cmake pkg-config libgl1-mesa-dev xorg-dev
```

**Recommended:** build and run tests:

```bash
./scripts/build_tests_linux.sh
./build/MindMapHelper
```

**Manual** (equivalent to the script):

```bash
cmake -S . -B build -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DBM_ENABLE_TESTS=ON
cmake --build build
ctest --test-dir build --output-on-failure
./build/MindMapHelper
```

If a system GLFW CMake config is not found, this project fetches and builds GLFW via FetchContent. That path sets **`GLFW_BUILD_WAYLAND=OFF`** (X11 only) so minimal CI images do not need `wayland-scanner` or Wayland dev packages.

---

## Windows (Visual Studio, MSVC)

Use **x64** and a multi-config build directory. **PowerShell** script (from the repo root):

```powershell
.\scripts\build_tests_windows.ps1
```

Run the Release binary:

```powershell
.\build\Release\MindMapHelper.exe
```

**Manual** commands:

```powershell
cmake -S . -B build -A x64 -DBM_ENABLE_TESTS=ON
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
.\build\Release\MindMapHelper.exe
```

---

## Configure without tests

Faster iteration when you only need the app:

```bash
cmake -S . -B build -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DBM_ENABLE_TESTS=OFF
cmake --build build
```

Then run `MindMapHelper` from the build output directory as above for your platform.

---

## Troubleshooting

If CMake reports that `CMakeCache.txt` was created for a **different source directory** (for example after renaming or moving the project folder), remove the build directory and configure again:

```bash
rm -rf build
```

Then rerun the configure and build steps for your platform.

## Contributing

See **`AGENTS.md`** for project conventions, source layout, and static analysis notes.
