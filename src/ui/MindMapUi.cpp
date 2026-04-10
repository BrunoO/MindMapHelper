#include "ui/MindMapUi.h"

#include "imgui.h"

#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>

namespace mind_map::ui {

namespace {

constexpr float kNodePad = 10.0F;
constexpr float kEdgeThickness = 2.0F;
constexpr float kHandleRadius = 5.0F;
constexpr float kMinZoom = 0.35F;
constexpr float kMaxZoom = 3.0F;

constexpr ImU32 kColorGridMajor = IM_COL32(60, 60, 72, 255);
constexpr ImU32 kColorGridMinor = IM_COL32(45, 45, 55, 255);
constexpr ImU32 kColorEdge = IM_COL32(140, 170, 210, 255);
constexpr ImU32 kColorNodeFill = IM_COL32(48, 52, 64, 255);
constexpr ImU32 kColorNodeBorder = IM_COL32(110, 120, 145, 255);
constexpr ImU32 kColorNodeBorderHot = IM_COL32(200, 200, 240, 255);

constexpr int kDemoNodeCount = 7;

struct DemoNodeSpec {
  const char* label_;
  int parent_;
};

// Initial layout: root at origin, children to the right (classic horizontal mind map).
constexpr std::array<DemoNodeSpec, kDemoNodeCount> kDemoSpecs = {{
    {"Root", -1},
    {"Idea A", 0},
    {"Idea B", 0},
    {"Idea C", 0},
    {"Detail A1", 1},
    {"Detail A2", 1},
    {"Detail B1", 2},
}};

[[nodiscard]] ImVec2 HalfExtentForLabel(const char* label) {
  assert(label != nullptr);
  const ImVec2 text_sz = ImGui::CalcTextSize(label);
  return {text_sz.x * 0.5F + kNodePad, text_sz.y * 0.5F + kNodePad};
}

[[nodiscard]] ImVec2 WorldToScreen(ImVec2 world, ImVec2 canvas_p0, ImVec2 pan_px, float zoom) {
  return {canvas_p0.x + pan_px.x + world.x * zoom, canvas_p0.y + pan_px.y + world.y * zoom};
}

[[nodiscard]] ImVec2 ScreenToWorld(ImVec2 screen, ImVec2 canvas_p0, ImVec2 pan_px, float zoom) {
  assert(zoom > 0.0F);
  return {(screen.x - canvas_p0.x - pan_px.x) / zoom, (screen.y - canvas_p0.y - pan_px.y) / zoom};
}

[[nodiscard]] bool IsInsideRect(ImVec2 p, ImVec2 rect_min, ImVec2 rect_max) {
  return p.x >= rect_min.x && p.x <= rect_max.x && p.y >= rect_min.y && p.y <= rect_max.y;
}

void DrawGrid(ImDrawList* draw_list, ImVec2 canvas_p0, ImVec2 canvas_p1, ImVec2 pan_px, float zoom) {
  assert(draw_list != nullptr);
  constexpr float kMinorStep = 32.0F;
  constexpr int kMajorEvery = 4;

  const ImVec2 world_min = ScreenToWorld(canvas_p0, canvas_p0, pan_px, zoom);
  const ImVec2 world_max = ScreenToWorld(canvas_p1, canvas_p0, pan_px, zoom);

  const int ix0 = static_cast<int>(std::floor(world_min.x / kMinorStep));
  const int ix1 = static_cast<int>(std::ceil(world_max.x / kMinorStep));
  const int iy0 = static_cast<int>(std::floor(world_min.y / kMinorStep));
  const int iy1 = static_cast<int>(std::ceil(world_max.y / kMinorStep));

  for (int ix = ix0; ix <= ix1; ++ix) {
    const float x = static_cast<float>(ix) * kMinorStep;
    const ImVec2 a = WorldToScreen({x, world_min.y}, canvas_p0, pan_px, zoom);
    const ImVec2 b = WorldToScreen({x, world_max.y}, canvas_p0, pan_px, zoom);
    const int mod = ((ix % kMajorEvery) + kMajorEvery) % kMajorEvery;
    const bool major = (mod == 0);
    draw_list->AddLine(a, b, major ? kColorGridMajor : kColorGridMinor, 1.0F);
  }
  for (int iy = iy0; iy <= iy1; ++iy) {
    const float y = static_cast<float>(iy) * kMinorStep;
    const ImVec2 a = WorldToScreen({world_min.x, y}, canvas_p0, pan_px, zoom);
    const ImVec2 b = WorldToScreen({world_max.x, y}, canvas_p0, pan_px, zoom);
    const int mod = ((iy % kMajorEvery) + kMajorEvery) % kMajorEvery;
    const bool major = (mod == 0);
    draw_list->AddLine(a, b, major ? kColorGridMajor : kColorGridMinor, 1.0F);
  }
}

void DrawMindMapEdges(ImDrawList* draw_list, ImVec2 canvas_p0, const std::array<ImVec2, kDemoNodeCount>& pos_world,
                      ImVec2 pan_px, float zoom) {
  assert(draw_list != nullptr);
  for (int child = 0; child < kDemoNodeCount; ++child) {
    const int parent = kDemoSpecs[static_cast<size_t>(child)].parent_;
    if (parent < 0) {
      continue;
    }
    assert(parent >= 0 && parent < kDemoNodeCount);

    const char* const parent_label = kDemoSpecs[static_cast<size_t>(parent)].label_;
    const char* const child_label = kDemoSpecs[static_cast<size_t>(child)].label_;
    const ImVec2 parent_half = HalfExtentForLabel(parent_label);
    const ImVec2 child_half = HalfExtentForLabel(child_label);

    const ImVec2 p0w = {pos_world[static_cast<size_t>(parent)].x + parent_half.x,
                        pos_world[static_cast<size_t>(parent)].y};
    const ImVec2 p3w = {pos_world[static_cast<size_t>(child)].x - child_half.x,
                        pos_world[static_cast<size_t>(child)].y};

    const float horizontal_span = std::abs(p3w.x - p0w.x);
    const float arm = (std::max)(96.0F, horizontal_span * 0.55F);
    const ImVec2 p1w = {p0w.x + arm, p0w.y};
    const ImVec2 p2w = {p3w.x - arm, p3w.y};

    const ImVec2 p0 = WorldToScreen(p0w, canvas_p0, pan_px, zoom);
    const ImVec2 p1 = WorldToScreen(p1w, canvas_p0, pan_px, zoom);
    const ImVec2 p2 = WorldToScreen(p2w, canvas_p0, pan_px, zoom);
    const ImVec2 p3 = WorldToScreen(p3w, canvas_p0, pan_px, zoom);

    draw_list->AddBezierCubic(p0, p1, p2, p3, kColorEdge, kEdgeThickness);
  }
}

[[nodiscard]] int HitTestNode(ImVec2 world_pos, const std::array<ImVec2, kDemoNodeCount>& pos_world) {
  for (int i = kDemoNodeCount - 1; i >= 0; --i) {
    const char* const label = kDemoSpecs[static_cast<size_t>(i)].label_;
    const ImVec2 half = HalfExtentForLabel(label);
    const ImVec2 c = pos_world[static_cast<size_t>(i)];
    const ImVec2 rmin = {c.x - half.x, c.y - half.y};
    const ImVec2 rmax = {c.x + half.x, c.y + half.y};
    if (IsInsideRect(world_pos, rmin, rmax)) {
      return i;
    }
  }
  return -1;
}

}  // namespace

void RenderMainUi() {
  static bool show_demo_window = true;
  if (show_demo_window) {
    ImGui::ShowDemoWindow(&show_demo_window);
  }

  static std::array<ImVec2, kDemoNodeCount> pos_world = {{
      {0.0F, 0.0F},
      {260.0F, -140.0F},
      {260.0F, 0.0F},
      {260.0F, 140.0F},
      {520.0F, -200.0F},
      {520.0F, -100.0F},
      {520.0F, 20.0F},
  }};
  static ImVec2 pan_px = {40.0F, 120.0F};
  static float zoom = 1.0F;
  static int dragging_node = -1;
  static bool dragging_canvas = false;
  static ImVec2 grab_offset_world = {0.0F, 0.0F};

  ImGui::Begin("bMindMap");
  ImGui::Checkbox("ImGui Demo", &show_demo_window);
  ImGui::Separator();
  ImGui::TextUnformatted("Canvas: drag nodes. Drag empty space to pan. Mouse wheel zooms.");
  ImGui::Text("Zoom %.2f", static_cast<double>(zoom));

  const ImVec2 canvas_p0 = ImGui::GetCursorScreenPos();
  const ImVec2 canvas_sz = ImGui::GetContentRegionAvail();
  const ImVec2 canvas_p1 = {canvas_p0.x + canvas_sz.x, canvas_p0.y + canvas_sz.y};

  ImGui::InvisibleButton("mind_map_canvas", canvas_sz, ImGuiButtonFlags_MouseButtonLeft);
  const bool canvas_hovered = ImGui::IsItemHovered();
  const ImGuiIO& io = ImGui::GetIO();

  if (canvas_hovered && io.MouseWheel != 0.0F) {
    const float prev_zoom = zoom;
    const float wheel = io.MouseWheel;
    zoom = (std::clamp)(zoom + wheel * 0.1F, kMinZoom, kMaxZoom);

    const ImVec2 mouse = io.MousePos;
    const ImVec2 world_under_cursor = ScreenToWorld(mouse, canvas_p0, pan_px, prev_zoom);
    pan_px.x += world_under_cursor.x * (prev_zoom - zoom);
    pan_px.y += world_under_cursor.y * (prev_zoom - zoom);
  }

  if (ImGui::IsItemActive() && ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
    if (dragging_node >= 0) {
      const ImVec2 mouse_world = ScreenToWorld(io.MousePos, canvas_p0, pan_px, zoom);
      pos_world[static_cast<size_t>(dragging_node)] = {mouse_world.x - grab_offset_world.x,
                                                       mouse_world.y - grab_offset_world.y};
    }
    else if (dragging_canvas) {
      pan_px.x += io.MouseDelta.x;
      pan_px.y += io.MouseDelta.y;
    }
  }

  if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) && canvas_hovered) {
    const ImVec2 mouse_world = ScreenToWorld(io.MousePos, canvas_p0, pan_px, zoom);
    const int hit = HitTestNode(mouse_world, pos_world);
    if (hit >= 0) {
      dragging_node = hit;
      dragging_canvas = false;
      const ImVec2 c = pos_world[static_cast<size_t>(hit)];
      grab_offset_world = {mouse_world.x - c.x, mouse_world.y - c.y};
    }
    else {
      dragging_node = -1;
      dragging_canvas = true;
    }
  }

  if (ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
    dragging_node = -1;
    dragging_canvas = false;
  }

  ImDrawList* const draw_list = ImGui::GetWindowDrawList();
  draw_list->PushClipRect(canvas_p0, canvas_p1, true);
  DrawGrid(draw_list, canvas_p0, canvas_p1, pan_px, zoom);
  DrawMindMapEdges(draw_list, canvas_p0, pos_world, pan_px, zoom);

  const int hot_node =
      canvas_hovered ? HitTestNode(ScreenToWorld(io.MousePos, canvas_p0, pan_px, zoom), pos_world) : -1;

  for (int i = 0; i < kDemoNodeCount; ++i) {
    const char* const label = kDemoSpecs[static_cast<size_t>(i)].label_;
    const ImVec2 half = HalfExtentForLabel(label);
    const ImVec2 c = pos_world[static_cast<size_t>(i)];
    const ImVec2 rmin_w = {c.x - half.x, c.y - half.y};
    const ImVec2 rmax_w = {c.x + half.x, c.y + half.y};
    const ImVec2 rmin = WorldToScreen(rmin_w, canvas_p0, pan_px, zoom);
    const ImVec2 rmax = WorldToScreen(rmax_w, canvas_p0, pan_px, zoom);

    const bool hot = (i == dragging_node) || (i == hot_node);
    const ImU32 border = hot ? kColorNodeBorderHot : kColorNodeBorder;
    draw_list->AddRectFilled(rmin, rmax, kColorNodeFill, 6.0F);
    draw_list->AddRect(rmin, rmax, border, 6.0F, 0, 1.5F);

    const ImVec2 text_sz = ImGui::CalcTextSize(label);
    const ImVec2 text_pos = {(rmin.x + rmax.x - text_sz.x) * 0.5F, (rmin.y + rmax.y - text_sz.y) * 0.5F};
    draw_list->AddText(text_pos, IM_COL32(235, 235, 245, 255), label);

    // Anchor handles (purely visual): where curves attach on left/right.
    const ImVec2 left = WorldToScreen({c.x - half.x, c.y}, canvas_p0, pan_px, zoom);
    const ImVec2 right = WorldToScreen({c.x + half.x, c.y}, canvas_p0, pan_px, zoom);
    draw_list->AddCircleFilled(left, kHandleRadius, IM_COL32(90, 200, 255, 200));
    draw_list->AddCircleFilled(right, kHandleRadius, IM_COL32(255, 190, 90, 200));
  }

  draw_list->PopClipRect();

  ImGui::End();
}

}  // namespace mind_map::ui
