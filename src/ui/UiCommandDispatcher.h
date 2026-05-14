#pragma once

#include "ui/UiCommandId.h"
#include "ui/UiState.h"

namespace mind_map::app { class DocumentSessionService; }
namespace mind_map::ui::commands { class CommandHistory; }

namespace mind_map::ui {

/// Routes UiCommandId actions from menus and shortcuts to mutations on UiState and DocumentSessionService.
class UiCommandDispatcher final {
 public:
  explicit UiCommandDispatcher(commands::CommandHistory& history);

  void Dispatch(UiCommandId command, UiState& state,
                mind_map::app::DocumentSessionService& session) const;

 private:
  commands::CommandHistory& history_;  // NOLINT(cppcoreguidelines-avoid-const-or-ref-data-members) - bound to MindMapUi lifetime
};

}  // namespace mind_map::ui
