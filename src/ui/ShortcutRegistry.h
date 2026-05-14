#pragma once

#include "ui/ImGuiInputHelpers.h"
#include "ui/UiCommandId.h"

#include "imgui.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>

namespace mind_map::ui {

/// All keyboard-triggered actions; each maps 1:1 to a UiCommandId via kShortcuts.
enum class ShortcutAction : std::uint8_t {
  ZoomIn,
  ZoomOut,
  ResetView,
  DeleteNode,
  InsertChildNode,
  Undo,
  Redo,
  PasteImage,
  CollapseNode,
  Count
};

/// Key, modifier, display label, and UiCommandId for one keyboard shortcut; stored in kShortcuts.
struct ShortcutDef {
  ImGuiKey key_ = ImGuiKey_None;
  bool primary_modifier_ = false;
  bool shift_ = false;
  bool want_text_input_exempt_ = false;  // fires even when io.WantTextInput is true
  const char* key_label_ = nullptr;
  const char* description_ = nullptr;
  UiCommandId command_id_ = UiCommandId::ResetLayout;  // command dispatched when triggered
  ImGuiKey alt_key_ = ImGuiKey_None;        // optional second key; same modifier rules apply
  const char* alt_key_label_ = nullptr;
};

// clang-format off
inline constexpr std::array<ShortcutDef, static_cast<size_t>(ShortcutAction::Count)> kShortcuts = {{
  /* ZoomIn         */ {ImGuiKey_Equal,  true,  false, false, "=",      "Zoom In",                          UiCommandId::ZoomIn},
  /* ZoomOut        */ {ImGuiKey_Minus,  true,  false, false, "-",      "Zoom Out",                         UiCommandId::ZoomOut},
  /* ResetView      */ {ImGuiKey_0,      true,  false, false, "0",      "Reset View",                       UiCommandId::ResetView},
  /* DeleteNode     */ {ImGuiKey_Delete, false, false, false, "Delete", "Delete selected node",             UiCommandId::DeleteNode},
  /* InsertChildNode*/ {ImGuiKey_Tab,    false, false, false, "Tab",    "Insert child node",                UiCommandId::InsertChildNode, ImGuiKey_Insert, "Insert"},
  /* Undo           */ {ImGuiKey_Z,      true,  false, true,  "Z",      "Undo",                             UiCommandId::Undo},
  /* Redo           */ {ImGuiKey_Z,      true,  true,  true,  "Z",      "Redo",                             UiCommandId::Redo},
  /* PasteImage     */ {ImGuiKey_V,      true,  false, false, "V",      "Paste image or text into selected node", UiCommandId::PasteImage},
  /* CollapseNode   */ {ImGuiKey_Space,  false, false, false, "Space",  "Collapse/expand selected node",    UiCommandId::ToggleCollapsed},
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
  if (ImGui::IsKeyPressed(def.key_, /*repeat=*/false)) {
    return true;
  }
  return def.alt_key_ != ImGuiKey_None && ImGui::IsKeyPressed(def.alt_key_, /*repeat=*/false);
}

[[nodiscard]] inline std::string FormatLabel(const ShortcutDef& def) {
  std::string label;
  if (def.primary_modifier_) {
    label = GetPrimaryModifierName();
    if (def.shift_) {
      label += "+Shift";
    }
    label += "+";
  }
  label += def.key_label_;
  if (def.alt_key_ != ImGuiKey_None && def.alt_key_label_ != nullptr) {
    label += " / ";
    label += def.alt_key_label_;
  }
  return label;
}

}  // namespace mind_map::ui
