#pragma once

#include "ui/branch/BranchStyle.h"

#include "imgui.h"

#include <string>

namespace mind_map::ui {

struct CanvasNode {
  std::string id_;
  std::string label_;
  ImVec2 pos_world_;
  int parent_idx_ = -1;
  std::string edge_id_;
  branch::BranchStyle branch_style_ = branch::BranchStyle::Bezier;
  bool active_ = true;
  std::string image_png_base64_;       // kept for serialization round-trip
  ImTextureID texture_id_ = 0;        // GPU texture; 0 means no image
  ImVec2 half_extent_override_ = {0.0F, 0.0F};  // override half-extents; {0,0} = auto-size from label
};

}  // namespace mind_map::ui
