#pragma once

#include "core/MindMapDocument.h"
#include "ui/branch/BranchStyle.h"
#include "ui/canvas/CanvasNode.h"

#include "imgui.h"

#include <cstddef>
#include <optional>
#include <string_view>
#include <unordered_map>
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
  float zoom_ = 1.0F;
};

// Single canvas view: pan/zoom stay in MindMapUi; this owns layout, drag, and drawing.
//
// Node model: each CanvasNode in nodes_ carries id, label, world position, optional parent index
// (nullopt for root), edge id, branch style, and active flag. LoadFrom rebuilds nodes_ from any
// MindMapDocument.
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
  // Any node currently selected (including root); nullopt when nothing selected.
  [[nodiscard]] std::optional<size_t> GetSelectedNode() const;
  [[nodiscard]] std::optional<size_t> GetSelectedChildForBranchStyle() const;
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
  void SetNodeActive(size_t idx, bool active);
  [[nodiscard]] bool IsNodeActive(size_t idx) const;
  // Returns the indices of all currently active nodes in the subtree rooted at idx.
  [[nodiscard]] std::vector<size_t> CollectActiveSubtree(size_t idx) const;

  // Collapse / expand — sets collapsed_ flag and toggles active_ on the subtree.
  void CollapseNode(size_t idx);
  void ExpandNode(size_t idx);
  [[nodiscard]] bool IsCollapsed(size_t idx) const;
  [[nodiscard]] bool NodeHasChildren(size_t idx) const;

  // Triangle click: OnPrimaryDown stores a pending toggle; caller reads and clears it.
  [[nodiscard]] std::optional<size_t> ConsumeCollapseToggleTarget();

  // Appends a new child node under parent_idx, selects it, and returns its index.
  // The node is appended to nodes_ and initial_pos_world_; undo toggles active_ only.
  size_t InsertChildNode(size_t parent_idx);

  // Sets (or clears when png_base64 is empty) the image on the node at idx.
  // Releases the previous GL texture and uploads a new one if png_base64 is non-empty.
  void SetNodeImage(size_t idx, std::string_view png_base64);
  [[nodiscard]] const std::string& GetNodeImageBase64(size_t idx) const;

  void SetNodeLabel(size_t idx, std::string_view label);
  [[nodiscard]] const std::string& GetNodeLabel(size_t idx) const;

 private:

  std::vector<CanvasNode> nodes_;
  std::vector<ImVec2> initial_pos_world_;
  std::optional<size_t> dragging_node_;
  ImVec2 grab_offset_world_;
  std::optional<size_t> selected_node_;
  std::optional<size_t> selected_child_for_edge_style_;

  // Resize state — active while a corner handle is being dragged.
  std::optional<size_t> resizing_node_;
  std::optional<size_t> resizing_corner_;  // 0=TL, 1=TR, 2=BR, 3=BL
  ImVec2 resize_anchor_world_;             // opposite corner; fixed during the drag
  ImVec2 resize_orig_half_;               // half-extents at drag start (for aspect-ratio lock)
  bool resize_lock_aspect_ = false;       // true when the node has a texture

  // Nodes deactivated by each collapse; used by ExpandNode to restore exactly the right set.
  std::unordered_map<size_t, std::vector<size_t>> collapse_affected_;
  // Set by OnPrimaryDown when a collapse triangle is clicked; consumed by MindMapUi.
  std::optional<size_t> collapse_toggle_target_;
};

}  // namespace mind_map::ui
