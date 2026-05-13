#pragma once

#include "ui/canvas/CanvasNode.h"
#include "ui/canvas/NodeGeometry.h"

namespace mind_map::canvas {

// Returns the effective half-extents for a canvas node: the stored override when non-zero,
// otherwise the label-fitted size from NodeHalfExtentForLabel.
[[nodiscard]] inline ImVec2 NodeHalfExtent(const mind_map::ui::CanvasNode& node) {
  if (node.half_extent_override_.x > 0.0F) {
    return node.half_extent_override_;
  }
  return NodeHalfExtentForLabel(node.label_.c_str());
}

}  // namespace mind_map::canvas
