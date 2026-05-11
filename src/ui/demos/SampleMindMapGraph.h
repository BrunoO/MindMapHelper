#pragma once

#include "ui/canvas/CanvasMath.h"

#include "imgui.h"

#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <cstddef>
#include <cstdint>

namespace mind_map::demos {

inline constexpr int kSampleMindMapNodeCount = 7;
inline constexpr float kSampleMindMapNodePad = 10.0F;
// Node model C1 (unified canvas): label-sized axis-aligned rounded rects in world space; all branch styles attach
// via SampleMapRoundedRectAttachment* and hit-test via HitTestSampleMap.
inline constexpr float kSampleMindMapNodeCornerRadiusWorld = 6.0F;
inline constexpr float kCornerRadiusClampFactor = 0.98F;
inline constexpr float kDefaultHorizontalStickiness = 1.12F;
inline constexpr float kDefaultBlendFromNormalized = 0.22F;
inline constexpr float kDefaultBlendToNormalized = 0.62F;

struct SampleMindMapNodeSpec {
  const char* label_;
  int parent_;
};

// Shared toy graph: root at origin, children to the right.
inline constexpr std::array<SampleMindMapNodeSpec, kSampleMindMapNodeCount> kSampleMindMapSpecs = {{
    {"Root", -1},
    {"Idea A", 0},
    {"Idea B", 0},
    {"Idea C", 0},
    {"Detail A1", 1},
    {"Detail A2", 1},
    {"Detail B1", 2},
}};

[[nodiscard]] inline ImVec2 SampleMapHalfExtentForLabel(const char* label) {
  assert(label != nullptr);
  const ImVec2 text_sz = ImGui::CalcTextSize(label);
  return {text_sz.x * 0.5F + kSampleMindMapNodePad, text_sz.y * 0.5F + kSampleMindMapNodePad};
}

[[nodiscard]] inline int HitTestSampleMap(ImVec2 world_pos, const std::array<ImVec2, kSampleMindMapNodeCount>& pos_world) {
  for (int i = kSampleMindMapNodeCount - 1; i >= 0; --i) {
    const char* const label = kSampleMindMapSpecs[static_cast<size_t>(i)].label_;
    const ImVec2 half = SampleMapHalfExtentForLabel(label);
    const ImVec2 c = pos_world[static_cast<size_t>(i)];
    const ImVec2 rmin = {c.x - half.x, c.y - half.y};
    const ImVec2 rmax = {c.x + half.x, c.y + half.y};
    if (mind_map::canvas::IsInsideRect(world_pos, rmin, rmax)) {
      return i;
    }
  }
  return -1;
}

// Max half-extent of the label’s rounded rect (world space). Used for branch-width scalings (e.g. organic taper), not
// for circular node geometry — C1 hit-test and outlines use the full rect + HitTestSampleMap.
[[nodiscard]] inline float SampleMapNodeRadiusWorld(const char* label) {
  assert(label != nullptr);
  const ImVec2 half = SampleMapHalfExtentForLabel(label);
  return (std::max)(half.x, half.y);
}

// Axis-aligned box boundary point from from_center toward toward_point (half = half-extents).
[[nodiscard]] inline ImVec2 SampleMapAttachmentToward(ImVec2 from_center, ImVec2 half_extents, ImVec2 toward_point) {
  constexpr float kEpsLenSq = 1.0e-12F;
  float vx = toward_point.x - from_center.x;
  float vy = toward_point.y - from_center.y;
  const float len_sq = vx * vx + vy * vy;
  if (len_sq < kEpsLenSq) {
    return {from_center.x + half_extents.x, from_center.y};
  }
  const float inv_len = 1.0F / std::sqrt(len_sq);
  vx *= inv_len;
  vy *= inv_len;
  constexpr float kEps = 1.0e-6F;
  constexpr float kHuge = 1.0e15F;
  float tx = kHuge;
  float ty = kHuge;
  if (vx > kEps) {
    tx = half_extents.x / vx;
  }
  else if (vx < -kEps) {
    tx = half_extents.x / (-vx);
  }
  if (vy > kEps) {
    ty = half_extents.y / vy;
  }
  else if (vy < -kEps) {
    ty = half_extents.y / (-vy);
  }
  const float t = (std::min)(tx, ty);
  return {from_center.x + vx * t, from_center.y + vy * t};
}

struct RoundedRectRay {
  float cx_ = 0.0F;
  float cy_ = 0.0F;
  float vx_ = 0.0F;
  float vy_ = 0.0F;
  float r_ = 0.0F;
  float eps_ = 0.0F;
};

[[nodiscard]] inline float ConsiderBestPositiveT(float t, float eps, float best_t) {
  if (t > eps && t < best_t) {
    return t;
  }
  return best_t;
}

[[nodiscard]] inline float ConsiderRoundedCornerArcIntersection(const RoundedRectRay& ray, ImVec2 corner_center, bool br,
                                                                bool bl, bool tr, bool tl, float best_t) {
  const float wx = ray.cx_ - corner_center.x;
  const float wy = ray.cy_ - corner_center.y;
  const float wd = wx * ray.vx_ + wy * ray.vy_;
  const float ww = wx * wx + wy * wy;
  const float inner = wd * wd - (ww - ray.r_ * ray.r_);
  if (inner < 0.0F) {
    return best_t;
  }
  const float srt = std::sqrt(inner);
  const std::array<float, 2> roots = {-wd + srt, -wd - srt};
  for (const float t : roots) {
    if (t <= ray.eps_) {
      continue;
    }
    const float px = ray.cx_ + ray.vx_ * t;
    const float py = ray.cy_ + ray.vy_ * t;
    const float rx = px - corner_center.x;
    const float ry = py - corner_center.y;
    bool ok = false;
    if (br) {
      ok = (rx >= -ray.eps_ && ry >= -ray.eps_);
    }
    else if (bl) {
      ok = (rx <= ray.eps_ && ry >= -ray.eps_);
    }
    else if (tr) {
      ok = (rx >= -ray.eps_ && ry <= ray.eps_);
    }
    else if (tl) {
      ok = (rx <= ray.eps_ && ry <= ray.eps_);
    }
    if (ok) {
      best_t = ConsiderBestPositiveT(t, ray.eps_, best_t);
    }
  }
  return best_t;
}

[[nodiscard]] inline float BestPositiveTForVerticalFlatEdges(float best_t, float k_epsilon, float cy, float vx,
                                                             float vy, float hx, float hy, float r) {
  if (vx > k_epsilon) {
    const float t = hx / vx;
    const float y = cy + vy * t;
    if (y >= cy - hy + r - k_epsilon && y <= cy + hy - r + k_epsilon) {
      return ConsiderBestPositiveT(t, k_epsilon, best_t);
    }
    return best_t;
  }
  if (vx < -k_epsilon) {
    const float t = -hx / vx;
    const float y = cy + vy * t;
    if (y >= cy - hy + r - k_epsilon && y <= cy + hy - r + k_epsilon) {
      return ConsiderBestPositiveT(t, k_epsilon, best_t);
    }
  }
  return best_t;
}

[[nodiscard]] inline float BestPositiveTForHorizontalFlatEdges(float best_t, float k_epsilon, float cx, float vx,
                                                               float vy, float hx, float hy, float r) {
  if (vy > k_epsilon) {
    const float t = hy / vy;
    const float x = cx + vx * t;
    if (x >= cx - hx + r - k_epsilon && x <= cx + hx - r + k_epsilon) {
      return ConsiderBestPositiveT(t, k_epsilon, best_t);
    }
    return best_t;
  }
  if (vy < -k_epsilon) {
    const float t = -hy / vy;
    const float x = cx + vx * t;
    if (x >= cx - hx + r - k_epsilon && x <= cx + hx - r + k_epsilon) {
      return ConsiderBestPositiveT(t, k_epsilon, best_t);
    }
  }
  return best_t;
}

// First hit from box center along toward_point on a rounded-rect outline (uniform corner radius, axis-aligned).
[[nodiscard]] inline ImVec2 SampleMapRoundedRectAttachmentToward(ImVec2 center, ImVec2 half_extents, float corner_r,
                                                                 ImVec2 toward_point) {
  constexpr float kEpsLenSq = 1.0e-12F;
  constexpr float kEps = 1.0e-5F;
  constexpr float kBestTInit = 1.0e15F;
  constexpr float kBestTNoHit = 1.0e14F;
  float vx = toward_point.x - center.x;
  float vy = toward_point.y - center.y;
  const float len_sq = vx * vx + vy * vy;
  if (len_sq < kEpsLenSq) {
    return {center.x + half_extents.x, center.y};
  }
  const float inv_len = 1.0F / std::sqrt(len_sq);
  vx *= inv_len;
  vy *= inv_len;

  const float hx = half_extents.x;
  const float hy = half_extents.y;
  const float r = (std::min)({corner_r, hx * kCornerRadiusClampFactor, hy * kCornerRadiusClampFactor});
  if (r < kEps) {
    return SampleMapAttachmentToward(center, half_extents, toward_point);
  }

  float best_t = kBestTInit;

  const float cx = center.x;
  const float cy = center.y;

  best_t = BestPositiveTForVerticalFlatEdges(best_t, kEps, cy, vx, vy, hx, hy, r);
  best_t = BestPositiveTForHorizontalFlatEdges(best_t, kEps, cx, vx, vy, hx, hy, r);
  const RoundedRectRay ray = {cx, cy, vx, vy, r, kEps};
  best_t = ConsiderRoundedCornerArcIntersection(ray, {cx + hx - r, cy + hy - r}, true, false, false, false, best_t);
  best_t = ConsiderRoundedCornerArcIntersection(ray, {cx - hx + r, cy + hy - r}, false, true, false, false, best_t);
  best_t = ConsiderRoundedCornerArcIntersection(ray, {cx + hx - r, cy - hy + r}, false, false, true, false, best_t);
  best_t = ConsiderRoundedCornerArcIntersection(ray, {cx - hx + r, cy - hy + r}, false, false, false, true, best_t);

  if (best_t >= kBestTNoHit) {
    return SampleMapAttachmentToward(center, half_extents, toward_point);
  }
  return {cx + vx * best_t, cy + vy * best_t};
}

enum class SampleMapBoxSide : std::uint8_t { Right, Left, Bottom, Top };  // NOLINT(performance-enum-size)

// Which face of the box `hit` lies on (normalized coords + horizontal_stickiness to prefer left/right ties).
[[nodiscard]] inline SampleMapBoxSide SampleMapSideOfHit(ImVec2 box_center, ImVec2 half_extents, ImVec2 hit,
                                                         float horizontal_stickiness = kDefaultHorizontalStickiness) {
  const float hx = (std::max)(half_extents.x, 1.0e-4F);
  const float hy = (std::max)(half_extents.y, 1.0e-4F);
  const float nax = std::abs(hit.x - box_center.x) / hx * horizontal_stickiness;
  if (const float nay = std::abs(hit.y - box_center.y) / hy; nax >= nay) {
    return (hit.x >= box_center.x) ? SampleMapBoxSide::Right : SampleMapBoxSide::Left;
  }
  return (hit.y >= box_center.y) ? SampleMapBoxSide::Bottom : SampleMapBoxSide::Top;
}

[[nodiscard]] inline float SampleMapSmoothstep(float edge0, float edge1, float x) {
  if (x <= edge0) {
    return 0.0F;
  }
  if (x >= edge1) {
    return 1.0F;
  }
  const float t = (x - edge0) / (edge1 - edge0);
  return t * t * (3.0F - 2.0F * t);
}

[[nodiscard]] inline bool SampleMapHitOnStraightSegment(ImVec2 c, ImVec2 h, float r, SampleMapBoxSide side,
                                                        ImVec2 hit) {
  constexpr float kEps = 0.85F;
  const float cx = c.x;
  const float cy = c.y;
  const float hx = h.x;
  const float hy = h.y;
  switch (side) {
    case SampleMapBoxSide::Right:  // NOLINT(bugprone-branch-clone)
      return std::abs(hit.x - (cx + hx)) <= kEps && hit.y >= cy - hy + r - kEps && hit.y <= cy + hy - r + kEps;
    case SampleMapBoxSide::Left:
      return std::abs(hit.x - (cx - hx)) <= kEps && hit.y >= cy - hy + r - kEps && hit.y <= cy + hy - r + kEps;
    case SampleMapBoxSide::Bottom:
      return std::abs(hit.y - (cy + hy)) <= kEps && hit.x >= cx - hx + r - kEps && hit.x <= cx + hx - r + kEps;
    case SampleMapBoxSide::Top:
      return std::abs(hit.y - (cy - hy)) <= kEps && hit.x >= cx - hx + r - kEps && hit.x <= cx + hx - r + kEps;
  }
  return false;
}

// Like SampleMapRoundedRectAttachmentToward but pulls toward the middle of the chosen flat edge; follows the ray hit
// only when the hit lies far along that edge (smoothstep). Corner hits are left unchanged.
[[nodiscard]] inline ImVec2 SampleMapRoundedRectAttachmentPreferEdgeMid(
    ImVec2 center, ImVec2 half_extents, float corner_r, ImVec2 toward_point,
    float blend_from_normalized = kDefaultBlendFromNormalized,
    float blend_to_normalized = kDefaultBlendToNormalized,
    float horizontal_stickiness = kDefaultHorizontalStickiness) {
  constexpr float kEps = 1.0e-5F;
  const ImVec2 hit = SampleMapRoundedRectAttachmentToward(center, half_extents, corner_r, toward_point);
  const SampleMapBoxSide side = SampleMapSideOfHit(center, half_extents, hit, horizontal_stickiness);
  if (!SampleMapHitOnStraightSegment(center, half_extents, corner_r, side, hit)) {
    return hit;
  }

  const float r = (std::min)({corner_r, half_extents.x * kCornerRadiusClampFactor,
                              half_extents.y * kCornerRadiusClampFactor});
  if (r < kEps) {
    return hit;
  }

  const float cx = center.x;
  const float cy = center.y;
  const float hx = half_extents.x;
  const float hy = half_extents.y;
  const float span_v = (std::max)(hy - r, 1.0e-4F);
  const float span_h = (std::max)(hx - r, 1.0e-4F);

  if (side == SampleMapBoxSide::Right || side == SampleMapBoxSide::Left) {
    const float offset = hit.y - cy;
    const float u = (std::min)(std::abs(offset) / span_v, 1.0F);
    const float w = SampleMapSmoothstep(blend_from_normalized, blend_to_normalized, u);
    const float y = cy + offset * w;
    const float y_clamped = std::clamp(y, cy - hy + r, cy + hy - r);
    const float x_side = (side == SampleMapBoxSide::Right) ? (cx + hx) : (cx - hx);
    return {x_side, y_clamped};
  }

  const float offset = hit.x - cx;
  const float u = (std::min)(std::abs(offset) / span_h, 1.0F);
  const float w = SampleMapSmoothstep(blend_from_normalized, blend_to_normalized, u);
  const float x = cx + offset * w;
  const float x_clamped = std::clamp(x, cx - hx + r, cx + hx - r);
  const float y_side = (side == SampleMapBoxSide::Bottom) ? (cy + hy) : (cy - hy);
  return {x_clamped, y_side};
}

struct SampleMapBezierArms {
  ImVec2 p1_;
  ImVec2 p2_;
};

struct SampleMapBezierArmInputs {
  ImVec2 parent_center_;
  ImVec2 parent_half_;
  ImVec2 child_center_;
  ImVec2 child_half_;
  ImVec2 p0w_;
  ImVec2 p3w_;
  float min_arm_world_ = 0.0F;
  float span_fraction_ = 0.0F;
  const ImVec2* p0_border_for_normal_ = nullptr;
  const ImVec2* p3_border_for_normal_ = nullptr;
};

// Outward axis normal for the flat edge (or dominant edge at corners) where attachment_point lies on the box.
[[nodiscard]] inline ImVec2 SampleMapEdgeOutwardAxis(ImVec2 box_center, ImVec2 half_extents, ImVec2 attachment_point) {
  constexpr float kTiny = 1.0e-6F;
  const float ax =
      half_extents.x > kTiny ? std::abs(attachment_point.x - box_center.x) / half_extents.x : 0.0F;
  if (const float ay = half_extents.y > kTiny ? std::abs(attachment_point.y - box_center.y) / half_extents.y
                                               : 0.0F;
      ax >= ay) {
    return (attachment_point.x >= box_center.x) ? ImVec2{1.0F, 0.0F} : ImVec2{-1.0F, 0.0F};
  }
  return (attachment_point.y >= box_center.y) ? ImVec2{0.0F, 1.0F} : ImVec2{0.0F, -1.0F};
}

// p1 / p2 extend along edge normals so B'(0) and B'(1) match the border; strong S without peeling off the node.
// If p0_border_for_normal / p3_border_for_normal are non-null, those points classify the edge for out0/out3 while
// p0w/p3w remain the actual cubic endpoints (e.g. p0 inside parent, normal from border hit).
[[nodiscard]] inline SampleMapBezierArms ComputeSampleMapBezierArmsWorld(const SampleMapBezierArmInputs& inputs) {
  constexpr float kMaxArmAsChordFraction = 0.45F;
  constexpr float kEpsChord = 1.0e-6F;
  const float dx = inputs.p3w_.x - inputs.p0w_.x;
  const float dy = inputs.p3w_.y - inputs.p0w_.y;
  const float adx = std::abs(dx);
  const float ady = std::abs(dy);
  const float chord = std::sqrt(dx * dx + dy * dy);
  if (chord < kEpsChord) {
    return {inputs.p0w_, inputs.p3w_};
  }
  const float sep_dom = (std::max)(adx, ady);
  float arm = (std::max)(inputs.min_arm_world_, sep_dom * inputs.span_fraction_);
  arm = (std::min)(arm, kMaxArmAsChordFraction * chord);

  const ImVec2 ref0 = (inputs.p0_border_for_normal_ != nullptr) ? *inputs.p0_border_for_normal_ : inputs.p0w_;
  const ImVec2 ref3 = (inputs.p3_border_for_normal_ != nullptr) ? *inputs.p3_border_for_normal_ : inputs.p3w_;
  const ImVec2 out0 = SampleMapEdgeOutwardAxis(inputs.parent_center_, inputs.parent_half_, ref0);
  const ImVec2 out3 = SampleMapEdgeOutwardAxis(inputs.child_center_, inputs.child_half_, ref3);
  return {{inputs.p0w_.x + out0.x * arm, inputs.p0w_.y + out0.y * arm},
          {inputs.p3w_.x + out3.x * arm, inputs.p3w_.y + out3.y * arm}};
}

[[nodiscard]] inline std::array<ImVec2, kSampleMindMapNodeCount> InitialSampleMapPositions() {
  constexpr float kDepth1X = 260.0F;
  constexpr float kDepth2X = 520.0F;
  constexpr float kDepth1Spread = 140.0F;
  constexpr float kDepth2TopY = 200.0F;
  constexpr float kDepth2BottomY = 20.0F;
  return {{
      {0.0F, 0.0F},
      {kDepth1X, -kDepth1Spread},
      {kDepth1X, 0.0F},
      {kDepth1X, kDepth1Spread},
      {kDepth2X, -kDepth2TopY},
      {kDepth2X, -100.0F},
      {kDepth2X, kDepth2BottomY},
  }};
}

}  // namespace mind_map::demos
