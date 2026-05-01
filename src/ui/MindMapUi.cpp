#include "ui/MindMapUi.h"

#include "ui/canvas/CanvasMath.h"
#include "ui/demos/IDemo.h"

#include "imgui.h"

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <memory>
#include <vector>

namespace mind_map::ui {

namespace {

constexpr float kMinZoom = 0.35F;
constexpr float kMaxZoom = 3.0F;
constexpr float kZoomStep = 0.1F;
constexpr float kStatusBarHeight = 26.0F;
constexpr float kInitialPanX = 40.0F;
constexpr float kInitialPanY = 120.0F;

enum class UiCommandId : std::uint8_t {
  ResetLayout,
  ZoomIn,
  ZoomOut,
  ResetView,
  ToggleStatusBar
};

struct UiState {
  std::vector<std::unique_ptr<mind_map::demos::IDemo>> demos_;
  int demo_index_ = 0;
  ImVec2 pan_px_ = {kInitialPanX, kInitialPanY};
  float zoom_ = 1.0F;
  bool show_status_bar_ = true;
};

class UiCommandDispatcher final {
 public:
  void Dispatch(UiCommandId command, mind_map::demos::IDemo& demo, ImVec2& pan_px, float& zoom, bool& show_status_bar) const {
    switch (command) {
      case UiCommandId::ResetLayout:
        demo.Reset();
        return;
      case UiCommandId::ZoomIn:
        zoom = std::clamp(zoom + kZoomStep, kMinZoom, kMaxZoom);
        return;
      case UiCommandId::ZoomOut:
        zoom = std::clamp(zoom - kZoomStep, kMinZoom, kMaxZoom);
        return;
      case UiCommandId::ResetView:
        zoom = 1.0F;
        pan_px = {kInitialPanX, kInitialPanY};
        return;
      case UiCommandId::ToggleStatusBar:
        show_status_bar = !show_status_bar;
        return;
    }
  }
};

[[nodiscard]] mind_map::demos::IDemo* GetSelectedDemo(UiState& state) {
  if (state.demos_.empty()) {
    state.demos_ = mind_map::demos::CreateDemoList();
  }
  assert(!state.demos_.empty());
  state.demo_index_ = std::clamp(state.demo_index_, 0, static_cast<int>(state.demos_.size()) - 1);
  return state.demos_.at(static_cast<size_t>(state.demo_index_)).get();
}

void RenderDemoSelector(UiState& state) {
  const mind_map::demos::IDemo* const selected_demo = GetSelectedDemo(state);
  assert(selected_demo != nullptr);
  if (!ImGui::BeginCombo("Demo", selected_demo->GetName())) {
    return;
  }

  for (int i = 0; i < static_cast<int>(state.demos_.size()); ++i) {
    const bool selected = (i == state.demo_index_);
    if (ImGui::Selectable(state.demos_.at(static_cast<size_t>(i))->GetName(), selected)) {
      state.demo_index_ = i;
    }
    if (selected) {
      ImGui::SetItemDefaultFocus();
    }
  }
  ImGui::EndCombo();
}

void RenderMainMenuBar(UiCommandDispatcher& dispatcher, mind_map::demos::IDemo& demo, ImVec2& pan_px, float& zoom,
                       bool& show_status_bar) {
  if (!ImGui::BeginMainMenuBar()) {
    return;
  }

  if (ImGui::BeginMenu("File")) {
    if (ImGui::MenuItem("Reset Layout")) {
      dispatcher.Dispatch(UiCommandId::ResetLayout, demo, pan_px, zoom, show_status_bar);
    }
    if (ImGui::MenuItem("Reset View")) {
      dispatcher.Dispatch(UiCommandId::ResetView, demo, pan_px, zoom, show_status_bar);
    }
    ImGui::EndMenu();
  }

  if (ImGui::BeginMenu("View")) {
    if (ImGui::MenuItem("Zoom In", "Cmd+=")) {
      dispatcher.Dispatch(UiCommandId::ZoomIn, demo, pan_px, zoom, show_status_bar);
    }
    if (ImGui::MenuItem("Zoom Out", "Cmd+-")) {
      dispatcher.Dispatch(UiCommandId::ZoomOut, demo, pan_px, zoom, show_status_bar);
    }
    if (ImGui::MenuItem("Reset View", "Cmd+0")) {
      dispatcher.Dispatch(UiCommandId::ResetView, demo, pan_px, zoom, show_status_bar);
    }
    if (ImGui::MenuItem("Show Status Bar", nullptr, show_status_bar)) {
      dispatcher.Dispatch(UiCommandId::ToggleStatusBar, demo, pan_px, zoom, show_status_bar);
    }
    ImGui::EndMenu();
  }

  if (ImGui::BeginMenu("Help")) {
    ImGui::MenuItem("Canvas: drag nodes. Drag empty space to pan. Mouse wheel zooms.", nullptr, false, false);
    ImGui::EndMenu();
  }

  ImGui::EndMainMenuBar();
}

void HandleCanvasZoom(const ImGuiIO& io, bool canvas_hovered, ImVec2 canvas_p0, ImVec2& pan_px, float& zoom) {
  if (!canvas_hovered || io.MouseWheel == 0.0F) {
    return;
  }
  const float previous_zoom = zoom;
  zoom = std::clamp(zoom + (io.MouseWheel * kZoomStep), kMinZoom, kMaxZoom);
  const ImVec2 world_under_cursor = mind_map::canvas::ScreenToWorld(io.MousePos, canvas_p0, pan_px, previous_zoom);
  pan_px.x += world_under_cursor.x * (previous_zoom - zoom);
  pan_px.y += world_under_cursor.y * (previous_zoom - zoom);
}

void HandleCanvasPointerInput(bool canvas_hovered, bool canvas_item_active, const ImGuiIO& io,
                              const mind_map::demos::DemoPointerState& pointer_state, ImVec2& pan_px,
                              mind_map::demos::IDemo& demo) {
  if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) && canvas_hovered) {
    demo.OnPrimaryDown(pointer_state);
  }

  if (canvas_item_active && ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
    if (demo.IsDraggingContent()) {
      demo.OnPrimaryDrag(pointer_state);
    }
    else {
      pan_px.x += io.MouseDelta.x;
      pan_px.y += io.MouseDelta.y;
    }
  }

  if (ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
    demo.OnPrimaryUp();
  }
}

void RenderCanvas(UiState& state, mind_map::demos::IDemo& demo) {
  const ImVec2 canvas_p0 = ImGui::GetCursorScreenPos();
  const ImVec2 canvas_sz = ImGui::GetContentRegionAvail();
  const ImVec2 canvas_p1 = {canvas_p0.x + canvas_sz.x, canvas_p0.y + canvas_sz.y};

  ImGui::InvisibleButton("mind_map_canvas", canvas_sz, ImGuiButtonFlags_MouseButtonLeft);
  const bool canvas_hovered = ImGui::IsItemHovered();
  const bool canvas_item_active = ImGui::IsItemActive();
  const ImGuiIO& io = ImGui::GetIO();

  HandleCanvasZoom(io, canvas_hovered, canvas_p0, state.pan_px_, state.zoom_);

  const ImVec2 mouse_world = mind_map::canvas::ScreenToWorld(io.MousePos, canvas_p0, state.pan_px_, state.zoom_);
  mind_map::demos::DemoPointerState pointer_state = {};
  pointer_state.mouse_screen = io.MousePos;
  pointer_state.mouse_world = mouse_world;
  pointer_state.canvas_hovered = canvas_hovered;
  HandleCanvasPointerInput(canvas_hovered, canvas_item_active, io, pointer_state, state.pan_px_, demo);

  mind_map::demos::DemoRenderContext render_context = {};
  render_context.draw_list = ImGui::GetWindowDrawList();
  render_context.canvas_p0 = canvas_p0;
  render_context.canvas_p1 = canvas_p1;
  render_context.pan_px = state.pan_px_;
  render_context.zoom = state.zoom_;
  render_context.canvas_hovered = canvas_hovered;
  render_context.mouse_world = mouse_world;

  render_context.draw_list->PushClipRect(canvas_p0, canvas_p1, true);
  demo.Render(render_context);
  render_context.draw_list->PopClipRect();
}

void RenderStatusBar(const UiState& state, const mind_map::demos::IDemo& demo) {
  if (!state.show_status_bar_) {
    return;
  }
  ImGui::Separator();
  ImGui::Text("Status  Demo: %s  |  Zoom: %.2f  |  Pan: (%.1f, %.1f)", demo.GetName(), static_cast<double>(state.zoom_),
              static_cast<double>(state.pan_px_.x), static_cast<double>(state.pan_px_.y));
}

}  // namespace

void RenderMainUi() {
  static UiState state;
  mind_map::demos::IDemo* const demo = GetSelectedDemo(state);
  assert(demo != nullptr);

  UiCommandDispatcher command_dispatcher;
  RenderMainMenuBar(command_dispatcher, *demo, state.pan_px_, state.zoom_, state.show_status_bar_);

  const ImGuiViewport* const viewport = ImGui::GetMainViewport();
  assert(viewport != nullptr);
  ImGui::SetNextWindowPos(viewport->WorkPos);
  ImGui::SetNextWindowSize(viewport->WorkSize);
  ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0F);
  ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0F);
  const auto root_flags = static_cast<ImGuiWindowFlags>(ImGuiWindowFlags_NoTitleBar + ImGuiWindowFlags_NoResize +
                                                        ImGuiWindowFlags_NoMove + ImGuiWindowFlags_NoCollapse +
                                                        ImGuiWindowFlags_NoSavedSettings);
  ImGui::Begin("bMindMap", nullptr, root_flags);

  const float content_height = state.show_status_bar_ ? -kStatusBarHeight : 0.0F;
  if (ImGui::BeginChild("bMindMapContent", ImVec2(0.0F, content_height), ImGuiChildFlags_None, ImGuiWindowFlags_None)) {
    RenderDemoSelector(state);
    ImGui::SameLine();
    if (ImGui::Button("Reset layout")) {
      command_dispatcher.Dispatch(UiCommandId::ResetLayout, *demo, state.pan_px_, state.zoom_, state.show_status_bar_);
    }
    ImGui::TextUnformatted("Canvas: drag nodes. Drag empty space to pan. Mouse wheel zooms.");
    ImGui::Text("Zoom %.2f", static_cast<double>(state.zoom_));
    RenderCanvas(state, *demo);
  }
  ImGui::EndChild();

  RenderStatusBar(state, *demo);
  ImGui::End();
  ImGui::PopStyleVar(2);
}

}  // namespace mind_map::ui
