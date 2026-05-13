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
constexpr float kResizeHandleSizePx = 5.0F;   // visual half-size in screen pixels
constexpr float kResizeHandleHitPx = 8.0F;    // hit-test half-size in screen pixels
constexpr float kMinHalfExtent = 15.0F;       // minimum world-space half-extent after resize
constexpr float kMinResizeScale = 0.05F;      // minimum scale factor for aspect-locked resize

// Corner index to sign: 0=TL(-1,-1), 1=TR(+1,-1), 2=BR(+1,+1), 3=BL(-1,+1)
constexpr std::array<float, 4> kCornerSignX = {-1.0F, +1.0F, +1.0F, -1.0F};
constexpr std::array<float, 4> kCornerSignY = {-1.0F, -1.0F, +1.0F, +1.0F};

constexpr std::string_view kMixedBranchStylesPreview = "Mixed (per child)";

[[nodiscard]] int HitTestNodes(ImVec2 world_pos, const std::vector<CanvasNode>& nodes) {
  for (int i = static_cast<int>(nodes.size()) - 1; i >= 0; --i) {
    const CanvasNode& node = nodes[static_cast<size_t>(i)];
    if (!node.active_) {
      continue;
    }
    const ImVec2 half = mind_map::canvas::NodeHalfExtent(node);
    const ImVec2 rmin = {node.pos_world_.x - half.x, node.pos_world_.y - half.y};
    const ImVec2 rmax = {node.pos_world_.x + half.x, node.pos_world_.y + half.y};
    if (mind_map::canvas::IsInsideRect(world_pos, rmin, rmax)) {
      return i;
    }
  }
  return -1;
}

void DrawResizeHandles(ImDrawList* draw_list, ImVec2 rmin, ImVec2 rmax) {
  const float hs = kResizeHandleSizePx;
  for (int c_idx = 0; c_idx < 4; ++c_idx) {
    const ImVec2 corner = {
      kCornerSignX[static_cast<size_t>(c_idx)] > 0.0F ? rmax.x : rmin.x,
      kCornerSignY[static_cast<size_t>(c_idx)] > 0.0F ? rmax.y : rmin.y
    };
    draw_list->AddRectFilled({corner.x - hs, corner.y - hs}, {corner.x + hs, corner.y + hs},
                             kColorResizeHandle);
    draw_list->AddRect({corner.x - hs, corner.y - hs}, {corner.x + hs, corner.y + hs},
                       kColorResizeHandleBorder);
  }
}

void DrawMindMapNodes(const MindMapCanvasRenderContext& ctx, int dragging_node, int selected_node,
                      int selected_child_for_edge, const std::vector<CanvasNode>& nodes) {
  assert(ctx.draw_list_ != nullptr);
  const int hot_node = ctx.canvas_hovered_ ? HitTestNodes(ctx.mouse_world_, nodes) : -1;

  for (int i = 0; i < static_cast<int>(nodes.size()); ++i) {
    const CanvasNode& node = nodes[static_cast<size_t>(i)];
    if (!node.active_) {
      continue;
    }
    const char* const label = node.label_.c_str();
    const ImVec2 half = mind_map::canvas::NodeHalfExtent(node);
    const ImVec2 c = node.pos_world_;
    const ImVec2 rmin_w = {c.x - half.x, c.y - half.y};
    const ImVec2 rmax_w = {c.x + half.x, c.y + half.y};
    const ImVec2 rmin = mind_map::canvas::WorldToScreen(rmin_w, ctx.canvas_p0_, ctx.pan_px_, ctx.zoom_);
    const ImVec2 rmax = mind_map::canvas::WorldToScreen(rmax_w, ctx.canvas_p0_, ctx.pan_px_, ctx.zoom_);

    const bool incoming_edge_selected = (i == selected_child_for_edge) && (node.parent_idx_ >= 0);
    const bool hot = (i == dragging_node) || (i == hot_node);
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

    if (i == selected_node) {
      DrawResizeHandles(ctx.draw_list_, rmin, rmax);
    }
  }
}

void DrawOneChildBranch(const mind_map::ui::branch::BranchRenderContext& branch_ctx, int child_index,
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
    if (node.active_ && node.parent_idx_ >= 0) {
      return node.branch_style_;
    }
  }
  return mind_map::ui::branch::BranchStyle::Bezier;
}

bool MindMapCanvasView::BranchStylesAreUniform_() const {
  const mind_map::ui::branch::BranchStyle ref = StyleOfFirstChildEdge_();
  return std::all_of(nodes_.begin(), nodes_.end(), [ref](const CanvasNode& node) {  // NOLINT(llvm-use-ranges)
    return !node.active_ || node.parent_idx_ < 0 || node.branch_style_ == ref;
  });
}

void MindMapCanvasView::Reset() {
  for (size_t i = 0; i < nodes_.size() && i < initial_pos_world_.size(); ++i) {
    nodes_[i].pos_world_ = initial_pos_world_[i];
    nodes_[i].active_ = true;
    nodes_[i].half_extent_override_ = {0.0F, 0.0F};
  }
  dragging_node_ = -1;
  resizing_node_ = -1;
  selected_node_ = -1;
  selected_child_for_edge_style_ = -1;
}

void MindMapCanvasView::LoadFrom(const mind_map::core::MindMapDocument& doc) {
  dragging_node_ = -1;
  resizing_node_ = -1;
  selected_node_ = -1;
  selected_child_for_edge_style_ = -1;

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
    int child_idx = -1;
    int parent_idx = -1;
    for (int i = 0; i < static_cast<int>(nodes_.size()); ++i) {
      const auto idx = static_cast<size_t>(i);
      if (nodes_[idx].id_ == edge.child_id_) {
        child_idx = i;
      }
      if (nodes_[idx].id_ == edge.parent_id_) {
        parent_idx = i;
      }
    }
    if (child_idx >= 0 && parent_idx >= 0) {
      auto& child_node = nodes_[static_cast<size_t>(child_idx)];
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

    if (node.parent_idx_ >= 0) {
      mind_map::core::MindMapEdge edge;
      edge.id_ = node.edge_id_;
      edge.parent_id_ = nodes_[static_cast<size_t>(node.parent_idx_)].id_;
      edge.child_id_ = node.id_;
      edge.style_ = std::string(BranchStyleToString(node.branch_style_));
      doc.edges_.push_back(std::move(edge));
    }
  }

  return doc;
}

void MindMapCanvasView::OnPrimaryDown(const MindMapPointerState& ptr) {
  if (!ptr.canvas_hovered_) {
    return;
  }

  // Check corner resize handles first (any node may be selected, including root).
  if (selected_node_ >= 0) {
    const int sel = selected_node_;
    const auto& sel_node = nodes_[static_cast<size_t>(sel)];
    const ImVec2 half = mind_map::canvas::NodeHalfExtent(sel_node);
    const float hs = kResizeHandleHitPx / (std::max)(ptr.zoom_, 0.01F);
    for (int c_idx = 0; c_idx < 4; ++c_idx) {
      const ImVec2 corner_w = {
        sel_node.pos_world_.x + kCornerSignX[static_cast<size_t>(c_idx)] * half.x,
        sel_node.pos_world_.y + kCornerSignY[static_cast<size_t>(c_idx)] * half.y
      };
      if (std::abs(ptr.mouse_world_.x - corner_w.x) <= hs &&
          std::abs(ptr.mouse_world_.y - corner_w.y) <= hs) {
        resizing_node_ = sel;
        resizing_corner_ = c_idx;
        resize_orig_half_ = half;
        resize_lock_aspect_ = (sel_node.texture_id_ != 0);
        resize_anchor_world_ = {
          sel_node.pos_world_.x - kCornerSignX[static_cast<size_t>(c_idx)] * half.x,
          sel_node.pos_world_.y - kCornerSignY[static_cast<size_t>(c_idx)] * half.y
        };
        return;
      }
    }
  }

  const int hit = HitTestNodes(ptr.mouse_world_, nodes_);
  if (hit >= 0) {
    dragging_node_ = hit;
    selected_node_ = hit;
    const ImVec2 c = nodes_[static_cast<size_t>(hit)].pos_world_;
    grab_offset_world_ = {ptr.mouse_world_.x - c.x, ptr.mouse_world_.y - c.y};
    if (nodes_[static_cast<size_t>(hit)].parent_idx_ >= 0) {
      selected_child_for_edge_style_ = hit;
    }
    else {
      selected_child_for_edge_style_ = -1;
    }
  }
  else {
    dragging_node_ = -1;
    selected_node_ = -1;
    selected_child_for_edge_style_ = -1;
  }
}

void MindMapCanvasView::OnPrimaryDrag(const MindMapPointerState& ptr) {  // NOLINT(readability-make-member-function-const)
  if (resizing_node_ >= 0) {
    auto& node = nodes_[static_cast<size_t>(resizing_node_)];
    const float csx = kCornerSignX[static_cast<size_t>(resizing_corner_)];
    const float csy = kCornerSignY[static_cast<size_t>(resizing_corner_)];
    const ImVec2 mouse = ptr.mouse_world_;
    const ImVec2& anchor = resize_anchor_world_;

    if (resize_lock_aspect_) {
      // Project mouse onto the resize diagonal to preserve aspect ratio.
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

  if (dragging_node_ < 0) {
    return;
  }
  nodes_[static_cast<size_t>(dragging_node_)].pos_world_ = {
      ptr.mouse_world_.x - grab_offset_world_.x, ptr.mouse_world_.y - grab_offset_world_.y};
}

void MindMapCanvasView::OnPrimaryUp() {
  dragging_node_ = -1;
  resizing_node_ = -1;
}

bool MindMapCanvasView::IsDraggingContent() const {
  return dragging_node_ >= 0 || resizing_node_ >= 0;
}

void MindMapCanvasView::SetBranchStyleForAllEdges(const mind_map::ui::branch::BranchStyle style) {
  for (auto& node : nodes_) {
    if (node.active_ && node.parent_idx_ >= 0) {
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
  if (selected_child_for_edge_style_ < 0 ||
      selected_child_for_edge_style_ >= static_cast<int>(nodes_.size())) {
    return false;
  }
  return nodes_[static_cast<size_t>(selected_child_for_edge_style_)].parent_idx_ >= 0;
}

int MindMapCanvasView::GetSelectedNode() const {
  return selected_node_;
}

int MindMapCanvasView::GetSelectedChildForBranchStyle() const {
  return selected_child_for_edge_style_;
}

const char* MindMapCanvasView::GetSelectedIncomingEdgeChildLabel() const {
  if (!HasSelectedIncomingEdgeStyleTarget()) {
    return nullptr;
  }
  return nodes_[static_cast<size_t>(selected_child_for_edge_style_)].label_.c_str();
}

mind_map::ui::branch::BranchStyle MindMapCanvasView::GetBranchStyleForSelectedChildEdge() const {
  if (!HasSelectedIncomingEdgeStyleTarget()) {
    return mind_map::ui::branch::BranchStyle::Bezier;
  }
  return nodes_[static_cast<size_t>(selected_child_for_edge_style_)].branch_style_;
}

void MindMapCanvasView::SetBranchStyleForSelectedChildEdge(mind_map::ui::branch::BranchStyle style) {  // NOLINT(readability-make-member-function-const)
  if (!HasSelectedIncomingEdgeStyleTarget()) {
    return;
  }
  nodes_[static_cast<size_t>(selected_child_for_edge_style_)].branch_style_ = style;
}

void MindMapCanvasView::Render(const MindMapCanvasRenderContext& ctx) {
  assert(ctx.draw_list_ != nullptr);
  mind_map::canvas::DrawGrid(ctx.draw_list_, ctx.canvas_p0_, ctx.canvas_p1_, ctx.pan_px_, ctx.zoom_);

  const mind_map::ui::branch::BranchRenderContext branch_ctx =
      mind_map::ui::branch::MakeBranchRenderContext(ctx.draw_list_, ctx.canvas_p0_, ctx.pan_px_, ctx.zoom_);

  for (int child = 0; child < static_cast<int>(nodes_.size()); ++child) {
    const CanvasNode& node = nodes_[static_cast<size_t>(child)];
    if (!node.active_ || node.parent_idx_ < 0) {
      continue;
    }
    DrawOneChildBranch(branch_ctx, child, node.branch_style_, nodes_);
  }

  DrawMindMapNodes(ctx, dragging_node_, selected_node_, selected_child_for_edge_style_, nodes_);
}

void MindMapCanvasView::SetNodeActive(int idx, bool active) {
  if (idx < 0 || idx >= static_cast<int>(nodes_.size())) {
    return;
  }
  nodes_[static_cast<size_t>(idx)].active_ = active;
  if (!active) {
    if (selected_node_ == idx)               { selected_node_ = -1; }
    if (selected_child_for_edge_style_ == idx) { selected_child_for_edge_style_ = -1; }
  }
}

bool MindMapCanvasView::IsNodeActive(int idx) const {
  if (idx < 0 || idx >= static_cast<int>(nodes_.size())) {
    return false;
  }
  return nodes_[static_cast<size_t>(idx)].active_;
}

std::vector<int> MindMapCanvasView::CollectActiveSubtree(int idx) const {
  std::vector<int> result;
  if (idx < 0 || idx >= static_cast<int>(nodes_.size()) ||
      !nodes_[static_cast<size_t>(idx)].active_) {
    return result;
  }
  std::vector<int> stack = {idx};
  while (!stack.empty()) {
    const int current = stack.back();
    stack.pop_back();
    result.push_back(current);
    for (int c = 0; c < static_cast<int>(nodes_.size()); ++c) {
      if (nodes_[static_cast<size_t>(c)].parent_idx_ == current &&
          nodes_[static_cast<size_t>(c)].active_) {
        stack.push_back(c);
      }
    }
  }
  return result;
}

void MindMapCanvasView::SetNodeImage(int idx, std::string_view png_base64) {
  if (idx < 0 || idx >= static_cast<int>(nodes_.size())) {
    return;
  }
  auto& node = nodes_[static_cast<size_t>(idx)];
  ReleaseTexture(node.texture_id_);
  node.texture_id_ = 0;
  node.image_png_base64_ = png_base64;
  if (!png_base64.empty()) {
    node.texture_id_ = UploadPngTexture(mind_map::core::Base64Decode(node.image_png_base64_));
  }
}

const std::string& MindMapCanvasView::GetNodeImageBase64(int idx) const {
  static const std::string kEmpty;
  if (idx < 0 || idx >= static_cast<int>(nodes_.size())) {
    return kEmpty;
  }
  return nodes_[static_cast<size_t>(idx)].image_png_base64_;
}

int MindMapCanvasView::InsertChildNode(int parent_idx) {
  assert(parent_idx >= 0 && parent_idx < static_cast<int>(nodes_.size()));
  const CanvasNode& parent = nodes_[static_cast<size_t>(parent_idx)];

  int sibling_count = 0;
  for (const auto& node : nodes_) {
    if (node.active_ && node.parent_idx_ == parent_idx) {
      ++sibling_count;
    }
  }

  constexpr float kChildOffsetX = 260.0F;
  constexpr float kSiblingStepY = 120.0F;

  CanvasNode child;
  child.id_ = mind_map::core::mindmap::GenerateUuidV4();
  child.edge_id_ = mind_map::core::mindmap::GenerateUuidV4();
  child.label_ = "New node";
  child.parent_idx_ = parent_idx;
  child.branch_style_ = parent.parent_idx_ < 0 ? RepresentativeChildEdgeStyle() : parent.branch_style_;
  child.pos_world_ = {parent.pos_world_.x + kChildOffsetX,
                      parent.pos_world_.y + static_cast<float>(sibling_count) * kSiblingStepY};
  child.active_ = true;

  const auto new_idx = static_cast<int>(nodes_.size());
  initial_pos_world_.push_back(child.pos_world_);
  nodes_.push_back(std::move(child));
  selected_node_ = new_idx;
  selected_child_for_edge_style_ = new_idx;
  return new_idx;
}

}  // namespace mind_map::ui
