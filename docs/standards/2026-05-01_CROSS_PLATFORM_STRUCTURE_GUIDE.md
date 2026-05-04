# Cross-Platform Structure Guide

This document defines the cross-platform structure for MindMap Helper (`MindMapHelper`) and the boundaries between shared logic and OS-specific code.

## Source layout

- `src/main.cpp`: thin launcher only.
- `src/app/`: app orchestration (`RunApp`) and top-level lifecycle.
- `src/core/`: platform-neutral domain logic (no ImGui/GLFW/OpenGL/OS headers).
- `src/ui/`: UI composition and demos.
- `src/ui/demos/`: sample interactive mind map demos and shared demo graph helpers.
- `src/platform/`: platform bootstrap contracts and implementations.
  - `src/platform/windows/`
  - `src/platform/macos/`
  - `src/platform/linux/`

## Layering rules

- `core` must not depend on UI, GLFW, OpenGL, or OS APIs.
- `ui` may depend on `core` and ImGui backends, but not OS-specific implementation files.
- `platform` owns GLFW/OpenGL setup and OS-specific context hints.
- `app` wires startup and shutdown by composing `core`, `ui`, and `platform`.

## Platform preprocessor discipline

- Keep platform conditionals at bootstrap/build edges.
- Prefer extracting platform adapters instead of expanding shared `#ifdef` branches.
- Always comment `#endif` lines using project style (`#endif  // _WIN32`).

## Build and test entrypoints

- macOS: `scripts/build_tests_macos.sh`
- Linux: `scripts/build_tests_linux.sh`
- Windows: `scripts/build_tests_windows.ps1`
- Canonical test target list: `scripts/test_targets.txt`

## Windows/MSVC specifics

- Use `(std::min)` and `(std::max)` forms.
- Use explicit lambda captures in template-heavy code.
- Use iterator types (not pointer deduction) for STL iterators.
- Keep Visual Studio multi-config flow explicit in CI and local runs (`--config Release`, `ctest -C Release`).

## Non-goal

- PGO is out of scope for this repository; do not introduce `ENABLE_PGO`, `/GENPROFILE`, `/USEPROFILE`, or `pgomgr` workflows.
