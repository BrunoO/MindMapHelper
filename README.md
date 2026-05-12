# MindMap Helper

Cross-platform **C++17** mind-map UI built with **Dear ImGui**, **GLFW 3**, and **OpenGL 3**. Dependencies are pulled on first configure via **CMake FetchContent** (GLFW 3.4, ImGui v1.91.9, and others—see **[`CREDITS.md`](CREDITS.md)**).

> **Status: proof of concept.** The current version demonstrates core rendering and interaction. New features are actively planned and will be added in future releases.

## Requirements

- **CMake** 3.16 or newer  
- A **C++17** compiler (Apple Clang, GCC, or MSVC)  
- **OpenGL** development libraries for your OS  
- Network access on the first **CMake configure** (CMake **FetchContent** downloads several dependencies). If automated downloads are blocked for zlib only, you can point the build at a **local zlib** tree; see [Local zlib (offline builds)](#local-zlib-offline-builds) under Troubleshooting.

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

### Local zlib (offline builds)

**zlib** is normally fetched from GitHub during configure (same as other FetchContent dependencies). On machines where that step fails—for example limited download rights or a policy-blocked automatic clone—you can use a copy of the sources you obtained separately (clone or zip of **[madler/zlib](https://github.com/madler/zlib)**).

1. Check out **tag `v1.3.1`** (the version pinned in `CMakeLists.txt`) where your environment allows, e.g.  
   `git clone --depth 1 --branch v1.3.1 https://github.com/madler/zlib.git C:\deps\zlib` (adjust the path).
2. Configure with an **absolute** path to that directory:

   ```powershell
   cmake -S . -B build -A x64 -DBM_ENABLE_TESTS=ON -DBM_ZLIB_SOURCE_DIR=C:/deps/zlib
   ```

   On macOS / Linux, pass `-DBM_ZLIB_SOURCE_DIR=/path/to/zlib` instead.

3. If you change `BM_ZLIB_SOURCE_DIR`, use a **fresh build directory** or clear the CMake cache for that variable so FetchContent does not reuse an old `_deps/zlib-src`.

CMake’s built-in override also works without project-specific names: set **`FETCHCONTENT_SOURCE_DIR_ZLIB`** to the same directory before the first successful zlib populate.

## Third-party libraries

Open-source components, upstream links, and pinned versions are listed in **[`CREDITS.md`](CREDITS.md)**.

## Contributing

See **`AGENTS.md`** for project conventions, source layout, and static analysis notes.
