#include "ui/MindMapCanvasView.h"

#include "core/Base64.h"
#include "core/mindmap/UuidGenerator.h"
#include "ui/branch/BranchRenderContext.h"
#include "ui/branch/DrawBranchTextOnPath.h"
#include "ui/branch/DrawBranchesBezier.h"
#include "ui/branch/DrawBranchesOrganicTaper.h"
#include "ui/branch/DrawBranchesOrthogonal.h"
#include "ui/canvas/CanvasMath.h"
#include "ui/canvas/InlineMarkup.h"
#include "ui/canvas/InlineMarkupRenderer.h"
#include "ui/canvas/NodeExtent.h"
#include "ui/canvas/NodeGeometry.h"
#include "ui/canvas/NodeTextureUtils.h"

#include "imgui.h"

#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_set>
#include <vector>

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

[[nodiscard]] branch::BranchStyle FirstActiveChildEdgeStyle(const std::vector<CanvasNode>& nodes) {
  for (const auto& node : nodes) {
    if (node.active_ && node.parent_idx_.has_value()) { return node.branch_style_; }
  }
  return branch::BranchStyle::Bezier;
}

[[nodiscard]] bool ChildEdgeStylesUniform(const std::vector<CanvasNode>& nodes) {
  const branch::BranchStyle ref = FirstActiveChildEdgeStyle(nodes);
  return std::all_of(nodes.begin(), nodes.end(), [ref](const CanvasNode& node) {  // NOLINT(llvm-use-ranges)
    return !node.active_ || !node.parent_idx_.has_value() || node.branch_style_ == ref;
  });
}

// Collects nodes marked collapsed_, resets their flag (CollapseNode will re-set it), and
// returns their indices sorted deepest-first so inner collapses are established before outer ones.
[[nodiscard]] std::vector<size_t> PrepareCollapseOrder(std::vector<CanvasNode>& nodes) {
  auto depth_of = [&nodes](size_t idx) {
    size_t depth = 0;
    auto p = nodes[idx].parent_idx_;
    while (p.has_value()) { ++depth; p = nodes[*p].parent_idx_; }
    return depth;
  };
  std::vector<size_t> indices;
  for (size_t i = 0; i < nodes.size(); ++i) {
    if (nodes[i].collapsed_) {
      nodes[i].collapsed_ = false;
      indices.push_back(i);
    }
  }
  std::stable_sort(indices.begin(), indices.end(),  // NOLINT(llvm-use-ranges)
                   [&depth_of](size_t a, size_t b) { return depth_of(a) > depth_of(b); });
  return indices;
}

constexpr float kHandleRadius = 5.0F;
constexpr float kCollapseTriangleOffsetX = 14.0F;  // world units right of the node edge
constexpr float kCollapseHitRadius = 10.0F;        // screen pixels; divided by zoom for world hit
constexpr float kCollapseTrianglePx = 5.0F;        // screen-space half-size of the triangle
constexpr ImU32 kColorNodeFill = IM_COL32(48, 52, 64, 255);             // NOLINT(hicpp-signed-bitwise)
constexpr ImU32 kColorNodeBorder = IM_COL32(110, 120, 145, 255);        // NOLINT(hicpp-signed-bitwise)
constexpr ImU32 kColorNodeBorderHot = IM_COL32(200, 200, 240, 255);     // NOLINT(hicpp-signed-bitwise)
constexpr ImU32 kColorNodeBorderSelected = IM_COL32(255, 220, 120, 255);// NOLINT(hicpp-signed-bitwise)
constexpr ImU32 kColorResizeHandle = IM_COL32(255, 220, 120, 230);      // NOLINT(hicpp-signed-bitwise)
constexpr ImU32 kColorResizeHandleBorder = IM_COL32(0, 0, 0, 180);      // NOLINT(hicpp-signed-bitwise)

constexpr float kNodeBorderThickness = 1.5F;
constexpr float kResizeHandleSizePx = 5.0F;
constexpr float kResizeHandleHitPx = 8.0F;
constexpr float kMinHalfExtent = 15.0F;
constexpr float kMinResizeScale = 0.05F;

// Corner indices: TL idx=0 sign=(-1,-1), TR idx=1 sign=(+1,-1), BR idx=2 sign=(+1,+1), BL idx=3 sign=(-1,+1)
constexpr std::array<float, 4> kCornerSignX = {-1.0F, +1.0F, +1.0F, -1.0F};
constexpr std::array<float, 4> kCornerSignY = {-1.0F, -1.0F, +1.0F, +1.0F};

constexpr std::string_view kMixedBranchStylesPreview = "Mixed (per child)";

[[nodiscard]] std::optional<size_t> HitTestNodes(ImVec2 world_pos, const std::vector<CanvasNode>& nodes) {
  for (size_t i = nodes.size(); i-- > 0; ) {
    const CanvasNode& node = nodes[i];
    if (!node.active_) { continue; }
    const ImVec2 half = mind_map::canvas::NodeHalfExtent(node);
    const ImVec2 rmin = {node.pos_world_.x - half.x, node.pos_world_.y - half.y};
    const ImVec2 rmax = {node.pos_world_.x + half.x, node.pos_world_.y + half.y};
    if (mind_map::canvas::IsInsideRect(world_pos, rmin, rmax)) {
      return i;
    }
  }
  return std::nullopt;
}

void DrawResizeHandles(ImDrawList* draw_list, ImVec2 rmin, ImVec2 rmax) {
  const float hs = kResizeHandleSizePx;
  for (size_t c_idx = 0; c_idx < 4; ++c_idx) {
    const ImVec2 corner = {
      kCornerSignX[c_idx] > 0.0F ? rmax.x : rmin.x,
      kCornerSignY[c_idx] > 0.0F ? rmax.y : rmin.y
    };
    draw_list->AddRectFilled({corner.x - hs, corner.y - hs}, {corner.x + hs, corner.y + hs},
                             kColorResizeHandle);
    draw_list->AddRect({corner.x - hs, corner.y - hs}, {corner.x + hs, corner.y + hs},
                       kColorResizeHandleBorder);
  }
}

void DrawCollapseTriangle(ImDrawList* draw_list, ImVec2 rmax, ImVec2 rmin, bool collapsed) {
  constexpr ImU32 kColorTriangle = IM_COL32(180, 200, 255, 200);  // NOLINT(hicpp-signed-bitwise)
  const float cx = rmax.x + kCollapseTriangleOffsetX;
  const float cy = (rmin.y + rmax.y) * 0.5F;
  const float r = kCollapseTrianglePx;
  if (collapsed) {
    // ▶ pointing right
    draw_list->AddTriangleFilled({cx + r, cy}, {cx - r, cy - r}, {cx - r, cy + r}, kColorTriangle);
  } else {
    // ▼ pointing down
    draw_list->AddTriangleFilled({cx, cy + r}, {cx - r, cy - r}, {cx + r, cy - r}, kColorTriangle);
  }
}

void DrawNodeLabel(ImDrawList* draw_list, const char* label, bool has_texture,
                   ImVec2 rmin, ImVec2 rmax, ImVec2 half, float zoom) {
  constexpr ImU32 kColorText = IM_COL32(235, 235, 245, 255);  // NOLINT(hicpp-signed-bitwise)
  const float font_size    = ImGui::GetFontSize() * zoom;
  const float node_w_world = half.x * 2.0F - mind_map::canvas::kNodePad * 2.0F;
  const float wrap_world   = node_w_world;
  const float wrap_screen  = wrap_world * zoom;
  if (canvas::ContainsMarkup(label)) {
    const std::vector<canvas::MarkupSpan> spans = canvas::ParseMarkup(label);
    const ImVec2 text_sz_w = canvas::MeasureMarkup(spans, wrap_world);
    const ImVec2 text_sz   = {text_sz_w.x * zoom, text_sz_w.y * zoom};
    const ImVec2 text_pos  = {(rmin.x + rmax.x - text_sz.x) * 0.5F,
                               (rmin.y + rmax.y - text_sz.y) * 0.5F};
    draw_list->PushClipRect(rmin, rmax, true);
    canvas::DrawMarkup(draw_list, text_pos, font_size, wrap_screen, kColorText, spans);
    draw_list->PopClipRect();
  } else {
    ImFont* const font = ImGui::GetFont();
    const ImVec2 text_sz_base = ImGui::CalcTextSize(label, nullptr, false, wrap_world);
    const ImVec2 text_sz  = {text_sz_base.x * zoom, text_sz_base.y * zoom};
    const ImVec2 text_pos = {(rmin.x + rmax.x - text_sz.x) * 0.5F,
                              (rmin.y + rmax.y - text_sz.y) * 0.5F};
    if (has_texture) {
      constexpr ImU32 kColorTextOutline = IM_COL32(0, 0, 0, 200);  // NOLINT(hicpp-signed-bitwise)
      draw_list->AddText(font, font_size, {text_pos.x - 1.0F, text_pos.y},         kColorTextOutline, label, nullptr, wrap_screen);
      draw_list->AddText(font, font_size, {text_pos.x + 1.0F, text_pos.y},         kColorTextOutline, label, nullptr, wrap_screen);
      draw_list->AddText(font, font_size, {text_pos.x,         text_pos.y - 1.0F}, kColorTextOutline, label, nullptr, wrap_screen);
      draw_list->AddText(font, font_size, {text_pos.x,         text_pos.y + 1.0F}, kColorTextOutline, label, nullptr, wrap_screen);
    }
    draw_list->AddText(font, font_size, text_pos, kColorText, label, nullptr, wrap_screen);
  }
}

void DrawMindMapNodes(const MindMapCanvasRenderContext& ctx,
                      std::optional<size_t> dragging_node,
                      std::optional<size_t> selected_node,
                      std::optional<size_t> selected_child_for_edge,
                      std::optional<size_t> editing_node,
                      const std::vector<CanvasNode>& nodes) {
  assert(ctx.draw_list_ != nullptr);
  const std::optional<size_t> hot_node = ctx.canvas_hovered_
      ? HitTestNodes(ctx.mouse_world_, nodes) : std::nullopt;

  // Pre-compute set of node indices that have at least one child (active or inactive).
  std::unordered_set<size_t> parent_set;
  for (const auto& n : nodes) {
    if (n.parent_idx_.has_value()) {
      parent_set.insert(*n.parent_idx_);
    }
  }

  for (size_t i = 0; i < nodes.size(); ++i) {
    const CanvasNode& node = nodes[i];
    if (!node.active_) { continue; }
    const char* const label = node.label_.c_str();
    const ImVec2 half = mind_map::canvas::NodeHalfExtent(node);
    const ImVec2 c = node.pos_world_;
    const ImVec2 rmin_w = {c.x - half.x, c.y - half.y};
    const ImVec2 rmax_w = {c.x + half.x, c.y + half.y};
    const ImVec2 rmin = mind_map::canvas::WorldToScreen(rmin_w, ctx.canvas_p0_, ctx.pan_px_, ctx.zoom_);
    const ImVec2 rmax = mind_map::canvas::WorldToScreen(rmax_w, ctx.canvas_p0_, ctx.pan_px_, ctx.zoom_);

    const bool incoming_edge_selected = (selected_child_for_edge == i) && node.parent_idx_.has_value();
    const bool hot = (dragging_node == i) || (hot_node == i);
    const ImU32 hot_border = hot ? kColorNodeBorderHot : kColorNodeBorder;
    const ImU32 border = incoming_edge_selected ? kColorNodeBorderSelected : hot_border;
    ctx.draw_list_->AddRectFilled(rmin, rmax, kColorNodeFill, mind_map::canvas::kNodeCornerRadiusWorld);
    if (node.texture_id_ != 0) {
      ctx.draw_list_->AddImageRounded(node.texture_id_, rmin, rmax,
                                      {0.0F, 0.0F}, {1.0F, 1.0F},
                                      IM_COL32_WHITE,  // NOLINT(hicpp-signed-bitwise)
                                      mind_map::canvas::kNodeCornerRadiusWorld);
    }
    ctx.draw_list_->AddRect(rmin, rmax, border, mind_map::canvas::kNodeCornerRadiusWorld, 0, kNodeBorderThickness);

    if (editing_node != i) {
      DrawNodeLabel(ctx.draw_list_, label, node.texture_id_ != 0, rmin, rmax, half, ctx.zoom_);
    }

    const ImVec2 left = mind_map::canvas::WorldToScreen({c.x - half.x, c.y}, ctx.canvas_p0_, ctx.pan_px_, ctx.zoom_);
    const ImVec2 right = mind_map::canvas::WorldToScreen({c.x + half.x, c.y}, ctx.canvas_p0_, ctx.pan_px_, ctx.zoom_);
    ctx.draw_list_->AddCircleFilled(left, kHandleRadius, IM_COL32(90, 200, 255, 200));   // NOLINT(hicpp-signed-bitwise)
    ctx.draw_list_->AddCircleFilled(right, kHandleRadius, IM_COL32(255, 190, 90, 200));  // NOLINT(hicpp-signed-bitwise)

    if (selected_node == i) {
      DrawResizeHandles(ctx.draw_list_, rmin, rmax);
    }

    if (parent_set.count(i) != 0U) {
      DrawCollapseTriangle(ctx.draw_list_, rmax, rmin, node.collapsed_);
    }
  }
}

void DrawOneChildBranch(const mind_map::ui::branch::BranchRenderContext& branch_ctx, size_t child_index,
                         mind_map::ui::branch::BranchStyle style,
                         const std::vector<CanvasNode>& nodes) {
  using BranchStyle = mind_map::ui::branch::BranchStyle;
  switch (style) {  // NOSONAR(cpp:S6177) - C++17; `using enum` to shorten cases requires C++20
    case BranchStyle::Bezier:
      mind_map::ui::branch::DrawMindMapBranchBezier(branch_ctx, child_index, nodes);
      return;
    case BranchStyle::Orthogonal:
      mind_map::ui::branch::DrawMindMapBranchOrthogonal(branch_ctx, child_index, nodes);
      return;
    case BranchStyle::OrganicTaper:
      mind_map::ui::branch::DrawMindMapBranchOrganicTaper(branch_ctx, child_index, nodes);
      return;
    default:
      assert(false && "DrawOneChildBranch: exhaustive switch");  // NOLINT(readability-implicit-bool-conversion)
      return;
  }
}

}  // namespace

MindMapCanvasView::MindMapCanvasView() = default;

MindMapCanvasView::~MindMapCanvasView() {
  for (auto& node : nodes_) {
    ReleaseTexture(node.texture_id_);
    node.texture_id_ = 0;
  }
}


void MindMapCanvasView::Reset() {
  for (size_t i = 0; i < nodes_.size() && i < initial_pos_world_.size(); ++i) {
    nodes_[i].pos_world_ = initial_pos_world_[i];
    nodes_[i].active_ = true;
    nodes_[i].collapsed_ = false;
    nodes_[i].half_extent_override_ = {0.0F, 0.0F};
    nodes_[i].branch_edge_label_ = {};
  }
  collapse_affected_.clear();
  dragging_node_ = std::nullopt;
  resizing_node_ = std::nullopt;
  selected_node_ = std::nullopt;
  selected_child_for_edge_style_ = std::nullopt;
  collapse_toggle_target_ = std::nullopt;
}

void MindMapCanvasView::LoadFrom(const mind_map::core::MindMapDocument& doc) {
  dragging_node_ = std::nullopt;
  resizing_node_ = std::nullopt;
  selected_node_ = std::nullopt;
  selected_child_for_edge_style_ = std::nullopt;
  collapse_toggle_target_ = std::nullopt;
  collapse_affected_.clear();

  for (auto& node : nodes_) {
    ReleaseTexture(node.texture_id_);
    node.texture_id_ = 0;
  }
  nodes_.clear();
  nodes_.reserve(doc.nodes_.size());

  for (const auto& n : doc.nodes_) {
    CanvasNode node;
    node.id_ = n.id_;
    node.label_ = n.label_;
    node.active_ = true;
    node.collapsed_ = n.collapsed_;
    if (!n.image_png_base64_.empty()) {
      node.image_png_base64_ = n.image_png_base64_;
      node.texture_id_ =
          UploadPngTexture(mind_map::core::Base64Decode(n.image_png_base64_), "node " + n.id_);
    }
    nodes_.push_back(std::move(node));
  }

  for (const auto& layout : doc.layouts_) {
    for (auto& node : nodes_) {
      if (node.id_ == layout.node_id_) {
        node.pos_world_ = {layout.position_.x_, layout.position_.y_};
        node.half_extent_override_ = {layout.size_w_, layout.size_h_};
        break;
      }
    }
  }

  for (const auto& edge : doc.edges_) {
    std::optional<size_t> child_idx;
    std::optional<size_t> parent_idx;
    for (size_t i = 0; i < nodes_.size(); ++i) {
      if (nodes_[i].id_ == edge.child_id_)  { child_idx  = i; }
      if (nodes_[i].id_ == edge.parent_id_) { parent_idx = i; }
    }
    if (child_idx.has_value() && parent_idx.has_value()) {
      auto& child_node = nodes_[*child_idx];
      child_node.parent_idx_ = parent_idx;
      child_node.edge_id_ = edge.id_;
      child_node.branch_style_ = BranchStyleFromString(edge.style_);
      child_node.branch_edge_label_ = edge.label_;
    }
  }

  for (const size_t idx : PrepareCollapseOrder(nodes_)) { CollapseNode(idx); }

  initial_pos_world_.resize(nodes_.size());
  for (size_t i = 0; i < nodes_.size(); ++i) {
    initial_pos_world_[i] = nodes_[i].pos_world_;
  }
}

mind_map::core::MindMapDocument MindMapCanvasView::ToDocument(const mind_map::core::MindMapViewport& viewport) const {
  mind_map::core::MindMapDocument doc;
  doc.viewport_ = viewport;

  for (const auto& node : nodes_) {
    mind_map::core::MindMapNode n;
    n.id_ = node.id_;
    n.label_ = node.label_;
    n.image_png_base64_ = node.image_png_base64_;
    n.collapsed_ = node.collapsed_;
    doc.nodes_.push_back(std::move(n));

    mind_map::core::MindMapNodeLayout layout;
    layout.node_id_ = node.id_;
    layout.position_ = {node.pos_world_.x, node.pos_world_.y};
    layout.size_w_ = node.half_extent_override_.x;
    layout.size_h_ = node.half_extent_override_.y;
    doc.layouts_.push_back(std::move(layout));

    if (node.parent_idx_.has_value()) {
      mind_map::core::MindMapEdge edge;
      edge.id_ = node.edge_id_;
      edge.parent_id_ = nodes_[*node.parent_idx_].id_;
      edge.child_id_ = node.id_;
      edge.style_ = std::string(BranchStyleToString(node.branch_style_));
      edge.label_ = node.branch_edge_label_;
      doc.edges_.push_back(std::move(edge));
    }
  }

  return doc;
}

void MindMapCanvasView::OnPrimaryDown(const MindMapPointerState& ptr) {
  if (!ptr.canvas_hovered_) { return; }

  // Check collapse triangles before everything else; they sit outside the node box.
  const float triangle_hit_r = kCollapseHitRadius / (std::max)(ptr.zoom_, 0.01F);
  for (size_t i = 0; i < nodes_.size(); ++i) {
    const CanvasNode& node = nodes_[i];
    if (!node.active_ || !NodeHasChildren(i)) { continue; }
    const ImVec2 half = mind_map::canvas::NodeHalfExtent(node);
    const float tx = node.pos_world_.x + half.x + kCollapseTriangleOffsetX;
    const float ty = node.pos_world_.y;
    if (std::abs(ptr.mouse_world_.x - tx) <= triangle_hit_r &&
        std::abs(ptr.mouse_world_.y - ty) <= triangle_hit_r) {
      collapse_toggle_target_ = i;
      return;
    }
  }

  // Check corner resize handles first (any node may be selected, including root).
  if (selected_node_.has_value()) {
    const size_t sel = *selected_node_;
    const auto& sel_node = nodes_[sel];
    const ImVec2 half = mind_map::canvas::NodeHalfExtent(sel_node);
    const float hs = kResizeHandleHitPx / (std::max)(ptr.zoom_, 0.01F);
    for (size_t c_idx = 0; c_idx < 4; ++c_idx) {
      const ImVec2 corner_w = {
        sel_node.pos_world_.x + kCornerSignX[c_idx] * half.x,
        sel_node.pos_world_.y + kCornerSignY[c_idx] * half.y
      };
      if (std::abs(ptr.mouse_world_.x - corner_w.x) <= hs &&
          std::abs(ptr.mouse_world_.y - corner_w.y) <= hs) {
        resizing_node_ = sel;
        resizing_corner_ = c_idx;
        resize_orig_half_ = half;
        resize_lock_aspect_ = (sel_node.texture_id_ != 0);
        resize_anchor_world_ = {
          sel_node.pos_world_.x - kCornerSignX[c_idx] * half.x,
          sel_node.pos_world_.y - kCornerSignY[c_idx] * half.y
        };
        return;
      }
    }
  }

  if (const auto hit = HitTestNodes(ptr.mouse_world_, nodes_); hit.has_value()) {
    dragging_node_ = hit;
    selected_node_ = hit;
    const ImVec2 c = nodes_[*hit].pos_world_;
    grab_offset_world_ = {ptr.mouse_world_.x - c.x, ptr.mouse_world_.y - c.y};
    if (nodes_[*hit].parent_idx_.has_value()) {
      selected_child_for_edge_style_ = hit;
    } else {
      selected_child_for_edge_style_ = std::nullopt;
    }
  } else {
    dragging_node_ = std::nullopt;
    selected_node_ = std::nullopt;
    selected_child_for_edge_style_ = std::nullopt;
  }
}

void MindMapCanvasView::OnPrimaryDrag(const MindMapPointerState& ptr) {  // NOLINT(readability-make-member-function-const)
  if (resizing_node_.has_value() && resizing_corner_.has_value()) {
    auto& node = nodes_[*resizing_node_];
    const float csx = kCornerSignX[*resizing_corner_];
    const float csy = kCornerSignY[*resizing_corner_];
    const ImVec2 mouse = ptr.mouse_world_;
    const ImVec2& anchor = resize_anchor_world_;

    if (resize_lock_aspect_) {
      const float ohx = resize_orig_half_.x;
      const float ohy = resize_orig_half_.y;
      const float denom = 2.0F * (ohx * ohx + ohy * ohy);
      const float proj = denom > 0.0F
          ? (csx * (mouse.x - anchor.x) * ohx + csy * (mouse.y - anchor.y) * ohy) / denom
          : kMinResizeScale;
      const float s = (std::max)(proj, kMinResizeScale);
      const float new_hx = (std::max)(ohx * s, kMinHalfExtent);
      const float new_hy = (std::max)(ohy * s, kMinHalfExtent);
      node.half_extent_override_ = {new_hx, new_hy};
      node.pos_world_ = {anchor.x + csx * new_hx, anchor.y + csy * new_hy};
    } else {
      const float new_hx = (std::max)(std::abs(mouse.x - anchor.x) * 0.5F, kMinHalfExtent);
      const float new_hy = (std::max)(std::abs(mouse.y - anchor.y) * 0.5F, kMinHalfExtent);
      node.half_extent_override_ = {new_hx, new_hy};
      node.pos_world_ = {(mouse.x + anchor.x) * 0.5F, (mouse.y + anchor.y) * 0.5F};
    }
    return;
  }

  if (!dragging_node_.has_value()) { return; }
  nodes_[*dragging_node_].pos_world_ = {
      ptr.mouse_world_.x - grab_offset_world_.x, ptr.mouse_world_.y - grab_offset_world_.y};
}

void MindMapCanvasView::OnPrimaryUp() {
  dragging_node_ = std::nullopt;
  resizing_node_ = std::nullopt;
}

bool MindMapCanvasView::IsDraggingContent() const {
  return dragging_node_.has_value() || resizing_node_.has_value();
}

void MindMapCanvasView::SetBranchStyleForAllEdges(const mind_map::ui::branch::BranchStyle style) {
  for (auto& node : nodes_) {
    if (node.active_ && node.parent_idx_.has_value()) {
      node.branch_style_ = style;
    }
  }
}

const char* MindMapCanvasView::GetBranchStyleComboPreviewLabel() const {
  if (!ChildEdgeStylesUniform(nodes_)) {
    return kMixedBranchStylesPreview.data();
  }
  return mind_map::ui::branch::GetBranchStyleDisplayName(FirstActiveChildEdgeStyle(nodes_));
}

bool MindMapCanvasView::BranchStylesAreUniform() const {
  return ChildEdgeStylesUniform(nodes_);
}

mind_map::ui::branch::BranchStyle MindMapCanvasView::RepresentativeChildEdgeStyle() const {
  return FirstActiveChildEdgeStyle(nodes_);
}

bool MindMapCanvasView::HasSelectedIncomingEdgeStyleTarget() const {
  if (!selected_child_for_edge_style_.has_value()) { return false; }
  if (*selected_child_for_edge_style_ >= nodes_.size()) { return false; }
  return nodes_[*selected_child_for_edge_style_].parent_idx_.has_value();
}

std::optional<size_t> MindMapCanvasView::GetSelectedNode() const {
  return selected_node_;
}

std::optional<size_t> MindMapCanvasView::GetSelectedChildForBranchStyle() const {
  return selected_child_for_edge_style_;
}

const char* MindMapCanvasView::GetSelectedIncomingEdgeChildLabel() const {
  if (const auto idx = selected_child_for_edge_style_; idx.has_value() && HasSelectedIncomingEdgeStyleTarget()) {
    return nodes_[*idx].label_.c_str();
  }
  return nullptr;
}

mind_map::ui::branch::BranchStyle MindMapCanvasView::GetBranchStyleForSelectedChildEdge() const {
  if (const auto idx = selected_child_for_edge_style_; idx.has_value() && HasSelectedIncomingEdgeStyleTarget()) {
    return nodes_[*idx].branch_style_;
  }
  return mind_map::ui::branch::BranchStyle::Bezier;
}

void MindMapCanvasView::SetBranchStyleForSelectedChildEdge(mind_map::ui::branch::BranchStyle style) {  // NOLINT(readability-make-member-function-const)
  if (const auto idx = selected_child_for_edge_style_; idx.has_value() && HasSelectedIncomingEdgeStyleTarget()) {
    nodes_[*idx].branch_style_ = style;
  }
}

void MindMapCanvasView::Render(const MindMapCanvasRenderContext& ctx) {
  assert(ctx.draw_list_ != nullptr);
  mind_map::canvas::DrawGrid(ctx.draw_list_, ctx.canvas_p0_, ctx.canvas_p1_, ctx.pan_px_, ctx.zoom_);

  const mind_map::ui::branch::BranchRenderContext branch_ctx =
      mind_map::ui::branch::MakeBranchRenderContext(ctx.draw_list_, ctx.canvas_p0_, ctx.pan_px_, ctx.zoom_);

  for (size_t child = 0; child < nodes_.size(); ++child) {
    const CanvasNode& node = nodes_[child];
    if (!node.active_ || !node.parent_idx_.has_value()) { continue; }
    DrawOneChildBranch(branch_ctx, child, node.branch_style_, nodes_);
    if (!node.branch_edge_label_.empty()) {
      mind_map::ui::branch::DrawMindMapBranchTextOnPath(branch_ctx, child, nodes_, node.branch_edge_label_, {});
    }
  }

  DrawMindMapNodes(ctx, dragging_node_, selected_node_, selected_child_for_edge_style_, editing_node_, nodes_);
}

void MindMapCanvasView::SetNodeActive(size_t idx, bool active) {
  if (idx >= nodes_.size()) { return; }
  nodes_[idx].active_ = active;
  if (!active) {
    if (selected_node_ == idx)                 { selected_node_ = std::nullopt; }
    if (selected_child_for_edge_style_ == idx) { selected_child_for_edge_style_ = std::nullopt; }
  }
}

bool MindMapCanvasView::IsNodeActive(size_t idx) const {
  if (idx >= nodes_.size()) { return false; }
  return nodes_[idx].active_;
}

std::vector<size_t> MindMapCanvasView::CollectActiveSubtree(size_t idx) const {
  std::vector<size_t> result;
  if (idx >= nodes_.size() || !nodes_[idx].active_) { return result; }
  std::vector<size_t> stack = {idx};
  while (!stack.empty()) {
    const size_t current = stack.back();
    stack.pop_back();
    result.push_back(current);
    for (size_t c = 0; c < nodes_.size(); ++c) {
      if (nodes_[c].parent_idx_ == current && nodes_[c].active_) {
        stack.push_back(c);
      }
    }
  }
  return result;
}

void MindMapCanvasView::CollapseNode(size_t idx) {
  if (idx >= nodes_.size() || nodes_[idx].collapsed_) { return; }
  nodes_[idx].collapsed_ = true;
  const std::vector<size_t> subtree = CollectActiveSubtree(idx);
  std::vector<size_t> affected;
  for (const size_t c : subtree) {
    if (c == idx) { continue; }
    affected.push_back(c);
    SetNodeActive(c, false);
  }
  collapse_affected_[idx] = std::move(affected);
}

void MindMapCanvasView::ExpandNode(size_t idx) {
  if (idx >= nodes_.size()) { return; }
  nodes_[idx].collapsed_ = false;
  const auto it = collapse_affected_.find(idx);
  if (it == collapse_affected_.end()) { return; }
  // Guard against undo/redo interleaving: collapse_affected_[idx] records every node that was
  // active when CollapseNode(idx) ran, but a subsequent undo/redo may have collapsed an inner
  // ancestor since then.  Reactivating such a node would bypass that inner collapse and make its
  // subtree visible when it should stay hidden.  Skip any node that still has a collapsed
  // ancestor between itself and idx.
  for (const size_t c : it->second) {
    auto p = nodes_[c].parent_idx_;
    bool has_collapsed_ancestor = false;
    while (p.has_value() && *p != idx) {
      if (nodes_[*p].collapsed_) { has_collapsed_ancestor = true; break; }
      p = nodes_[*p].parent_idx_;
    }
    if (!has_collapsed_ancestor) { SetNodeActive(c, true); }
  }
  collapse_affected_.erase(it);
}

bool MindMapCanvasView::IsCollapsed(size_t idx) const {
  if (idx >= nodes_.size()) { return false; }
  return nodes_[idx].collapsed_;
}

bool MindMapCanvasView::NodeHasChildren(size_t idx) const {
  return std::any_of(nodes_.begin(), nodes_.end(), [idx](const CanvasNode& node) {  // NOLINT(llvm-use-ranges)
    return node.parent_idx_ == idx;
  });
}

std::optional<size_t> MindMapCanvasView::ConsumeCollapseToggleTarget() {
  const auto target = collapse_toggle_target_;
  collapse_toggle_target_ = std::nullopt;
  return target;
}

size_t MindMapCanvasView::InsertChildNode(size_t parent_idx) {
  assert(parent_idx < nodes_.size());
  const CanvasNode& parent = nodes_[parent_idx];

  size_t sibling_count = 0;
  for (const auto& node : nodes_) {
    if (node.active_ && node.parent_idx_ == parent_idx) {
      ++sibling_count;
    }
  }

  constexpr float kChildOffsetX = 260.0F;
  constexpr float kSiblingStepY = 120.0F;

  CanvasNode child;
  child.id_          = mind_map::core::mindmap::GenerateUuidV4();
  child.edge_id_     = mind_map::core::mindmap::GenerateUuidV4();
  child.label_       = "New node";
  child.parent_idx_  = parent_idx;
  child.branch_style_ = !parent.parent_idx_.has_value()
      ? RepresentativeChildEdgeStyle() : parent.branch_style_;
  child.pos_world_   = {parent.pos_world_.x + kChildOffsetX,
                        parent.pos_world_.y + static_cast<float>(sibling_count) * kSiblingStepY};
  child.active_      = true;

  const size_t new_idx = nodes_.size();
  initial_pos_world_.push_back(child.pos_world_);
  nodes_.push_back(std::move(child));
  selected_node_ = new_idx;
  selected_child_for_edge_style_ = new_idx;
  return new_idx;
}

void MindMapCanvasView::SelectNode(std::optional<size_t> idx) {
  selected_node_ = idx;
  selected_child_for_edge_style_ =
      (idx && nodes_[*idx].parent_idx_.has_value()) ? idx : std::nullopt;
}

ImVec2 MindMapCanvasView::GetNodeWorldPos(size_t idx) const {
  assert(idx < nodes_.size());
  return nodes_[idx].pos_world_;
}

void MindMapCanvasView::BeginEditing(size_t idx) {
  assert(idx < nodes_.size());
  editing_node_ = idx;
  edit_original_ = nodes_[idx].label_;
  edit_buffer_   = nodes_[idx].label_;
  SelectNode(idx);
}

void MindMapCanvasView::CancelEditing() {
  editing_node_ = std::nullopt;
  edit_buffer_.clear();
  edit_original_.clear();
}

}  // namespace mind_map::ui
