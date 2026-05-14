# Plan: remove the default sample mind map

**Status:** implemented (2026-05-14).
**Date:** 2026-05-13.

---

## 1. Goal

`File > New` (`DocumentSessionService::New`) already produces a minimal one-root-node document. With the new-node UI in place, the seven-node demo topology (`Root` / `Idea A..C` / `Detail A1/A2/B1`) is no longer needed as startup content. Delete it everywhere it leaks into product code and replace startup with the same one-root-node document `File > New` produces.

This is a **product-surface deletion plus dead-code cleanup**; it does not change persistence, branch rendering, undo/redo, or any test-covered behavior.

---

## 2. Inventory — where the sample lives today

### 2.1 Files to delete entirely

| File                                          | Why it can go                                                                                                                                                                                                                                                                |
| --------------------------------------------- | --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| `src/core/mindmap/SampleMindMapTopology.h`    | Defines `kSampleMindMapNodeCount`, `SampleMindMapNodeSpec`, `kSampleMindMapSpecs` — the seven hard-coded labels and parent indices. Used only by `SampleMindMapGraph.h/.cpp`.                                                                                                |
| `src/ui/demos/SampleMindMapGraph.h`           | Declares `BuildSampleDocument()` and `InitialSampleMapPositions()`; re-exports the three symbols from `SampleMindMapTopology.h`. Used only by `MindMapCanvasView.cpp` and `MindMapUi.cpp`.                                                                                   |
| `src/ui/demos/SampleMindMapGraph.cpp`         | Implements `BuildSampleDocument()` (sample topology → `MindMapDocument` with stable UUIDs, edge styles `bezier/orthogonal/organic_taper`, sample edge label `"supports"`, pan `(40, 120)`, zoom `1.0`).                                                                       |
| `src/ui/demos/` (directory)                   | After the two files above are removed it is empty. Delete the folder so `src/ui/demos/` stops existing as a layer concept (referenced by `.cursor/rules/source-layer-boundaries-cpp.mdc`, `docs/standards/2026-05-01_CROSS_PLATFORM_STRUCTURE_GUIDE.md`, and `AGENTS.md`).    |

### 2.2 Files to keep, but rename — name is misleading once the sample is gone

| File                                                   | Action                                                                                                                                                                                                                            |
| ------------------------------------------------------ | --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| `src/ui/branch/SampleMindMapBranchAttachments.h`       | Rename to `src/ui/branch/BranchEdgeAttachments.h`. Contents (`BranchEdgeData`, `FillBranchEdgeData`) are general-purpose helpers over `CanvasNode` — nothing sample-specific. All four branch drawers and the text-on-path renderer consume them. |

> The rename respects the user rule "we do not need backward compatibility": no `_compat` headers, just update every `#include`.

### 2.3 Files to edit (no rename / no delete)

| File                                              | Change                                                                                                                                                                                                                                                                                                                                              |
| ------------------------------------------------- | --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| `src/ui/MindMapCanvasView.cpp`                    | Drop `#include "ui/demos/SampleMindMapGraph.h"`. Make `MindMapCanvasView()` a trivial default constructor — empty `nodes_` / `initial_pos_world_`. Startup content becomes the responsibility of `AppMain` (see §3).                                                                                                                                |
| `src/ui/MindMapUi.cpp`                            | Drop `#include "ui/demos/SampleMindMapGraph.h"`. In `RenderFileMenu`'s `"New"` handler, stop calling `BuildSampleDocument()`. Use the `dummy` document `DocumentSessionService::New(dummy)` already populated (one-root, label `"New Mind Map"`, layout at origin), then `state.canvas_.LoadFrom(dummy); state.ApplyViewport(dummy.viewport_);`.        |
| `src/app/AppMain.cpp`                             | When no `startup_path` is provided, call `session.New(doc)` and feed `doc` into the deferred-startup-document path so the canvas opens on a one-root-node document instead of an empty canvas. Same one-shot post-GL-init load already used for `--open`.                                                                                              |
| `src/ui/branch/DrawBranchesBezier.cpp`            | Replace `#include "ui/branch/SampleMindMapBranchAttachments.h"` with `#include "ui/branch/BranchEdgeAttachments.h"`.                                                                                                                                                                                                                                |
| `src/ui/branch/DrawBranchesOrthogonal.cpp`        | Same include rename.                                                                                                                                                                                                                                                                                                                                |
| `src/ui/branch/DrawBranchesOrganicTaper.cpp`      | Same include rename (currently includes it twice via the two render functions — single header, no behavior change).                                                                                                                                                                                                                                 |
| `src/ui/branch/DrawBranchTextOnPath.cpp`          | Same include rename.                                                                                                                                                                                                                                                                                                                                |
| `CMakeLists.txt`                                  | In `BM_UI_SOURCES` (around line 204): remove `src/ui/demos/SampleMindMapGraph.cpp`. No new sources to add — header-only rename does not need a list update.                                                                                                                                                                                          |

### 2.4 Docs to update

| File                                                                                | Change                                                                                                                                                                                                                                                              |
| ----------------------------------------------------------------------------------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| `CLAUDE.md`                                                                         | Render-loop snippet still says `DrawSampleMindMapNodes()` and "Key state ownership" still mentions `SampleMindMapGraph.h`. Replace with the current geometry home (`NodeGeometry.h`) and a neutral node-draw description. The render path no longer dispatches a sample-specific helper.                                                                                              |
| `docs/2026-05-12_architecture_diagram.svg`                                          | Line 81 text mentions `SampleMindMapGraph / SampleMindMapTopology`. Replace with a one-liner that points at `BranchEdgeAttachments.h` + `NodeGeometry.h` (or remove the line if it no longer adds value).                                                          |
| `specs/2026-05-13_branch-edge-label-and-display-plan.md`                            | Active spec. The table row for `src/ui/demos/SampleMindMapGraph.cpp` ("Set `label_` on sample edges where useful") becomes stale — drop the row.                                                                                                                    |
| `internal-docs/2026-05-02_mindmap-demos-to-branch-styles-plan.md`                   | Historical/dated plan. Leave content as-is for the record, but append a short closing note: "Superseded: the demo layer was removed on 2026-05-13; see `specs/2026-05-13_remove-default-sample-mindmap-plan.md`."                                                  |
| `internal-docs/2026-05-11_file-management-plan-context.md`                          | Same treatment — append a short note that the sample-deletion step it predicted has now happened, with a back-link to this plan.                                                                                                                                    |
| `.cursor/rules/source-layer-boundaries-cpp.mdc`                                     | Line "`src/ui/`: UI composition and demos." — remove the "and demos" tail; the demos layer no longer exists.                                                                                                                                                       |
| `docs/standards/2026-05-01_CROSS_PLATFORM_STRUCTURE_GUIDE.md`                       | If it lists `src/ui/demos/` as part of the canonical tree (verify when editing), remove that entry.                                                                                                                                                                |
| `AGENTS.md`                                                                         | No edit anticipated — it does not name `demos/` directly. Re-check the "Documentation placement" / source layout summary while editing; touch only if `demos/` is mentioned.                                                                                       |

### 2.5 Things explicitly **not** changed

- `MindMapEdge::style_` values (`"bezier"`, `"orthogonal"`, `"organic_taper"`) — these are the persistence vocabulary, not sample-specific.
- The three branch renderers themselves (`DrawBranchesBezier.cpp`, `DrawBranchesOrthogonal.cpp`, `DrawBranchesOrganicTaper.cpp`) and `DrawBranchTextOnPath.cpp` — only their include line changes.
- `UuidGenerator`, `Base64`, `MindMapDocument` — untouched.
- All `tests/*.cpp` — no test references the sample (verified by ripgrep). No new test required for the deletion itself; the existing smoke test continues to cover startup.
- `Reset Layout` command and the underlying `Reset()` semantics — still meaningful: it restores `initial_pos_world_` captured at the most recent `LoadFrom`. After this change, on a fresh app start that initial layout is the single root at origin.

---

## 3. Startup behaviour after deletion

Today:

```
MindMapCanvasView()                       // sample loaded here
  └─ LoadFrom(BuildSampleDocument())      // 7 nodes, 6 edges, mixed styles
```

After:

```
MindMapCanvasView()                       // empty canvas (nodes_.empty())
AppMain::RunApp:
  if startup_path empty:
    session.New(doc);                     // already exists; one root, "New Mind Map"
    deferred_startup_document = doc;
  // ... ImGui + GL init ...
  ui_state.canvas_.LoadFrom(*deferred_startup_document);
  ui_state.ApplyViewport(deferred_startup_document->viewport_);
```

Justification:

- **SRP (SOLID):** `MindMapCanvasView` already stops owning topology; with this change it also stops owning the "what to show on launch" decision. That belongs to `AppMain` / `DocumentSessionService` — the same components that already handle `--open <path>` and `File > New`.
- **DRY:** "What is a brand-new document?" is defined in exactly one place, `DocumentSessionService::New`. The view never has its own answer.
- **KISS:** No conditional logic in the canvas constructor; the launch path mirrors the `File > New` path verbatim.
- **YAGNI:** No splash screen, no "recent files" picker, no empty-state UI work — the user immediately sees a single editable root node, which is the next interaction surface anyway.

---

## 4. Ordered execution steps (when the user gives the go-ahead)

1. **Rename header** `src/ui/branch/SampleMindMapBranchAttachments.h` → `src/ui/branch/BranchEdgeAttachments.h`.
   - Update the four `#include` lines in `DrawBranchesBezier.cpp`, `DrawBranchesOrthogonal.cpp`, `DrawBranchesOrganicTaper.cpp`, `DrawBranchTextOnPath.cpp`.
   - Build + run tests. Expect green; this is a pure rename.

2. **Stop using the sample in product code.**
   - `MindMapCanvasView.cpp`: remove include + remove the body of the default constructor (leave `= default` or an empty `{}`).
   - `MindMapUi.cpp`: remove include; in `"New"` handler, load `dummy` (which `session.New(dummy)` already filled) instead of `BuildSampleDocument()`.
   - `AppMain.cpp`: when `startup_path.empty()`, call `session.New(doc)` and assign `deferred_startup_document = std::move(doc)`.
   - Build + run tests. Smoke test must still launch; `File > New` must produce a single root.

3. **Delete the demos layer.**
   - Remove `src/ui/demos/SampleMindMapGraph.cpp` from `CMakeLists.txt`.
   - Delete `src/ui/demos/SampleMindMapGraph.h`, `src/ui/demos/SampleMindMapGraph.cpp`.
   - Delete `src/core/mindmap/SampleMindMapTopology.h`.
   - Delete the now-empty `src/ui/demos/` directory.
   - Build + run tests.

4. **Docs and rules sweep** (see §2.4). Single docs-only commit, no build impact.

Each step is independently buildable and testable, matching the workspace `crash-triage-discipline.mdc` "one hypothesis per commit" rule. Step 1 and step 2 must precede step 3 (compilation depends on it).

---

## 5. Risk register

| Risk                                                                                          | Mitigation                                                                                                                                                                                                                                                                                                |
| --------------------------------------------------------------------------------------------- | --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| `MindMapCanvasView` with `nodes_.empty()` between construction and `LoadFrom` is dereferenced (hit-test, render, selection).           | `AppMain` already deferred the first `LoadFrom` until after GL init, and the render loop tolerates empty `nodes_` today (every loop is `for (size_t i = 0; i < nodes_.size(); ++i)`). Worth a quick read of `Render`, `OnPrimaryDown`, `HitTest*` before the change to confirm no `nodes_[0]` assumption. |
| `File > New` previously showed the sample; some users might rely on it as a "load demo" command. | Acceptable: the project rule "we do not need backward compatibility" applies, and the spec is explicit about the new behavior (one root, label `"New Mind Map"`).                                                                                                                                       |
| `SampleMindMapBranchAttachments.h` rename misses a caller in CMake or platform-specific code. | Ripgrep confirms only the four `.cpp` files include it. Re-run the same search after the rename to be sure.                                                                                                                                                                                              |
| Architecture diagram (`docs/2026-05-12_architecture_diagram.svg`) becomes inconsistent.       | Listed in §2.4. Update text in the same commit as the source rename so the diagram does not lag behind the tree.                                                                                                                                                                                         |

---

## 6. Post-change verification

- `./scripts/build_tests_macos.sh` — green build, all `ctest` targets pass (`scripts/test_targets.txt`).
- Manual smoke: launch with no arguments → single root labelled `New Mind Map` at origin; launch with `--open existing.mmh` → unchanged behavior; `File > New` → single root, `Reset Layout` resets that one node to origin.
- `rg "SampleMindMap|BuildSampleDocument|InitialSampleMapPositions|src/ui/demos"` returns matches only inside `internal-docs/` (intentional historical record) and this spec file.
