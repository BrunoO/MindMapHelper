#include "ui/MindMapCanvasView.h"

#include "core/Base64.h"
#include "core/mindmap/UuidGenerator.h"
#include "ui/branch/BranchRenderContext.h"
#include "ui/branch/DrawBranchesBezier.h"
#include "ui/branch/DrawBranchesOrganicTaper.h"
#include "ui/branch/DrawBranchesOrthogonal.h"
#include "ui/canvas/CanvasMath.h"
#include "ui/canvas/NodeExtent.h"
#include "ui/canvas/NodeGeometry.h"
#include "ui/canvas/NodeTextureUtils.h"
#include "ui/demos/SampleMindMapGraph.h"

#include "imgui.h"

#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
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

constexpr float kHandleRadius = 5.0F;
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

// Corner index to sign: 0=TL(-1,-1), 1=TR(+1,-1), 2=BR(+1,+1), 3=BL(-1,+1)
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

void DrawMindMapNodes(const MindMapCanvasRenderContext& ctx,
                      std::optional<size_t> dragging_node,
                      std::optional<size_t> selected_node,
                      std::optional<size_t> selected_child_for_edge,
                      const std::vector<CanvasNode>& nodes) {
  assert(ctx.draw_list_ != nullptr);
  const std::optional<size_t> hot_node = ctx.canvas_hovered_
      ? HitTestNodes(ctx.mouse_world_, nodes) : std::nullopt;

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

    ImFont* const font = ImGui::GetFont();
    const float font_size = ImGui::GetFontSize() * ctx.zoom_;
    const ImVec2 text_sz_base = ImGui::CalcTextSize(label);
    const ImVec2 text_sz = {text_sz_base.x * ctx.zoom_, text_sz_base.y * ctx.zoom_};
    const ImVec2 text_pos = {(rmin.x + rmax.x - text_sz.x) * 0.5F, (rmin.y + rmax.y - text_sz.y) * 0.5F};
    if (node.texture_id_ != 0) {
      // 4-direction dark outline so white text stays legible on any image.
      constexpr ImU32 kColorTextOutline = IM_COL32(0, 0, 0, 200);  // NOLINT(hicpp-signed-bitwise)
      ctx.draw_list_->AddText(font, font_size, {text_pos.x - 1.0F, text_pos.y},         kColorTextOutline, label);
      ctx.draw_list_->AddText(font, font_size, {text_pos.x + 1.0F, text_pos.y},         kColorTextOutline, label);
      ctx.draw_list_->AddText(font, font_size, {text_pos.x,         text_pos.y - 1.0F}, kColorTextOutline, label);
      ctx.draw_list_->AddText(font, font_size, {text_pos.x,         text_pos.y + 1.0F}, kColorTextOutline, label);
    }
    ctx.draw_list_->AddText(font, font_size, text_pos, IM_COL32(235, 235, 245, 255), label);  // NOLINT(hicpp-signed-bitwise)

    const ImVec2 left = mind_map::canvas::WorldToScreen({c.x - half.x, c.y}, ctx.canvas_p0_, ctx.pan_px_, ctx.zoom_);
    const ImVec2 right = mind_map::canvas::WorldToScreen({c.x + half.x, c.y}, ctx.canvas_p0_, ctx.pan_px_, ctx.zoom_);
    ctx.draw_list_->AddCircleFilled(left, kHandleRadius, IM_COL32(90, 200, 255, 200));   // NOLINT(hicpp-signed-bitwise)
    ctx.draw_list_->AddCircleFilled(right, kHandleRadius, IM_COL32(255, 190, 90, 200));  // NOLINT(hicpp-signed-bitwise)

    if (selected_node == i) {
      DrawResizeHandles(ctx.draw_list_, rmin, rmax);
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

MindMapCanvasView::MindMapCanvasView() {
  LoadFrom(mind_map::demos::BuildSampleDocument());
}

MindMapCanvasView::~MindMapCanvasView() {
  for (auto& node : nodes_) {
    ReleaseTexture(node.texture_id_);
    node.texture_id_ = 0;
  }
}

mind_map::ui::branch::BranchStyle MindMapCanvasView::StyleOfFirstChildEdge_() const {
  for (const auto& node : nodes_) {
    if (node.active_ && node.parent_idx_) {
      return node.branch_style_;
    }
  }
  return mind_map::ui::branch::BranchStyle::Bezier;
}

bool MindMapCanvasView::BranchStylesAreUniform_() const {
  const mind_map::ui::branch::BranchStyle ref = StyleOfFirstChildEdge_();
  return std::all_of(nodes_.begin(), nodes_.end(), [ref](const CanvasNode& node) {  // NOLINT(llvm-use-ranges)
    return !node.active_ || !node.parent_idx_ || node.branch_style_ == ref;
  });
}

void MindMapCanvasView::Reset() {
  for (size_t i = 0; i < nodes_.size() && i < initial_pos_world_.size(); ++i) {
    nodes_[i].pos_world_ = initial_pos_world_[i];
    nodes_[i].active_ = true;
    nodes_[i].half_extent_override_ = {0.0F, 0.0F};
  }
  dragging_node_ = std::nullopt;
  resizing_node_ = std::nullopt;
  selected_node_ = std::nullopt;
  selected_child_for_edge_style_ = std::nullopt;
}

void MindMapCanvasView::LoadFrom(const mind_map::core::MindMapDocument& doc) {
  dragging_node_ = std::nullopt;
  resizing_node_ = std::nullopt;
  selected_node_ = std::nullopt;
  selected_child_for_edge_style_ = std::nullopt;

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
    if (!n.image_png_base64_.empty()) {
      node.image_png_base64_ = n.image_png_base64_;
      node.texture_id_ = UploadPngTexture(mind_map::core::Base64Decode(n.image_png_base64_));
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
    if (child_idx && parent_idx) {
      auto& child_node = nodes_[*child_idx];
      child_node.parent_idx_ = parent_idx;
      child_node.edge_id_ = edge.id_;
      child_node.branch_style_ = BranchStyleFromString(edge.style_);
    }
  }

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
    doc.nodes_.push_back(std::move(n));

    mind_map::core::MindMapNodeLayout layout;
    layout.node_id_ = node.id_;
    layout.position_ = {node.pos_world_.x, node.pos_world_.y};
    layout.size_w_ = node.half_extent_override_.x;
    layout.size_h_ = node.half_extent_override_.y;
    doc.layouts_.push_back(std::move(layout));

    if (node.parent_idx_) {
      mind_map::core::MindMapEdge edge;
      edge.id_ = node.edge_id_;
      edge.parent_id_ = nodes_[*node.parent_idx_].id_;
      edge.child_id_ = node.id_;
      edge.style_ = std::string(BranchStyleToString(node.branch_style_));
      doc.edges_.push_back(std::move(edge));
    }
  }

  return doc;
}

void MindMapCanvasView::OnPrimaryDown(const MindMapPointerState& ptr) {
  if (!ptr.canvas_hovered_) { return; }

  // Check corner resize handles first (any node may be selected, including root).
  if (selected_node_) {
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

  if (const auto hit = HitTestNodes(ptr.mouse_world_, nodes_)) {
    dragging_node_ = hit;
    selected_node_ = hit;
    const ImVec2 c = nodes_[*hit].pos_world_;
    grab_offset_world_ = {ptr.mouse_world_.x - c.x, ptr.mouse_world_.y - c.y};
    if (nodes_[*hit].parent_idx_) {
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
  if (resizing_node_ && resizing_corner_) {
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

  if (!dragging_node_) { return; }
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
    if (node.active_ && node.parent_idx_) {
      node.branch_style_ = style;
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
  if (!selected_child_for_edge_style_) { return false; }
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
  if (const auto idx = selected_child_for_edge_style_; idx && HasSelectedIncomingEdgeStyleTarget()) {
    return nodes_[*idx].label_.c_str();
  }
  return nullptr;
}

mind_map::ui::branch::BranchStyle MindMapCanvasView::GetBranchStyleForSelectedChildEdge() const {
  if (const auto idx = selected_child_for_edge_style_; idx && HasSelectedIncomingEdgeStyleTarget()) {
    return nodes_[*idx].branch_style_;
  }
  return mind_map::ui::branch::BranchStyle::Bezier;
}

void MindMapCanvasView::SetBranchStyleForSelectedChildEdge(mind_map::ui::branch::BranchStyle style) {  // NOLINT(readability-make-member-function-const)
  if (const auto idx = selected_child_for_edge_style_; idx && HasSelectedIncomingEdgeStyleTarget()) {
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
    if (!node.active_ || !node.parent_idx_) { continue; }
    DrawOneChildBranch(branch_ctx, child, node.branch_style_, nodes_);
  }

  DrawMindMapNodes(ctx, dragging_node_, selected_node_, selected_child_for_edge_style_, nodes_);
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

void MindMapCanvasView::SetNodeImage(size_t idx, std::string_view png_base64) {
  if (idx >= nodes_.size()) { return; }
  auto& node = nodes_[idx];
  ReleaseTexture(node.texture_id_);
  node.texture_id_ = 0;
  node.image_png_base64_ = png_base64;
  if (!png_base64.empty()) {
    node.texture_id_ = UploadPngTexture(mind_map::core::Base64Decode(node.image_png_base64_));
  }
}

const std::string& MindMapCanvasView::GetNodeImageBase64(size_t idx) const {
  static const std::string kEmpty;
  if (idx >= nodes_.size()) { return kEmpty; }
  return nodes_[idx].image_png_base64_;
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
  child.branch_style_ = !parent.parent_idx_
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

}  // namespace mind_map::ui
