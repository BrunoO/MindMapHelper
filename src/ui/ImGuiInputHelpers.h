#pragma once

#include "imgui.h"

namespace mind_map::ui {

// ImGui normalises the primary modifier to io.KeyCtrl on all platforms:
// when ConfigMacOSXBehaviors is true (the default on Apple), ImGui swaps
// ImGuiMod_Super <-> ImGuiMod_Ctrl internally, so pressing Cmd on macOS sets
// io.KeyCtrl = true. Using io.KeySuper here would detect the physical Ctrl key.
[[nodiscard]] inline bool IsPrimaryShortcutModifierDown(const ImGuiIO& io) {
  return io.KeyCtrl;
}

[[nodiscard]] inline const char* GetPrimaryModifierName() {
#ifdef __APPLE__
  return "Cmd";
#else
  return "Ctrl";
#endif  // __APPLE__
}

}  // namespace mind_map::ui
