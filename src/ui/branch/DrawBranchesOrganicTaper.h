#pragma once

#include "ui/branch/BranchRenderContext.h"
#include "ui/canvas/CanvasNode.h"

#include <vector>

namespace mind_map::ui::branch {

// child_index must satisfy nodes[child_index].parent_idx_ >= 0.
void DrawMindMapBranchOrganicTaper(
    const BranchRenderContext& ctx, int child_index,
    const std::vector<mind_map::ui::CanvasNode>& nodes);

}  // namespace mind_map::ui::branch
