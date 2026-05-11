#pragma once

#include "ui/demos/SampleMindMapGraph.h"

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
    int child_index, const std::array<ImVec2, mind_map::demos::kSampleMindMapNodeCount>& pos_world,
    SampleMindMapBranchRoundedAttachments* out) {
  assert(out != nullptr);
  assert(child_index >= 0 && child_index < mind_map::demos::kSampleMindMapNodeCount);
  const int parent = mind_map::demos::kSampleMindMapSpecs[static_cast<size_t>(child_index)].parent_;
  assert(parent >= 0 && parent < mind_map::demos::kSampleMindMapNodeCount);

  out->parent_ = parent;
  out->parent_label_ = mind_map::demos::kSampleMindMapSpecs[static_cast<size_t>(parent)].label_;
  out->child_label_ = mind_map::demos::kSampleMindMapSpecs[static_cast<size_t>(child_index)].label_;
  out->parent_half_ = mind_map::demos::SampleMapHalfExtentForLabel(out->parent_label_);
  out->child_half_ = mind_map::demos::SampleMapHalfExtentForLabel(out->child_label_);
  out->pw_ = pos_world[static_cast<size_t>(parent)];
  out->cw_ = pos_world[static_cast<size_t>(child_index)];
  out->p0_attachment_ = mind_map::demos::SampleMapRoundedRectAttachmentPreferEdgeMid(
      out->pw_, out->parent_half_, mind_map::demos::kSampleMindMapNodeCornerRadiusWorld, out->cw_);
  out->p3_attachment_ = mind_map::demos::SampleMapRoundedRectAttachmentPreferEdgeMid(
      out->cw_, out->child_half_, mind_map::demos::kSampleMindMapNodeCornerRadiusWorld, out->pw_);
}

}  // namespace mind_map::ui::branch
