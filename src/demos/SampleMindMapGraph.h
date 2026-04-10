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

[[nodiscard]] inline SampleMapBezierArms ComputeSampleMapBezierArmsWorld(ImVec2 p0w, ImVec2 p3w, float min_arm_world,
                                                                         float span_fraction) {
  float dx = p3w.x - p0w.x;
  float dy = p3w.y - p0w.y;
  const float dist = std::sqrt(dx * dx + dy * dy);
  const float arm = (std::max)(min_arm_world, dist * span_fraction);
  if (dist > 1.0e-6F) {
    dx /= dist;
    dy /= dist;
  }
  else {
    dx = 1.0F;
    dy = 0.0F;
  }
  return {{p0w.x + dx * arm, p0w.y + dy * arm}, {p3w.x - dx * arm, p3w.y - dy * arm}};
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
