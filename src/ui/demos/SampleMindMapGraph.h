#pragma once

#include "core/MindMapDocument.h"
#include "core/mindmap/SampleMindMapTopology.h"

#include "imgui.h"

#include <array>

namespace mind_map::demos {

using mind_map::core::mindmap::kSampleMindMapNodeCount;
using mind_map::core::mindmap::kSampleMindMapSpecs;
using mind_map::core::mindmap::SampleMindMapNodeSpec;

// Builds a MindMapDocument from the sample topology with stable UUIDs and initial layout.
// Deleted together with InitialSampleMapPositions when node-creation UI lands.
[[nodiscard]] mind_map::core::MindMapDocument BuildSampleDocument();

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
