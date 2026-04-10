#include "demos/DemoStraightMindMap.h"

#include "imgui.h"

#include "ui/canvas/CanvasMath.h"

#include <cassert>
#include <cstddef>

namespace mind_map::demos {

namespace {

constexpr float kEdgeThickness = 2.0F;
constexpr ImU32 kColorEdge = IM_COL32(120, 200, 150, 255);
constexpr ImU32 kColorNodeFill = IM_COL32(42, 56, 52, 255);
constexpr ImU32 kColorNodeBorder = IM_COL32(100, 150, 120, 255);
constexpr ImU32 kColorNodeBorderHot = IM_COL32(190, 240, 210, 255);

void DrawOrthogonalEdges(ImDrawList* draw_list, ImVec2 canvas_p0,
                         const std::array<ImVec2, kSampleMindMapNodeCount>& pos_world, ImVec2 pan_px, float zoom) {
  assert(draw_list != nullptr);
  for (int child = 0; child < kSampleMindMapNodeCount; ++child) {
    const int parent = kSampleMindMapSpecs[static_cast<size_t>(child)].parent_;
    if (parent < 0) {
      continue;
    }
    assert(parent >= 0 && parent < kSampleMindMapNodeCount);

    const char* const parent_label = kSampleMindMapSpecs[static_cast<size_t>(parent)].label_;
    const char* const child_label = kSampleMindMapSpecs[static_cast<size_t>(child)].label_;
    const float pr = SampleMapNodeRadiusWorld(parent_label);
    const float cr = SampleMapNodeRadiusWorld(child_label);

    const ImVec2 pw = pos_world[static_cast<size_t>(parent)];
    const ImVec2 cw = pos_world[static_cast<size_t>(child)];
    const ImVec2 p0w = SampleMapCircleAttachmentToward(pw, pr, cw);
    const ImVec2 p3w = SampleMapCircleAttachmentToward(cw, cr, pw);
    const float mid_x = 0.5F * (p0w.x + p3w.x);
    const ImVec2 p1w = {mid_x, p0w.y};
    const ImVec2 p2w = {mid_x, p3w.y};

    const ImVec2 s0 = mind_map::canvas::WorldToScreen(p0w, canvas_p0, pan_px, zoom);
    const ImVec2 s1 = mind_map::canvas::WorldToScreen(p1w, canvas_p0, pan_px, zoom);
    const ImVec2 s2 = mind_map::canvas::WorldToScreen(p2w, canvas_p0, pan_px, zoom);
    const ImVec2 s3 = mind_map::canvas::WorldToScreen(p3w, canvas_p0, pan_px, zoom);

    draw_list->AddLine(s0, s1, kColorEdge, kEdgeThickness);
    draw_list->AddLine(s1, s2, kColorEdge, kEdgeThickness);
    draw_list->AddLine(s2, s3, kColorEdge, kEdgeThickness);
  }
}

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
  DrawOrthogonalEdges(ctx.draw_list, ctx.canvas_p0, pos_world_, ctx.pan_px, ctx.zoom);

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
