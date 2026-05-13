#include "ui/branch/DrawBranchTextOnPath.h"

#include "ui/branch/DrawBranchesOrganicTaper.h"
#include "ui/branch/SampleMindMapBranchAttachments.h"
#include "ui/canvas/CanvasMath.h"
#include "ui/canvas/NodeGeometry.h"

#include <cassert>
#include <cmath>
#include <cstddef>

namespace mind_map::ui::branch {

namespace {

constexpr float kNearZero = 1.0e-6F;
constexpr float kMinZoomForEdgeLabels = 0.5F;
constexpr int kCenterlineSamples = 8;

[[nodiscard]] ImVec2 SampleCubicBezier(ImVec2 p0, ImVec2 p1, ImVec2 p2, ImVec2 p3, float t) {
  const float u = 1.0F - t;
  const float a = u * u * u;
  const float b = 3.0F * u * u * t;
  const float c = 3.0F * u * t * t;
  const float d = t * t * t;
  return {a * p0.x + b * p1.x + c * p2.x + d * p3.x, a * p0.y + b * p1.y + c * p2.y + d * p3.y};
}

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
  BranchTextPathPolyline out{};
  if (child_index >= nodes.size()) { return out; }
  if (!nodes[child_index].parent_idx_.has_value()) { return out; }

  using mind_map::ui::branch::BranchStyle;
  switch (style) {  // NOSONAR(cpp:S6177)
    case BranchStyle::Bezier: {
      BranchEdgeData g{};
      FillBranchEdgeData(child_index, nodes, &g);
      const mind_map::canvas::BezierArmInputs arm_inputs = {
          g.pw_, g.parent_half_, g.cw_, g.child_half_,
          g.p0_attachment_, g.p3_attachment_, 96.0F, 0.55F, nullptr, nullptr};
      const mind_map::canvas::BezierArms arms = mind_map::canvas::ComputeBezierArmsWorld(arm_inputs);
      out.points_world_.reserve(static_cast<size_t>(kCenterlineSamples) + 1U);
      for (int i = 0; i <= kCenterlineSamples; ++i) {
        const float t = static_cast<float>(i) / static_cast<float>(kCenterlineSamples);
        out.points_world_.push_back(
            SampleCubicBezier(g.p0_attachment_, arms.p1_, arms.p2_, g.p3_attachment_, t));
      }
      return out;
    }
    case BranchStyle::Orthogonal: {
      BranchEdgeData g{};
      FillBranchEdgeData(child_index, nodes, &g);
      const ImVec2 p0w = g.p0_attachment_;
      const ImVec2 p3w = g.p3_attachment_;
      const float mid_x = (p0w.x + p3w.x) * 0.5F;
      out.points_world_ = {p0w, {mid_x, p0w.y}, {mid_x, p3w.y}, p3w};
      return out;
    }
    case BranchStyle::OrganicTaper:
      out.points_world_ = SampleOrganicTaperCenterlineWorld(child_index, nodes, kCenterlineSamples);
      return out;
    default:
      return out;
  }
}

void DrawMindMapBranchTextOnPath(const BranchRenderContext& ctx, const size_t child_index,
                                 const std::vector<mind_map::ui::CanvasNode>& nodes, const std::string_view label,
                                 const BranchTextRenderOptions& options) {
  assert(ctx.draw_list_ != nullptr);
  if (label.empty()) { return; }
  if (ctx.zoom_ < kMinZoomForEdgeLabels) { return; }
  if (child_index >= nodes.size()) { return; }
  const mind_map::ui::CanvasNode& child = nodes[child_index];
  if (!child.parent_idx_.has_value()) { return; }

  const BranchTextPathPolyline poly =
      BuildMindMapBranchTextPathWorld(child_index, nodes, child.branch_style_);
  if (poly.points_world_.empty()) { return; }

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
