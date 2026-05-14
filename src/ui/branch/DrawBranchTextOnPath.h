#pragma once

#include "ui/branch/BranchRenderContext.h"
#include "ui/branch/BranchStyle.h"
#include "ui/canvas/CanvasNode.h"

#include "imgui.h"

#include <cstdint>
#include <string_view>
#include <vector>

namespace mind_map::ui::branch {

// Screen-space offset from the path along the left normal (positive = "above" in a y-down space).
/// Path-relative anchor for edge label placement along the branch centerline.
enum class BranchTextAlongPathAnchor : std::uint8_t {
  Start = 0,
  Mid = 1,
  End = 2,
};

/// Rendering options for text drawn along a branch path by DrawMindMapBranchTextOnPath.
struct BranchTextRenderOptions {
  ImFont* font_ = nullptr;
  float font_size_ = 0.0F;  // 0: use ImGui::GetFont() / default size at draw site
  ImU32 color_ = IM_COL32(235, 235, 245, 255);
  float normal_offset_px_ = 0.0F;
  BranchTextAlongPathAnchor anchor_along_path_ = BranchTextAlongPathAnchor::Mid;
};

/// World-space polyline approximating a branch centerline; built by BuildMindMapBranchTextPathWorld.
struct BranchTextPathPolyline {
  std::vector<ImVec2> points_world_;
};

/// Builds the world-space centerline polyline for a branch; dispatches to the correct sampler per BranchStyle.
// Preconditions: child_index < nodes.size(); nodes[child_index].parent_idx_.has_value().
[[nodiscard]] BranchTextPathPolyline BuildMindMapBranchTextPathWorld(size_t child_index,
                                                                   const std::vector<mind_map::ui::CanvasNode>& nodes,
                                                                   BranchStyle style);

/// Renders an edge label along the branch centerline; called by MindMapCanvasView when CanvasNode::branch_edge_label_ is set.
// Preconditions: same as BuildMindMapBranchTextPathWorld; ctx.draw_list_ non-null.
void DrawMindMapBranchTextOnPath(const BranchRenderContext& ctx, size_t child_index,
                                 const std::vector<mind_map::ui::CanvasNode>& nodes, std::string_view label,
                                 const BranchTextRenderOptions& options);

}  // namespace mind_map::ui::branch
