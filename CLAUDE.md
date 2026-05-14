# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

> **Primary reference:** `AGENTS.md` — authoritative project standards. This file summarises what matters most for rapid orientation.

---

## Build

```bash
# macOS (single entrypoint)
./scripts/build_tests_macos.sh

# Linux
./scripts/build_tests_linux.sh

# Windows (MSVC multi-config)
./scripts/build_tests_windows.ps1

# Manual (any platform)
cmake -S . -B build -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DBM_ENABLE_TESTS=ON
cmake --build build
./build/MindMapHelper          # macOS / Linux
# build\Release\MindMapHelper.exe   # Windows
```

Dependencies (`glfw` 3.4, Dear ImGui v1.91.9) are fetched by CMake FetchContent on first configure — network required.

## Tests

```bash
ctest --test-dir build --output-on-failure                     # macOS / Linux
ctest --test-dir build -C Release --output-on-failure          # Windows
```

## Static analysis

```bash
clang-tidy -p build src/path/to/file.cpp    # single file
./scripts/run_clang_tidy.sh --summary-only  # full tree (Linux CI)
```

After editing `.clang-tidy`, run `clang-tidy --config-file=.clang-tidy --dump-config` and verify the `Checks:` field is intact. Never put `#` on the same line as a check name inside the `Checks: >-` block.

---

## Architecture

C++17 desktop UI prototype — Dear ImGui (immediate mode) + GLFW + OpenGL 3, built with CMake.

### Layer boundaries (enforced by CMake and clang-tidy)

| Layer | Path | Rule |
|---|---|---|
| **core** | `src/core/` | Platform-neutral data structures only |
| **ui** | `src/ui/` | ImGui rendering, canvas, geometry |
| **platform** | `src/platform/{macos,windows,linux}/` | Window creation, GL context |
| **app** | `src/app/` | Main loop, orchestration |

`core` must not depend on `ui` or `platform`. `platform` must not depend on `ui`.

### Render loop

```
AppMain::RunApp()
  └─ each frame:
       ImGui::NewFrame()
       RenderMainUi()                       ← MindMapUi.cpp
         └─ MindMapCanvasView::Render()     ← canvas pan/zoom, node drag, edge selection
              ├─ DrawMindMapNodes()
              └─ DrawOneChildBranch()       ← dispatches to Bezier / Orthogonal / OrganicTaper
       ImGui::Render()  →  OpenGL swap
```

### Key state ownership

- **`MindMapCanvasView`** owns all canvas state: node world positions, drag state, per-child branch styles (`branch_style_by_child_[]`), and selection for edge-style editing (`selected_child_for_edge_`).
- **`UiCommandDispatcher`** + `UiCommandId` enum decouples menu/toolbar actions from state mutation.
- **`NodeGeometry.h`** (`mind_map::canvas`) and **`BranchEdgeAttachments.h`** (`mind_map::ui::branch`) hold rounded-rect attachment and shared edge geometry used by branch renderers.
- **`CanvasMath.h`** handles world↔screen transforms and hit testing.

### Branch styles

Three rendering strategies selected per-edge (Milestone D) or globally via UI combo:
1. **Bezier** — cubic splines with rounded-rect attachment points
2. **Orthogonal** — L-shaped polylines
3. **OrganicTaper** — tapered width-modulated curves

---

## Naming conventions

| Kind | Style | Example |
|---|---|---|
| Types | PascalCase | `MindNode` |
| Functions / methods | PascalCase | `GetRoot()` |
| Locals / parameters | snake_case | `node_id` |
| Member variables | snake_case_ | `root_node_` |
| Globals | g_snake_case | `g_instance` |
| Constants | kPascalCase | `kMaxDepth` |
| Namespaces | snake_case | `mind_map` |

---

## Critical C++17 rules

- Float literals: uppercase `F` suffix — `3.14F`, `0.5F`.
- `(std::min)` / `(std::max)` — parentheses required to prevent MSVC macro collisions.
- `[[nodiscard]]` on functions returning error codes or resource handles.
- Lambda explicit captures: when the capture list has no default mode (`[&]`/`[=]`), every local used in the body must be listed — **including `constexpr` locals**. MSVC C3493 + cascading C2064; GCC/Clang silently accept the omission.
- `explicit` on single-argument constructors and conversion operators.
- Prefer `if (auto x = F(); x)` init-statement when the variable is block-local only.
- `std::string_view` for read-only string parameters where lifetime is clear.
- No `malloc`/`free`/`new`/`delete` for routine ownership — use containers and smart pointers.
- No C-style casts.
- `#endif` comments required: `#endif  // _WIN32`.

Anti-patterns (Sonar / clang-tidy enforced):
- Max 3-level nesting — prefer early returns.
- Never empty `catch` blocks — log or rethrow.
- No `} if (` on same line — use `else if` or a new line.

## ImGui rules

- All ImGui calls on the **main thread only**.
- `OpenPopup` and `BeginPopupModal` must share the same window context.
- Call `SetNextWindowPos` every frame before `BeginPopupModal`.
- `CloseCurrentPopup` inside the modal block; close controls outside `CollapsingHeader`.

---

## Documentation placement

| Folder | Use |
|---|---|
| `docs/` | Architecture, build guides, standards (contributor-facing) |
| `internal-docs/` | Maintainer notes, dated analysis, plans |
| `specs/` | Feature specs before implementation |
