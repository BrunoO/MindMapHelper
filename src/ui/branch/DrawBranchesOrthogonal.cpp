#include "ui/branch/DrawBranchesOrthogonal.h"

#include "ui/canvas/CanvasMath.h"

#include <cassert>
#include <cstddef>

namespace mind_map::ui::branch {

namespace {

constexpr float kEdgeThickness = 2.0F;
constexpr ImU32 kColorEdge = IM_COL32(120, 200, 150, 255);

}  // namespace

void DrawAllSampleMindMapBranchesOrthogonal(
    const BranchRenderContext& ctx,
    const std::array<ImVec2, mind_map::demos::kSampleMindMapNodeCount>& pos_world) {
  assert(ctx.draw_list != nullptr);
  for (int child = 0; child < mind_map::demos::kSampleMindMapNodeCount; ++child) {
    const int parent = mind_map::demos::kSampleMindMapSpecs[static_cast<size_t>(child)].parent_;
    if (parent < 0) {
      continue;
    }
    assert(parent >= 0 && parent < mind_map::demos::kSampleMindMapNodeCount);

    const char* const parent_label = mind_map::demos::kSampleMindMapSpecs[static_cast<size_t>(parent)].label_;
    const char* const child_label = mind_map::demos::kSampleMindMapSpecs[static_cast<size_t>(child)].label_;
    const float pr = mind_map::demos::SampleMapNodeRadiusWorld(parent_label);
    const float cr = mind_map::demos::SampleMapNodeRadiusWorld(child_label);

    const ImVec2 pw = pos_world[static_cast<size_t>(parent)];
    const ImVec2 cw = pos_world[static_cast<size_t>(child)];
    const ImVec2 p0w = mind_map::demos::SampleMapCircleAttachmentToward(pw, pr, cw);
    const ImVec2 p3w = mind_map::demos::SampleMapCircleAttachmentToward(cw, cr, pw);
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
}

}  // namespace mind_map::ui::branch
