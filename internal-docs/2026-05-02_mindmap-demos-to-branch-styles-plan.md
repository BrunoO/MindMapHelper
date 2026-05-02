# Plan: Demos → unified canvas + per-branch styles

**Date:** 2026-05-02  
**Context:** Replace the three `IDemo` implementations with one canvas view, extract branch rendering into reusable units, then support a display style per branch (edge), each style matching one of today’s three demos.

---

## Outcomes

1. **No demo plug-in layer** — Remove `IDemo`, `DemoRegistry`, `DemoBezierMindMap`, `DemoStraightMindMap`, `DemoTaperOrganicMindMap` and their registration from `MindMapUi.cpp` / CMake.
2. **Preserved behavior** — Same sample tree, pan/zoom, node drag, grid, and three branch **visual** modes still achievable (initially via one global style control for regression parity).
3. **Per-branch styles** — Each child edge can select orthogonal / bezier / organic taper independently; UI to edit styles can follow in a later slice.

---

## Milestone A — Extract branch renderers (no behavior change to the user)

**Goal:** Three drawing code paths live in dedicated translation units with a small shared API; demos still call them (or thin wrappers) so the combo still switches demos.

**Tasks**

1. **Define a minimal `BranchRenderContext` (or reuse fields)**  
   ImDrawList, `canvas_p0`, `pan_px`, `zoom`, and edge-specific world geometry. Avoid a 8+ parameter free function per Sonar guidance — group in one struct.

2. **Define neutral edge inputs**  
   Parent/child centers, parent/child “shape” parameters (half-extents + corner radius vs circle radii — see Milestone C), attachment points or directions as needed. Prefer computing attachments inside each style or via existing helpers in `SampleMindMapGraph.h` until that header is split.

3. **Move implementation bodies**  
   - From `DemoBezierMindMap.cpp` → e.g. `src/ui/branch/DrawBranchBezier.*`  
   - From `DemoStraightMindMap.cpp` → `src/ui/branch/DrawBranchOrthogonal.*`  
   - From `DemoTaperOrganicMindMap.cpp` → `src/ui/branch/DrawBranchOrganicTaper.*` (keep private helpers file-local or in a `OrganicTaperDetail.cpp` if the file is large)

4. **Demos become thin**  
   Each `Demo*::Render` calls grid + node drawing + a single function per edge loop, or one `DrawAllBranches(style, ...)` that takes style as template/enum.  
   **Acceptance:** Build passes; visually switching the three demos is unchanged; `scripts/build_tests_macos.sh` (or project test entrypoint) passes.

**Risk:** Organic taper file is large — extract in one PR focused on move-only + namespaces to ease review.

---

## Milestone B — Single canvas view replaces demos

**Goal:** `MindMapUi` owns one scene object; no `CreateDemoList`, no `IDemo`.

**Tasks**

1. **Introduce `MindMapCanvasView` (or equivalent)** in `src/ui/`  
   - State: `std::array<ImVec2, kSampleMindMapNodeCount> pos_world_` (or vector if you generalize count later), drag index, grab offset.  
   - Methods: `Reset()`, `OnPrimaryDown/Drag/Up`, `IsDraggingContent()`, `Render(DemoRenderContext)` — same responsibilities as a demo today.

2. **Unify render order**  
   Match the best current practice (e.g. taper demo: branches under nodes). Pick one order for all styles unless a style documents an exception.

3. **Unify hit-testing for drag**  
   Pick one node representation for the unified view (see Milestone C). All three branch styles use the same hit test.

4. **Temporary global style**  
   `BranchStyle global_style` + combo or menu until Milestone D. Wire `Render` to dispatch the extracted drawers per edge using `global_style` for every edge.

5. **Delete**  
   `IDemo.h`, `DemoRegistry.cpp`, three demo `.h/.cpp`; update root `CMakeLists.txt`; remove `demos/` includes from `MindMapUi.cpp`.

   **Acceptance:** One combo (or menu) switches branch appearance for the whole map; no `mind_map::demos::IDemo` references outside deleted files; build/tests green.

---

## Milestone C — Node model consistency (decision gate)

**Problem:** Straight demo uses **circles**; bezier/taper use **rounded rects** (from `SampleMindMapGraph.h`).

**Options**

| Option | Pros | Cons |
|--------|------|------|
| **C1 — Unified rounded-rect nodes** | One hit test, one outline; orthogonal style draws toward rounded-rect attachments (may need circle→rounded helper or reuse `SampleMapRoundedRectAttachment*`) | Orthogonal path geometry changes slightly vs current demo |
| **C2 — Unified circles** | Keeps straight demo geometry | Bezier/taper need port to circular attachments or hybrid |
| **C3 — Style-specific node chrome** | Pixel-perfect parity with old demos | Per-branch style can look inconsistent on one map; more code |

**Recommendation for “one map, mixed branches”:** **C1** — one node type, all branch drawers take the same attachment API.

**Acceptance:** Document chosen option in this file or in code comment at view; hit-test and outline match the chosen model everywhere.

---

## Milestone D — Per-branch style data + render dispatch

**Goal:** Each edge (non-root node) has its own `BranchStyle`.

**Tasks**

1. **`enum class BranchStyle`** — `Orthogonal`, `Bezier`, `OrganicTaper` (names can match product language).

2. **Storage** — `std::array<BranchStyle, kSampleMindMapNodeCount>` indexed by **child** id; root unused or default. Initialize to a sensible mix or all one style for first integration.

3. **Render loop** — For each `child` with `parent >= 0`, read `branch_style[child]`, call the corresponding extracted drawer with the same node geometry.

4. **Reset** — Decide whether `Reset()` resets positions only or also styles (default: positions only; styles compile-time or fixed array until UI exists).

**Acceptance:** Hard-code two different styles on two edges in the array, confirm both render correctly in one frame; no global style required for correctness (global can remain as “default for new edges” later).

---

## Milestone E — UI to edit per-branch style (optional follow-up)

**Tasks**

- Selection model: click edge vs click child node (child is easier with tree topology).  
- ImGui: combo on selection, or context menu on node “Branch style”.  
- Status bar: show style of selected branch.

**Acceptance:** User can change style at runtime without recompile; persistence out of scope unless requested.

---

## Topology and layering cleanup (parallel or after B)

- **Single source of truth** for `kSampleMindMapSpecs` / node count: prefer `mind_map::core::mindmap` definitions; UI includes measured sizes from ImGui where needed.  
- Move **ImGui-free** math (bezier point/tangent, attachments that only need vectors) toward `core` only if it clears dependencies and tests still run; otherwise keep under `src/ui/` until a second consumer exists (**YAGNI**).

---

## Suggested PR order

1. **PR1 — Milestone A** (extract branch modules; demos thin).  
2. **PR2 — Milestone B + C** (single view, delete demos, unified nodes).  
3. **PR3 — Milestone D** (per-edge array + dispatch).  
4. **PR4 — Milestone E** (editor UI).  
5. **Topology dedup** can land in PR2 or a small PR between PR1 and PR2.

---

## Verification checklist (each PR)

- [ ] macOS: `scripts/build_tests_macos.sh` (or documented equivalent)  
- [ ] No new Sonar/clang-tidy suppressions unless justified same-line  
- [ ] `core` does not include ImGui headers (if touched)  
- [ ] Float literals use `F` suffix in new/changed C++

---

## Dependencies / non-goals

- **Non-goals for this plan:** file save/load, arbitrary graph size, undo — add when product requires them.  
- **Depends on:** ImGui main thread only (unchanged); existing `mind_map::canvas` world/screen helpers.

---

## Rollback

Each milestone is revert-friendly if PRs stay focused: A is behavior-neutral; B is the largest delete — keep a branch or tag before demo removal until Milestone B is signed off visually.
