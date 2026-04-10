#pragma once

namespace mind_map::ui {

// Called each frame between ImGui::NewFrame() and ImGui::Render(). Main thread only.
void RenderMainUi();

}  // namespace mind_map::ui
