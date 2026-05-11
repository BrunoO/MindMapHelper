# 2026-05-11 — IMX mind map import specification

## Purpose

Define how MindMapHelper imports **`.imx`** files (iMindMap / ThinkBuzan Gaia-style archives: ZIP + XML), replacing or complementing a **PowerShell prototype** with a **C++17** implementation that fits the project’s planned **import adapter** architecture (`IImportAdapter`, `ImportService`; see `internal-docs/2026-05-11_file-management-plan-context.md`).

## Format assumptions

- An `.imx` file is a **ZIP** archive containing at least:
  - **`data.xml`** — graph structure, cells, properties.
  - **`mapmeta.xml`** — document metadata (e.g. `MapMeta` attributes).
- All user-visible text and outputs use **UTF-8** (requirement: correct handling of French and other accented Latin scripts).

## Functional requirements

### 1. Decompression

- Open `.imx` as a ZIP container using a C++ ZIP API (no dependency on an external unzip executable for core behavior).
- Read required entries (`data.xml`, `mapmeta.xml`) into memory or temporary buffers; fail clearly if entries are missing.

### 2. XML parsing

- Parse `data.xml` and `mapmeta.xml` with a **DOM-oriented** XML library suitable for attribute-based queries and nested element walks.
- Reject or report **malformed XML** without undefined behavior.

### 3. Tree construction (`data.xml`)

- Locate the root **`floatingIdea`** node.
- Build an index of elements by **`id`** (or document if IDs are duplicated and how to resolve).
- Resolve parent–child (or attachment) relationships using **`branch`** elements: **`@source` → `@target`**, matching the semantics validated in the PowerShell prototype (this spec treats the prototype as the reference for edge cases).

### 4. Text extraction

- For each logical cell/node, extract text from nested **`property`** elements where **`@key` equals `com.thinkbuzan.gaia.cell.text`** (exact string match).
- Associate extracted text with the correct node in the in-memory tree.

### 5. Metadata extraction (`mapmeta.xml`)

- Read **`MapMeta`** (or equivalent) attributes and any nested fields required for parity with the prototype, including at minimum:
  - title
  - author
  - creation date  
- Map metadata into the canonical document model’s metadata fields when the core `MindMapDocument` supports them.

### 6. Output mapping (in-app import)

- Convert the parsed IMX tree into the project’s canonical **`MindMapDocument`** (nodes, edges, layout where available).
- Expose this through an **`IImportAdapter`** implementation (e.g. `ImxImportAdapter`) consumed by **`ImportService`** so **File → Import** follows the same path as other formats after the session layer exists.

### 7. Optional sidecar export (prototype parity)

If a standalone extractor or regression tooling is desired, support writing to a caller-specified **output directory**:

| Format | Description |
|--------|-------------|
| **JSON** | Hierarchical tree + metadata; stable field names for tests. |
| **OPML 2.0** | Valid OPML XML with correct entity escaping. |
| **Plain text** | Recursive, indentation-based outline. |

These outputs are **optional** for the GUI app but useful for **golden-file** comparison against the PowerShell prototype.

### 8. Command-line interface (optional)

- Accept **input `.imx` path** and **output directory** when built as a small tool or when extending the main binary:
  - Example: `imx-tool <input.imx> <output-dir>`  
- Alternatively, rely solely on in-app **Import** once `argv[1]` startup open / session wiring exists; CLI remains recommended for CI and batch validation.

### 9. Robustness and errors

Handle without silent failure:

- invalid or non-ZIP files
- missing `data.xml` or `mapmeta.xml`
- malformed XML
- missing or duplicate `id` where the tree cannot be built
- dangling **`branch`** references (`source`/`target` not found)
- general I/O errors (open, read, write)

Errors should be **reportable** to the UI or stderr with a stable error code or message category for tests.

## Recommended C++ dependency stack

| Concern | Library (primary) | Alternatives |
|---------|-------------------|----------------|
| ZIP read | **libzip** | miniz, zlib (more manual) |
| XML DOM | **pugixml** | libxml2, Expat (event-style) |
| JSON | **nlohmann/json** | RapidJSON, simdjson (already planned for native format in-repo) |
| OPML / plain text | **none** (hand-written serializers) | — |
| CLI | **manual `argc`/`argv`** or **CLI11** | — |
| Unicode | **UTF-8 `std::string` / `std::string_view`** + careful I/O | ICU only if normalization proves necessary (defer per YAGNI) |

**Rationale (from prototype analysis):** libzip gives direct archive access; pugixml gives a small DOM API for `property` and `branch` queries; nlohmann/json matches the existing native JSON direction and supports exporter tests.

## Layering (AGENTS.md)

- **core**: IMX parse result DTOs, ZIP+XML orchestration, mapping to `MindMapDocument` — **no** ImGui, GLFW, or platform UI.
- **app**: session/use-case wiring (`Import` → adapter → replace document).
- **ui**: file dialog and user-facing error strings only.

## Testing strategy

1. **Minimal fixture**: a tiny ZIP in tests containing minimal `data.xml` / `mapmeta.xml` that exercises `floatingIdea`, `branch`, and `com.thinkbuzan.gaia.cell.text`.
2. **Golden files** (optional): compare JSON or plain-text output to the PowerShell prototype on one or more reference `.imx` files stored outside the repo or under `test/data/` if licensing permits.
3. **Adapter contract**: tests that `ImxImportAdapter` returns a consistent `MindMapDocument` for valid input and a defined failure for each error class in §9.

## Incremental delivery order

1. Add **libzip** and **pugixml** to CMake (FetchContent or `third_party/` submodule, consistent with other vendored deps).
2. Implement **ZIP open → XML load → internal IMX tree** in core.
3. Implement **`ImxImportAdapter`** mapping IMX tree → **`MindMapDocument`**.
4. Wire **Import** in the app once `ImportService` / `DocumentSessionService` exist.
5. Add optional **`imx-tool`** CLI and/or JSON/OPML/text exporters for regression parity.

## References

- PowerShell prototype: behavioral reference for ZIP layout, XML paths, and `branch`/`property` rules.
- `internal-docs/2026-05-11_file-management-plan-context.md` — native JSON, import adapters, session, file dialog.

## Open points

- Exact semantics when **`branch`** cycles or multiple parents appear (match prototype or document divergence).
- Whether real **`.imx`** samples can be committed to the repo (license/size); otherwise use synthetic fixtures only.
