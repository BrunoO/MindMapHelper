#pragma once

#include "imgui.h"

namespace mind_map::ui::branch {

/// ImGui draw-list and canvas transform passed to all branch-draw functions; built by MindMapCanvasView::Render.
struct BranchRenderContext {
  ImDrawList* draw_list_ = nullptr;
  ImVec2 canvas_p0_;
  ImVec2 pan_px_;
  float zoom_ = 1.0F;
};

[[nodiscard]] inline BranchRenderContext MakeBranchRenderContext(ImDrawList* draw_list, ImVec2 canvas_p0,
                                                                 ImVec2 pan_px, float zoom) {
  return {draw_list, canvas_p0, pan_px, zoom};
}

}  // namespace mind_map::ui::branch
