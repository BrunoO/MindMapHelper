#include "ui/branch/DrawBranchTextOnPath.h"

#include "ui/branch/SampleMindMapBranchAttachments.h"
#include "ui/canvas/CanvasMath.h"

#include <cassert>
#include <cmath>
#include <cstddef>

namespace mind_map::ui::branch {

namespace {

constexpr float kNearZero = 1.0e-6F;

[[nodiscard]] ImVec2 MidpointOrFallback(const BranchTextPathPolyline& poly) {
  if (poly.points_world_.empty()) {
    return {0.0F, 0.0F};
  }
  if (poly.points_world_.size() == 1U) {
    return poly.points_world_.front();
  }
  const ImVec2& a = poly.points_world_.front();
  const ImVec2& b = poly.points_world_.back();
  return {(a.x + b.x) * 0.5F, (a.y + b.y) * 0.5F};
}

}  // namespace

BranchTextPathPolyline BuildMindMapBranchTextPathWorld(const size_t child_index,
                                                       const std::vector<mind_map::ui::CanvasNode>& nodes,
                                                       const BranchStyle style) {
  (void)style;  // TODO(BrunoO): sample Bezier / orthogonal / organic centerlines per BranchStyle
  BranchTextPathPolyline out{};
  if (child_index >= nodes.size()) {
    return out;
  }
  const mind_map::ui::CanvasNode& child = nodes[child_index];
  if (!child.parent_idx_.has_value()) {
    return out;
  }

  BranchEdgeData g{};
  FillBranchEdgeData(child_index, nodes, &g);
  out.points_world_.reserve(2);
  out.points_world_.push_back(g.p0_attachment_);
  out.points_world_.push_back(g.p3_attachment_);
  return out;
}

void DrawMindMapBranchTextOnPath(const BranchRenderContext& ctx, const size_t child_index,
                                 const std::vector<mind_map::ui::CanvasNode>& nodes, const std::string_view label,
                                 const BranchTextRenderOptions& options) {
  assert(ctx.draw_list_ != nullptr);
  if (label.empty()) {
    return;
  }
  if (child_index >= nodes.size()) {
    return;
  }
  const mind_map::ui::CanvasNode& child = nodes[child_index];
  if (!child.parent_idx_.has_value()) {
    return;
  }

  const BranchTextPathPolyline poly =
      BuildMindMapBranchTextPathWorld(child_index, nodes, child.branch_style_);
  if (poly.points_world_.empty()) {
    return;
  }

  ImFont* font = options.font_;
  if (font == nullptr) {
    font = ImGui::GetFont();
  }
  float font_size = options.font_size_;
  if (font_size <= 0.0F) {
    font_size = ImGui::GetFontSize();
  }

  ImVec2 anchor_world = MidpointOrFallback(poly);
  if (options.anchor_along_path_ == BranchTextAlongPathAnchor::Start) {
    anchor_world = poly.points_world_.front();
  }
  else if (options.anchor_along_path_ == BranchTextAlongPathAnchor::End) {
    anchor_world = poly.points_world_.back();
  }

  const ImVec2 p0w = poly.points_world_.front();
  const ImVec2 p1w = poly.points_world_.size() > 1U ? poly.points_world_.back() : poly.points_world_.front();
  float dx = p1w.x - p0w.x;
  float dy = p1w.y - p0w.y;
  if (const float len = std::sqrt(dx * dx + dy * dy); len > kNearZero) {
    dx /= len;
    dy /= len;
  }
  else {
    dx = 1.0F;
    dy = 0.0F;
  }
  const ImVec2 text_pos =
      mind_map::canvas::WorldToScreen(anchor_world, ctx.canvas_p0_, ctx.pan_px_, ctx.zoom_);
  float sx = dx * ctx.zoom_;
  float sy = dy * ctx.zoom_;
  if (const float slen = std::sqrt(sx * sx + sy * sy); slen > kNearZero) {
    sx /= slen;
    sy /= slen;
  }
  else {
    sx = 1.0F;
    sy = 0.0F;
  }
  const ImVec2 n_screen = {-sy, sx};
  const ImVec2 text_pos_offset = {text_pos.x + n_screen.x * options.normal_offset_px_,
                                  text_pos.y + n_screen.y * options.normal_offset_px_};
  const char* const text_begin = label.data();
  const char* const text_end = text_begin + static_cast<std::ptrdiff_t>(label.size());
  const ImVec2 text_size = font->CalcTextSizeA(font_size, FLT_MAX, 0.0F, text_begin, text_end);
  const ImVec2 adjusted = {text_pos_offset.x - text_size.x * 0.5F, text_pos_offset.y - text_size.y * 0.5F};

  ctx.draw_list_->AddText(font, font_size, adjusted, options.color_, text_begin, text_end);
  (void)options.max_glyph_count_;
}

}  // namespace mind_map::ui::branch
