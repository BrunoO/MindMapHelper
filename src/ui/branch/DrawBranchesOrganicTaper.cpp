#include "ui/branch/DrawBranchesOrganicTaper.h"

#include "ui/canvas/CanvasMath.h"

#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <cstddef>

namespace mind_map::ui::branch {

namespace {

constexpr int kBranchStripSegments = 32;
constexpr float kTaperPower = 1.65F;
constexpr float kMinParentHalfWidthWorld = 9.0F;
constexpr float kMaxParentHalfWidthScale = 0.42F;
constexpr float kMinChildHalfWidthWorld = 2.0F;
constexpr float kMaxChildHalfWidthScale = 0.14F;

constexpr float kHobbyMidWaypointTension = 0.8F;
constexpr float kHobbyMidJointVelocityScale = 1.55F;
constexpr float kHobbyMidChordPerpOffsetFraction = 0.1F;

constexpr ImU32 kColorBranchFill = IM_COL32(175, 140, 105, 255);
constexpr ImU32 kColorBranchOutline = IM_COL32(95, 72, 52, 220);

constexpr float kTaperP0BlendCenterToBorder = 0.22F;
constexpr float kTaperP3BlendCenterToBorder = 0.22F;

[[nodiscard]] ImVec2 CubicBezierPoint(const ImVec2& p0, const ImVec2& p1, const ImVec2& p2, const ImVec2& p3,
                                      float t) {
  const float u = 1.0F - t;
  const float uu = u * u;
  const float tt = t * t;
  const float a = uu * u;
  const float b = 3.0F * uu * t;
  const float c = 3.0F * u * tt;
  const float d = t * tt;
  return {a * p0.x + b * p1.x + c * p2.x + d * p3.x, a * p0.y + b * p1.y + c * p2.y + d * p3.y};
}

[[nodiscard]] ImVec2 CubicBezierTangent(const ImVec2& p0, const ImVec2& p1, const ImVec2& p2, const ImVec2& p3,
                                        float t) {
  const float u = 1.0F - t;
  const float f0 = 3.0F * u * u;
  const float f1 = 6.0F * u * t;
  const float f2 = 3.0F * t * t;
  return {f0 * (p1.x - p0.x) + f1 * (p2.x - p1.x) + f2 * (p3.x - p2.x),
          f0 * (p1.y - p0.y) + f1 * (p2.y - p1.y) + f2 * (p3.y - p2.y)};
}

void NormalizeOrDefault(ImVec2* v) {
  assert(v != nullptr);
  const float len_sq = v->x * v->x + v->y * v->y;
  if (len_sq < 1.0e-8F) {
    *v = {1.0F, 0.0F};
    return;
  }
  const float inv = 1.0F / std::sqrt(len_sq);
  v->x *= inv;
  v->y *= inv;
}

[[nodiscard]] ImVec2 PerpLeft(const ImVec2& tangent) {
  return {-tangent.y, tangent.x};
}

[[nodiscard]] float BranchEndHalfWidthWorld(float child_node_radius_world) {
  return (std::max)(kMinChildHalfWidthWorld, child_node_radius_world * kMaxChildHalfWidthScale);
}

[[nodiscard]] float BranchRootStartHalfWidthWorld(float root_node_radius_world) {
  return (std::max)(kMinParentHalfWidthWorld, root_node_radius_world * kMaxParentHalfWidthScale);
}

[[nodiscard]] float BranchHalfWidthAlongEdge(float t, float half_width_start, float half_width_end) {
  const float narrow = std::pow(t, kTaperPower);
  return half_width_start + (half_width_end - half_width_start) * narrow;
}

void DrawTaperBezierBranch(ImDrawList* draw_list, ImVec2 canvas_p0, ImVec2 pan_px, float zoom, const ImVec2& p0w,
                           const ImVec2& p1w, const ImVec2& p2w, const ImVec2& p3w, float half_width_start,
                           float half_width_end) {
  assert(draw_list != nullptr);
  std::array<ImVec2, static_cast<size_t>(kBranchStripSegments + 1)> left_s{};
  std::array<ImVec2, static_cast<size_t>(kBranchStripSegments + 1)> right_s{};

  for (int i = 0; i <= kBranchStripSegments; ++i) {
    const float t = static_cast<float>(i) / static_cast<float>(kBranchStripSegments);
    const ImVec2 pw = CubicBezierPoint(p0w, p1w, p2w, p3w, t);
    ImVec2 tan_w = CubicBezierTangent(p0w, p1w, p2w, p3w, t);
    NormalizeOrDefault(&tan_w);
    ImVec2 n_w = PerpLeft(tan_w);
    NormalizeOrDefault(&n_w);
    const float half_w = BranchHalfWidthAlongEdge(t, half_width_start, half_width_end);
    const ImVec2 lw = {pw.x + n_w.x * half_w, pw.y + n_w.y * half_w};
    const ImVec2 rw = {pw.x - n_w.x * half_w, pw.y - n_w.y * half_w};
    left_s[static_cast<size_t>(i)] = mind_map::canvas::WorldToScreen(lw, canvas_p0, pan_px, zoom);
    right_s[static_cast<size_t>(i)] = mind_map::canvas::WorldToScreen(rw, canvas_p0, pan_px, zoom);
  }

  for (int i = 0; i < kBranchStripSegments; ++i) {
    const ImVec2& l0 = left_s[static_cast<size_t>(i)];
    const ImVec2& r0 = right_s[static_cast<size_t>(i)];
    const ImVec2& l1 = left_s[static_cast<size_t>(i + 1)];
    const ImVec2& r1 = right_s[static_cast<size_t>(i + 1)];
    draw_list->AddTriangleFilled(l0, r0, l1, kColorBranchFill);
    draw_list->AddTriangleFilled(r0, r1, l1, kColorBranchFill);
  }

  draw_list->AddPolyline(left_s.data(), kBranchStripSegments + 1, kColorBranchOutline, 0, 1.25F);
  draw_list->AddPolyline(right_s.data(), kBranchStripSegments + 1, kColorBranchOutline, 0, 1.25F);
}

[[nodiscard]] bool BuildHobbyMidWaypointTwoCubics(ImVec2 p0w, ImVec2 p3w, ImVec2 out0_unit, ImVec2 out3_unit,
                                                  float tension, float mid_joint_scale, float chord_perp_offset_fraction,
                                                  ImVec2* seg0, ImVec2* seg1) {
  assert(seg0 != nullptr);
  assert(seg1 != nullptr);
  constexpr float kMinChord = 1.0e-4F;
  const float chord_dx = p3w.x - p0w.x;
  const float chord_dy = p3w.y - p0w.y;
  const float chord_len = std::sqrt(chord_dx * chord_dx + chord_dy * chord_dy);
  if (chord_len < kMinChord) {
    return false;
  }
  const ImVec2 chord_hat = {chord_dx / chord_len, chord_dy / chord_len};
  ImVec2 perp = PerpLeft(chord_hat);
  NormalizeOrDefault(&perp);
  const ImVec2 mid_linear = {(p0w.x + p3w.x) * 0.5F, (p0w.y + p3w.y) * 0.5F};
  const float perp_shift = chord_perp_offset_fraction * chord_len;
  const ImVec2 mid = {mid_linear.x + perp.x * perp_shift, mid_linear.y + perp.y * perp_shift};

  const float ax = mid.x - p0w.x;
  const float ay = mid.y - p0w.y;
  const float bx = p3w.x - mid.x;
  const float by = p3w.y - mid.y;
  const float a = std::sqrt(ax * ax + ay * ay);
  const float b = std::sqrt(bx * bx + by * by);
  if (a < kMinChord || b < kMinChord) {
    return false;
  }
  const ImVec2 u0 = {ax / a, ay / a};
  const ImVec2 u1 = {bx / b, by / b};
  ImVec2 m_dir = {u0.x + u1.x, u0.y + u1.y};
  const float m_len_sq = m_dir.x * m_dir.x + m_dir.y * m_dir.y;
  if (m_len_sq < 1.0e-10F) {
    m_dir = PerpLeft(u0);
  }
  NormalizeOrDefault(&m_dir);

  const float s0 = tension * a;
  const float s2 = tension * b;
  const float vm = tension * mid_joint_scale * (std::min)(a, b);
  const ImVec2 v0 = {out0_unit.x * s0, out0_unit.y * s0};
  const ImVec2 v_end = {-out3_unit.x * s2, -out3_unit.y * s2};
  const ImVec2 vm_vec = {m_dir.x * vm, m_dir.y * vm};

  seg0[0] = p0w;
  seg0[1] = {p0w.x + v0.x / 3.0F, p0w.y + v0.y / 3.0F};
  seg0[2] = {mid.x - vm_vec.x / 3.0F, mid.y - vm_vec.y / 3.0F};
  seg0[3] = mid;

  seg1[0] = mid;
  seg1[1] = {mid.x + vm_vec.x / 3.0F, mid.y + vm_vec.y / 3.0F};
  seg1[2] = {p3w.x - v_end.x / 3.0F, p3w.y - v_end.y / 3.0F};
  seg1[3] = p3w;
  return true;
}

void DrawTaperTwoSegmentBezierBranch(ImDrawList* draw_list, ImVec2 canvas_p0, ImVec2 pan_px, float zoom,
                                    const ImVec2* seg0, const ImVec2* seg1, float half_width_start,
                                    float half_width_end) {
  assert(draw_list != nullptr);
  assert(seg0 != nullptr);
  assert(seg1 != nullptr);
  std::array<ImVec2, static_cast<size_t>(kBranchStripSegments + 1)> left_s{};
  std::array<ImVec2, static_cast<size_t>(kBranchStripSegments + 1)> right_s{};

  for (int i = 0; i <= kBranchStripSegments; ++i) {
    const float u = static_cast<float>(i) / static_cast<float>(kBranchStripSegments);
    float t_curve = 0.0F;
    const ImVec2* c = nullptr;
    if (u <= 0.5F) {
      t_curve = u * 2.0F;
      c = seg0;
    }
    else {
      t_curve = (u - 0.5F) * 2.0F;
      c = seg1;
    }
    const ImVec2 pw = CubicBezierPoint(c[0], c[1], c[2], c[3], t_curve);
    ImVec2 tan_w = CubicBezierTangent(c[0], c[1], c[2], c[3], t_curve);
    NormalizeOrDefault(&tan_w);
    ImVec2 n_w = PerpLeft(tan_w);
    NormalizeOrDefault(&n_w);
    const float half_w = BranchHalfWidthAlongEdge(u, half_width_start, half_width_end);
    const ImVec2 lw = {pw.x + n_w.x * half_w, pw.y + n_w.y * half_w};
    const ImVec2 rw = {pw.x - n_w.x * half_w, pw.y - n_w.y * half_w};
    left_s[static_cast<size_t>(i)] = mind_map::canvas::WorldToScreen(lw, canvas_p0, pan_px, zoom);
    right_s[static_cast<size_t>(i)] = mind_map::canvas::WorldToScreen(rw, canvas_p0, pan_px, zoom);
  }

  for (int i = 0; i < kBranchStripSegments; ++i) {
    const ImVec2& l0 = left_s[static_cast<size_t>(i)];
    const ImVec2& r0 = right_s[static_cast<size_t>(i)];
    const ImVec2& l1 = left_s[static_cast<size_t>(i + 1)];
    const ImVec2& r1 = right_s[static_cast<size_t>(i + 1)];
    draw_list->AddTriangleFilled(l0, r0, l1, kColorBranchFill);
    draw_list->AddTriangleFilled(r0, r1, l1, kColorBranchFill);
  }

  draw_list->AddPolyline(left_s.data(), kBranchStripSegments + 1, kColorBranchOutline, 0, 1.25F);
  draw_list->AddPolyline(right_s.data(), kBranchStripSegments + 1, kColorBranchOutline, 0, 1.25F);
}

}  // namespace

void DrawAllSampleMindMapBranchesOrganicTaper(
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
    const ImVec2 parent_half = mind_map::demos::SampleMapHalfExtentForLabel(parent_label);
    const ImVec2 child_half = mind_map::demos::SampleMapHalfExtentForLabel(child_label);

    const ImVec2 pw = pos_world[static_cast<size_t>(parent)];
    const ImVec2 cw = pos_world[static_cast<size_t>(child)];
    const ImVec2 p0_border = mind_map::demos::SampleMapRoundedRectAttachmentPreferEdgeMid(
        pw, parent_half, mind_map::demos::kSampleMindMapNodeCornerRadiusWorld, cw);
    const ImVec2 p3_border = mind_map::demos::SampleMapRoundedRectAttachmentPreferEdgeMid(
        cw, child_half, mind_map::demos::kSampleMindMapNodeCornerRadiusWorld, pw);

    const float b0 = (std::clamp)(kTaperP0BlendCenterToBorder, 0.0F, 1.0F);
    const float b3 = (std::clamp)(kTaperP3BlendCenterToBorder, 0.0F, 1.0F);
    const ImVec2 p0w = {pw.x + (p0_border.x - pw.x) * b0, pw.y + (p0_border.y - pw.y) * b0};
    const ImVec2 p3w = {cw.x + (p3_border.x - cw.x) * b3, cw.y + (p3_border.y - cw.y) * b3};

    const int grandparent = mind_map::demos::kSampleMindMapSpecs[static_cast<size_t>(parent)].parent_;
    const float parent_radius = mind_map::demos::SampleMapNodeRadiusWorld(parent_label);
    const float child_radius = mind_map::demos::SampleMapNodeRadiusWorld(child_label);
    const float half_width_start = (grandparent < 0) ? BranchRootStartHalfWidthWorld(parent_radius)
                                                     : BranchEndHalfWidthWorld(parent_radius);
    const float half_width_end_raw = BranchEndHalfWidthWorld(child_radius);
    const float half_width_end = (std::min)(half_width_end_raw, half_width_start);

    const ImVec2 out0 = mind_map::demos::SampleMapEdgeOutwardAxis(pw, parent_half, p0_border);
    const ImVec2 out3 = mind_map::demos::SampleMapEdgeOutwardAxis(cw, child_half, p3_border);
    std::array<ImVec2, 4> seg0{};
    std::array<ImVec2, 4> seg1{};
    if (BuildHobbyMidWaypointTwoCubics(p0w, p3w, out0, out3, kHobbyMidWaypointTension, kHobbyMidJointVelocityScale,
                                       kHobbyMidChordPerpOffsetFraction, seg0.data(), seg1.data())) {
      DrawTaperTwoSegmentBezierBranch(ctx.draw_list, ctx.canvas_p0, ctx.pan_px, ctx.zoom, seg0.data(), seg1.data(),
                                      half_width_start, half_width_end);
    }
    else {
      const mind_map::demos::SampleMapBezierArms arms = mind_map::demos::ComputeSampleMapBezierArmsWorld(
          pw, parent_half, cw, child_half, p0w, p3w, 96.0F, 0.55F, &p0_border, &p3_border);
      DrawTaperBezierBranch(ctx.draw_list, ctx.canvas_p0, ctx.pan_px, ctx.zoom, p0w, arms.p1, arms.p2, p3w,
                            half_width_start, half_width_end);
    }
  }
}

}  // namespace mind_map::ui::branch
