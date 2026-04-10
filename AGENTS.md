# AGENTS.md — bMindMap

C++17 project standards imported from `../USN_Windows`. Adjust build paths and scripts as this repository grows.

---

## Build and tests

### App (`bmindmap` — ImGui + GLFW + OpenGL3)

```bash
cmake -S . -B build -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
cmake --build build
./build/bmindmap          # macOS / Linux
# build\Release\bmindmap.exe   # Windows MSVC (multi-config)
```

Dependencies are fetched with **CMake FetchContent** (`glfw` 3.4, Dear **ImGui** v1.91.9) on first configure.

- **Optional MSVC PGO:** `-DENABLE_PGO=ON` (Release only); see `.cursor/rules/cmake-safe.mdc`.

### Tests

- **Default:** When tests exist, use `ctest` from the build directory.
- **macOS:** If this repo adds `scripts/build_tests_macos.sh` (USN_Windows pattern), prefer that as the single entrypoint; until then, CMake + `ctest` as above.
- **Windows / Linux:** CMake directly; when adding libraries on Windows, follow PGO rules in `.cursor/rules/cmake-safe.mdc` and user rules for `CMakeLists.txt`.

### Analysis

- **clang-tidy:** `clang-tidy -p <build-dir> <file.cpp>` with config from repo root `.clang-tidy`.
- **SonarQube:** Use project scripts when present; see `docs/standards/SONAR_CPP_RULES_REFERENCE.md`.

---

## Naming conventions

See **`docs/standards/CXX17_NAMING_CONVENTIONS.md`**. Summary:

| Kind | Style | Example |
|---|---|---|
| Types | PascalCase | `MindNode`, `DocumentModel` |
| Functions / methods | PascalCase | `GetRoot()`, `AddChild()` |
| Locals / parameters | snake_case | `node_id`, `offset` |
| Member variables | snake_case_ | `root_node_`, `mutex_` |
| Globals | g_snake_case | `g_instance` |
| Constants | kPascalCase | `kMaxDepth`, `kDefaultZoom` |
| Namespaces | snake_case | `mind_map`, `persistence` |

---

## Core C++17 conventions

### Always

- `(std::min)` / `(std::max)` — never plain `min`/`max` (MSVC ambiguity).
- `[[nodiscard]]` on functions returning error codes or resource handles.
- `std::scoped_lock` for locking mutexes (CTAD).
- `explicit` on single-argument constructors and `operator bool()` where appropriate.
- C++17 init-statement: `if (auto x = F(); x)` when the variable is block-local only (`docs/standards/CXX17_INIT_STATEMENT_EXAMPLES.md`).
- `std::string_view` for read-only string parameters where lifetime is clear.
- `return { value };` / `return {};` for braced returns where it improves clarity.
- Explicit lambda captures in template-heavy code — avoid `[&]` / `[=]` where MSVC or lifetime is fragile.
- Const correctness: `const` on non-mutating parameters, locals, and member functions.
- Comment every `#endif`: `#endif  // _WIN32`
- Includes at top of file; prefer stable lowercase paths for system headers.

### Never

- `malloc` / `free` / `new` / `delete` for routine ownership — prefer containers and smart pointers.
- C-style casts — use `static_cast`, `const_cast`, `reinterpret_cast` with care.
- Raw pointers to mutex-guarded internals across threads without a documented lifetime contract.

### Float literals

- Use an uppercase `F` suffix (e.g. `3.14F`) to satisfy clang-tidy and consistent style.

---

## Modifying `.clang-tidy` safely

YAML treats `#` as end-of-line comment. **Do not** put `#` on the same line as a check name under `Checks: >-` or inside other values — the check name would be truncated. Document disabled checks in a separate comment block. After edits, run `clang-tidy --config-file=.clang-tidy --dump-config` and confirm `Checks:` is correct.

Full reminder: `.cursor/rules/clang-tidy-yaml.mdc`.

---

## Modifying `CMakeLists.txt` safely

- Mirror existing patterns; do not casually change PGO or toolchain blocks.
- On Windows, if `ENABLE_PGO` (or equivalent) is used, new `add_subdirectory()` / `FetchContent` libraries linked into the main executable must receive the same PGO-related settings as other deps (avoids LNK1269).
- Test targets that compile the same sources as the main target must match the main target’s PGO compiler flags where applicable.

Full reminder: `.cursor/rules/cmake-safe.mdc`.

---

## ImGui / immediate mode

Dear ImGui is **immediate mode**: no widget objects, UI is a function of your data each frame, **all ImGui calls on the main thread**.

**Popup discipline:** `OpenPopup` and `BeginPopupModal` in the same window context; `SetNextWindowPos` every frame before `BeginPopupModal`; `CloseCurrentPopup` inside the modal block; close controls outside `CollapsingHeader` when using that pattern.

**Cursor rules:** `.cursor/rules/imgui-ui.mdc`, `.cursor/rules/imgui-test-engine-preconditions.mdc` (for future ImGui Test Engine tests).

**Optional:** Copy USN_Windows `docs/design/IMGUI_IMMEDIATE_MODE_PARADIGM.md` into `docs/design/` if you want the long-form design note in-tree.

---

## Cursor rules (`.cursor/rules/*.mdc`)

| Rule | Topic |
|---|---|
| `naming-conventions-cpp.mdc` | PascalCase / snake_case / kPascalCase |
| `const-correctness-cpp.mdc` | `const` on params / members / locals |
| `cxx17-init-statement.mdc` | `if (init; cond)` |
| `cxx17-string-view-params.mdc` | `std::string_view` parameters |
| `cxx17-style-tool-friendly.mdc` | Braced return, `scoped_lock`, `explicit` |
| `windows-msvc-cpp.mdc` | MSVC / Windows headers |
| `platform-preprocessor-cpp.mdc` | `#endif` comments; platform blocks |
| `preprocessor-macros-define.mdc` | Prefer `constexpr` over macros |
| `async-ownership-lifetime-cpp.mdc` | Cross-thread ownership |
| `production-async-future-cpp.mdc` | `std::future` cleanup |
| `production-exception-logging-cpp.mdc` | try/catch discipline |
| `ui-state-write-confinement-cpp.mdc` | UI-thread vs worker mutation |
| `string-view-pool-remap-safety-cpp.mdc` | `string_view` into storage |
| `lock-ordering-no-io-under-lock.mdc` | Locks and I/O |
| `dry-extract-helpers.mdc` | DRY / small helpers |
| `assertions-debug-builds.mdc` | Debug preconditions |
| `crash-triage-discipline.mdc` | Focused fixes |
| `sonar-cpp-avoid-new-issues.mdc` | Sonar / static analysis hygiene |
| `suppressions-same-line.mdc` | NOLINT / NOSONAR on same line as issue |
| `documentation-placement.mdc` | `docs/` vs `internal-docs/` |
| `imgui-ui.mdc` | ImGui immediate mode, popups |
| `imgui-test-engine-preconditions.mdc` | UI regression test preconditions |
| `cmake-safe.mdc` | CMake / PGO / tests |
| `clang-tidy-yaml.mdc` | Safe `.clang-tidy` edits |
| `macos-build-tests.mdc` | macOS build entrypoint |

---

## Critical anti-patterns

| Rule | Why |
|---|---|
| Max 3-level nesting (cpp:S134) | Prefer early returns |
| Never empty `catch` (cpp:S2486) | Log or rethrow |
| No `} if (` on same line (cpp:S3972) | New line or `else if` |

Details: `docs/standards/SONAR_CPP_RULES_REFERENCE.md`.

---

## Documentation placement

| Folder | Use |
|---|---|
| `docs/` | Build guides, architecture, standards (contributor-facing) |
| `internal-docs/` | Maintainer notes, dated analysis, plans |
| `specs/` | Feature specs before implementation |

When you add stable public docs under `docs/`, keep an index file updated if the project uses one (e.g. `docs/DOCUMENTATION_INDEX.md`).

---

## Boy scout rule

When touching code, leave it slightly better: fix obvious issues, improve names, add `const`, remove dead code.
