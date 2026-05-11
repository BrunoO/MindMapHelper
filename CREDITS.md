# Third-party libraries and credits

MindMapHelper bundles or links several open-source projects. This file lists them for recognition and traceability. **Pinned versions** match `CMakeLists.txt` at the time of writing; after dependency bumps, update this document in the same change.

## Libraries fetched by CMake (FetchContent)

| Library | Role in MindMapHelper | Upstream | Pinned ref | SPDX / license (summary) |
|--------|------------------------|----------|------------|---------------------------|
| **GLFW** | Windowing, input, OpenGL context (when not using a system `glfw3` package) | https://github.com/glfw/glfw | `3.4` | Zlib |
| **Dear ImGui** | Immediate-mode UI | https://github.com/ocornut/imgui | `v1.91.9` | MIT |
| **nlohmann/json** | JSON for native document persistence and DTOs | https://github.com/nlohmann/json | `v3.12.0` | MIT |
| **zlib** | DEFLATE compression (used by libzip) | https://github.com/madler/zlib | `v1.3.1` | Zlib |
| **libzip** | Reading and writing ZIP archives (e.g. `.imx`) | https://github.com/nih-at/libzip | `v1.11.3` | BSD-3-Clause |
| **pugixml** | XML DOM parsing | https://github.com/zeux/pugixml | `v1.15` | MIT |
| **ImGuiFileDialog** | File open/save dialog widget for ImGui | https://github.com/aiekick/ImGuiFileDialog | `v0.6.8` | MIT |

Full license texts live in each dependency’s repository (or in the generated `_deps/*-src` tree after a local configure).

## Optional system dependency

| Library | Role | Notes |
|--------|------|--------|
| **GLFW** | Same as above | If `find_package(glfw3)` provides `glfw` or `glfw::glfw`, the build uses the **system** package instead of FetchContent. |

## Platform APIs (not vendored)

| Component | Role |
|-----------|------|
| **OpenGL** | Rendering (loaded via GLFW on desktop). |
| **Apple frameworks** | On macOS, OpenGL and **CommonCrypto** (via libzip) as provided by the SDK. |
| **Windows** | **bcrypt** (via libzip) when building for Windows. |

## Maintainer note

After changing `GIT_TAG` / `GIT_REPOSITORY` entries in `CMakeLists.txt`, update the table above so release engineering and security review stay aligned with what ships in binaries.
