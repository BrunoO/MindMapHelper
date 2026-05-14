#pragma once

#include "ui/canvas/CanvasNode.h"
#include "ui/canvas/NodeExtent.h"
#include "ui/canvas/NodeGeometry.h"

#include "imgui.h"

#include <cassert>
#include <cstddef>
#include <vector>

namespace mind_map::ui::branch {

// Shared world-space attachment points for mind-map edges (rounded-rect toward opposite node).
struct BranchEdgeData {
  bool parent_is_root_ = false;
  const char* parent_label_ = nullptr;
  const char* child_label_ = nullptr;
  ImVec2 parent_half_;
  ImVec2 child_half_;
  ImVec2 pw_;
  ImVec2 cw_;
  ImVec2 p0_attachment_;
  ImVec2 p3_attachment_;
};

// Preconditions: child_index is valid; nodes[child_index].parent_idx_.has_value().
inline void FillBranchEdgeData(
    size_t child_index,
    const std::vector<mind_map::ui::CanvasNode>& nodes,
    BranchEdgeData* out) {
  assert(out != nullptr);
  assert(child_index < nodes.size());
  const mind_map::ui::CanvasNode& child = nodes[child_index];
  assert(child.parent_idx_.has_value());
  const size_t parent = *child.parent_idx_;
  assert(parent < nodes.size());
  const mind_map::ui::CanvasNode& par = nodes[parent];

  out->parent_is_root_ = !par.parent_idx_.has_value();
  out->parent_label_ = par.label_.c_str();
  out->child_label_ = child.label_.c_str();
  out->parent_half_ = mind_map::canvas::NodeHalfExtent(par);
  out->child_half_ = mind_map::canvas::NodeHalfExtent(child);
  out->pw_ = par.pos_world_;
  out->cw_ = child.pos_world_;
  out->p0_attachment_ = mind_map::canvas::RoundedRectAttachmentPreferEdgeMid(
      out->pw_, out->parent_half_, mind_map::canvas::kNodeCornerRadiusWorld, out->cw_);
  out->p3_attachment_ = mind_map::canvas::RoundedRectAttachmentPreferEdgeMid(
      out->cw_, out->child_half_, mind_map::canvas::kNodeCornerRadiusWorld, out->pw_);
}

}  // namespace mind_map::ui::branch
