#pragma once

#include "ui/UiState.h"

#include <cstdint>

namespace mind_map::app { class DocumentSessionService; }
namespace mind_map::ui::commands { class CommandHistory; }

namespace mind_map::ui {

enum class UiCommandId : std::uint8_t {  // NOLINT(performance-enum-size)
  ResetLayout,
  ZoomIn,
  ZoomOut,
  ResetView,
  ToggleStatusBar,
  DeleteNode,
  Undo,
  Redo
};

class UiCommandDispatcher final {
 public:
  explicit UiCommandDispatcher(commands::CommandHistory& history);

  void Dispatch(UiCommandId command, UiState& state,
                mind_map::app::DocumentSessionService& session) const;

 private:
  commands::CommandHistory& history_;  // NOLINT(cppcoreguidelines-avoid-const-or-ref-data-members) - bound to MindMapUi lifetime
};

}  // namespace mind_map::ui
