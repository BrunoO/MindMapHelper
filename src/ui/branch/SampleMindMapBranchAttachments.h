#pragma once

#include "core/mindmap/SampleMindMapTopology.h"

#include "ui/canvas/NodeGeometry.h"

#include "imgui.h"

#include <array>
#include <cassert>
#include <cstddef>

namespace mind_map::ui::branch {

// Shared world-space attachment points for sample-mind-map edges (rounded-rect toward opposite node).
struct SampleMindMapBranchRoundedAttachments {
  int parent_ = -1;
  const char* parent_label_ = nullptr;
  const char* child_label_ = nullptr;
  ImVec2 parent_half_;
  ImVec2 child_half_;
  ImVec2 pw_;
  ImVec2 cw_;
  ImVec2 p0_attachment_;
  ImVec2 p3_attachment_;
};

// Preconditions: child_index is a valid sample node index; caller must only invoke for nodes with parent >= 0.
inline void FillSampleMindMapBranchRoundedAttachments(
    int child_index,
    const std::array<ImVec2, mind_map::core::mindmap::kSampleMindMapNodeCount>& pos_world,
    SampleMindMapBranchRoundedAttachments* out) {
  assert(out != nullptr);
  assert(child_index >= 0 && child_index < mind_map::core::mindmap::kSampleMindMapNodeCount);
  const int parent =
      mind_map::core::mindmap::kSampleMindMapSpecs[static_cast<size_t>(child_index)].parent_;
  assert(parent >= 0 && parent < mind_map::core::mindmap::kSampleMindMapNodeCount);

  out->parent_ = parent;
  out->parent_label_ =
      mind_map::core::mindmap::kSampleMindMapSpecs[static_cast<size_t>(parent)].label_;
  out->child_label_ =
      mind_map::core::mindmap::kSampleMindMapSpecs[static_cast<size_t>(child_index)].label_;
  out->parent_half_ = mind_map::canvas::NodeHalfExtentForLabel(out->parent_label_);
  out->child_half_ = mind_map::canvas::NodeHalfExtentForLabel(out->child_label_);
  out->pw_ = pos_world[static_cast<size_t>(parent)];
  out->cw_ = pos_world[static_cast<size_t>(child_index)];
  out->p0_attachment_ = mind_map::canvas::RoundedRectAttachmentPreferEdgeMid(
      out->pw_, out->parent_half_, mind_map::canvas::kNodeCornerRadiusWorld, out->cw_);
  out->p3_attachment_ = mind_map::canvas::RoundedRectAttachmentPreferEdgeMid(
      out->cw_, out->child_half_, mind_map::canvas::kNodeCornerRadiusWorld, out->pw_);
}

template <typename DrawOneFn>
inline void ForEachSampleMindMapChildBranch(const DrawOneFn& draw_one) {
  for (int child = 0; child < mind_map::core::mindmap::kSampleMindMapNodeCount; ++child) {
    if (mind_map::core::mindmap::kSampleMindMapSpecs[static_cast<size_t>(child)].parent_ < 0) {
      continue;
    }
    draw_one(child);
  }
}

}  // namespace mind_map::ui::branch
