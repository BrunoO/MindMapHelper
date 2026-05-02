#include "ui/demos/DemoBezierMindMap.h"

#include "imgui.h"

#include "ui/branch/DrawBranchesBezier.h"
#include "ui/canvas/CanvasMath.h"

#include <cassert>

namespace mind_map::demos {

namespace {

constexpr float kHandleRadius = 5.0F;
constexpr ImU32 kColorNodeFill = IM_COL32(48, 52, 64, 255);
constexpr ImU32 kColorNodeBorder = IM_COL32(110, 120, 145, 255);
constexpr ImU32 kColorNodeBorderHot = IM_COL32(200, 200, 240, 255);

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
  const mind_map::ui::branch::BranchRenderContext branch_ctx =
      mind_map::ui::branch::MakeBranchRenderContext(ctx.draw_list, ctx.canvas_p0, ctx.pan_px, ctx.zoom);
  mind_map::ui::branch::DrawAllSampleMindMapBranchesBezier(branch_ctx, pos_world_);

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
    ctx.draw_list->AddRectFilled(rmin, rmax, kColorNodeFill, kSampleMindMapNodeCornerRadiusWorld);
    ctx.draw_list->AddRect(rmin, rmax, border, kSampleMindMapNodeCornerRadiusWorld, 0, 1.5F);

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