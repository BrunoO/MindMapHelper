#pragma once

#include <cstdint>

namespace mind_map::ui {

/// All user-level actions; dispatched from menus and ShortcutRegistry via UiCommandDispatcher.
enum class UiCommandId : std::uint8_t {  // NOLINT(performance-enum-size)
  ResetLayout,
  ZoomIn,
  ZoomOut,
  ResetView,
  ToggleStatusBar,
  DeleteNode,
  InsertChildNode,
  Undo,
  Redo,
  Paste,
  ToggleCollapsed
};

}  // namespace mind_map::ui
