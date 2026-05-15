#pragma once

#include "imgui.h"

#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <cstdint>

namespace mind_map::canvas {

inline constexpr float kNodePad = 10.0F;
inline constexpr float kNodeMaxLabelWidth = 320.0F;
inline constexpr float kNodeCornerRadiusWorld = 6.0F;
inline constexpr float kCornerRadiusClampFactor = 0.98F;
inline constexpr float kDefaultHorizontalStickiness = 1.12F;
inline constexpr float kDefaultBlendFromNormalized = 0.22F;
inline constexpr float kDefaultBlendToNormalized = 0.62F;

/// Returns the label-fitted half-extents for a node box (ImGui text size + kNodePad padding).
/// Long labels wrap at kNodeMaxLabelWidth and grow the node downward.
[[nodiscard]] inline ImVec2 NodeHalfExtentForLabel(const char* label) {
  assert(label != nullptr);
  if (const ImVec2 single = ImGui::CalcTextSize(label); single.x <= kNodeMaxLabelWidth) {
    return {single.x * 0.5F + kNodePad, single.y * 0.5F + kNodePad};
  }
  const ImVec2 wrapped = ImGui::CalcTextSize(label, nullptr, false, kNodeMaxLabelWidth);
  return {kNodeMaxLabelWidth * 0.5F + kNodePad, wrapped.y * 0.5F + kNodePad};
}

/// Half-extents for a pasted or loaded image: 1 world unit per pixel (half-width = width/2), aspect ratio preserved.
/// Images larger than a label-max node are scaled down uniformly on the longest side.
[[nodiscard]] inline ImVec2 NodeHalfExtentForImagePixels(int width_px, int height_px) {
  assert(width_px > 0);
  assert(height_px > 0);
  float hx = static_cast<float>(width_px) * 0.5F;
  float hy = static_cast<float>(height_px) * 0.5F;
  const float max_half = kNodeMaxLabelWidth * 0.5F + kNodePad;
  const float longest = (std::max)(hx, hy);
  if (longest > max_half) {
    const float scale = max_half / longest;
    hx *= scale;
    hy *= scale;
  }
  return {hx, hy};
}

[[nodiscard]] inline float NodeRadiusWorld(const char* label) {
  assert(label != nullptr);
  const ImVec2 half = NodeHalfExtentForLabel(label);
  return (std::max)(half.x, half.y);
}

/// Returns the point on a sharp-cornered box surface along the ray toward toward_point; fallback for RoundedRectAttachmentToward.
[[nodiscard]] inline ImVec2 AttachmentToward(ImVec2 from_center, ImVec2 half_extents, ImVec2 toward_point) {
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

[[nodiscard]] inline float ConsiderRoundedCornerArcIntersection(const RoundedRectRay& ray, ImVec2 corner_center,
                                                                bool br, bool bl, bool tr, bool tl, float best_t) {
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

struct RoundedRectFlatRayContext {
  float k_epsilon_ = 0.0F;
  float vx_ = 0.0F;
  float vy_ = 0.0F;
  float hx_ = 0.0F;
  float hy_ = 0.0F;
  float r_ = 0.0F;
};

[[nodiscard]] inline float BestPositiveTForVerticalFlatEdges(float best_t, float cy,
                                                             const RoundedRectFlatRayContext& ctx) {
  if (ctx.vx_ > ctx.k_epsilon_) {
    const float t = ctx.hx_ / ctx.vx_;
    if (const float y = cy + ctx.vy_ * t;
        y >= cy - ctx.hy_ + ctx.r_ - ctx.k_epsilon_ && y <= cy + ctx.hy_ - ctx.r_ + ctx.k_epsilon_) {
      return ConsiderBestPositiveT(t, ctx.k_epsilon_, best_t);
    }
    return best_t;
  }
  if (ctx.vx_ < -ctx.k_epsilon_) {
    const float t = -ctx.hx_ / ctx.vx_;
    if (const float y = cy + ctx.vy_ * t;
        y >= cy - ctx.hy_ + ctx.r_ - ctx.k_epsilon_ && y <= cy + ctx.hy_ - ctx.r_ + ctx.k_epsilon_) {
      return ConsiderBestPositiveT(t, ctx.k_epsilon_, best_t);
    }
  }
  return best_t;
}

[[nodiscard]] inline float BestPositiveTForHorizontalFlatEdges(float best_t, float cx,
                                                               const RoundedRectFlatRayContext& ctx) {
  if (ctx.vy_ > ctx.k_epsilon_) {
    const float t = ctx.hy_ / ctx.vy_;
    if (const float x = cx + ctx.vx_ * t;
        x >= cx - ctx.hx_ + ctx.r_ - ctx.k_epsilon_ && x <= cx + ctx.hx_ - ctx.r_ + ctx.k_epsilon_) {
      return ConsiderBestPositiveT(t, ctx.k_epsilon_, best_t);
    }
    return best_t;
  }
  if (ctx.vy_ < -ctx.k_epsilon_) {
    const float t = -ctx.hy_ / ctx.vy_;
    if (const float x = cx + ctx.vx_ * t;
        x >= cx - ctx.hx_ + ctx.r_ - ctx.k_epsilon_ && x <= cx + ctx.hx_ - ctx.r_ + ctx.k_epsilon_) {
      return ConsiderBestPositiveT(t, ctx.k_epsilon_, best_t);
    }
  }
  return best_t;
}

/// Returns the point on a rounded-rect surface along the ray toward toward_point; used by all branch renderers via BranchEdgeAttachments.
[[nodiscard]] inline ImVec2 RoundedRectAttachmentToward(ImVec2 center, ImVec2 half_extents, float corner_r,
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
    return AttachmentToward(center, half_extents, toward_point);
  }

  float best_t = kBestTInit;
  const float cx = center.x;
  const float cy = center.y;

  const RoundedRectFlatRayContext flat_ray_ctx = {kEps, vx, vy, hx, hy, r};
  best_t = BestPositiveTForVerticalFlatEdges(best_t, cy, flat_ray_ctx);
  best_t = BestPositiveTForHorizontalFlatEdges(best_t, cx, flat_ray_ctx);
  const RoundedRectRay ray = {cx, cy, vx, vy, r, kEps};
  best_t = ConsiderRoundedCornerArcIntersection(ray, {cx + hx - r, cy + hy - r}, true, false, false, false, best_t);
  best_t = ConsiderRoundedCornerArcIntersection(ray, {cx - hx + r, cy + hy - r}, false, true, false, false, best_t);
  best_t = ConsiderRoundedCornerArcIntersection(ray, {cx + hx - r, cy - hy + r}, false, false, true, false, best_t);
  best_t = ConsiderRoundedCornerArcIntersection(ray, {cx - hx + r, cy - hy + r}, false, false, false, true, best_t);

  if (best_t >= kBestTNoHit) {
    return AttachmentToward(center, half_extents, toward_point);
  }
  return {cx + vx * best_t, cy + vy * best_t};
}

enum class BoxSide : std::uint8_t { Right, Left, Bottom, Top };  // NOLINT(performance-enum-size)

[[nodiscard]] inline BoxSide BoxSideOfHit(ImVec2 box_center, ImVec2 half_extents, ImVec2 hit,
                                          float horizontal_stickiness = kDefaultHorizontalStickiness) {
  const float hx = (std::max)(half_extents.x, 1.0e-4F);
  const float hy = (std::max)(half_extents.y, 1.0e-4F);
  const float nax = std::abs(hit.x - box_center.x) / hx * horizontal_stickiness;
  if (const float nay = std::abs(hit.y - box_center.y) / hy; nax >= nay) {
    return (hit.x >= box_center.x) ? BoxSide::Right : BoxSide::Left;
  }
  return (hit.y >= box_center.y) ? BoxSide::Bottom : BoxSide::Top;
}

[[nodiscard]] inline float Smoothstep(float edge0, float edge1, float x) {
  if (x <= edge0) {
    return 0.0F;
  }
  if (x >= edge1) {
    return 1.0F;
  }
  const float t = (x - edge0) / (edge1 - edge0);
  return t * t * (3.0F - 2.0F * t);
}

[[nodiscard]] inline bool BoxHitOnStraightSegment(ImVec2 c, ImVec2 h, float r, BoxSide side, ImVec2 hit) {
  constexpr float kEps = 0.85F;
  const float cx = c.x;
  const float cy = c.y;
  const float hx = h.x;
  const float hy = h.y;
  switch (side) {
    case BoxSide::Right:  // NOLINT(bugprone-branch-clone)
      return std::abs(hit.x - (cx + hx)) <= kEps && hit.y >= cy - hy + r - kEps && hit.y <= cy + hy - r + kEps;
    case BoxSide::Left:
      return std::abs(hit.x - (cx - hx)) <= kEps && hit.y >= cy - hy + r - kEps && hit.y <= cy + hy - r + kEps;
    case BoxSide::Bottom:
      return std::abs(hit.y - (cy + hy)) <= kEps && hit.x >= cx - hx + r - kEps && hit.x <= cx + hx - r + kEps;
    case BoxSide::Top:
      return std::abs(hit.y - (cy - hy)) <= kEps && hit.x >= cx - hx + r - kEps && hit.x <= cx + hx - r + kEps;
  }
  return false;
}

/// Like RoundedRectAttachmentToward but snaps toward the flat edge midpoint for a cleaner connection; called by FillBranchEdgeData.
[[nodiscard]] inline ImVec2 RoundedRectAttachmentPreferEdgeMid(
    ImVec2 center, ImVec2 half_extents, float corner_r, ImVec2 toward_point,
    float blend_from_normalized = kDefaultBlendFromNormalized,
    float blend_to_normalized = kDefaultBlendToNormalized,
    float horizontal_stickiness = kDefaultHorizontalStickiness) {
  constexpr float kEps = 1.0e-5F;
  const ImVec2 hit = RoundedRectAttachmentToward(center, half_extents, corner_r, toward_point);
  const BoxSide side = BoxSideOfHit(center, half_extents, hit, horizontal_stickiness);
  if (!BoxHitOnStraightSegment(center, half_extents, corner_r, side, hit)) {
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

  if (side == BoxSide::Right || side == BoxSide::Left) {
    const float offset = hit.y - cy;
    const float u = (std::min)(std::abs(offset) / span_v, 1.0F);
    const float w = Smoothstep(blend_from_normalized, blend_to_normalized, u);
    const float y = cy + offset * w;
    const float y_clamped = std::clamp(y, cy - hy + r, cy + hy - r);
    const float x_side = (side == BoxSide::Right) ? (cx + hx) : (cx - hx);
    return {x_side, y_clamped};
  }

  const float offset = hit.x - cx;
  const float u = (std::min)(std::abs(offset) / span_h, 1.0F);
  const float w = Smoothstep(blend_from_normalized, blend_to_normalized, u);
  const float x = cx + offset * w;
  const float x_clamped = std::clamp(x, cx - hx + r, cx + hx - r);
  const float y_side = (side == BoxSide::Bottom) ? (cy + hy) : (cy - hy);
  return {x_clamped, y_side};
}

[[nodiscard]] inline ImVec2 EdgeOutwardAxis(ImVec2 box_center, ImVec2 half_extents, ImVec2 attachment_point) {
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

struct BezierArms {
  ImVec2 p1_;
  ImVec2 p2_;
};

struct BezierArmInputs {
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

/// Computes Bézier control arms (p1, p2) for a parent→child edge using outward face normals and a span fraction.
[[nodiscard]] inline BezierArms ComputeBezierArmsWorld(const BezierArmInputs& inputs) {
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
  const ImVec2 out0 = EdgeOutwardAxis(inputs.parent_center_, inputs.parent_half_, ref0);
  const ImVec2 out3 = EdgeOutwardAxis(inputs.child_center_, inputs.child_half_, ref3);
  return {{inputs.p0w_.x + out0.x * arm, inputs.p0w_.y + out0.y * arm},
          {inputs.p3w_.x + out3.x * arm, inputs.p3w_.y + out3.y * arm}};
}

}  // namespace mind_map::canvas
