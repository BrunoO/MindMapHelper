#pragma once

#include "ui/canvas/CanvasNode.h"
#include "ui/canvas/NodeGeometry.h"

namespace mind_map::canvas {

/// Returns the half-extents for a node: stored override if set, otherwise label-fitted from NodeHalfExtentForLabel.
[[nodiscard]] inline ImVec2 NodeHalfExtent(const mind_map::ui::CanvasNode& node) {
  if (node.half_extent_override_.x > 0.0F) {
    return node.half_extent_override_;
  }
  return NodeHalfExtentForLabel(node.label_.c_str());
}

}  // namespace mind_map::canvas
