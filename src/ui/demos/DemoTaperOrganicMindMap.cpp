#include "demos/DemoTaperOrganicMindMap.h"

#include "imgui.h"

#include "ui/canvas/CanvasMath.h"

#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <cstddef>

namespace mind_map::demos {

namespace {

constexpr int kBranchStripSegments = 32;
constexpr float kTaperPower = 1.65F;
constexpr float kMinParentHalfWidthWorld = 9.0F;
constexpr float kMaxParentHalfWidthScale = 0.42F;
constexpr float kMinChildHalfWidthWorld = 2.0F;
constexpr float kMaxChildHalfWidthScale = 0.14F;

// Hobby mid-waypoint spine (BuildHobbyMidWaypointTwoCubics):
// - kHobbyMidWaypointTension — scales Hermite speeds vs chord length at both ends and (partially) overall bend;
//   lower → straighter; higher → curvier (can feel “tight” if pushed too far).
// - kHobbyMidJointVelocityScale — multiplier for tangent magnitude at the middle waypoint only; raises/lowers the
//   “knee” without changing endpoint port emphasis as much as tension alone.
// - kHobbyMidChordPerpOffsetFraction — midpoint = chord midpoint + fraction * chord_length * PerpLeft(unit parent→child);
//   0 = on the chord; sign picks which side; typical |fraction| ~ 0.06–0.14.
constexpr float kHobbyMidWaypointTension = 0.8F;
constexpr float kHobbyMidJointVelocityScale = 1.55F;
constexpr float kHobbyMidChordPerpOffsetFraction = 0.1F;

constexpr ImU32 kColorBranchFill = IM_COL32(175, 140, 105, 255);
constexpr ImU32 kColorBranchOutline = IM_COL32(95, 72, 52, 220);
constexpr ImU32 kColorNodeFill = IM_COL32(52, 48, 44, 255);
constexpr ImU32 kColorNodeBorder = IM_COL32(130, 110, 90, 255);
constexpr ImU32 kColorNodeBorderHot = IM_COL32(230, 200, 160, 255);
constexpr float kNodeCornerRadius = kSampleMindMapNodeCornerRadiusWorld;
constexpr float kNodeBorderThickness = 1.5F;

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

// Half-width at t=1 along an edge (matches where the branch meets the child node).
[[nodiscard]] float BranchEndHalfWidthWorld(float child_node_radius_world) {
  return (std::max)(kMinChildHalfWidthWorld, child_node_radius_world * kMaxChildHalfWidthScale);
}

// Half-width at t=0 for edges leaving the root (no incoming branch).
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

// Automatic middle waypoint + Hobby-style interior tangent (bisector of chord directions, chord-length scalings).
// Endpoint Hermite directions match rounded-rect ports: departure along parent outward normal, arrival along -child
// outward normal. Emits two C1 cubics: p0w→mid and mid→p3w. Midpoint starts at chord center, then shifts perpendicular
// to the parent→child chord when chord_perp_offset_fraction != 0.
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
    const ImVec2 pw =
        CubicBezierPoint(c[0], c[1], c[2], c[3], t_curve);
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

// Branch geometry (taper demo only): p0 / p3 inside nodes + draw order
// -----------------------------------------------------------------------
// Problem: cubics pinned exactly on rounded borders can look slightly "detached" when nodes move (AA, strip width,
// corner vs straight segment).
//
// Approach:
// 1) p0_border / p3_border = logical ports on parent/child outlines (prefer-edge-mid + rounded hit).
// 2) p0_curve = lerp(parent_center, p0_border, kTaperP0BlendCenterToBorder); same for child with kTaperP3....
//    Strokes start/end *inside* the boxes; filled nodes are drawn after all branches (Render), so each node hides
//    the stub on its side.
// 3) Branch spine: waypoint at chord midpoint of p0w–p3w, offset perpendicular to that chord (tunable fraction);
//    Hobby-style interior tangent (chord bisector + chord length scaling) with Hermite ends along parent out0 and
//    -child out3. Two C1 cubics; fallback to single cubic (ComputeSampleMapBezierArmsWorld) if chords degenerate.
//
// Blend factor (0 = node center, 1 = border) for each end:
// - 0.0 — maximum occlusion; curve endpoint at center (odd if arms are large vs small nodes).
// - 0.15–0.30 — default band; short hidden segment, direction still clear after a few pixels.
// - 0.5 — halfway; avoid with semi-transparent fills.
// - 1.0 — flush border (no occlusion trick).

constexpr float kTaperP0BlendCenterToBorder = 0.22F;
constexpr float kTaperP3BlendCenterToBorder = 0.22F;

void DrawOrganicEdges(ImDrawList* draw_list, ImVec2 canvas_p0,
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
    const ImVec2 parent_half = SampleMapHalfExtentForLabel(parent_label);
    const ImVec2 child_half = SampleMapHalfExtentForLabel(child_label);

    const ImVec2 pw = pos_world[static_cast<size_t>(parent)];
    const ImVec2 cw = pos_world[static_cast<size_t>(child)];
    const ImVec2 p0_border =
        SampleMapRoundedRectAttachmentPreferEdgeMid(pw, parent_half, kSampleMindMapNodeCornerRadiusWorld, cw);
    const ImVec2 p3_border =
        SampleMapRoundedRectAttachmentPreferEdgeMid(cw, child_half, kSampleMindMapNodeCornerRadiusWorld, pw);

    const float b0 = (std::clamp)(kTaperP0BlendCenterToBorder, 0.0F, 1.0F);
    const float b3 = (std::clamp)(kTaperP3BlendCenterToBorder, 0.0F, 1.0F);
    const ImVec2 p0w = {pw.x + (p0_border.x - pw.x) * b0, pw.y + (p0_border.y - pw.y) * b0};
    const ImVec2 p3w = {cw.x + (p3_border.x - cw.x) * b3, cw.y + (p3_border.y - cw.y) * b3};

    const int grandparent = kSampleMindMapSpecs[static_cast<size_t>(parent)].parent_;
    const float parent_radius = SampleMapNodeRadiusWorld(parent_label);
    const float child_radius = SampleMapNodeRadiusWorld(child_label);
    const float half_width_start = (grandparent < 0) ? BranchRootStartHalfWidthWorld(parent_radius)
                                                     : BranchEndHalfWidthWorld(parent_radius);
    const float half_width_end_raw = BranchEndHalfWidthWorld(child_radius);
    const float half_width_end = (std::min)(half_width_end_raw, half_width_start);

    const ImVec2 out0 = SampleMapEdgeOutwardAxis(pw, parent_half, p0_border);
    const ImVec2 out3 = SampleMapEdgeOutwardAxis(cw, child_half, p3_border);
    std::array<ImVec2, 4> seg0{};
    std::array<ImVec2, 4> seg1{};
    if (BuildHobbyMidWaypointTwoCubics(p0w, p3w, out0, out3, kHobbyMidWaypointTension, kHobbyMidJointVelocityScale,
                                       kHobbyMidChordPerpOffsetFraction, seg0.data(), seg1.data())) {
      DrawTaperTwoSegmentBezierBranch(draw_list, canvas_p0, pan_px, zoom, seg0.data(), seg1.data(), half_width_start,
                                      half_width_end);
    }
    else {
      const SampleMapBezierArms arms = ComputeSampleMapBezierArmsWorld(pw, parent_half, cw, child_half, p0w, p3w,
                                                                       96.0F, 0.55F, &p0_border, &p3_border);
      DrawTaperBezierBranch(draw_list, canvas_p0, pan_px, zoom, p0w, arms.p1, arms.p2, p3w, half_width_start,
                            half_width_end);
    }
  }
}

}  // namespace

const char* DemoTaperOrganicMindMap::GetName() const {
  return "Organic taper (rounded rects)";
}

void DemoTaperOrganicMindMap::Reset() {
  pos_world_ = InitialSampleMapPositions();
  dragging_node_ = -1;
}

void DemoTaperOrganicMindMap::OnPrimaryDown(const DemoPointerState& ptr) {
  if (!ptr.canvas_hovered) {
    return;
  }
  const int hit = HitTestSampleMap(ptr.mouse_world, pos_world_);
  if (hit >= 0) {
    dragging_node_ = hit;
    const ImVec2 c = pos_world_[static_cast<size_t>(hit)];
    grab_offset_world_ = {ptr.mouse_world.x - c.x, ptr.mouse_world.y - c.y};
  }
  else {
    dragging_node_ = -1;
  }
}

void DemoTaperOrganicMindMap::OnPrimaryDrag(const DemoPointerState& ptr) {
  if (dragging_node_ < 0) {
    return;
  }
  pos_world_[static_cast<size_t>(dragging_node_)] = {ptr.mouse_world.x - grab_offset_world_.x,
                                                     ptr.mouse_world.y - grab_offset_world_.y};
}

void DemoTaperOrganicMindMap::OnPrimaryUp() {
  dragging_node_ = -1;
}

bool DemoTaperOrganicMindMap::IsDraggingContent() const {
  return dragging_node_ >= 0;
}

void DemoTaperOrganicMindMap::Render(const DemoRenderContext& ctx) {
  assert(ctx.draw_list != nullptr);
  mind_map::canvas::DrawGrid(ctx.draw_list, ctx.canvas_p0, ctx.canvas_p1, ctx.pan_px, ctx.zoom);
  // Branches first, then filled nodes — occludes interior stubs at parent (p0) and child (p3).
  DrawOrganicEdges(ctx.draw_list, ctx.canvas_p0, pos_world_, ctx.pan_px, ctx.zoom);

  const int hot_node = ctx.canvas_hovered ? HitTestSampleMap(ctx.mouse_world, pos_world_) : -1;

  for (int i = 0; i < kSampleMindMapNodeCount; ++i) {
    const char* const label = kSampleMindMapSpecs[static_cast<size_t>(i)].label_;
    const ImVec2 half = SampleMapHalfExtentForLabel(label);
    const ImVec2 c = pos_world_[static_cast<size_t>(i)];
    const ImVec2 rmin_w = {c.x - half.x, c.y - half.y};
    const ImVec2 rmax_w = {c.x + half.x, c.y + half.y};
    const ImVec2 rmin = mind_map::canvas::WorldToScreen(rmin_w, ctx.canvas_p0, ctx.pan_px, ctx.zoom);
    const ImVec2 rmax = mind_map::canvas::WorldToScreen(rmax_w, ctx.canvas_p0, ctx.pan_px, ctx.zoom);

    const bool hot = (i == dragging_node_) || (i == hot_node);
    const ImU32 border = hot ? kColorNodeBorderHot : kColorNodeBorder;
    ctx.draw_list->AddRectFilled(rmin, rmax, kColorNodeFill, kNodeCornerRadius);
    ctx.draw_list->AddRect(rmin, rmax, border, kNodeCornerRadius, 0, kNodeBorderThickness);

    const ImVec2 text_sz = ImGui::CalcTextSize(label);
    const ImVec2 text_pos = {(rmin.x + rmax.x - text_sz.x) * 0.5F, (rmin.y + rmax.y - text_sz.y) * 0.5F};
    ctx.draw_list->AddText(text_pos, IM_COL32(240, 230, 215, 255), label);
  }
}

}  // namespace mind_map::demos
