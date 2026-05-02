#pragma once

#include "ui/branch/BranchRenderContext.h"
#include "ui/demos/SampleMindMapGraph.h"

#include <array>

namespace mind_map::ui::branch {

void DrawSampleMindMapBranchOrganicTaper(
    const BranchRenderContext& ctx, int child_index,
    const std::array<ImVec2, mind_map::demos::kSampleMindMapNodeCount>& pos_world);

void DrawAllSampleMindMapBranchesOrganicTaper(
    const BranchRenderContext& ctx,
    const std::array<ImVec2, mind_map::demos::kSampleMindMapNodeCount>& pos_world);

}  // namespace mind_map::ui::branch
