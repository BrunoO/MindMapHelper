#include "ui/demos/DemoStraightMindMap.h"

#include "imgui.h"

#include "ui/branch/DrawBranchesOrthogonal.h"
#include "ui/canvas/CanvasMath.h"

#include <cassert>

namespace mind_map::demos {

namespace {

constexpr ImU32 kColorNodeFill = IM_COL32(42, 56, 52, 255);
constexpr ImU32 kColorNodeBorder = IM_COL32(100, 150, 120, 255);
constexpr ImU32 kColorNodeBorderHot = IM_COL32(190, 240, 210, 255);

}  // namespace

const char* DemoStraightMindMap::GetName() const {
  return "Orthogonal polylines (circles)";
}

void DemoStraightMindMap::Reset() {
  pos_world_ = InitialSampleMapPositions();
  dragging_node_ = -1;
}

void DemoStraightMindMap::OnPrimaryDown(const DemoPointerState& ptr) {
  if (!ptr.canvas_hovered) {
    return;
  }
  const int hit = HitTestSampleMapCircles(ptr.mouse_world, pos_world_);
  if (hit >= 0) {
    dragging_node_ = hit;
    const ImVec2 c = pos_world_[static_cast<size_t>(hit)];
    grab_offset_world_ = {ptr.mouse_world.x - c.x, ptr.mouse_world.y - c.y};
  }
  else {
    dragging_node_ = -1;
  }
}

void DemoStraightMindMap::OnPrimaryDrag(const DemoPointerState& ptr) {
  if (dragging_node_ < 0) {
    return;
  }
  pos_world_[static_cast<size_t>(dragging_node_)] = {ptr.mouse_world.x - grab_offset_world_.x,
                                                     ptr.mouse_world.y - grab_offset_world_.y};
}

void DemoStraightMindMap::OnPrimaryUp() {
  dragging_node_ = -1;
}

bool DemoStraightMindMap::IsDraggingContent() const {
  return dragging_node_ >= 0;
}

void DemoStraightMindMap::Render(const DemoRenderContext& ctx) {
  assert(ctx.draw_list != nullptr);
  mind_map::canvas::DrawGrid(ctx.draw_list, ctx.canvas_p0, ctx.canvas_p1, ctx.pan_px, ctx.zoom);
  const mind_map::ui::branch::BranchRenderContext branch_ctx =
      mind_map::ui::branch::MakeBranchRenderContext(ctx.draw_list, ctx.canvas_p0, ctx.pan_px, ctx.zoom);
  mind_map::ui::branch::DrawAllSampleMindMapBranchesOrthogonal(branch_ctx, pos_world_);

  const int hot_node = ctx.canvas_hovered ? HitTestSampleMapCircles(ctx.mouse_world, pos_world_) : -1;

  for (int i = 0; i < kSampleMindMapNodeCount; ++i) {
    const char* const label = kSampleMindMapSpecs[static_cast<size_t>(i)].label_;
    const ImVec2 c = pos_world_[static_cast<size_t>(i)];
    const float r = SampleMapNodeRadiusWorld(label);
    const ImVec2 center = mind_map::canvas::WorldToScreen(c, ctx.canvas_p0, ctx.pan_px, ctx.zoom);
    const float radius_px = r * ctx.zoom;

    const bool hot = (i == dragging_node_) || (i == hot_node);
    const ImU32 border = hot ? kColorNodeBorderHot : kColorNodeBorder;
    ctx.draw_list->AddCircleFilled(center, radius_px, kColorNodeFill);
    ctx.draw_list->AddCircle(center, radius_px, border, 0, 2.0F);

    const ImVec2 text_sz = ImGui::CalcTextSize(label);
    const ImVec2 text_pos = {center.x - text_sz.x * 0.5F, center.y - text_sz.y * 0.5F};
    ctx.draw_list->AddText(text_pos, IM_COL32(235, 245, 238, 255), label);
  }
}

}  // namespace mind_map::demos
