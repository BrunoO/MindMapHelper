#pragma once

#include "ui/ImGuiInputHelpers.h"

#include "imgui.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>

namespace mind_map::ui {

enum class ShortcutAction : std::uint8_t {
  ZoomIn,
  ZoomOut,
  ResetView,
  DeleteNode,
  Undo,
  Redo,
  Count
};

struct ShortcutDef {
  ImGuiKey key_;
  bool primary_modifier_;
  bool shift_;
  bool want_text_input_exempt_;  // fires even when io.WantTextInput is true
  const char* key_label_;
  const char* description_;
};

// clang-format off
inline constexpr std::array<ShortcutDef, static_cast<size_t>(ShortcutAction::Count)> kShortcuts = {{
  /* ZoomIn    */ {ImGuiKey_Equal,  true,  false, false, "=",      "Zoom In"},
  /* ZoomOut   */ {ImGuiKey_Minus,  true,  false, false, "-",      "Zoom Out"},
  /* ResetView */ {ImGuiKey_0,      true,  false, false, "0",      "Reset View"},
  /* DeleteNode*/ {ImGuiKey_Delete, false, false, false, "Delete", "Delete selected node"},
  /* Undo      */ {ImGuiKey_Z,      true,  false, true,  "Z",      "Undo"},
  /* Redo      */ {ImGuiKey_Z,      true,  true,  true,  "Z",      "Redo"},
}};
// clang-format on

static_assert(std::size(kShortcuts) == static_cast<size_t>(ShortcutAction::Count),
              "kShortcuts must have one entry per ShortcutAction");

[[nodiscard]] constexpr const ShortcutDef& FindShortcut(ShortcutAction action) {
  return kShortcuts[static_cast<size_t>(action)];
}

[[nodiscard]] inline bool IsTriggered(const ShortcutDef& def, const ImGuiIO& io) {
  const bool primary_held = IsPrimaryShortcutModifierDown(io);
  if (def.primary_modifier_ && !primary_held) {
    return false;
  }
  if (!def.primary_modifier_ && primary_held) {
    return false;  // prevent Cmd+Delete from firing the bare Delete chord
  }
  if (def.primary_modifier_ && (def.shift_ != io.KeyShift)) {
    return false;  // shift must match when a primary modifier is used
  }
  return ImGui::IsKeyPressed(def.key_, /*repeat=*/false);
}

[[nodiscard]] inline std::string FormatLabel(const ShortcutDef& def) {
  if (!def.primary_modifier_) {
    return {def.key_label_};
  }
  std::string label = GetPrimaryModifierName();
  if (def.shift_) {
    label += "+Shift";
  }
  label += "+";
  label += def.key_label_;
  return label;
}

}  // namespace mind_map::ui
