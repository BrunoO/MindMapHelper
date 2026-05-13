# C++17 Cast Conventions — When Casts Are Wrong vs Necessary

## Overview

Type casts are a code smell **until proven necessary**. Every cast is the compiler asking you: "are you sure?" The right answer in this codebase is, in priority order:

1. **No cast** — fix the type at the declaration so the cast is unnecessary.
2. **`static_cast`** — when widening/narrowing between numeric types or crossing enum/integer boundaries that the type system genuinely requires.
3. **`reinterpret_cast`** — only at byte-reinterpretation boundaries (e.g. `char*` → `unsigned char*` for stb_image, libzip).
4. **`const_cast`** — practically never; flag if you encounter it.
5. **C-style casts** — forbidden (`cppcoreguidelines-pro-type-cstyle-cast` is enabled).

This document captures the rules derived from the 2026‑05‑13 cast audit, which reduced the project's cast count from ~91 to ~38 occurrences (−58%) while removing two structural anti-patterns.

---

## Decision tree

Before adding a `static_cast`, ask three questions in order:

### Q1. Is there a better type?

If the source/destination type would naturally make the cast unnecessary, **change the type** instead of casting:

| Symptom | Better type |
|---|---|
| `static_cast<size_t>(i)` to index `std::vector` / `std::array` | Make `i` a `std::size_t` from the start |
| `static_cast<int>(SomeEnum::Count)` to iterate | Range-for over the underlying `std::array` |
| `static_cast<size_t>(parent_int_with_-1_sentinel)` | `std::optional<std::size_t>` instead of `int` sentinel |
| `static_cast<float>(int_count)` for layout | `float` if the value is only ever used in math, or keep `int` and cast once |

### Q2. Is this a genuine boundary?

If the cast is at the edge of the type system rather than the middle of it, it stays. The four legitimate boundaries are:

1. **Third-party C / FFI APIs** — libzip's signed/unsigned indices, stb_image's `unsigned char*`, OpenGL ↔ ImGui texture handles, Objective-C `NSData` bridge.
2. **Inherent `int ↔ float` math** — parametric `t = i / N` for curves, `floor`/`ceil` for grid quantization.
3. **Byte arithmetic** — reading a `char` as an unsigned byte to avoid sign-extension UB; centralised in `core::ToUnsignedByte`.
4. **Discarding `[[nodiscard]]` returns** — `static_cast<void>(std::filesystem::remove(p))` when the failure mode is documented and acceptable.

### Q3. Does this pattern repeat?

If the same multi-step cast appears **three or more times**, extract a named helper. Existing examples:

- `core::ToUnsignedByte(char) -> uint8_t` in `src/core/ByteOps.h` — replaces the `static_cast<uint32_t>(static_cast<uint8_t>(c))` double-cast across Base64 and the UTF-8 decoder.
- Named `constexpr` for FFI-required scalar conversions: `constexpr int kPolylinePointCount = static_cast<int>(kBranchStripSegments) + 1;` states the cast once instead of inline at each `AddPolyline` call site.

---

## Patterns to refactor away

### 1. Loop counter `int` against `size_t` container

```cpp
// ❌ Cast at every index
for (int i = 0; i < kBranchStripSegments; ++i) {
  array[static_cast<size_t>(i)] = ...;
}

// ✅ Pick the right type at the declaration
for (std::size_t i = 0; i < kBranchStripSegments; ++i) {
  array[i] = ...;
}
```

If the loop bound is a `constexpr int`, also promote it: `constexpr std::size_t kBranchStripSegments = 32U;`.

### 2. `enum class` ↔ `int` round-trip for iteration

```cpp
// ❌ Two casts plus a hand-maintained parallel switch
for (int i = 0; i < static_cast<int>(SA::Count); ++i) {
  const auto action = static_cast<SA>(i);
  const ShortcutDef& def = FindShortcut(action);
  switch (action) {
    case SA::ZoomIn: dispatcher.Dispatch(UiCommandId::ZoomIn, ...); break;
    case SA::ZoomOut: dispatcher.Dispatch(UiCommandId::ZoomOut, ...); break;
    // ...
  }
}

// ✅ Store the target in the table; range-for it
for (const ShortcutDef& def : kShortcuts) {
  if (!IsTriggered(def, io)) continue;
  dispatcher.Dispatch(def.command_id_, state, session);
}
```

The table becomes the single source of truth (Open/Closed).

### 3. `int` sentinel for "maybe an index"

```cpp
// ❌ -1 is the magic value, callers cast and compare
struct NodeSpec {
  const char* label_;
  int parent_;   // -1 means "no parent"
};
if (spec.parent_ >= 0) {
  parent_id = ids[static_cast<size_t>(spec.parent_)];
}

// ✅ Make absence explicit; type system enforces the check
struct NodeSpec {
  const char* label_;
  std::optional<std::size_t> parent_idx_;
};
if (spec.parent_idx_.has_value()) {
  parent_id = ids[*spec.parent_idx_];
}
```

This also aligns with `CanvasNode::parent_idx_` — one mental model for "may have a parent" across the codebase.

### 4. `static_cast<double>(float)` for `printf`-style varargs

```cpp
// ❌ Pure noise — vararg promotion is automatic
ImGui::Text("Zoom %.2f", static_cast<double>(state.zoom_));

// ✅ Just pass the float
ImGui::Text("Zoom %.2f", state.zoom_);
```

`cppcoreguidelines-pro-type-vararg` and `hicpp-vararg` are explicitly disabled in `.clang-tidy` for ImGui interop. The cast adds nothing and obscures the call.

---

## Boundaries where casts stay

These are intentional. Don't "fix" them.

### A. Third-party API interop

```cpp
// stb_image expects unsigned char*; string_view::data() is const char*
const auto* raw_ptr = reinterpret_cast<const stbi_uc*>(png_bytes.data());  // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)

// ImTextureID is ImU64; GLuint is uint32_t — interop between OpenGL and ImGui
return static_cast<ImTextureID>(tex);

// libzip indexes with unsigned, but counts/errors are signed
zip_get_name(archive, static_cast<zip_uint64_t>(i), 0);
```

### B. Inherent int ↔ float math

```cpp
// Parametric curve sampling — t must be float, i must be a non-negative integer
const float t = static_cast<float>(i) / static_cast<float>(kBranchStripSegments);

// Grid quantization — floor/ceil produces a float, indexing needs int
const auto ix0 = static_cast<int>(std::floor(world_min.x / kMinorStep));
```

### C. The canonical UB-safe byte/`char` idioms

```cpp
// std::tolower takes unsigned char to avoid UB on negative char; output back to char
std::transform(ext.begin(), ext.end(), ext.begin(),
               [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

// Reading a raw byte as unsigned — use the centralised helper
const std::uint32_t b = mind_map::core::ToUnsignedByte(raw[i]);
```

### D. FFI scalar boundaries — name the cast once

```cpp
// ❌ Cast at every call site
draw_list->AddPolyline(left_s.data(), static_cast<int>(kBranchStripSegments + 1U), ...);
draw_list->AddPolyline(right_s.data(), static_cast<int>(kBranchStripSegments + 1U), ...);

// ✅ One named constexpr explains the boundary
constexpr int kPolylinePointCount = static_cast<int>(kBranchStripSegments) + 1;  // AddPolyline takes int
draw_list->AddPolyline(left_s.data(),  kPolylinePointCount, ...);
draw_list->AddPolyline(right_s.data(), kPolylinePointCount, ...);
```

---

## DRY trigger rule

When the same cast sequence appears **≥3 times**, extract a helper or named constant. Examples already in the codebase:

| Repeated pattern | Helper |
|---|---|
| `static_cast<uint32_t>(static_cast<uint8_t>(c))` (×14 across Base64 + UTF-8 decoder) | `core::ToUnsignedByte(char) -> std::uint8_t` |
| `static_cast<int>(kBranchStripSegments + 1)` at two `AddPolyline` sites | `constexpr int kPolylinePointCount` |

Place byte/numeric helpers in `src/core/` (platform-neutral). Place rendering-specific named bindings in the smallest scope that contains every call site.

---

## What about `static_cast<void>(...)`?

Tolerated for discarding `[[nodiscard]]` returns when the failure mode is acceptable, e.g. in test cleanup:

```cpp
static_cast<void>(std::filesystem::remove(tmp));
```

Prefer the `std::error_code` overload (`std::filesystem::remove(tmp, ec)`) when available — it removes the need to discard at all.

---

## Enforcement

| Tool | Check | What it catches |
|---|---|---|
| **clang-tidy** | `cppcoreguidelines-pro-type-cstyle-cast` | C-style casts (forbidden) |
| **clang-tidy** | `cppcoreguidelines-pro-type-const-cast` | `const_cast` (avoid) |
| **clang-tidy** | `cppcoreguidelines-pro-type-reinterpret-cast` | `reinterpret_cast` (FFI only; NOLINT with rationale) |
| **clang-tidy** | `bugprone-narrowing-conversions` | Implicit narrowing; explicit `static_cast` here is correct |
| **Sonar** | cpp:S859, cpp:S3630, cpp:S3670 | Mirror the above three |

Before committing C++ changes that add or modify a cast, ask the three questions in the decision tree. If you still need the cast after answering them, leave a one-line comment explaining the boundary (or a NOLINT/NOSONAR on the same line per `.cursor/rules/suppressions-same-line.mdc`).

---

## Reference

- Quick rule: `.cursor/rules/cast-conventions-cpp.mdc`
- Anti-patterns enforcement: `docs/standards/SONAR_CPP_RULES_REFERENCE.md`
- AGENTS.md — Core C++17 conventions → Never (C-style casts)
- Audit commits: `3edf503`, `f9a7b94`, `a834c57`
