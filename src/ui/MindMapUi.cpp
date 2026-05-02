#include "ui/MindMapUi.h"

#include "ui/MindMapCanvasView.h"
#include "ui/branch/BranchStyle.h"
#include "ui/canvas/CanvasMath.h"

#include "imgui.h"

#include <algorithm>
#include <cassert>
#include <cstdint>

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
  MindMapCanvasView canvas_;
  ImVec2 pan_px_ = {kInitialPanX, kInitialPanY};
  float zoom_ = 1.0F;
  bool show_status_bar_ = true;
};

class UiCommandDispatcher final {
 public:
  void Dispatch(UiCommandId command, MindMapCanvasView& canvas, ImVec2& pan_px, float& zoom,
                bool& show_status_bar) const {
    switch (command) {
      case UiCommandId::ResetLayout:
        canvas.Reset();
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

void RenderBranchStyleSelector(MindMapCanvasView& canvas) {
  const mind_map::ui::branch::BranchStyle current = canvas.GetBranchStyle();
  const char* const preview = mind_map::ui::branch::GetBranchStyleDisplayName(current);
  if (!ImGui::BeginCombo("Branch style", preview)) {
    return;
  }

  for (int i = 0; i < mind_map::ui::branch::kBranchStyleCount; ++i) {
    const mind_map::ui::branch::BranchStyle style = mind_map::ui::branch::BranchStyleFromIndex(i);
    const bool selected = (style == current);
    if (ImGui::Selectable(mind_map::ui::branch::GetBranchStyleDisplayName(style), selected)) {
      canvas.SetBranchStyle(style);
    }
    if (selected) {
      ImGui::SetItemDefaultFocus();
    }
  }
  ImGui::EndCombo();
}

void RenderMainMenuBar(UiCommandDispatcher& dispatcher, MindMapCanvasView& canvas, ImVec2& pan_px, float& zoom,
                       bool& show_status_bar) {
  if (!ImGui::BeginMainMenuBar()) {
    return;
  }

  if (ImGui::BeginMenu("File")) {
    if (ImGui::MenuItem("Reset Layout")) {
      dispatcher.Dispatch(UiCommandId::ResetLayout, canvas, pan_px, zoom, show_status_bar);
    }
    if (ImGui::MenuItem("Reset View")) {
      dispatcher.Dispatch(UiCommandId::ResetView, canvas, pan_px, zoom, show_status_bar);
    }
    ImGui::EndMenu();
  }

  if (ImGui::BeginMenu("View")) {
    if (ImGui::MenuItem("Zoom In", "Cmd+=")) {
      dispatcher.Dispatch(UiCommandId::ZoomIn, canvas, pan_px, zoom, show_status_bar);
    }
    if (ImGui::MenuItem("Zoom Out", "Cmd+-")) {
      dispatcher.Dispatch(UiCommandId::ZoomOut, canvas, pan_px, zoom, show_status_bar);
    }
    if (ImGui::MenuItem("Reset View", "Cmd+0")) {
      dispatcher.Dispatch(UiCommandId::ResetView, canvas, pan_px, zoom, show_status_bar);
    }
    if (ImGui::MenuItem("Show Status Bar", nullptr, show_status_bar)) {
      dispatcher.Dispatch(UiCommandId::ToggleStatusBar, canvas, pan_px, zoom, show_status_bar);
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
                              const MindMapPointerState& pointer_state, ImVec2& pan_px, MindMapCanvasView& canvas) {
  if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) && canvas_hovered) {
    canvas.OnPrimaryDown(pointer_state);
  }

  if (canvas_item_active && ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
    if (canvas.IsDraggingContent()) {
      canvas.OnPrimaryDrag(pointer_state);
    }
    else {
      pan_px.x += io.MouseDelta.x;
      pan_px.y += io.MouseDelta.y;
    }
  }

  if (ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
    canvas.OnPrimaryUp();
  }
}

void RenderCanvas(UiState& state) {
  const ImVec2 canvas_p0 = ImGui::GetCursorScreenPos();
  const ImVec2 canvas_sz = ImGui::GetContentRegionAvail();
  const ImVec2 canvas_p1 = {canvas_p0.x + canvas_sz.x, canvas_p0.y + canvas_sz.y};

  ImGui::InvisibleButton("mind_map_canvas", canvas_sz, ImGuiButtonFlags_MouseButtonLeft);
  const bool canvas_hovered = ImGui::IsItemHovered();
  const bool canvas_item_active = ImGui::IsItemActive();
  const ImGuiIO& io = ImGui::GetIO();

  HandleCanvasZoom(io, canvas_hovered, canvas_p0, state.pan_px_, state.zoom_);

  const ImVec2 mouse_world = mind_map::canvas::ScreenToWorld(io.MousePos, canvas_p0, state.pan_px_, state.zoom_);
  MindMapPointerState pointer_state = {};
  pointer_state.mouse_screen = io.MousePos;
  pointer_state.mouse_world = mouse_world;
  pointer_state.canvas_hovered = canvas_hovered;
  HandleCanvasPointerInput(canvas_hovered, canvas_item_active, io, pointer_state, state.pan_px_, state.canvas_);

  MindMapCanvasRenderContext render_context = {};
  render_context.draw_list = ImGui::GetWindowDrawList();
  render_context.canvas_p0 = canvas_p0;
  render_context.canvas_p1 = canvas_p1;
  render_context.pan_px = state.pan_px_;
  render_context.zoom = state.zoom_;
  render_context.canvas_hovered = canvas_hovered;
  render_context.mouse_world = mouse_world;

  render_context.draw_list->PushClipRect(canvas_p0, canvas_p1, true);
  state.canvas_.Render(render_context);
  render_context.draw_list->PopClipRect();
}

void RenderStatusBar(const UiState& state) {
  if (!state.show_status_bar_) {
    return;
  }
  ImGui::Separator();
  ImGui::Text("Status  Branch style: %s  |  Zoom: %.2f  |  Pan: (%.1f, %.1f)",
              mind_map::ui::branch::GetBranchStyleDisplayName(state.canvas_.GetBranchStyle()),
              static_cast<double>(state.zoom_), static_cast<double>(state.pan_px_.x),
              static_cast<double>(state.pan_px_.y));
}

}  // namespace

void RenderMainUi() {
  static UiState state;
  UiCommandDispatcher command_dispatcher;
  RenderMainMenuBar(command_dispatcher, state.canvas_, state.pan_px_, state.zoom_, state.show_status_bar_);

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
    RenderBranchStyleSelector(state.canvas_);
    ImGui::SameLine();
    if (ImGui::Button("Reset layout")) {
      command_dispatcher.Dispatch(UiCommandId::ResetLayout, state.canvas_, state.pan_px_, state.zoom_,
                                   state.show_status_bar_);
    }
    ImGui::TextUnformatted("Canvas: drag nodes. Drag empty space to pan. Mouse wheel zooms.");
    ImGui::Text("Zoom %.2f", static_cast<double>(state.zoom_));
    RenderCanvas(state);
  }
  ImGui::EndChild();

  RenderStatusBar(state);
  ImGui::End();
  ImGui::PopStyleVar(2);
}

}  // namespace mind_map::ui
