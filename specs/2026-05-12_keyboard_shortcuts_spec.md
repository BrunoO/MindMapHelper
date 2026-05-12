# 2026-05-12 — Keyboard shortcuts specification

## Purpose

Define how MindMapHelper implements **15–20 keyboard shortcuts** using **Option A** (separate `ShortcutAction` from `UiCommandId`), reusing the **proven patterns** from the sibling project **USN_Windows** without coupling the repositories. The outcome is a maintainable registry, consistent **Ctrl vs Cmd** behavior, safe interaction with **text input**, and a single dispatch path shared with menus and toolbar actions.

## References

- USN_Windows (informal reference): `src/gui/ShortcutRegistry.h`, `src/core/ApplicationLogic.cpp` (`HandleKeyboardShortcuts`, `DispatchGlobalShortcut`), `src/gui/ImGuiUtils.h` (`IsPrimaryShortcutModifierDown`).
- MindMapHelper: `UiCommandId` / `UiCommandDispatcher` in `src/ui/MindMapUi.cpp`; layering rules in `AGENTS.md` (shortcut code lives under `**src/ui/`**, not `core/`).

## Goals

- **Declarative registry:** One `constexpr` table describes each shortcut (key, modifiers, scope, platform flags, display labels, help description).
- **Option A:** `ShortcutAction` identifies bindings; an explicit mapping connects each bound action to `**UiCommandId`** where the behavior is a real command; non-command UI-only effects may use a small `ShortcutAction` branch without polluting the command enum.
- **Single behavior path:** Menu items, buttons, and keys invoke the same `**UiCommandDispatcher::Dispatch`** for shared semantics (DRY).
- **Cross-platform correctness:** Primary modifier = **Ctrl** on Windows/Linux and **Cmd** (Super) on macOS; do not use `io.KeyCtrl` alone for “primary shortcut” semantics.
- **Discoverability:** Help UI lists shortcuts using `**FindShortcut`** + `**FormatLabel**` so labels stay aligned with behavior.

## Non-goals (initially)

- A dynamic plugin or user-rebindable keymap system.
- Porting USN_Windows `**ResultsTableKeyboard.cpp**`-scale logic until the canvas (or another surface) has comparable shortcut density and conflicts; until then, keep canvas-specific keys out of the main registry or add a **dedicated handler** with explicit conflict rules.
- Broad USN `**ImGuiUtils`** ports; only **input-related** helpers required for shortcuts.

## Architecture


| Component                                | Responsibility                                                                                                                         |
| ---------------------------------------- | -------------------------------------------------------------------------------------------------------------------------------------- |
| `ShortcutAction`                         | Stable enum for **bindings** and help (~15–20 values).                                                                                 |
| `ShortcutDef` / `kShortcuts`             | Immutable row: `ImGuiKey`, primary modifier flag, shift flag, scope, platform, `key_label`, description.                               |
| `UiCommandId`                            | Application **command** semantics; not every command requires a shortcut.                                                              |
| Mapping `ShortcutAction` → `UiCommandId` | Explicit `constexpr` table or function; may use `std::optional<UiCommandId>` when an action is UI-only or dispatched outside the enum. |
| `UiCommandDispatcher`                    | Implements command effects (shared with menus).                                                                                        |
| `HandleMindMapKeyboardShortcuts(...)`    | Per-frame: scan `kShortcuts`, apply filters, `**IsTriggered`**, map, `**Dispatch**` or minimal `switch` for non-command cases.         |


### Reuse from USN (required patterns)

1. `**IsPrimaryShortcutModifierDown(const ImGuiIO& io)**` — `io.KeyCtrl || io.KeySuper` (place in a small header under `src/ui/`, e.g. `ImGuiInputHelpers.h`).
2. `**IsTriggered(const ShortcutDef&, const ImGuiIO&)**` — primary modifier required when `ShortcutDef::primary_modifier`; **Shift must match** `ShortcutDef::shift` when a primary modifier is used (prevents e.g. Ctrl+Shift+X from firing the Ctrl+X chord).
3. **One-shot detection** — `ImGui::IsKeyPressed(key, false)` in the registry path (ignore repeat unless a future shortcut explicitly documents otherwise).
4. `**WantTextInput` gating** — When `io.WantTextInput` is true, **do not** fire bare-key globals that would override normal typing/IME (mirror USN: allow exceptions per shortcut, e.g. a dedicated refresh key if added).
5. `**GetPrimaryModifierName` / `FormatLabel`** — Help and menu shortcut strings show **Cmd** on Apple platforms and **Ctrl** elsewhere.

### Optional types (add when needed)

- `**ShortcutScope`:** Start with `**Global`**. Add `**CanvasFocused**` (or equivalent) when canvas chords would steal keys from global typing; second pass or ordered scan with documented precedence.
- `**ShortcutPlatform`:** Add only when a binding is genuinely Windows-only (or similar); otherwise all entries use “all platforms.”

## Functional requirements

### FR1 — Registry completeness

- Every `**ShortcutAction`** used for input must appear **exactly once** in `kShortcuts`.
- `**FindShortcut(ShortcutAction)`** returns the definition; debug builds may assert on missing entries (same discipline as USN).

### FR2 — Keyboard dispatch

- `**HandleMindMapKeyboardShortcuts**` runs **once per frame** from UI code (e.g. early in `RenderMainUi` or at a stable point after `ImGui::NewFrame()`), with access to `ImGui::GetIO()`, `UiCommandDispatcher`, `UiState`, and `DocumentSessionService` as needed.
- Processing order: filter by **scope** → **platform** → `**WantTextInput` policy** → `**IsTriggered`** → map → `**Dispatch**` or shortcut-only branch.

### FR3 — Mapping Option A

- For command-backed shortcuts: `**ShortcutAction` → `UiCommandId` → `UiCommandDispatcher::Dispatch**`.
- For chrome-only shortcuts (focus, toggle panel with no `UiCommandId`): handle in a `**switch (ShortcutAction)**` that calls small UI helpers; avoid inflating `UiCommandId` with non-commands.

### FR4 — Menus and help

- Menu `**MenuItem**` shortcut strings should be derived from `**FormatLabel(FindShortcut(...))**` where practical so menus match Help.
- Help section lists shortcuts in `**kShortcuts**` order (order the array for sensible grouping).

### FR5 — Layering and C++17

- All new shortcut types and registry live under `**src/ui/**` (ImGui dependency).
- C++17 only; follow `AGENTS.md` naming and literals (`F` suffix on floats where applicable in new code).

## Implementation phases

### Phase 1 — Vertical slice

- Add `**ShortcutRegistry.h**` (and `**.cpp**` only if non-inline helpers match USN split).
- Add `**ImGuiInputHelpers.h**` with `**IsPrimaryShortcutModifierDown**`.
- Wire `**HandleMindMapKeyboardShortcuts**` with **2–3** shortcuts mapped to existing `**UiCommandId`** values to validate the pipeline.

### Phase 2 — Full shortcut set

- Expand `**ShortcutAction**` and `**kShortcuts**` toward **15–20** entries; extend `**UiCommandId`** and `**UiCommandDispatcher**` for any new commands.
- Complete `**ShortcutAction` → `UiCommandId**` mapping for all command-backed actions.

### Phase 3 — Help and polish

- Extend **Help** UI to render bullets from the registry (pattern similar to USN `**HelpWindow.cpp`**).
- Align any remaining hardcoded accelerator strings in menus with `**FormatLabel**`.

### Phase 4 — Scoped shortcuts (as needed)

- Introduce `**ShortcutScope::CanvasFocused**` (or equivalent) and predicates (“canvas hovered / active, no text capture”) **only** when global shortcuts conflict with canvas interaction.

### Phase 5 — High-conflict surface (as needed)

- If the canvas gains many overlapping keys, add `**HandleCanvasKeyboardShortcuts`** with local `**KeyPressedOnce**` helpers and documented conflict rules; keep the main registry for **simple global chords** only.

## Quality and testing

- **Manual:** Verify matrix on **macOS** and **Windows** (or Linux) for primary modifier and Help labels.
- **Automated (optional):** Debug-only or test-only assertion that every `ShortcutAction` in the enum used for input has a registry row and a dispatch path; optional golden tests for `**FormatLabel`** if platform macros are stubbed in tests.

## Success criteria

- Adding a shortcut is **mechanical**: one `**ShortcutAction`**, one `**kShortcuts**` row, one **mapping** line, and (if new behavior) one `**UiCommandDispatcher`** case plus menu/Help consistency.
- Identical `**UiCommandId**` behavior whether invoked from **menu**, **button**, or **keyboard**.
- No accidental fires from **key repeat** on the registry path unless explicitly documented for a specific key.

## Open decisions (resolve during implementation)

- Exact `**WantTextInput`** exceptions per shortcut (whitelist for keys that must work in inputs).
- Whether `**FindShortcut**` remains O(n) linear search (fine for ~20 entries) or switches to a generated/static map for marginal clarity.
- Final `**ShortcutScope**` set once canvas shortcuts are specified.