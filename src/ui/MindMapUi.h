#pragma once

#include "ui/UiState.h"

namespace mind_map::ui {

// Called each frame between ImGui::NewFrame() and ImGui::Render(). Main thread only.
void RenderMainUi(UiState& state);

}  // namespace mind_map::ui
