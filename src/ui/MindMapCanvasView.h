#pragma once

#include "ui/branch/BranchStyle.h"
#include "ui/demos/SampleMindMapGraph.h"

#include "imgui.h"

#include <array>

namespace mind_map::ui {

struct MindMapCanvasRenderContext {
  ImDrawList* draw_list = nullptr;
  ImVec2 canvas_p0{};
  ImVec2 canvas_p1{};
  ImVec2 pan_px{};
  float zoom = 1.0F;
  bool canvas_hovered = false;
  ImVec2 mouse_world{};
};

struct MindMapPointerState {
  ImVec2 mouse_screen{};
  ImVec2 mouse_world{};
  bool canvas_hovered = false;
};

// Single sample-map canvas: pan/zoom stay in MindMapUi; this owns layout, drag, and drawing.
//
// Node model **C1** (see internal-docs/2026-05-02_mindmap-demos-to-branch-styles-plan.md, Milestone C): one canonical
// representation — rounded rectangles from SampleMapHalfExtentForLabel + kSampleMindMapNodeCornerRadiusWorld.
// Pointer hit-testing uses mind_map::demos::HitTestSampleMap only; branch styles may differ in edge geometry but share
// the same attachments (e.g. orthogonal uses SampleMapRoundedRectAttachmentPreferEdgeMid, not circles).
class MindMapCanvasView {
 public:
  MindMapCanvasView();

  void Reset();

  void OnPrimaryDown(const MindMapPointerState& ptr);
  void OnPrimaryDrag(const MindMapPointerState& ptr);
  void OnPrimaryUp();

  [[nodiscard]] bool IsDraggingContent() const;

  void Render(const MindMapCanvasRenderContext& ctx);

  [[nodiscard]] mind_map::ui::branch::BranchStyle GetBranchStyle() const { return branch_style_; }
  void SetBranchStyle(mind_map::ui::branch::BranchStyle style) { branch_style_ = style; }

 private:
  std::array<ImVec2, mind_map::demos::kSampleMindMapNodeCount> pos_world_{};
  int dragging_node_ = -1;
  ImVec2 grab_offset_world_{};
  mind_map::ui::branch::BranchStyle branch_style_ = mind_map::ui::branch::BranchStyle::Bezier;
};

}  // namespace mind_map::ui
