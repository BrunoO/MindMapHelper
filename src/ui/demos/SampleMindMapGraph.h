#pragma once

#include "core/MindMapDocument.h"
#include "core/mindmap/SampleMindMapTopology.h"

#include "ui/canvas/CanvasMath.h"
#include "ui/canvas/NodeGeometry.h"

#include "imgui.h"

#include <array>
#include <cassert>
#include <cstddef>

namespace mind_map::demos {

using mind_map::core::mindmap::kSampleMindMapNodeCount;
using mind_map::core::mindmap::kSampleMindMapSpecs;
using mind_map::core::mindmap::SampleMindMapNodeSpec;

// Builds a MindMapDocument from the sample topology with stable UUIDs and initial layout.
// Deleted together with HitTestSampleMap and InitialSampleMapPositions when node-creation UI lands.
[[nodiscard]] mind_map::core::MindMapDocument BuildSampleDocument();

// Hit-tests the fixed sample node array. Deleted in step 2 when topology moves to MindMapDocument.
[[nodiscard]] inline int HitTestSampleMap(ImVec2 world_pos,
                                          const std::array<ImVec2, kSampleMindMapNodeCount>& pos_world) {
  for (int i = kSampleMindMapNodeCount - 1; i >= 0; --i) {
    const char* const label = kSampleMindMapSpecs[static_cast<size_t>(i)].label_;
    const ImVec2 half = mind_map::canvas::NodeHalfExtentForLabel(label);
    const ImVec2 c = pos_world[static_cast<size_t>(i)];
    const ImVec2 rmin = {c.x - half.x, c.y - half.y};
    const ImVec2 rmax = {c.x + half.x, c.y + half.y};
    if (mind_map::canvas::IsInsideRect(world_pos, rmin, rmax)) {
      return i;
    }
  }
  return -1;
}

// Hardcoded starting layout. Deleted in step 2 when positions come from MindMapDocument.
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
