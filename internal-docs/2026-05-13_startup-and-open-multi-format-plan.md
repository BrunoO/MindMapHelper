# Plan: CLI and open path supporting `.mmh` and `.imx` (and future formats)

**Date:** 2026-05-13  
**Status:** planning — not implemented  
**Context:** Today the first CLI argument is passed to `RunApp` and only `DocumentSessionService::Open` → `JsonNativeDocumentRepository::Load` is used. `.imx` import exists via `ImportService` and the File → Import dialog but is not wired for startup. This note captures a sound way to unify behavior.

---

## Current behavior (reference)

- **CLI:** `src/main.cpp` passes `argv[1]` to `mind_map::app::RunApp` as `startup_path`. `AppMain.cpp` calls `session.Open(startup_path, doc)` when non-empty.
- **Native load:** `JsonNativeDocumentRepository::Load` expects native JSON (`format` / `version`). The `.mmh` suffix is convention (dialogs, title); the repository does not require a specific extension.
- **Import:** `ImportService::ImportFile` routes `.imx` (lowercased extension) to `ImxImportAdapter`. The UI import flow uses `session.New`, loads the document into the canvas, and `MarkDirty()` so **Save** does not overwrite the foreign file without **Save As**.

---

## Design goals

1. **Single rule** for “load document from path” so CLI, future drag-and-drop, and optional unified file dialogs do not duplicate dispatch logic.
2. **Preserve semantics:**
   - **Native open:** bound to path, not dirty after successful load; **Save** writes JSON to that path via `JsonNativeDocumentRepository`.
   - **Import open:** treat like **Import** — do **not** set `current_path_` to the `.imx` (etc.) path, or **Save** would write JSON into a non-native file. Clear path and mark dirty (match existing import menu behavior).
3. **Future formats:** add adapters + registration, not scattered `if (ext == …)` in `AppMain` and UI.

---

## Recommended implementation shape

### 1. Central dispatcher (`app` or thin `core` facade)

Single entry used from `RunApp` (and optionally from file UI later), e.g. load outcome:

- `NativeAtPath` — document from repo, path is the native file.
- `ImportedNoPath` — document from import pipeline; session must not bind `current_path_` to the source foreign path.
- `Failed` — optional message for stderr/logging.

**Dispatch:**

- Normalize extension (see DRY below).
- If extension is **native** (e.g. `.mmh`, or strict policy only): `repo_->Load(path)`.
- If extension matches a **registered import** (`.imx` today): `ImportService::ImportFile(path)` (or a renamed `ImportByPath`).

**Extension policy (KISS):** Prefer **extension-first** — predictable for CLI and OS associations. Avoid “try JSON then fall back to import” unless there is a strong product reason (adds ambiguity and failed-parse cost).

### 2. Session API clarity

Avoid overloading `Open` with two meanings. Prefer:

- **`OpenNative(path, doc)`** (or keep `Open` as this only): load via repository, set `current_path_`, clear dirty.
- **`ApplyImportedDocument(doc)`** (or explicit sequence): equivalent to import menu — `New` + load document + `MarkDirty()`, **`current_path_` cleared**.

`RunApp` calls the appropriate method after dispatch succeeds.

### 3. `ImportService`: registry instead of a growing `if` chain

Replace a single `.imx` branch with a small **extension → adapter** table (e.g. vector of pairs or map) populated at construction. New format = new `IImportAdapter` + one registration line (**Open/Closed** at the import boundary).

### 4. DRY: shared extension helper

Extract `LowercaseExtensionOf` (or equivalent) from the anonymous namespace in `ImportService.cpp` into a tiny shared `core` helper (e.g. `PathExtension.h`) used by `ImportService` and the dispatcher so extension rules live in one place.

### 5. `main.cpp` / CLI (later)

One positional path remains sufficient for v1. If flags or multiple paths are needed later, add a minimal argv parser in `app`; keep `core` free of argv parsing.

### 6. UI alignment (optional)

If **Open** dialog should accept both `.mmh` and `.imx`, use the **same dispatcher** and apply **import session rules** for non-native extensions. Alternatively keep **Open** = native only and **Import** = foreign formats for clearer UX.

### 7. Tests

- **Unit:** dispatcher selects native vs import for representative extensions; unknown extension fails cleanly.
- **Integration-style:** temp `.mmh` and fixture `.imx` through the same path as startup (or shared helper): assert `current_path_` and dirty flags match the rules above.

---

## Principle mapping

| Principle | Application |
|-----------|--------------|
| **YAGNI** | One dispatcher + explicit session methods; no full CLI framework until needed. |
| **SOLID** | Session owns save/open invariants; imports stay behind `IImportAdapter` + registry; `app` composes repo + import service. |
| **KISS** | Extension-first dispatch; no magic-byte sniffing in v1. |
| **DRY** | Single extension normalization + single “load from path” entry for startup (and optional UI reuse). |

---

## Critical invariant

**Imported source paths must not become `current_path_` for `JsonNativeDocumentRepository::Save`** unless the product explicitly adds a different save backend for that format. Until then, match File → Import: no native path after import, dirty so **Save As** `.mmh` is the safe path.

---

## Related in-repo material

- `internal-docs/2026-05-11_file-management-plan-context.md` — file management and layer split.
- `specs/2026-05-11_imx-import-spec.md` — `.imx` format and adapter direction.
- `src/core/ImportService.cpp`, `src/app/DocumentSessionService.cpp`, `src/app/AppMain.cpp`, `src/ui/MindMapUi.cpp` (import vs open flows).
