#include "ui/MindMapUi.h"

#include "imgui.h"

namespace mind_map::ui {

void RenderMainUi() {
  static bool show_demo_window = true;
  if (show_demo_window) {
    ImGui::ShowDemoWindow(&show_demo_window);
  }

  ImGui::Begin("bMindMap");
  ImGui::Text("ImGui + GLFW + OpenGL3 scaffold. Build mind-map UI here.");
  ImGui::Checkbox("ImGui Demo", &show_demo_window);
  const float ms = 1000.0F / ImGui::GetIO().Framerate;
  ImGui::Text("Average %.3f ms/frame (%.1f FPS)", static_cast<double>(ms),
              static_cast<double>(ImGui::GetIO().Framerate));
  ImGui::End();
}

}  // namespace mind_map::ui
