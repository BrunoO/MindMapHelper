#include "ui/branch/DrawBranchTextOnPath.h"

#include "core/ByteOps.h"
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
constexpr std::size_t kCenterlineSamples = 8U;

[[nodiscard]] ImVec2 SampleCubicBezier(ImVec2 p0, ImVec2 p1, ImVec2 p2, ImVec2 p3, float t) {
  const float u = 1.0F - t;
  const float a = u * u * u;
  const float b = 3.0F * u * u * t;
  const float c = 3.0F * u * t * t;
  const float d = t * t * t;
  return {a * p0.x + b * p1.x + c * p2.x + d * p3.x, a * p0.y + b * p1.y + c * p2.y + d * p3.y};
}

[[nodiscard]] size_t AnchorIndex(const BranchTextPathPolyline& poly,
                                  const BranchTextAlongPathAnchor anchor) {
  const size_t n = poly.points_world_.size();
  if (n == 0U) { return 0U; }
  if (anchor == BranchTextAlongPathAnchor::Start) { return 0U; }
  if (anchor == BranchTextAlongPathAnchor::End)   { return n - 1U; }
  return n / 2U;
}

// Central difference at idx; forward/backward difference at the ends.
[[nodiscard]] ImVec2 LocalTangent(const BranchTextPathPolyline& poly, const size_t idx) {
  const size_t n = poly.points_world_.size();
  if (n < 2U) { return {1.0F, 0.0F}; }
  const ImVec2& prev = poly.points_world_[idx > 0U ? idx - 1U : 0U];
  const ImVec2& next = poly.points_world_[idx + 1U < n ? idx + 1U : n - 1U];
  return {next.x - prev.x, next.y - prev.y};
}

// UTF-8 byte-mask constants (RFC 3629)
constexpr unsigned int kUtf8ContMask  = 0x3FU;  // data bits in a continuation byte
constexpr unsigned int kUtf8Lead2Test = 0xE0U;  // test mask for 2-byte lead
constexpr unsigned int kUtf8Lead2Val  = 0xC0U;  // expected value of 2-byte lead
constexpr unsigned int kUtf8Lead3Test = 0xF0U;  // test mask for 3-byte lead
constexpr unsigned int kUtf8Lead3Val  = 0xE0U;  // expected value of 3-byte lead
constexpr unsigned int kUtf8ContShift = 6U;     // bits per continuation byte
constexpr unsigned int kUtf8Lead2Bits = 5U;     // data bits in a 2-byte lead
constexpr unsigned int kUtf8Lead3Bits = 4U;     // data bits in a 3-byte lead
constexpr unsigned int kUtf8Lead4Bits = 3U;     // data bits in a 4-byte lead
constexpr int kQuadIdxCount           = 6;      // 2 triangles × 3 indices per quad
constexpr int kQuadVtxCount           = 4;      // vertices per quad

// Returns number of bytes consumed and sets *out to the Unicode codepoint.
[[nodiscard]] int NextUtf8Codepoint(const char* s, unsigned int* out) {
  const unsigned int b = mind_map::core::ToUnsignedByte(*s);
  if (b < 0x80U) { *out = b; return 1; }
  if ((b & kUtf8Lead2Test) == kUtf8Lead2Val) {
    const unsigned int b1 = mind_map::core::ToUnsignedByte(s[1]);
    *out = ((b & ((1U << kUtf8Lead2Bits) - 1U)) << kUtf8ContShift) | (b1 & kUtf8ContMask);
    return 2;
  }
  if ((b & kUtf8Lead3Test) == kUtf8Lead3Val) {
    const unsigned int b1 = mind_map::core::ToUnsignedByte(s[1]);
    const unsigned int b2 = mind_map::core::ToUnsignedByte(s[2]);
    *out = ((b & ((1U << kUtf8Lead3Bits) - 1U)) << (kUtf8ContShift * 2U))
         | ((b1 & kUtf8ContMask) << kUtf8ContShift)
         | (b2 & kUtf8ContMask);
    return 3;
  }
  const unsigned int b1 = mind_map::core::ToUnsignedByte(s[1]);
  const unsigned int b2 = mind_map::core::ToUnsignedByte(s[2]);
  const unsigned int b3 = mind_map::core::ToUnsignedByte(s[3]);
  *out = ((b & ((1U << kUtf8Lead4Bits) - 1U)) << (kUtf8ContShift * 3U))
       | ((b1 & kUtf8ContMask) << (kUtf8ContShift * 2U))
       | ((b2 & kUtf8ContMask) << kUtf8ContShift)
       | (b3 & kUtf8ContMask);
  return 4;
}

// Renders text centered on `center` and rotated by `angle_rad` (screen-space, y-down).
void AddTextRotated(ImDrawList* draw_list, ImFont* font, float font_size,
                    ImVec2 center, ImU32 col, float angle_rad, std::string_view text) {
  if ((col & IM_COL32_A_MASK) == 0U) { return; }
  const char* text_begin = text.data();
  const char* text_end   = text.data() + text.size();
  const float scale  = font_size / font->FontSize;
  const float cos_a  = std::cos(angle_rad);
  const float sin_a  = std::sin(angle_rad);
  const ImVec2 tsize = font->CalcTextSizeA(font_size, FLT_MAX, 0.0F, text_begin, text_end);
  const float half_w = tsize.x * 0.5F;
  const float half_h = tsize.y * 0.5F;
  const auto rot = [cos_a, sin_a, center](float lx, float ly) -> ImVec2 {
    return {center.x + lx * cos_a - ly * sin_a, center.y + lx * sin_a + ly * cos_a};
  };
  draw_list->PushTextureID(font->ContainerAtlas->TexID);
  float cx = 0.0F;
  const char* s = text_begin;
  while (s < text_end) {
    unsigned int c = 0U;
    s += NextUtf8Codepoint(s, &c);
    if (c == 0U) { break; }
    if (c < 32U) { continue; }
    const ImFontGlyph* g = font->FindGlyph(static_cast<ImWchar>(c));
    if (g == nullptr) { continue; }
    if (g->Visible) {
      const float lx0 = (cx + g->X0) * scale - half_w;
      const float ly0 = g->Y0 * scale - half_h;
      const float lx1 = (cx + g->X1) * scale - half_w;
      const float ly1 = g->Y1 * scale - half_h;
      draw_list->PrimReserve(kQuadIdxCount, kQuadVtxCount);
      draw_list->PrimQuadUV(
          rot(lx0, ly0), rot(lx1, ly0), rot(lx1, ly1), rot(lx0, ly1),
          {g->U0, g->V0}, {g->U1, g->V0}, {g->U1, g->V1}, {g->U0, g->V1}, col);
    }
    cx += g->AdvanceX;
  }
  draw_list->PopTextureID();
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
      out.points_world_.reserve(kCenterlineSamples + 1U);
      for (std::size_t i = 0; i <= kCenterlineSamples; ++i) {
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

  const size_t anchor_idx = AnchorIndex(poly, options.anchor_along_path_);
  const ImVec2 anchor_world = poly.points_world_[anchor_idx];
  const ImVec2 raw_tangent = LocalTangent(poly, anchor_idx);
  float dx = raw_tangent.x;
  float dy = raw_tangent.y;
  if (const float len = std::sqrt(dx * dx + dy * dy); len > kNearZero) {
    dx /= len;
    dy /= len;
  }
  else {
    dx = 1.0F;
    dy = 0.0F;
  }
  if (dx < -kNearZero) { dx = -dx; dy = -dy; }  // keep text readable left-to-right
  const float angle = std::atan2(dy, dx);
  const ImVec2 text_pos =
      mind_map::canvas::WorldToScreen(anchor_world, ctx.canvas_p0_, ctx.pan_px_, ctx.zoom_);
  const ImVec2 n_screen = {-dy, dx};
  const ImVec2 text_center = {text_pos.x + n_screen.x * options.normal_offset_px_,
                               text_pos.y + n_screen.y * options.normal_offset_px_};
  AddTextRotated(ctx.draw_list_, font, font_size, text_center, options.color_, angle, label);
}

}  // namespace mind_map::ui::branch
