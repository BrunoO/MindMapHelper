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
