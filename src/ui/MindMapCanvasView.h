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
//
// Milestone D: each child node index owns the style of its incoming edge (`branch_style_by_child_[child]`); root slot
// is unused. Reset() restores positions only, not per-edge styles.
class MindMapCanvasView {
 public:
  MindMapCanvasView();

  void Reset();

  void OnPrimaryDown(const MindMapPointerState& ptr);
  void OnPrimaryDrag(const MindMapPointerState& ptr);
  void OnPrimaryUp();

  [[nodiscard]] bool IsDraggingContent() const;

  void Render(const MindMapCanvasRenderContext& ctx);

  // Applies one style to every edge (all child indices with a parent). Used by the toolbar combo until per-edge UI
  // exists (Milestone E).
  void SetBranchStyleForAllEdges(mind_map::ui::branch::BranchStyle style);

  // Combo preview: common style name, or a short label when edges disagree.
  [[nodiscard]] const char* GetBranchStyleComboPreviewLabel() const;

  [[nodiscard]] bool BranchStylesAreUniform() const;
  [[nodiscard]] mind_map::ui::branch::BranchStyle RepresentativeChildEdgeStyle() const;

 private:
  void InitDefaultPerChildBranchStyles();

  [[nodiscard]] mind_map::ui::branch::BranchStyle StyleOfFirstChildEdge_() const;
  [[nodiscard]] bool BranchStylesAreUniform_() const;

  std::array<ImVec2, mind_map::demos::kSampleMindMapNodeCount> pos_world_{};
  int dragging_node_ = -1;
  ImVec2 grab_offset_world_{};
  std::array<mind_map::ui::branch::BranchStyle, mind_map::demos::kSampleMindMapNodeCount> branch_style_by_child_{};
};

}  // namespace mind_map::ui
