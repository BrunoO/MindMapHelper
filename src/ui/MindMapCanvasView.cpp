#include "ui/MindMapCanvasView.h"

#include "ui/branch/BranchRenderContext.h"
#include "ui/branch/DrawBranchesBezier.h"
#include "ui/branch/DrawBranchesOrganicTaper.h"
#include "ui/branch/DrawBranchesOrthogonal.h"
#include "ui/canvas/CanvasMath.h"

#include "imgui.h"

#include <cassert>
#include <string_view>

namespace mind_map::ui {

namespace {

constexpr float kHandleRadius = 5.0F;
constexpr ImU32 kColorNodeFill = IM_COL32(48, 52, 64, 255);           // NOLINT(hicpp-signed-bitwise)
constexpr ImU32 kColorNodeBorder = IM_COL32(110, 120, 145, 255);      // NOLINT(hicpp-signed-bitwise)
constexpr ImU32 kColorNodeBorderHot = IM_COL32(200, 200, 240, 255);   // NOLINT(hicpp-signed-bitwise)
constexpr ImU32 kColorNodeBorderSelected = IM_COL32(255, 220, 120, 255); // NOLINT(hicpp-signed-bitwise)

constexpr float kNodeBorderThickness = 1.5F;
constexpr std::string_view kMixedBranchStylesPreview = "Mixed (per child)";

void DrawSampleMindMapNodes(const MindMapCanvasRenderContext& ctx, int dragging_node, int selected_child_for_edge,
                            const std::array<ImVec2, mind_map::demos::kSampleMindMapNodeCount>& pos_world) {
  assert(ctx.draw_list != nullptr);
  const int hot_node = ctx.canvas_hovered ? mind_map::demos::HitTestSampleMap(ctx.mouse_world, pos_world) : -1;

  for (int i = 0; i < mind_map::demos::kSampleMindMapNodeCount; ++i) {
    const char* const label = mind_map::demos::kSampleMindMapSpecs[static_cast<size_t>(i)].label_;
    const ImVec2 half = mind_map::demos::SampleMapHalfExtentForLabel(label);
    const ImVec2 c = pos_world[static_cast<size_t>(i)];
    const ImVec2 rmin_w = {c.x - half.x, c.y - half.y};
    const ImVec2 rmax_w = {c.x + half.x, c.y + half.y};
    const ImVec2 rmin = mind_map::canvas::WorldToScreen(rmin_w, ctx.canvas_p0, ctx.pan_px, ctx.zoom);
    const ImVec2 rmax = mind_map::canvas::WorldToScreen(rmax_w, ctx.canvas_p0, ctx.pan_px, ctx.zoom);

    const bool incoming_edge_selected =
        (i == selected_child_for_edge) &&
        (mind_map::demos::kSampleMindMapSpecs[static_cast<size_t>(i)].parent_ >= 0);
    const bool hot = (i == dragging_node) || (i == hot_node);
    const ImU32 hot_border = hot ? kColorNodeBorderHot : kColorNodeBorder;
    const ImU32 border = incoming_edge_selected ? kColorNodeBorderSelected : hot_border;
    ctx.draw_list->AddRectFilled(rmin, rmax, kColorNodeFill, mind_map::demos::kSampleMindMapNodeCornerRadiusWorld);
    ctx.draw_list->AddRect(rmin, rmax, border, mind_map::demos::kSampleMindMapNodeCornerRadiusWorld, 0, kNodeBorderThickness);

    const ImVec2 text_sz = ImGui::CalcTextSize(label);
    const ImVec2 text_pos = {(rmin.x + rmax.x - text_sz.x) * 0.5F, (rmin.y + rmax.y - text_sz.y) * 0.5F};
    ctx.draw_list->AddText(text_pos, IM_COL32(235, 235, 245, 255), label);  // NOLINT(hicpp-signed-bitwise)

    const ImVec2 left = mind_map::canvas::WorldToScreen({c.x - half.x, c.y}, ctx.canvas_p0, ctx.pan_px, ctx.zoom);
    const ImVec2 right = mind_map::canvas::WorldToScreen({c.x + half.x, c.y}, ctx.canvas_p0, ctx.pan_px, ctx.zoom);
    ctx.draw_list->AddCircleFilled(left, kHandleRadius, IM_COL32(90, 200, 255, 200));   // NOLINT(hicpp-signed-bitwise)
    ctx.draw_list->AddCircleFilled(right, kHandleRadius, IM_COL32(255, 190, 90, 200));  // NOLINT(hicpp-signed-bitwise)
  }
}

void DrawOneChildBranch(const mind_map::ui::branch::BranchRenderContext& branch_ctx, int child_index,
                         mind_map::ui::branch::BranchStyle style,
                         const std::array<ImVec2, mind_map::demos::kSampleMindMapNodeCount>& pos_world) {
  switch (style) {
    case mind_map::ui::branch::BranchStyle::Bezier:
      mind_map::ui::branch::DrawSampleMindMapBranchBezier(branch_ctx, child_index, pos_world);
      return;
    case mind_map::ui::branch::BranchStyle::Orthogonal:
      mind_map::ui::branch::DrawSampleMindMapBranchOrthogonal(branch_ctx, child_index, pos_world);
      return;
    case mind_map::ui::branch::BranchStyle::OrganicTaper:
      mind_map::ui::branch::DrawSampleMindMapBranchOrganicTaper(branch_ctx, child_index, pos_world);
      return;
    default:
      assert(false && "DrawOneChildBranch: exhaustive switch");  // NOLINT(readability-implicit-bool-conversion)
      return;
  }
}

}  // namespace

MindMapCanvasView::MindMapCanvasView() {
  pos_world_ = mind_map::demos::InitialSampleMapPositions();
  InitDefaultPerChildBranchStyles();
}

void MindMapCanvasView::InitDefaultPerChildBranchStyles() {
  using mind_map::ui::branch::BranchStyle;
  // Root index 0 unused; children 1–6 use mixed styles to validate Milestone D (per-edge dispatch).
  branch_style_by_child_ = {{
      BranchStyle::Bezier,
      BranchStyle::Bezier,
      BranchStyle::Orthogonal,
      BranchStyle::OrganicTaper,
      BranchStyle::Orthogonal,
      BranchStyle::Bezier,
      BranchStyle::OrganicTaper,
  }};
}

mind_map::ui::branch::BranchStyle MindMapCanvasView::StyleOfFirstChildEdge_() const {
  for (int c = 0; c < mind_map::demos::kSampleMindMapNodeCount; ++c) {
    if (mind_map::demos::kSampleMindMapSpecs[static_cast<size_t>(c)].parent_ >= 0) {
      return branch_style_by_child_[static_cast<size_t>(c)];
    }
  }
  assert(false && "sample graph must have at least one child edge");  // NOLINT(readability-implicit-bool-conversion)
  return mind_map::ui::branch::BranchStyle::Bezier;
}

bool MindMapCanvasView::BranchStylesAreUniform_() const {
  const mind_map::ui::branch::BranchStyle ref = StyleOfFirstChildEdge_();
  for (int c = 0; c < mind_map::demos::kSampleMindMapNodeCount; ++c) {
    if (mind_map::demos::kSampleMindMapSpecs[static_cast<size_t>(c)].parent_ < 0) {
      continue;
    }
    if (branch_style_by_child_[static_cast<size_t>(c)] != ref) {
      return false;
    }
  }
  return true;
}

void MindMapCanvasView::Reset() {
  pos_world_ = mind_map::demos::InitialSampleMapPositions();
  dragging_node_ = -1;
  selected_child_for_edge_style_ = -1;
}

void MindMapCanvasView::OnPrimaryDown(const MindMapPointerState& ptr) {
  if (!ptr.canvas_hovered) {
    return;
  }
  const int hit = mind_map::demos::HitTestSampleMap(ptr.mouse_world, pos_world_);
  if (hit >= 0) {
    dragging_node_ = hit;
    const ImVec2 c = pos_world_[static_cast<size_t>(hit)];
    grab_offset_world_ = {ptr.mouse_world.x - c.x, ptr.mouse_world.y - c.y};
    if (mind_map::demos::kSampleMindMapSpecs[static_cast<size_t>(hit)].parent_ >= 0) {
      selected_child_for_edge_style_ = hit;
    }
    else {
      selected_child_for_edge_style_ = -1;
    }
  }
  else {
    dragging_node_ = -1;
    selected_child_for_edge_style_ = -1;
  }
}

void MindMapCanvasView::OnPrimaryDrag(const MindMapPointerState& ptr) {  // NOLINT(readability-make-member-function-const)
  if (dragging_node_ < 0) {
    return;
  }
  pos_world_[static_cast<size_t>(dragging_node_)] = {ptr.mouse_world.x - grab_offset_world_.x,
                                                     ptr.mouse_world.y - grab_offset_world_.y};
}

void MindMapCanvasView::OnPrimaryUp() {
  dragging_node_ = -1;
}

bool MindMapCanvasView::IsDraggingContent() const {
  return dragging_node_ >= 0;
}

void MindMapCanvasView::SetBranchStyleForAllEdges(const mind_map::ui::branch::BranchStyle style) {
  for (int c = 0; c < mind_map::demos::kSampleMindMapNodeCount; ++c) {
    if (mind_map::demos::kSampleMindMapSpecs[static_cast<size_t>(c)].parent_ >= 0) {
      branch_style_by_child_[static_cast<size_t>(c)] = style;
    }
  }
}

const char* MindMapCanvasView::GetBranchStyleComboPreviewLabel() const {
  if (!BranchStylesAreUniform_()) {
    return kMixedBranchStylesPreview.data();
  }
  return mind_map::ui::branch::GetBranchStyleDisplayName(StyleOfFirstChildEdge_());
}

bool MindMapCanvasView::BranchStylesAreUniform() const {
  return BranchStylesAreUniform_();
}

mind_map::ui::branch::BranchStyle MindMapCanvasView::RepresentativeChildEdgeStyle() const {
  return StyleOfFirstChildEdge_();
}

bool MindMapCanvasView::HasSelectedIncomingEdgeStyleTarget() const {
  if (selected_child_for_edge_style_ < 0 ||
      selected_child_for_edge_style_ >= mind_map::demos::kSampleMindMapNodeCount) {
    return false;
  }
  return mind_map::demos::kSampleMindMapSpecs[static_cast<size_t>(selected_child_for_edge_style_)].parent_ >= 0;
}

int MindMapCanvasView::GetSelectedChildForBranchStyle() const {
  return selected_child_for_edge_style_;
}

const char* MindMapCanvasView::GetSelectedIncomingEdgeChildLabel() const {
  if (!HasSelectedIncomingEdgeStyleTarget()) {
    return nullptr;
  }
  return mind_map::demos::kSampleMindMapSpecs[static_cast<size_t>(selected_child_for_edge_style_)].label_;
}

mind_map::ui::branch::BranchStyle MindMapCanvasView::GetBranchStyleForSelectedChildEdge() const {
  if (!HasSelectedIncomingEdgeStyleTarget()) {
    return mind_map::ui::branch::BranchStyle::Bezier;
  }
  return branch_style_by_child_[static_cast<size_t>(selected_child_for_edge_style_)];
}

void MindMapCanvasView::SetBranchStyleForSelectedChildEdge(mind_map::ui::branch::BranchStyle style) {  // NOLINT(readability-make-member-function-const)
  if (!HasSelectedIncomingEdgeStyleTarget()) {
    return;
  }
  branch_style_by_child_[static_cast<size_t>(selected_child_for_edge_style_)] = style;
}

void MindMapCanvasView::Render(const MindMapCanvasRenderContext& ctx) {
  assert(ctx.draw_list != nullptr);
  mind_map::canvas::DrawGrid(ctx.draw_list, ctx.canvas_p0, ctx.canvas_p1, ctx.pan_px, ctx.zoom);

  const mind_map::ui::branch::BranchRenderContext branch_ctx =
      mind_map::ui::branch::MakeBranchRenderContext(ctx.draw_list, ctx.canvas_p0, ctx.pan_px, ctx.zoom);

  for (int child = 0; child < mind_map::demos::kSampleMindMapNodeCount; ++child) {
    if (mind_map::demos::kSampleMindMapSpecs[static_cast<size_t>(child)].parent_ < 0) {
      continue;
    }
    DrawOneChildBranch(branch_ctx, child, branch_style_by_child_[static_cast<size_t>(child)], pos_world_);
  }

  DrawSampleMindMapNodes(ctx, dragging_node_, selected_child_for_edge_style_, pos_world_);
}

}  // namespace mind_map::ui
