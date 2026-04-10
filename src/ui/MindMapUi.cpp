#include "ui/MindMapUi.h"

#include "demos/IDemo.h"
#include "ui/canvas/CanvasMath.h"

#include "imgui.h"

#include <algorithm>
#include <cassert>
#include <memory>
#include <vector>

namespace mind_map::ui {

namespace {

constexpr float kMinZoom = 0.35F;
constexpr float kMaxZoom = 3.0F;

}  // namespace

void RenderMainUi() {
  static bool show_demo_window = true;
  if (show_demo_window) {
    ImGui::ShowDemoWindow(&show_demo_window);
  }

  static std::vector<std::unique_ptr<mind_map::demos::IDemo>> demos = mind_map::demos::CreateDemoList();
  static int demo_index = 0;
  assert(!demos.empty());
  demo_index = (std::clamp)(demo_index, 0, static_cast<int>(demos.size()) - 1);

  static ImVec2 pan_px = {40.0F, 120.0F};
  static float zoom = 1.0F;

  mind_map::demos::IDemo* const demo = demos[static_cast<size_t>(demo_index)].get();
  assert(demo != nullptr);

  ImGui::Begin("bMindMap");
  ImGui::Checkbox("ImGui Demo", &show_demo_window);
  ImGui::Separator();

  if (ImGui::BeginCombo("Demo", demo->GetName())) {
    for (int i = 0; i < static_cast<int>(demos.size()); ++i) {
      const bool selected = (i == demo_index);
      if (ImGui::Selectable(demos[static_cast<size_t>(i)]->GetName(), selected)) {
        demo_index = i;
      }
      if (selected) {
        ImGui::SetItemDefaultFocus();
      }
    }
    ImGui::EndCombo();
  }
  ImGui::SameLine();
  if (ImGui::Button("Reset layout")) {
    demo->Reset();
  }

  ImGui::TextUnformatted("Canvas: drag nodes. Drag empty space to pan. Mouse wheel zooms.");
  ImGui::Text("Zoom %.2f", static_cast<double>(zoom));

  const ImVec2 canvas_p0 = ImGui::GetCursorScreenPos();
  const ImVec2 canvas_sz = ImGui::GetContentRegionAvail();
  const ImVec2 canvas_p1 = {canvas_p0.x + canvas_sz.x, canvas_p0.y + canvas_sz.y};

  ImGui::InvisibleButton("mind_map_canvas", canvas_sz, ImGuiButtonFlags_MouseButtonLeft);
  const bool canvas_hovered = ImGui::IsItemHovered();
  const bool canvas_item_active = ImGui::IsItemActive();
  const ImGuiIO& io = ImGui::GetIO();

  if (canvas_hovered && io.MouseWheel != 0.0F) {
    const float prev_zoom = zoom;
    const float wheel = io.MouseWheel;
    zoom = (std::clamp)(zoom + wheel * 0.1F, kMinZoom, kMaxZoom);

    const ImVec2 mouse = io.MousePos;
    const ImVec2 world_under_cursor = mind_map::canvas::ScreenToWorld(mouse, canvas_p0, pan_px, prev_zoom);
    pan_px.x += world_under_cursor.x * (prev_zoom - zoom);
    pan_px.y += world_under_cursor.y * (prev_zoom - zoom);
  }

  const ImVec2 mouse_world = mind_map::canvas::ScreenToWorld(io.MousePos, canvas_p0, pan_px, zoom);
  mind_map::demos::DemoPointerState ptr = {};
  ptr.mouse_screen = io.MousePos;
  ptr.mouse_world = mouse_world;
  ptr.canvas_hovered = canvas_hovered;

  if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) && canvas_hovered) {
    demo->OnPrimaryDown(ptr);
  }

  if (canvas_item_active && ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
    if (demo->IsDraggingContent()) {
      demo->OnPrimaryDrag(ptr);
    }
    else {
      pan_px.x += io.MouseDelta.x;
      pan_px.y += io.MouseDelta.y;
    }
  }

  if (ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
    demo->OnPrimaryUp();
  }

  mind_map::demos::DemoRenderContext render_ctx = {};
  render_ctx.draw_list = ImGui::GetWindowDrawList();
  render_ctx.canvas_p0 = canvas_p0;
  render_ctx.canvas_p1 = canvas_p1;
  render_ctx.pan_px = pan_px;
  render_ctx.zoom = zoom;
  render_ctx.canvas_hovered = canvas_hovered;
  render_ctx.mouse_world = mouse_world;

  render_ctx.draw_list->PushClipRect(canvas_p0, canvas_p1, true);
  demo->Render(render_ctx);
  render_ctx.draw_list->PopClipRect();

  ImGui::End();
}

}  // namespace mind_map::ui
