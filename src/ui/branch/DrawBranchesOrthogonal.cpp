#include "ui/branch/DrawBranchesOrthogonal.h"

#include "ui/canvas/CanvasMath.h"

#include <cassert>
#include <cstddef>

namespace mind_map::ui::branch {

namespace {

constexpr float kEdgeThickness = 2.0F;
constexpr ImU32 kColorEdge = IM_COL32(120, 200, 150, 255);

}  // namespace

void DrawSampleMindMapBranchOrthogonal(
    const BranchRenderContext& ctx, const int child_index,
    const std::array<ImVec2, mind_map::demos::kSampleMindMapNodeCount>& pos_world) {
  assert(ctx.draw_list != nullptr);
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
  const float mid_x = 0.5F * (p0w.x + p3w.x);
  const ImVec2 p1w = {mid_x, p0w.y};
  const ImVec2 p2w = {mid_x, p3w.y};

  const ImVec2 s0 = mind_map::canvas::WorldToScreen(p0w, ctx.canvas_p0, ctx.pan_px, ctx.zoom);
  const ImVec2 s1 = mind_map::canvas::WorldToScreen(p1w, ctx.canvas_p0, ctx.pan_px, ctx.zoom);
  const ImVec2 s2 = mind_map::canvas::WorldToScreen(p2w, ctx.canvas_p0, ctx.pan_px, ctx.zoom);
  const ImVec2 s3 = mind_map::canvas::WorldToScreen(p3w, ctx.canvas_p0, ctx.pan_px, ctx.zoom);

  ctx.draw_list->AddLine(s0, s1, kColorEdge, kEdgeThickness);
  ctx.draw_list->AddLine(s1, s2, kColorEdge, kEdgeThickness);
  ctx.draw_list->AddLine(s2, s3, kColorEdge, kEdgeThickness);
}

void DrawAllSampleMindMapBranchesOrthogonal(
    const BranchRenderContext& ctx,
    const std::array<ImVec2, mind_map::demos::kSampleMindMapNodeCount>& pos_world) {
  assert(ctx.draw_list != nullptr);
  for (int child = 0; child < mind_map::demos::kSampleMindMapNodeCount; ++child) {
    if (mind_map::demos::kSampleMindMapSpecs[static_cast<size_t>(child)].parent_ < 0) {
      continue;
    }
    DrawSampleMindMapBranchOrthogonal(ctx, child, pos_world);
  }
}

}  // namespace mind_map::ui::branch
