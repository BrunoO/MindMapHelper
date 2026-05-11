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

[[nodiscard]] std::string_view BranchStyleToString(mind_map::ui::branch::BranchStyle style) {
  using mind_map::ui::branch::BranchStyle;
  switch (style) {  // NOSONAR(cpp:S6177)
    case BranchStyle::Bezier:       return "bezier";
    case BranchStyle::Orthogonal:   return "orthogonal";
    case BranchStyle::OrganicTaper: return "organic_taper";
    default:                        return "bezier";
  }
}

[[nodiscard]] mind_map::ui::branch::BranchStyle BranchStyleFromString(std::string_view s) {
  using mind_map::ui::branch::BranchStyle;
  if (s == "orthogonal")    { return BranchStyle::Orthogonal; }
  if (s == "organic_taper") { return BranchStyle::OrganicTaper; }
  return BranchStyle::Bezier;
}

constexpr float kHandleRadius = 5.0F;
constexpr ImU32 kColorNodeFill = IM_COL32(48, 52, 64, 255);           // NOLINT(hicpp-signed-bitwise)
constexpr ImU32 kColorNodeBorder = IM_COL32(110, 120, 145, 255);      // NOLINT(hicpp-signed-bitwise)
constexpr ImU32 kColorNodeBorderHot = IM_COL32(200, 200, 240, 255);   // NOLINT(hicpp-signed-bitwise)
constexpr ImU32 kColorNodeBorderSelected = IM_COL32(255, 220, 120, 255); // NOLINT(hicpp-signed-bitwise)

constexpr float kNodeBorderThickness = 1.5F;
constexpr std::string_view kMixedBranchStylesPreview = "Mixed (per child)";

void DrawSampleMindMapNodes(const MindMapCanvasRenderContext& ctx, int dragging_node, int selected_child_for_edge,
                            const std::array<ImVec2, mind_map::demos::kSampleMindMapNodeCount>& pos_world) {
  assert(ctx.draw_list_ != nullptr);
  const int hot_node = ctx.canvas_hovered_ ? mind_map::demos::HitTestSampleMap(ctx.mouse_world_, pos_world) : -1;

  for (int i = 0; i < mind_map::demos::kSampleMindMapNodeCount; ++i) {
    const char* const label = mind_map::demos::kSampleMindMapSpecs[static_cast<size_t>(i)].label_;
    const ImVec2 half = mind_map::canvas::NodeHalfExtentForLabel(label);
    const ImVec2 c = pos_world[static_cast<size_t>(i)];
    const ImVec2 rmin_w = {c.x - half.x, c.y - half.y};
    const ImVec2 rmax_w = {c.x + half.x, c.y + half.y};
    const ImVec2 rmin = mind_map::canvas::WorldToScreen(rmin_w, ctx.canvas_p0_, ctx.pan_px_, ctx.zoom_);
    const ImVec2 rmax = mind_map::canvas::WorldToScreen(rmax_w, ctx.canvas_p0_, ctx.pan_px_, ctx.zoom_);

    const bool incoming_edge_selected =
        (i == selected_child_for_edge) &&
        (mind_map::demos::kSampleMindMapSpecs[static_cast<size_t>(i)].parent_ >= 0);
    const bool hot = (i == dragging_node) || (i == hot_node);
    const ImU32 hot_border = hot ? kColorNodeBorderHot : kColorNodeBorder;
    const ImU32 border = incoming_edge_selected ? kColorNodeBorderSelected : hot_border;
    ctx.draw_list_->AddRectFilled(rmin, rmax, kColorNodeFill, mind_map::canvas::kNodeCornerRadiusWorld);
    ctx.draw_list_->AddRect(rmin, rmax, border, mind_map::canvas::kNodeCornerRadiusWorld, 0, kNodeBorderThickness);

    const ImVec2 text_sz = ImGui::CalcTextSize(label);
    const ImVec2 text_pos = {(rmin.x + rmax.x - text_sz.x) * 0.5F, (rmin.y + rmax.y - text_sz.y) * 0.5F};
    ctx.draw_list_->AddText(text_pos, IM_COL32(235, 235, 245, 255), label);  // NOLINT(hicpp-signed-bitwise)

    const ImVec2 left = mind_map::canvas::WorldToScreen({c.x - half.x, c.y}, ctx.canvas_p0_, ctx.pan_px_, ctx.zoom_);
    const ImVec2 right = mind_map::canvas::WorldToScreen({c.x + half.x, c.y}, ctx.canvas_p0_, ctx.pan_px_, ctx.zoom_);
    ctx.draw_list_->AddCircleFilled(left, kHandleRadius, IM_COL32(90, 200, 255, 200));   // NOLINT(hicpp-signed-bitwise)
    ctx.draw_list_->AddCircleFilled(right, kHandleRadius, IM_COL32(255, 190, 90, 200));  // NOLINT(hicpp-signed-bitwise)
  }
}

void DrawOneChildBranch(const mind_map::ui::branch::BranchRenderContext& branch_ctx, int child_index,
                         mind_map::ui::branch::BranchStyle style,
                         const std::array<ImVec2, mind_map::demos::kSampleMindMapNodeCount>& pos_world) {
  using BranchStyle = mind_map::ui::branch::BranchStyle;
  switch (style) {  // NOSONAR(cpp:S6177) - C++17; `using enum` to shorten cases requires C++20
    case BranchStyle::Bezier:
      mind_map::ui::branch::DrawSampleMindMapBranchBezier(branch_ctx, child_index, pos_world);
      return;
    case BranchStyle::Orthogonal:
      mind_map::ui::branch::DrawSampleMindMapBranchOrthogonal(branch_ctx, child_index, pos_world);
      return;
    case BranchStyle::OrganicTaper:
      mind_map::ui::branch::DrawSampleMindMapBranchOrganicTaper(branch_ctx, child_index, pos_world);
      return;
    default:
      assert(false && "DrawOneChildBranch: exhaustive switch");  // NOLINT(readability-implicit-bool-conversion)
      return;
  }
}

}  // namespace

MindMapCanvasView::MindMapCanvasView() {
  LoadFrom(mind_map::demos::BuildSampleDocument());
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

void MindMapCanvasView::LoadFrom(const mind_map::core::MindMapDocument& doc) {
  dragging_node_ = -1;
  selected_child_for_edge_style_ = -1;

  const size_t node_count = std::min(doc.nodes_.size(), node_ids_.size());
  for (size_t i = 0; i < node_count; ++i) {
    node_ids_[i] = doc.nodes_[i].id_;
  }

  for (const auto& layout : doc.layouts_) {
    for (size_t i = 0; i < node_count; ++i) {
      if (node_ids_[i] == layout.node_id_) {
        pos_world_[i] = {layout.position_.x_, layout.position_.y_};
        break;
      }
    }
  }

  branch_style_by_child_ = {};
  for (const auto& edge : doc.edges_) {
    for (size_t i = 0; i < node_count; ++i) {
      if (node_ids_[i] == edge.child_id_) {
        edge_ids_[i] = edge.id_;
        branch_style_by_child_[i] = BranchStyleFromString(edge.style_);
        break;
      }
    }
  }
}

mind_map::core::MindMapDocument MindMapCanvasView::ToDocument(const mind_map::core::MindMapViewport& viewport) const {
  using mind_map::core::mindmap::kSampleMindMapNodeCount;
  using mind_map::core::mindmap::kSampleMindMapSpecs;

  mind_map::core::MindMapDocument doc;
  doc.viewport_ = viewport;

  for (int i = 0; i < kSampleMindMapNodeCount; ++i) {
    const auto idx = static_cast<size_t>(i);

    mind_map::core::MindMapNode node;
    node.id_ = node_ids_[idx];
    node.label_ = kSampleMindMapSpecs[idx].label_;
    doc.nodes_.push_back(std::move(node));

    mind_map::core::MindMapNodeLayout layout;
    layout.node_id_ = node_ids_[idx];
    layout.position_ = {pos_world_[idx].x, pos_world_[idx].y};
    doc.layouts_.push_back(std::move(layout));

    if (kSampleMindMapSpecs[idx].parent_ >= 0) {
      mind_map::core::MindMapEdge edge;
      edge.id_ = edge_ids_[idx];
      edge.parent_id_ = node_ids_[static_cast<size_t>(kSampleMindMapSpecs[idx].parent_)];
      edge.child_id_ = node_ids_[idx];
      edge.style_ = std::string(BranchStyleToString(branch_style_by_child_[idx]));
      doc.edges_.push_back(std::move(edge));
    }
  }

  return doc;
}

void MindMapCanvasView::OnPrimaryDown(const MindMapPointerState& ptr) {
  if (!ptr.canvas_hovered_) {
    return;
  }
  const int hit = mind_map::demos::HitTestSampleMap(ptr.mouse_world_, pos_world_);
  if (hit >= 0) {
    dragging_node_ = hit;
    const ImVec2 c = pos_world_[static_cast<size_t>(hit)];
    grab_offset_world_ = {ptr.mouse_world_.x - c.x, ptr.mouse_world_.y - c.y};
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
  pos_world_[static_cast<size_t>(dragging_node_)] = {ptr.mouse_world_.x - grab_offset_world_.x,
                                                     ptr.mouse_world_.y - grab_offset_world_.y};
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
  assert(ctx.draw_list_ != nullptr);
  mind_map::canvas::DrawGrid(ctx.draw_list_, ctx.canvas_p0_, ctx.canvas_p1_, ctx.pan_px_, ctx.zoom_);

  const mind_map::ui::branch::BranchRenderContext branch_ctx =
      mind_map::ui::branch::MakeBranchRenderContext(ctx.draw_list_, ctx.canvas_p0_, ctx.pan_px_, ctx.zoom_);

  for (int child = 0; child < mind_map::demos::kSampleMindMapNodeCount; ++child) {
    if (mind_map::demos::kSampleMindMapSpecs[static_cast<size_t>(child)].parent_ < 0) {
      continue;
    }
    DrawOneChildBranch(branch_ctx, child, branch_style_by_child_[static_cast<size_t>(child)], pos_world_);
  }

  DrawSampleMindMapNodes(ctx, dragging_node_, selected_child_for_edge_style_, pos_world_);
}

}  // namespace mind_map::ui
