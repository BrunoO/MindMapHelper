#pragma once

#include "ui/branch/BranchRenderContext.h"
#include "ui/canvas/CanvasNode.h"

#include "imgui.h"

#include <cstddef>
#include <vector>

namespace mind_map::ui::branch {

// child_index must satisfy nodes[child_index].parent_idx_.has_value().
void DrawMindMapBranchOrganicTaper(
    const BranchRenderContext& ctx, size_t child_index,
    const std::vector<mind_map::ui::CanvasNode>& nodes);

// Returns n_samples+1 world-space points sampled uniformly along the organic centerline.
[[nodiscard]] std::vector<ImVec2> SampleOrganicTaperCenterlineWorld(
    size_t child_index, const std::vector<mind_map::ui::CanvasNode>& nodes, std::size_t n_samples);

}  // namespace mind_map::ui::branch
