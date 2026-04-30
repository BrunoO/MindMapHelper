#pragma once

#include "ui/canvas/CanvasMath.h"

#include "imgui.h"

#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <cstddef>

namespace mind_map::demos {

inline constexpr int kSampleMindMapNodeCount = 7;
inline constexpr float kSampleMindMapNodePad = 10.0F;
// Matches rounded-rect nodes in Bezier / organic taper demos (world space).
inline constexpr float kSampleMindMapNodeCornerRadiusWorld = 6.0F;

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

[[nodiscard]] inline float SampleMapNodeRadiusWorld(const char* label) {
  assert(label != nullptr);
  const ImVec2 half = SampleMapHalfExtentForLabel(label);
  return (std::max)(half.x, half.y);
}

// Axis-aligned box boundary point from from_center toward toward_point (half = half-extents).
[[nodiscard]] inline ImVec2 SampleMapAttachmentToward(ImVec2 from_center, ImVec2 half_extents, ImVec2 toward_point) {
  float vx = toward_point.x - from_center.x;
  float vy = toward_point.y - from_center.y;
  const float len_sq = vx * vx + vy * vy;
  if (len_sq < 1.0e-12F) {
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

// First hit from box center along toward_point on a rounded-rect outline (uniform corner radius, axis-aligned).
[[nodiscard]] inline ImVec2 SampleMapRoundedRectAttachmentToward(ImVec2 center, ImVec2 half_extents, float corner_r,
                                                                   ImVec2 toward_point) {
  float vx = toward_point.x - center.x;
  float vy = toward_point.y - center.y;
  const float len_sq = vx * vx + vy * vy;
  if (len_sq < 1.0e-12F) {
    return {center.x + half_extents.x, center.y};
  }
  const float inv_len = 1.0F / std::sqrt(len_sq);
  vx *= inv_len;
  vy *= inv_len;

  const float hx = half_extents.x;
  const float hy = half_extents.y;
  float r = corner_r;
  if (r > hx * 0.98F) {
    r = hx * 0.98F;
  }
  if (r > hy * 0.98F) {
    r = hy * 0.98F;
  }
  if (r < 1.0e-5F) {
    return SampleMapAttachmentToward(center, half_extents, toward_point);
  }

  constexpr float kEps = 1.0e-5F;
  float best_t = 1.0e15F;
  const auto consider = [&best_t](float t) {
    if (t > kEps && t < best_t) {
      best_t = t;
    }
  };

  const float cx = center.x;
  const float cy = center.y;

  if (vx > kEps) {
    const float t = hx / vx;
    const float y = cy + vy * t;
    if (y >= cy - hy + r - kEps && y <= cy + hy - r + kEps) {
      consider(t);
    }
  }
  else if (vx < -kEps) {
    const float t = -hx / vx;
    const float y = cy + vy * t;
    if (y >= cy - hy + r - kEps && y <= cy + hy - r + kEps) {
      consider(t);
    }
  }

  if (vy > kEps) {
    const float t = hy / vy;
    const float x = cx + vx * t;
    if (x >= cx - hx + r - kEps && x <= cx + hx - r + kEps) {
      consider(t);
    }
  }
  else if (vy < -kEps) {
    const float t = -hy / vy;
    const float x = cx + vx * t;
    if (x >= cx - hx + r - kEps && x <= cx + hx - r + kEps) {
      consider(t);
    }
  }

  const auto consider_arc = [&](ImVec2 k, bool br, bool bl, bool tr, bool tl) {
    const float wx = cx - k.x;
    const float wy = cy - k.y;
    const float wd = wx * vx + wy * vy;
    const float ww = wx * wx + wy * wy;
    const float inner = wd * wd - (ww - r * r);
    if (inner < 0.0F) {
      return;
    }
    const float srt = std::sqrt(inner);
    const float roots[2] = {-wd + srt, -wd - srt};
    for (float t : roots) {
      if (t <= kEps) {
        continue;
      }
      const float px = cx + vx * t;
      const float py = cy + vy * t;
      const float rx = px - k.x;
      const float ry = py - k.y;
      bool ok = false;
      if (br) {
        ok = (rx >= -kEps && ry >= -kEps);
      }
      else if (bl) {
        ok = (rx <= kEps && ry >= -kEps);
      }
      else if (tr) {
        ok = (rx >= -kEps && ry <= kEps);
      }
      else if (tl) {
        ok = (rx <= kEps && ry <= kEps);
      }
      if (ok) {
        consider(t);
      }
    }
  };

  consider_arc({cx + hx - r, cy + hy - r}, true, false, false, false);
  consider_arc({cx - hx + r, cy + hy - r}, false, true, false, false);
  consider_arc({cx + hx - r, cy - hy + r}, false, false, true, false);
  consider_arc({cx - hx + r, cy - hy + r}, false, false, false, true);

  if (best_t >= 1.0e14F) {
    return SampleMapAttachmentToward(center, half_extents, toward_point);
  }
  return {cx + vx * best_t, cy + vy * best_t};
}

enum class SampleMapBoxSide { kRight, kLeft, kBottom, kTop };

// Which face of the box `hit` lies on (normalized coords + horizontal_stickiness to prefer left/right ties).
[[nodiscard]] inline SampleMapBoxSide SampleMapSideOfHit(ImVec2 box_center, ImVec2 half_extents, ImVec2 hit,
                                                         float horizontal_stickiness = 1.12F) {
  const float hx = (std::max)(half_extents.x, 1.0e-4F);
  const float hy = (std::max)(half_extents.y, 1.0e-4F);
  const float nax = std::abs(hit.x - box_center.x) / hx * horizontal_stickiness;
  const float nay = std::abs(hit.y - box_center.y) / hy;
  if (nax >= nay) {
    return (hit.x >= box_center.x) ? SampleMapBoxSide::kRight : SampleMapBoxSide::kLeft;
  }
  return (hit.y >= box_center.y) ? SampleMapBoxSide::kBottom : SampleMapBoxSide::kTop;
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
    case SampleMapBoxSide::kRight:
      return std::abs(hit.x - (cx + hx)) <= kEps && hit.y >= cy - hy + r - kEps && hit.y <= cy + hy - r + kEps;
    case SampleMapBoxSide::kLeft:
      return std::abs(hit.x - (cx - hx)) <= kEps && hit.y >= cy - hy + r - kEps && hit.y <= cy + hy - r + kEps;
    case SampleMapBoxSide::kBottom:
      return std::abs(hit.y - (cy + hy)) <= kEps && hit.x >= cx - hx + r - kEps && hit.x <= cx + hx - r + kEps;
    case SampleMapBoxSide::kTop:
      return std::abs(hit.y - (cy - hy)) <= kEps && hit.x >= cx - hx + r - kEps && hit.x <= cx + hx - r + kEps;
  }
  return false;
}

// Like SampleMapRoundedRectAttachmentToward but pulls toward the middle of the chosen flat edge; follows the ray hit
// only when the hit lies far along that edge (smoothstep). Corner hits are left unchanged.
[[nodiscard]] inline ImVec2 SampleMapRoundedRectAttachmentPreferEdgeMid(
    ImVec2 center, ImVec2 half_extents, float corner_r, ImVec2 toward_point,
    float blend_from_normalized = 0.22F, float blend_to_normalized = 0.62F, float horizontal_stickiness = 1.12F) {
  const ImVec2 hit = SampleMapRoundedRectAttachmentToward(center, half_extents, corner_r, toward_point);
  const SampleMapBoxSide side = SampleMapSideOfHit(center, half_extents, hit, horizontal_stickiness);
  if (!SampleMapHitOnStraightSegment(center, half_extents, corner_r, side, hit)) {
    return hit;
  }

  float r = corner_r;
  if (r > half_extents.x * 0.98F) {
    r = half_extents.x * 0.98F;
  }
  if (r > half_extents.y * 0.98F) {
    r = half_extents.y * 0.98F;
  }
  if (r < 1.0e-5F) {
    return hit;
  }

  const float cx = center.x;
  const float cy = center.y;
  const float hx = half_extents.x;
  const float hy = half_extents.y;
  const float span_v = (std::max)(hy - r, 1.0e-4F);
  const float span_h = (std::max)(hx - r, 1.0e-4F);

  if (side == SampleMapBoxSide::kRight || side == SampleMapBoxSide::kLeft) {
    const float offset = hit.y - cy;
    const float u = (std::min)(std::abs(offset) / span_v, 1.0F);
    const float w = SampleMapSmoothstep(blend_from_normalized, blend_to_normalized, u);
    const float y = cy + offset * w;
    const float y_clamped = (std::clamp)(y, cy - hy + r, cy + hy - r);
    const float x_side = (side == SampleMapBoxSide::kRight) ? (cx + hx) : (cx - hx);
    return {x_side, y_clamped};
  }

  const float offset = hit.x - cx;
  const float u = (std::min)(std::abs(offset) / span_h, 1.0F);
  const float w = SampleMapSmoothstep(blend_from_normalized, blend_to_normalized, u);
  const float x = cx + offset * w;
  const float x_clamped = (std::clamp)(x, cx - hx + r, cx + hx - r);
  const float y_side = (side == SampleMapBoxSide::kBottom) ? (cy + hy) : (cy - hy);
  return {x_clamped, y_side};
}

[[nodiscard]] inline ImVec2 SampleMapCircleAttachmentToward(ImVec2 from_center, float radius, ImVec2 toward_point) {
  assert(radius >= 0.0F);
  float vx = toward_point.x - from_center.x;
  float vy = toward_point.y - from_center.y;
  const float len_sq = vx * vx + vy * vy;
  if (len_sq < 1.0e-12F) {
    return {from_center.x + radius, from_center.y};
  }
  const float inv = radius / std::sqrt(len_sq);
  return {from_center.x + vx * inv, from_center.y + vy * inv};
}

struct SampleMapBezierArms {
  ImVec2 p1;
  ImVec2 p2;
};

// Outward axis normal for the flat edge (or dominant edge at corners) where attachment_point lies on the box.
[[nodiscard]] inline ImVec2 SampleMapEdgeOutwardAxis(ImVec2 box_center, ImVec2 half_extents, ImVec2 attachment_point) {
  constexpr float kTiny = 1.0e-6F;
  const float ax =
      half_extents.x > kTiny ? std::abs(attachment_point.x - box_center.x) / half_extents.x : 0.0F;
  const float ay =
      half_extents.y > kTiny ? std::abs(attachment_point.y - box_center.y) / half_extents.y : 0.0F;
  if (ax >= ay) {
    return (attachment_point.x >= box_center.x) ? ImVec2{1.0F, 0.0F} : ImVec2{-1.0F, 0.0F};
  }
  return (attachment_point.y >= box_center.y) ? ImVec2{0.0F, 1.0F} : ImVec2{0.0F, -1.0F};
}

// p1 / p2 extend along edge normals so B'(0) and B'(1) match the border; strong S without peeling off the node.
// If p0_border_for_normal / p3_border_for_normal are non-null, those points classify the edge for out0/out3 while
// p0w/p3w remain the actual cubic endpoints (e.g. p0 inside parent, normal from border hit).
[[nodiscard]] inline SampleMapBezierArms ComputeSampleMapBezierArmsWorld(
    ImVec2 parent_center, ImVec2 parent_half, ImVec2 child_center, ImVec2 child_half, ImVec2 p0w, ImVec2 p3w,
    float min_arm_world, float span_fraction, const ImVec2* p0_border_for_normal = nullptr,
    const ImVec2* p3_border_for_normal = nullptr) {
  constexpr float kMaxArmAsChordFraction = 0.45F;
  const float dx = p3w.x - p0w.x;
  const float dy = p3w.y - p0w.y;
  const float adx = std::abs(dx);
  const float ady = std::abs(dy);
  const float chord = std::sqrt(dx * dx + dy * dy);
  if (chord < 1.0e-6F) {
    return {p0w, p3w};
  }
  const float sep_dom = (std::max)(adx, ady);
  float arm = (std::max)(min_arm_world, sep_dom * span_fraction);
  arm = (std::min)(arm, kMaxArmAsChordFraction * chord);

  const ImVec2 ref0 = (p0_border_for_normal != nullptr) ? *p0_border_for_normal : p0w;
  const ImVec2 ref3 = (p3_border_for_normal != nullptr) ? *p3_border_for_normal : p3w;
  const ImVec2 out0 = SampleMapEdgeOutwardAxis(parent_center, parent_half, ref0);
  const ImVec2 out3 = SampleMapEdgeOutwardAxis(child_center, child_half, ref3);
  return {{p0w.x + out0.x * arm, p0w.y + out0.y * arm}, {p3w.x + out3.x * arm, p3w.y + out3.y * arm}};
}

[[nodiscard]] inline int HitTestSampleMapCircles(ImVec2 world_pos,
                                                 const std::array<ImVec2, kSampleMindMapNodeCount>& pos_world) {
  for (int i = kSampleMindMapNodeCount - 1; i >= 0; --i) {
    const char* const label = kSampleMindMapSpecs[static_cast<size_t>(i)].label_;
    const float r = SampleMapNodeRadiusWorld(label);
    const ImVec2 c = pos_world[static_cast<size_t>(i)];
    const float dx = world_pos.x - c.x;
    const float dy = world_pos.y - c.y;
    if (dx * dx + dy * dy <= r * r) {
      return i;
    }
  }
  return -1;
}

[[nodiscard]] inline std::array<ImVec2, kSampleMindMapNodeCount> InitialSampleMapPositions() {
  return {{
      {0.0F, 0.0F},
      {260.0F, -140.0F},
      {260.0F, 0.0F},
      {260.0F, 140.0F},
      {520.0F, -200.0F},
      {520.0F, -100.0F},
      {520.0F, 20.0F},
  }};
}

}  // namespace mind_map::demos
