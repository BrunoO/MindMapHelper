#pragma once

#include <cstdint>

namespace mind_map::ui {

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
  PasteImage
};

}  // namespace mind_map::ui
