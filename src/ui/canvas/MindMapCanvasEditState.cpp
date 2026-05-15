#include "ui/canvas/MindMapCanvasEditState.h"

#include "ui/MindMapCanvasView.h"

namespace mind_map::ui::canvas {

bool IsEditing(const MindMapCanvasView& view) {
  return view.editing_node_.has_value();
}

std::optional<size_t> GetEditingNode(const MindMapCanvasView& view) {
  return view.editing_node_;
}

std::string& GetEditBuffer(MindMapCanvasView& view) {
  return view.edit_buffer_;
}

}  // namespace mind_map::ui::canvas
