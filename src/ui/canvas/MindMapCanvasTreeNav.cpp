#include "ui/canvas/MindMapCanvasTreeNav.h"

#include "ui/MindMapCanvasView.h"

#include <cassert>

namespace mind_map::ui::canvas {

std::optional<size_t> GetParentOf(const MindMapCanvasView& view, size_t idx) {
  assert(idx < view.nodes_.size());
  return view.nodes_[idx].parent_idx_;
}

std::optional<size_t> GetFirstActiveChildOf(const MindMapCanvasView& view, size_t idx) {
  for (size_t i = 0; i < view.nodes_.size(); ++i) {
    if (view.nodes_[i].active_ && view.nodes_[i].parent_idx_ == idx) {
      return i;
    }
  }
  return std::nullopt;
}

std::optional<size_t> GetPrevSiblingOf(const MindMapCanvasView& view, size_t idx) {
  assert(idx < view.nodes_.size());
  const auto parent = view.nodes_[idx].parent_idx_;
  std::optional<size_t> prev;
  for (size_t i = 0; i < view.nodes_.size(); ++i) {
    if (!view.nodes_[i].active_ || view.nodes_[i].parent_idx_ != parent) {
      continue;
    }
    if (i == idx) {
      return prev;
    }
    prev = i;
  }
  return std::nullopt;
}

std::optional<size_t> GetNextSiblingOf(const MindMapCanvasView& view, size_t idx) {
  assert(idx < view.nodes_.size());
  const auto parent = view.nodes_[idx].parent_idx_;
  bool found = false;
  for (size_t i = 0; i < view.nodes_.size(); ++i) {
    if (!view.nodes_[i].active_ || view.nodes_[i].parent_idx_ != parent) {
      continue;
    }
    if (found) {
      return i;
    }
    if (i == idx) {
      found = true;
    }
  }
  return std::nullopt;
}

}  // namespace mind_map::ui::canvas
