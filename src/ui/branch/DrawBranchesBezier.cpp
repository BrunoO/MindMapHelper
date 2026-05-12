#include "ui/branch/DrawBranchesBezier.h"

#include "ui/branch/SampleMindMapBranchAttachments.h"
#include "ui/canvas/CanvasMath.h"

#include <cassert>

namespace mind_map::ui::branch {

namespace {

constexpr float kEdgeThickness = 2.0F;
constexpr ImU32 kColorEdge = IM_COL32(140, 170, 210, 255);  // NOLINT(hicpp-signed-bitwise)

}  // namespace

void DrawMindMapBranchBezier(
    const BranchRenderContext& ctx, const int child_index,
    const std::vector<mind_map::ui::CanvasNode>& nodes) {
  assert(ctx.draw_list_ != nullptr);
  BranchEdgeData g{};
  FillBranchEdgeData(child_index, nodes, &g);
  const ImVec2 p0w = g.p0_attachment_;
  const ImVec2 p3w = g.p3_attachment_;
  const mind_map::canvas::BezierArmInputs arm_inputs = {
      g.pw_, g.parent_half_, g.cw_, g.child_half_, p0w, p3w, 96.0F, 0.55F, nullptr, nullptr};
  const mind_map::canvas::BezierArms arms = mind_map::canvas::ComputeBezierArmsWorld(arm_inputs);
  const ImVec2 p1w = arms.p1_;
  const ImVec2 p2w = arms.p2_;

  const ImVec2 p0 = mind_map::canvas::WorldToScreen(p0w, ctx.canvas_p0_, ctx.pan_px_, ctx.zoom_);
  const ImVec2 p1 = mind_map::canvas::WorldToScreen(p1w, ctx.canvas_p0_, ctx.pan_px_, ctx.zoom_);
  const ImVec2 p2 = mind_map::canvas::WorldToScreen(p2w, ctx.canvas_p0_, ctx.pan_px_, ctx.zoom_);
  const ImVec2 p3 = mind_map::canvas::WorldToScreen(p3w, ctx.canvas_p0_, ctx.pan_px_, ctx.zoom_);

  ctx.draw_list_->AddBezierCubic(p0, p1, p2, p3, kColorEdge, kEdgeThickness);
}

}  // namespace mind_map::ui::branch
