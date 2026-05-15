#pragma once

#include "ui/canvas/CanvasNode.h"
#include "ui/canvas/InlineMarkup.h"
#include "ui/canvas/InlineMarkupRenderer.h"
#include "ui/canvas/NodeGeometry.h"

namespace mind_map::canvas {

/// Returns the half-extents for a node: stored override if set, markup-measured if the
/// label contains markup syntax, otherwise plain-text fitted via NodeHalfExtentForLabel.
[[nodiscard]] inline ImVec2 NodeHalfExtent(const mind_map::ui::CanvasNode& node) {
  if (node.half_extent_override_.x > 0.0F) {
    return node.half_extent_override_;
  }
  if (mind_map::ui::canvas::ContainsMarkup(node.label_)) {
    const std::vector<mind_map::ui::canvas::MarkupSpan> spans =
        mind_map::ui::canvas::ParseMarkup(node.label_);
    const ImVec2 content = mind_map::ui::canvas::MeasureMarkup(spans, kNodeMaxLabelWidth);
    return {content.x * 0.5F + kNodePad, content.y * 0.5F + kNodePad};
  }
  return NodeHalfExtentForLabel(node.label_.c_str());
}

}  // namespace mind_map::canvas
