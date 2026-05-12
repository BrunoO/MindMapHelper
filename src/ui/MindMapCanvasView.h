#pragma once

#include "core/MindMapDocument.h"
#include "ui/branch/BranchStyle.h"
#include "ui/demos/SampleMindMapGraph.h"

#include "imgui.h"

#include <array>
#include <string>
#include <vector>

namespace mind_map::ui {

struct MindMapCanvasRenderContext {
  ImDrawList* draw_list_ = nullptr;
  ImVec2 canvas_p0_;
  ImVec2 canvas_p1_;
  ImVec2 pan_px_;
  float zoom_ = 1.0F;
  bool canvas_hovered_ = false;
  ImVec2 mouse_world_;
};

struct MindMapPointerState {
  ImVec2 mouse_screen_;
  ImVec2 mouse_world_;
  bool canvas_hovered_ = false;
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
//
// Milestone E: click a non-root node to select it; the incoming edge (parent → child) style is edited via the toolbar
// combo. Click empty canvas or root to clear selection.
class MindMapCanvasView {
 public:
  MindMapCanvasView();

  void Reset();

  void LoadFrom(const mind_map::core::MindMapDocument& doc);
  [[nodiscard]] mind_map::core::MindMapDocument ToDocument(const mind_map::core::MindMapViewport& viewport) const;

  void OnPrimaryDown(const MindMapPointerState& ptr);
  void OnPrimaryDrag(const MindMapPointerState& ptr);
  void OnPrimaryUp();

  [[nodiscard]] bool IsDraggingContent() const;

  void Render(const MindMapCanvasRenderContext& ctx);

  // Applies one style to every edge (all child indices with a parent).
  void SetBranchStyleForAllEdges(mind_map::ui::branch::BranchStyle style);

  [[nodiscard]] bool HasSelectedIncomingEdgeStyleTarget() const;
  [[nodiscard]] int GetSelectedChildForBranchStyle() const;
  // nullptr when no non-root node is selected for incoming-edge editing.
  [[nodiscard]] const char* GetSelectedIncomingEdgeChildLabel() const;
  [[nodiscard]] mind_map::ui::branch::BranchStyle GetBranchStyleForSelectedChildEdge() const;
  void SetBranchStyleForSelectedChildEdge(mind_map::ui::branch::BranchStyle style);

  // Combo preview: common style name, or a short label when edges disagree.
  [[nodiscard]] const char* GetBranchStyleComboPreviewLabel() const;

  [[nodiscard]] bool BranchStylesAreUniform() const;
  [[nodiscard]] mind_map::ui::branch::BranchStyle RepresentativeChildEdgeStyle() const;

  // Node activation — used by DeleteNodeCommand to hide/restore nodes without
  // modifying the document model (deletion is view-layer only; not serialized).
  // Pending persistence: node_active_ is view-only until the document format can store it.
  void SetNodeActive(int idx, bool active);
  [[nodiscard]] bool IsNodeActive(int idx) const;
  // Returns the indices of all currently active nodes in the subtree rooted at idx.
  [[nodiscard]] std::vector<int> CollectActiveSubtree(int idx) const;

 private:
  [[nodiscard]] mind_map::ui::branch::BranchStyle StyleOfFirstChildEdge_() const;
  [[nodiscard]] bool BranchStylesAreUniform_() const;

  std::array<ImVec2, mind_map::demos::kSampleMindMapNodeCount> pos_world_{};
  int dragging_node_ = -1;
  ImVec2 grab_offset_world_;
  int selected_child_for_edge_style_ = -1;
  std::array<mind_map::ui::branch::BranchStyle, mind_map::demos::kSampleMindMapNodeCount> branch_style_by_child_{};
  std::array<std::string, mind_map::demos::kSampleMindMapNodeCount> node_ids_{};
  std::array<std::string, mind_map::demos::kSampleMindMapNodeCount> edge_ids_{};
  std::array<bool, mind_map::demos::kSampleMindMapNodeCount> node_active_{};
};

}  // namespace mind_map::ui
