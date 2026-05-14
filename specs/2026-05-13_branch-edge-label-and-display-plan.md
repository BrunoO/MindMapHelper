# Plan: branch edge labels (`MindMapEdge::label_`), IMX import, and canvas display

## Goals

- Each **incoming edge** (parent → child) can carry its own **optional text**, distinct from the **child node’s** title when desired.
- **IMX import** maps MindManager-style `<branch>` cell text into that edge field instead of only folding it into the node title.
- The **canvas** draws that text along (or above) the branch using the existing `DrawMindMapBranchTextOnPath` helper (geometry to be refined later).
- **Persistence** round-trips through native JSON and through `MindMapCanvasView::ToDocument` / `LoadFrom`.

---

## 1) Extend `MindMapEdge`

**File:** `src/core/MindMapDocument.h`

- Add `std::string label_;` to `MindMapEdge` (empty = no edge-specific caption).
- Document semantics in a short comment: label is for the **edge from `parent_id_` to `child_id_`**; it does not rename the child node by itself.

**Call sites to touch (non-exhaustive; search for `MindMapEdge` / `edges_`):**


| Area                                        | Action                                                                                                                                                                                                          |
| ------------------------------------------- | --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| `src/core/JsonNativeDocumentRepository.cpp` | `ToJson`: emit `"label"` when non-empty (or always emit for simplicity). `FromJson`: `e.value("label", std::string{})` for backward compatibility.                                                              |
| JSON `version`                              | Today only `version == 1` loads. Prefer **optional `label` on v1** (no bump) **or** bump to `2` and teach `FromJson` to accept both versions; choose one policy and document it in this spec once decided.      |
| `src/ui/MindMapCanvasView.cpp`              | `LoadFrom`: when applying `doc.edges_`, copy `edge.label_` onto the **child** view node (see §3). `ToDocument`: when emitting an edge for a child with a parent, set `edge.label_` from that stored view field. |
| `tests/JsonRoundTripTest.cpp`               | Assert `label_` survives save/load.                                                                                                                                                                             |


**New nodes / edges created in-app** (e.g. `InsertChildNode`): initialize `MindMapEdge`-backed state with **empty** `label_` unless the product defines a default.

---

## 2) Improve IMX import to use `label_`

**Problem today:** `ImxMindMapLoader` builds `branch_text_by_target` and merges it into a **single** `ImxNode::text_` via `resolve_text`, so edge copy and node copy are indistinguishable in `MindMapDocument`.

**Direction:**

1. **Extend the IMX model** (`src/core/imx/ImxMindMapModel.h`) so branch-edge text is **preserved separately**, e.g.
  - `std::unordered_map<std::string, std::string> incoming_branch_text_by_target_id_` on `ImxMindMapModel`, **or**  
  - `std::string incoming_branch_text_` on `ImxNode` (filled when that node is the `target` of a `<branch>` with cell text).  
   Prefer whichever keeps `BuildDocumentEdges` / `BuildDocumentNodes` simple and avoids ambiguous multi-parent graphs (current model is tree-shaped).
2. `**ParseDataXml` / loader** (`src/core/imx/ImxMindMapLoader.cpp`): continue collecting `ImxBranchEdge` with `text_`, but **stop** folding branch-only text into the same string used for the node box if you want a clean split. Concretely:
  - Populate **element/body text** into one field.  
  - Populate `**<branch>` cell text** into the new per-target map / `incoming_branch_text_`.
3. `**BuildDocumentNodes` / `BuildDocumentEdges`** (`src/core/imx/ImxImportAdapter.cpp`):
  - `MindMapNode::label_`: from **node/body** text (and your existing fallbacks like `(no text)`), **not** from branch-edge-only captions unless you explicitly want duplicate display.  
  - `MindMapEdge::label_`: from the incoming branch text for that **child** id (UTF-8 string already extracted by `ExtractCellText`).
4. **Tests** (`tests/ImxImportAdapterTest.cpp` and any loader-focused tests): fixture or snippet where `<branch>` carries text; assert `edges_[i].label_` matches and **node** label behavior matches the rule you chose.
5. **Semantics note for IMX:** MindManager often puts the “idea” text on the **branch**; after this change, that text should land in `**MindMapEdge::label_`**. If the target `branchNode` has no body text, the **node** may still need a non-empty `label_` for the UI (placeholder vs duplicate edge text — **product decision**; document it in the test comments).

---

## 3) Wire display on the canvas

**View model:** extend `mind_map::ui::CanvasNode` with something like `std::string edge_label_;` (or reuse a name aligned with core: `branch_edge_label_`), cleared for root, set in `LoadFrom` from `MindMapEdge::label_`, serialized back in `ToDocument`.

**Render:** in `MindMapCanvasView::Render`, after `DrawOneChildBranch` (same loop over children with parent), if `!node.edge_label_.empty()` (and node active), call:

`mind_map::ui::branch::DrawMindMapBranchTextOnPath(branch_ctx, child_index, nodes_, node.edge_label_, options);`

**Options:** start with defaults from `BranchTextRenderOptions` (font/size/color, small `normal_offset_px_`). Later: outline for contrast, zoom gating.

**Insert / commands:** when adding a child, set `edge_label_` empty. Any future **edit edge label** command updates this field and marks the document dirty.

---

## 4) Important items often missed


| Topic                    | Notes                                                                                                                                                                                                    |
| ------------------------ | -------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| **Round-trip parity**    | `ToDocument` must emit `label_` for every reconstructed edge; `LoadFrom` must restore it. Any code path that **synthesizes** edges (demo graph, tests) must set the new field.                           |
| **JSON compatibility**   | Old files without `label`: treat as empty. If you bump `version`, implement read path for old version or reject with a clear error.                                                                      |
| **UUID stability**       | Edge `id_` already on `CanvasNode::edge_id_`; keep using it when re-saving so references stay stable.                                                                                                    |
| **Undo/redo**            | If edge labels become editable, add command objects mirroring `DeleteNodeCommand` / `InsertChildNodeCommand` patterns; include label in dirty/session sync.                                              |
| **UX (later milestone)** | Inline edit, properties panel, or double-click on edge to edit `edge_label_`; not required for read-only import + display.                                                                               |
| **Rendering quality**    | `BuildMindMapBranchTextPathWorld` still ignores real geometry per `BranchStyle` — plan a follow-up milestone to align path with Bezier / orthogonal / organic centerlines and optional per-glyph layout. |
| **Empty vs duplicate**   | Decide whether empty `edge.label_` means “draw nothing” vs “fall back to child `label_` on the path.” Write the rule once and use it in both import and render.                                          |
| **i18n / encoding**      | IMX already UTF-8; keep `std::string` UTF-8 end-to-end; no change if you stay consistent.                                                                                                                |
| **Documentation**        | If the repo keeps a JSON schema or user-facing format doc, update it with the new `label` field.                                                                                                         |


---

## Suggested implementation order

1. `MindMapEdge::label_` + JSON optional `label` + round-trip test + sample graph.
2. `CanvasNode` field + `LoadFrom` / `ToDocument` + `InsertChildNode` default.
3. IMX model + loader + adapter + import test.
4. `MindMapCanvasView::Render` → `DrawMindMapBranchTextOnPath`.
5. Follow-up: true path sampling + per-glyph text + editing + commands.

---

## Success criteria

- Saving and loading a `.mmh` preserves `MindMapEdge::label_`.  
- Importing representative IMX where labels live on `<branch>` yields non-empty `edges_[*].label_` and expected node titles per your chosen split rule.  
- Canvas shows imported edge text on the corresponding branch without manual duplication in node labels (unless you explicitly chose fallback duplication).

