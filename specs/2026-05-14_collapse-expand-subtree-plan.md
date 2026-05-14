# Collapse / Expand Subtrees — Implementation Plan

**Status:** Approved for implementation  
**Date:** 2026-05-14

---

## User-visible behaviour

- Every node that has children shows a small triangle affordance to the right of its box.  
  - **▶** (pointing right) when the subtree is collapsed — children hidden.  
  - **▼** (pointing down) when the subtree is expanded — children visible.  
- **Click the triangle** to toggle collapse/expand.  
- **Space** (when a node with children is selected) toggles collapse/expand.  
- **Edit → Collapse / Expand Node** menu item (label switches with state; disabled when no node or no children selected).  
- Toggle is **undoable / redoable** via the existing `CommandHistory`.  
- Collapsed state is **persisted** in `.mmh` JSON (optional `"collapsed": true` field on each node).

---

## Data model

`MindMapNode` (core) gains `bool collapsed_ = false`.  
`CanvasNode` (ui) gains `bool collapsed_ = false`.

No format version bump — the new field is optional in JSON; absent = `false`.

---

## Canvas state

`MindMapCanvasView` gets a private map:

```
std::unordered_map<size_t, std::vector<size_t>> collapse_affected_;
// key = collapsed node index; value = node indices deactivated by that collapse
```

Two new public methods:

| Method | Behaviour |
|---|---|
| `CollapseNode(idx)` | Set `collapsed_=true`; collect active descendants via `CollectActiveSubtree` (excluding `idx` itself); store in `collapse_affected_[idx]`; set all `active_=false` |
| `ExpandNode(idx)` | Set `collapsed_=false`; reactivate every index in `collapse_affected_[idx]`; erase the map entry |

**Nested collapse correctness**: `CollectActiveSubtree` only walks active nodes. If a grandchild subtree was already hidden by an inner collapse, it is not in `collapse_affected_[outer]` and therefore not restored when the outer node expands — correct.

**LoadFrom with pre-existing collapses**: After building all nodes (all `active_=true`), apply collapses from **deepest node first** (sort collapsed nodes by parent-chain length, descending). This ensures inner collapses are applied before outer ones, so outer `CollapseNode` calls see the same active-subtree boundaries that the user originally produced.

---

## New command

```
CollapseNodeCommand(MindMapCanvasView&, size_t node_idx, bool collapsing)
```

| | `collapsing = true` | `collapsing = false` |
|---|---|---|
| `Execute()` | `canvas_.CollapseNode(idx)` | `canvas_.ExpandNode(idx)` |
| `Undo()` | `canvas_.ExpandNode(idx)` | `canvas_.CollapseNode(idx)` |

---

## Interaction wiring

### Triangle mouse click

`OnPrimaryDown` checks for a triangle hit **before** the node drag hit-test.  
Triangle world centre: `{node.pos_world_.x + half.x + kCollapseTriangleOffsetX, node.pos_world_.y}`.  
Hit radius (world units): `kCollapseHitRadius / zoom`.  
On hit: store the index in a private `std::optional<size_t> collapse_toggle_target_`; do **not** start a drag.

A new method `ConsumeCollapseToggleTarget()` returns and clears the stored target.  
`HandleCanvasPointerInput` (in `MindMapUi.cpp`) calls it after `OnPrimaryDown` and, if set, pushes a `CollapseNodeCommand` directly to `CommandHistory`.

### Space shortcut

`UiCommandDispatcher` handles `UiCommandId::ToggleCollapsed`:  
uses `GetSelectedNode()`, guards `NodeHasChildren(idx)`, pushes `CollapseNodeCommand`.

---

## Rendering

Inside `DrawMindMapNodes` (anonymous namespace in `MindMapCanvasView.cpp`):  
Pre-compute a `std::unordered_set<size_t> parent_set` from all nodes (active or not) to know which nodes have children.  
For each active node in the parent set: draw the triangle at `rmax.x + kTriangleOffsetPx` screen pixels, vertically centred.  
▶ = collapsed; ▼ = expanded.

---

## JSON serialisation

`ToJson`: adds `"collapsed": true` only when `collapsed_ == true` (omit otherwise).  
`FromJson`: reads `n.value("collapsed", false)`.

---

## Files changed

| File | Change |
|---|---|
| `src/core/MindMapDocument.h` | `bool collapsed_ = false` on `MindMapNode` |
| `src/ui/canvas/CanvasNode.h` | `bool collapsed_ = false` on `CanvasNode` |
| `src/ui/MindMapCanvasView.h` | new methods + member |
| `src/ui/MindMapCanvasView.cpp` | `CollapseNode`, `ExpandNode`, `LoadFrom`, `Reset`, `OnPrimaryDown`, `DrawMindMapNodes` |
| `src/ui/commands/CollapseNodeCommand.h` | new |
| `src/ui/commands/CollapseNodeCommand.cpp` | new |
| `src/ui/UiCommandId.h` | `ToggleCollapsed` |
| `src/ui/ShortcutRegistry.h` | `CollapseNode` + Space |
| `src/ui/UiCommandDispatcher.cpp` | `ToggleCollapsed` case |
| `src/ui/MindMapUi.cpp` | `HandleCanvasPointerInput`, `RenderEditMenu`, help text, `RenderCanvas` |
| `src/core/JsonNativeDocumentRepository.cpp` | read/write `collapsed_` |
| `CMakeLists.txt` | add `CollapseNodeCommand.cpp` |
