#include "ui/branch/DrawBranchesOrthogonal.h"

#include "ui/branch/SampleMindMapBranchAttachments.h"
#include "ui/canvas/CanvasMath.h"

#include <cassert>

namespace mind_map::ui::branch {

namespace {

constexpr float kEdgeThickness = 2.0F;
constexpr ImU32 kColorEdge = IM_COL32(120, 200, 150, 255);  // NOLINT(hicpp-signed-bitwise)

}  // namespace

void DrawMindMapBranchOrthogonal(
    const BranchRenderContext& ctx, const int child_index,
    const std::vector<mind_map::ui::CanvasNode>& nodes) {
  assert(ctx.draw_list_ != nullptr);
  BranchEdgeData g{};
  FillBranchEdgeData(child_index, nodes, &g);
  const ImVec2 p0w = g.p0_attachment_;
  const ImVec2 p3w = g.p3_attachment_;
  const float mid_x = 0.5F * (p0w.x + p3w.x);
  const ImVec2 p1w = {mid_x, p0w.y};
  const ImVec2 p2w = {mid_x, p3w.y};

  const ImVec2 s0 = mind_map::canvas::WorldToScreen(p0w, ctx.canvas_p0_, ctx.pan_px_, ctx.zoom_);
  const ImVec2 s1 = mind_map::canvas::WorldToScreen(p1w, ctx.canvas_p0_, ctx.pan_px_, ctx.zoom_);
  const ImVec2 s2 = mind_map::canvas::WorldToScreen(p2w, ctx.canvas_p0_, ctx.pan_px_, ctx.zoom_);
  const ImVec2 s3 = mind_map::canvas::WorldToScreen(p3w, ctx.canvas_p0_, ctx.pan_px_, ctx.zoom_);

  ctx.draw_list_->AddLine(s0, s1, kColorEdge, kEdgeThickness);
  ctx.draw_list_->AddLine(s1, s2, kColorEdge, kEdgeThickness);
  ctx.draw_list_->AddLine(s2, s3, kColorEdge, kEdgeThickness);
}

}  // namespace mind_map::ui::branch
