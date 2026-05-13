#pragma once

#include "ui/branch/BranchStyle.h"

#include "imgui.h"

#include <cstddef>
#include <optional>
#include <string>

namespace mind_map::ui {

struct CanvasNode {
  std::string id_;
  std::string label_;
  ImVec2 pos_world_;
  std::optional<size_t> parent_idx_;  // nullopt = root (no parent)
  std::string edge_id_;
  branch::BranchStyle branch_style_ = branch::BranchStyle::Bezier;
  bool active_ = true;
  std::string image_png_base64_;       // kept for serialization round-trip
  ImTextureID texture_id_ = 0;        // GPU texture; 0 means no image
  ImVec2 half_extent_override_ = {0.0F, 0.0F};  // override half-extents; {0,0} = auto-size from label
  std::string branch_edge_label_;     // caption on the incoming edge; empty = none
};

}  // namespace mind_map::ui
