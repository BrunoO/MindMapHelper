#pragma once

#include "ui/UiState.h"

namespace mind_map::app { class DocumentSessionService; }
namespace mind_map::ui::commands { class CommandHistory; }

namespace mind_map::ui {

// Called each frame between ImGui::NewFrame() and ImGui::Render(). Main thread only.
void RenderMainUi(UiState& state, mind_map::app::DocumentSessionService& session,
                  commands::CommandHistory& history);

}  // namespace mind_map::ui
