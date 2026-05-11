#pragma once

#include "ui/MindMapCanvasView.h"

#include "imgui.h"

namespace mind_map::ui {

inline constexpr float kInitialPanX = 40.0F;
inline constexpr float kInitialPanY = 120.0F;

struct UiState {
  MindMapCanvasView canvas_;
  ImVec2 pan_px_ = {kInitialPanX, kInitialPanY};
  float zoom_ = 1.0F;
  bool show_status_bar_ = true;
};

}  // namespace mind_map::ui
