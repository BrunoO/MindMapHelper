# ImGui text-styling options — research notes
_2026-05-15_

## Context

Investigated what Dear ImGui v1.91.9 (+ imgui_freetype) provides for enriching
node-label editing and rendering without pulling in external libraries.

---

## What is already wired up

| Capability | Status | Location |
|---|---|---|
| FreeType renderer | **Active** — `#define IMGUI_ENABLE_FREETYPE` | `src/ui/imconfig.h:2`, `CMakeLists.txt:213` |
| Inter-Regular font | Loaded at DPI-aware size (base 15 px) | `src/app/AppMain.cpp:124` |
| `PushFont` / `PopFont` | Available; no second font loaded yet | — |
| `PushStyleColor` / `PushStyleVar` | Already used in `NodeEditOverlay.cpp` | `src/ui/NodeEditOverlay.cpp:77-80` |
| `ColorEdit4` / `ColorButton` | Available, not yet used | — |

---

## FreeType synthetic variants

Because FreeType is active, bold and oblique variants can be generated from the
same `Inter-Regular.ttf` at atlas-build time — no separate font file needed:

```cpp
ImFontConfig cfg;
cfg.FontBuilderFlags = ImGuiFreeTypeBuilderFlags_Bold;
io.Fonts->AddFontFromFileTTF("Inter-Regular.ttf", font_size_px, &cfg);
```

Available flags: `Bold`, `Oblique`, `LightHinting`, `MonoHinting`, `LoadColor`, `Bitmap`.

---

## Low-effort enrichments (ranked by effort)

### 1. Per-node text color — Low
- Add `ImU32 text_color_` to `CanvasNode` (default `IM_COL32_WHITE`).
- Serialise as a hex string in `LoadFrom` / `ToDocument`.
- Pass to the existing `AddText` call in `DrawMindMapNodes`.
- Expose via `ColorButton` in the toolbar (same row as branch style).

### 2. Bold / Oblique toggle — Low
- Load `Inter-Bold` (FreeType synthetic) into the atlas at startup alongside `Inter-Regular`.
- Add `bool bold_` (and optionally `bool italic_`) to `CanvasNode`.
- In `DrawMindMapNodes`: `ImGui::PushFont(bold_font); … ImGui::PopFont();`
- `NodeHalfExtentForLabel` already calls `ImGui::CalcTextSize`; push the correct font before measuring.

### 3. Node background color — Low
- Add `ImU32 bg_color_` to `CanvasNode`.
- Pass to the `AddRectFilled` call that already exists in node rendering.
- Toolbar: `ColorButton` opens inline picker.

### 4. Font size presets (S / M / L) — Medium
- Load Inter-Regular at 2–3 additional sizes at startup.
- Add `uint8_t font_size_idx_` to `CanvasNode`.
- Push the matching font before `CalcTextSize` **and** before `AddText` so geometry and rendering agree.
- Node size changes with font size — this is the main complexity.

---

## `InputTextMultiline` flags worth knowing

Currently only `ImGuiInputTextFlags_AutoSelectAll` is used (`NodeEditOverlay.cpp:83`).

| Flag | Use |
|---|---|
| `CallbackCharFilter` | Block/transform characters before insertion |
| `CallbackEdit` | Trigger live preview on every keystroke |
| `CallbackCompletion` | Tab-key autocomplete hook |
| `AllowTabInput` | Tab inserts `\t` instead of changing focus |
| `AlwaysOverwrite` | Overwrite mode |

---

## What ImGui cannot do cheaply

- **Mixed inline styling** (bold one word, regular the next) — no rich-text widget; would require drawing word-by-word manually.
- **Underline / strikethrough** — not supported natively; manual `AddLine` calls needed.
- **Font picker widget** — no built-in; would need a custom combo over loaded fonts.

---

## Markdown in node labels — imgui_markdown

Library: https://github.com/enkisoftware/imgui_markdown (header-only).

### What's relevant for mind maps
- ✓ **Bold / italic** — useful for emphasis within a label
- ✓ **Inline code** — monospace run inside a label
- ✗ **Headers (H1-H3)** — redundant; node hierarchy already expresses this
- ✗ **Bullet lists** — better represented as child nodes
- ✗ **Links** — edges already carry relationships

### Technical blockers (assessed 2026-05-15)

**1. Node sizing — the hard problem.**
`NodeHalfExtentForLabel` (`src/ui/canvas/NodeGeometry.h`) uses `ImGui::CalcTextSize` on the raw label string. Markdown syntax characters (`**`, `*`, `` ` ``) are counted as literal characters, so the predicted node rectangle will differ from the actual rendered layout height. Fixing this requires either a hidden dry-pass measuring markdown output, an approximation, or decoupling node size from label content (fixed size).

**2. Zoom scaling.**
`imgui_markdown` calls `ImGui::Text` at whatever the current ImGui font size is; it does not know about canvas zoom. The existing draw-list text bakes zoom in via `font_size = ImGui::GetFontSize() * ctx.zoom_` (`MindMapCanvasView.cpp:193`). A workaround is `ImGui::SetWindowFontScale(ctx.zoom_)` scoped around the markdown call, but that affects all widgets in the window context.

**3. Two-layer render split.**
All nodes currently draw in one draw-list pass. Markdown nodes would need a second widget-layer pass (same technique as `NodeEditOverlay` — `SetCursorScreenPos` + widget call), looping over visible nodes a second time.

**4. Font requirements.**
`imgui_markdown` takes explicit `ImFont*` pointers for bold / italic / heading fonts. FreeType synthetic bold and oblique (already available — see "FreeType synthetic variants" above) satisfy this without extra `.ttf` files.

### Recommended path

**Phase 1 — custom inline-markup renderer (low effort, draw-list native):**
Parse a small subset (`**bold**`, `*italic*`, `` `code` ``) in the label string manually. Split into styled runs. For each run, push the appropriate font (Inter-Bold synthetic, Inter-Oblique synthetic) and call `ctx.draw_list_->AddText`. `CalcTextSize` still works because each run is measured with its own font before rendering. No imgui_markdown dependency, no zoom issues, no sizing problem.

**Phase 2 — full imgui_markdown (medium/high effort):**
Only worthwhile if richer block-level formatting (bullets, rule separators) inside nodes is genuinely needed. The node-sizing dry-pass is the main design task to solve first.

---

## Serialisation notes

Any new per-node field must be round-tripped in:
- `src/ui/MindMapCanvasView.cpp` → `LoadFrom` (read) and `ToDocument` (write)
- `src/core/JsonNativeDocumentRepository.cpp` — if the field belongs on `MindMapNode` / `MindMapEdge` in the core model
- Alternatively, store UI-only fields only in `CanvasNode` and omit from the core model (then they are lost on save — usually not desirable for styling).
