#include "demos/DemoBezierMindMap.h"

#include "imgui.h"

#include "ui/canvas/CanvasMath.h"

#include <cassert>
#include <cmath>
#include <cstddef>

namespace mind_map::demos {

namespace {

constexpr float kEdgeThickness = 2.0F;
constexpr float kHandleRadius = 5.0F;
constexpr ImU32 kColorEdge = IM_COL32(140, 170, 210, 255);
constexpr ImU32 kColorNodeFill = IM_COL32(48, 52, 64, 255);
constexpr ImU32 kColorNodeBorder = IM_COL32(110, 120, 145, 255);
constexpr ImU32 kColorNodeBorderHot = IM_COL32(200, 200, 240, 255);

void DrawMindMapEdgesBezier(ImDrawList* draw_list, ImVec2 canvas_p0,
                            const std::array<ImVec2, kSampleMindMapNodeCount>& pos_world, ImVec2 pan_px,
                            float zoom) {
  assert(draw_list != nullptr);
  for (int child = 0; child < kSampleMindMapNodeCount; ++child) {
    const int parent = kSampleMindMapSpecs[static_cast<size_t>(child)].parent_;
    if (parent < 0) {
      continue;
    }
    assert(parent >= 0 && parent < kSampleMindMapNodeCount);

    const char* const parent_label = kSampleMindMapSpecs[static_cast<size_t>(parent)].label_;
    const char* const child_label = kSampleMindMapSpecs[static_cast<size_t>(child)].label_;
    const ImVec2 parent_half = SampleMapHalfExtentForLabel(parent_label);
    const ImVec2 child_half = SampleMapHalfExtentForLabel(child_label);

    const ImVec2 pw = pos_world[static_cast<size_t>(parent)];
    const ImVec2 cw = pos_world[static_cast<size_t>(child)];
    const ImVec2 p0w = SampleMapAttachmentToward(pw, parent_half, cw);
    const ImVec2 p3w = SampleMapAttachmentToward(cw, child_half, pw);
    const SampleMapBezierArms arms = ComputeSampleMapBezierArmsWorld(p0w, p3w, 96.0F, 0.55F);
    const ImVec2 p1w = arms.p1;
    const ImVec2 p2w = arms.p2;

    const ImVec2 p0 = mind_map::canvas::WorldToScreen(p0w, canvas_p0, pan_px, zoom);
    const ImVec2 p1 = mind_map::canvas::WorldToScreen(p1w, canvas_p0, pan_px, zoom);
    const ImVec2 p2 = mind_map::canvas::WorldToScreen(p2w, canvas_p0, pan_px, zoom);
    const ImVec2 p3 = mind_map::canvas::WorldToScreen(p3w, canvas_p0, pan_px, zoom);

    draw_list->AddBezierCubic(p0, p1, p2, p3, kColorEdge, kEdgeThickness);
  }
}

}  // namespace

const char* DemoBezierMindMap::GetName() const {
  return "Bezier branches (rounded boxes)";
}

void DemoBezierMindMap::Reset() {
  pos_world_ = InitialSampleMapPositions();
  dragging_node_ = -1;
}

void DemoBezierMindMap::OnPrimaryDown(const DemoPointerState& ptr) {
  if (!ptr.canvas_hovered) {
    return;
  }
  const int hit = HitTestSampleMap(ptr.mouse_world, pos_world_);
  if (hit >= 0) {
    dragging_node_ = hit;
    const ImVec2 c = pos_world_[static_cast<size_t>(hit)];
    grab_offset_world_ = {ptr.mouse_world.x - c.x, ptr.mouse_world.y - c.y};
  }
  else {
    dragging_node_ = -1;
  }
}

void DemoBezierMindMap::OnPrimaryDrag(const DemoPointerState& ptr) {
  if (dragging_node_ < 0) {
    return;
  }
  pos_world_[static_cast<size_t>(dragging_node_)] = {ptr.mouse_world.x - grab_offset_world_.x,
                                                     ptr.mouse_world.y - grab_offset_world_.y};
}

void DemoBezierMindMap::OnPrimaryUp() {
  dragging_node_ = -1;
}

bool DemoBezierMindMap::IsDraggingContent() const {
  return dragging_node_ >= 0;
}

void DemoBezierMindMap::Render(const DemoRenderContext& ctx) {
  assert(ctx.draw_list != nullptr);
  mind_map::canvas::DrawGrid(ctx.draw_list, ctx.canvas_p0, ctx.canvas_p1, ctx.pan_px, ctx.zoom);
  DrawMindMapEdgesBezier(ctx.draw_list, ctx.canvas_p0, pos_world_, ctx.pan_px, ctx.zoom);

  const int hot_node =
      ctx.canvas_hovered ? HitTestSampleMap(ctx.mouse_world, pos_world_) : -1;

  for (int i = 0; i < kSampleMindMapNodeCount; ++i) {
    const char* const label = kSampleMindMapSpecs[static_cast<size_t>(i)].label_;
    const ImVec2 half = SampleMapHalfExtentForLabel(label);
    const ImVec2 c = pos_world_[static_cast<size_t>(i)];
    const ImVec2 rmin_w = {c.x - half.x, c.y - half.y};
    const ImVec2 rmax_w = {c.x + half.x, c.y + half.y};
    const ImVec2 rmin = mind_map::canvas::WorldToScreen(rmin_w, ctx.canvas_p0, ctx.pan_px, ctx.zoom);
    const ImVec2 rmax = mind_map::canvas::WorldToScreen(rmax_w, ctx.canvas_p0, ctx.pan_px, ctx.zoom);

    const bool hot = (i == dragging_node_) || (i == hot_node);
    const ImU32 border = hot ? kColorNodeBorderHot : kColorNodeBorder;
    ctx.draw_list->AddRectFilled(rmin, rmax, kColorNodeFill, 6.0F);
    ctx.draw_list->AddRect(rmin, rmax, border, 6.0F, 0, 1.5F);

    const ImVec2 text_sz = ImGui::CalcTextSize(label);
    const ImVec2 text_pos = {(rmin.x + rmax.x - text_sz.x) * 0.5F, (rmin.y + rmax.y - text_sz.y) * 0.5F};
    ctx.draw_list->AddText(text_pos, IM_COL32(235, 235, 245, 255), label);

    const ImVec2 left = mind_map::canvas::WorldToScreen({c.x - half.x, c.y}, ctx.canvas_p0, ctx.pan_px, ctx.zoom);
    const ImVec2 right = mind_map::canvas::WorldToScreen({c.x + half.x, c.y}, ctx.canvas_p0, ctx.pan_px, ctx.zoom);
    ctx.draw_list->AddCircleFilled(left, kHandleRadius, IM_COL32(90, 200, 255, 200));
    ctx.draw_list->AddCircleFilled(right, kHandleRadius, IM_COL32(255, 190, 90, 200));
  }
}

}  // namespace mind_map::demos