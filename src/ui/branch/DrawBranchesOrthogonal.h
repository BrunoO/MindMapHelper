#pragma once

#include "ui/branch/BranchRenderContext.h"
#include "ui/canvas/CanvasNode.h"

#include <vector>

namespace mind_map::ui::branch {

/// Draws an L-shaped polyline branch from parent to child using BranchEdgeData rounded-rect attachment points.
// child_index must satisfy nodes[child_index].parent_idx_.has_value().
void DrawMindMapBranchOrthogonal(
    const BranchRenderContext& ctx, size_t child_index,
    const std::vector<mind_map::ui::CanvasNode>& nodes);

}  // namespace mind_map::ui::branch
