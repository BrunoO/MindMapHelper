#include "ui/branch/DrawBranchesBezier.h"

#include "ui/canvas/CanvasMath.h"

#include <cassert>
#include <cstddef>

namespace mind_map::ui::branch {

namespace {

constexpr float kEdgeThickness = 2.0F;
constexpr ImU32 kColorEdge = IM_COL32(140, 170, 210, 255);  // NOLINT(hicpp-signed-bitwise)

}  // namespace

void DrawSampleMindMapBranchBezier(
    const BranchRenderContext& ctx, const int child_index,
    const std::array<ImVec2, mind_map::demos::kSampleMindMapNodeCount>& pos_world) {
  assert(ctx.draw_list_ != nullptr);
  assert(child_index >= 0 && child_index < mind_map::demos::kSampleMindMapNodeCount);
  const int parent = mind_map::demos::kSampleMindMapSpecs[static_cast<size_t>(child_index)].parent_;
  assert(parent >= 0 && parent < mind_map::demos::kSampleMindMapNodeCount);

  const char* const parent_label = mind_map::demos::kSampleMindMapSpecs[static_cast<size_t>(parent)].label_;
  const char* const child_label = mind_map::demos::kSampleMindMapSpecs[static_cast<size_t>(child_index)].label_;
  const ImVec2 parent_half = mind_map::demos::SampleMapHalfExtentForLabel(parent_label);
  const ImVec2 child_half = mind_map::demos::SampleMapHalfExtentForLabel(child_label);

  const ImVec2 pw = pos_world[static_cast<size_t>(parent)];
  const ImVec2 cw = pos_world[static_cast<size_t>(child_index)];
  const ImVec2 p0w = mind_map::demos::SampleMapRoundedRectAttachmentPreferEdgeMid(
      pw, parent_half, mind_map::demos::kSampleMindMapNodeCornerRadiusWorld, cw);
  const ImVec2 p3w = mind_map::demos::SampleMapRoundedRectAttachmentPreferEdgeMid(
      cw, child_half, mind_map::demos::kSampleMindMapNodeCornerRadiusWorld, pw);
  const mind_map::demos::SampleMapBezierArmInputs arm_inputs = {
      pw, parent_half, cw, child_half, p0w, p3w, 96.0F, 0.55F, nullptr, nullptr};
  const mind_map::demos::SampleMapBezierArms arms = mind_map::demos::ComputeSampleMapBezierArmsWorld(arm_inputs);
  const ImVec2 p1w = arms.p1_;
  const ImVec2 p2w = arms.p2_;

  const ImVec2 p0 = mind_map::canvas::WorldToScreen(p0w, ctx.canvas_p0_, ctx.pan_px_, ctx.zoom_);
  const ImVec2 p1 = mind_map::canvas::WorldToScreen(p1w, ctx.canvas_p0_, ctx.pan_px_, ctx.zoom_);
  const ImVec2 p2 = mind_map::canvas::WorldToScreen(p2w, ctx.canvas_p0_, ctx.pan_px_, ctx.zoom_);
  const ImVec2 p3 = mind_map::canvas::WorldToScreen(p3w, ctx.canvas_p0_, ctx.pan_px_, ctx.zoom_);

  ctx.draw_list_->AddBezierCubic(p0, p1, p2, p3, kColorEdge, kEdgeThickness);
}

void DrawAllSampleMindMapBranchesBezier(
    const BranchRenderContext& ctx,
    const std::array<ImVec2, mind_map::demos::kSampleMindMapNodeCount>& pos_world) {
  assert(ctx.draw_list_ != nullptr);
  for (int child = 0; child < mind_map::demos::kSampleMindMapNodeCount; ++child) {
    if (mind_map::demos::kSampleMindMapSpecs[static_cast<size_t>(child)].parent_ < 0) {
      continue;
    }
    DrawSampleMindMapBranchBezier(ctx, child, pos_world);
  }
}

}  // namespace mind_map::ui::branch
