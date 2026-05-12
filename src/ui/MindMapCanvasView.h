#pragma once

#include "core/MindMapDocument.h"
#include "ui/branch/BranchStyle.h"
#include "ui/canvas/CanvasNode.h"

#include "imgui.h"

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

// Single canvas view: pan/zoom stay in MindMapUi; this owns layout, drag, and drawing.
//
// Node model: each CanvasNode in nodes_ carries id, label, world position, parent index (-1 for
// root), edge id, branch style, and active flag. LoadFrom rebuilds nodes_ from any MindMapDocument.
//
// Per-edge branch styles: nodes_[child].branch_style_ owns the style of the incoming edge.
// Reset() restores world positions from the initial_pos_world_ snapshot taken at LoadFrom time.
//
// Click a non-root node to select it; its incoming edge style is edited via the toolbar combo.
// Click empty canvas or root to clear selection.
class MindMapCanvasView {
 public:
  MindMapCanvasView();
  ~MindMapCanvasView();

  MindMapCanvasView(const MindMapCanvasView&) = delete;
  MindMapCanvasView& operator=(const MindMapCanvasView&) = delete;

  void Reset();

  void LoadFrom(const mind_map::core::MindMapDocument& doc);
  [[nodiscard]] mind_map::core::MindMapDocument ToDocument(const mind_map::core::MindMapViewport& viewport) const;

  void OnPrimaryDown(const MindMapPointerState& ptr);
  void OnPrimaryDrag(const MindMapPointerState& ptr);
  void OnPrimaryUp();

  [[nodiscard]] bool IsDraggingContent() const;

  void Render(const MindMapCanvasRenderContext& ctx);

  // Applies one style to every edge (all child nodes with a parent).
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
  // Pending persistence: node active_ is view-only until the document format can store it.
  void SetNodeActive(int idx, bool active);
  [[nodiscard]] bool IsNodeActive(int idx) const;
  // Returns the indices of all currently active nodes in the subtree rooted at idx.
  [[nodiscard]] std::vector<int> CollectActiveSubtree(int idx) const;

  // Appends a new child node under parent_idx, selects it, and returns its index.
  // The node is appended to nodes_ and initial_pos_world_; undo toggles active_ only.
  int InsertChildNode(int parent_idx);

 private:
  [[nodiscard]] mind_map::ui::branch::BranchStyle StyleOfFirstChildEdge_() const;
  [[nodiscard]] bool BranchStylesAreUniform_() const;

  std::vector<CanvasNode> nodes_;
  std::vector<ImVec2> initial_pos_world_;
  int dragging_node_ = -1;
  ImVec2 grab_offset_world_;
  int selected_child_for_edge_style_ = -1;
};

}  // namespace mind_map::ui
