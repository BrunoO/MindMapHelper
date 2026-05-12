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
};

}  // namespace mind_map::ui
